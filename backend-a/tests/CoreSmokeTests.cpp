#include "Fft.h"
#include "Asm.h"
#include "Axial.h"
#include "Metrics.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

// 测试基础：记录检查结果，并用直接 DFT 提供独立的数值参考。
namespace {
const double PI = 3.14159265358979323846;
int checks = 0;
int failures = 0;
int lastProgress = -1;
bool progressValid = true;
std::atomic_bool* cancelFromCallback = nullptr;

void Check(bool passed, const char* name) {
    ++checks;
    if (!passed) {
        ++failures;
        std::cout << "FAIL: " << name << '\n';
    }
}

double MaxError(const std::vector<std::complex<double>>& a,
                const std::vector<std::complex<double>>& b) {
    if (a.size() != b.size()) {
        return std::numeric_limits<double>::infinity();
    }
    double error = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        error = (std::max)(error, std::abs(a[i] - b[i]));
    }
    return error;
}

std::vector<std::complex<double>> DirectDft(
    const std::vector<std::complex<double>>& input) {
    const int n = static_cast<int>(input.size());
    std::vector<std::complex<double>> result(input.size());
    for (int k = 0; k < n; ++k) {
        for (int j = 0; j < n; ++j) {
            const double angle = -2.0 * PI * k * j / n;
            result[k] += input[j] * std::polar(1.0, angle);
        }
    }
    return result;
}

void RecordProgress(const ProgressInfo& info) {
    if (info.percent < lastProgress || info.percent < 0 || info.percent > 100 ||
        info.stage.empty()) {
        progressValid = false;
    }
    lastProgress = info.percent;
    if (cancelFromCallback != nullptr && info.percent > 0) {
        cancelFromCallback->store(true);
    }
}

// FFT 验证：与直接公式比较，并覆盖逆变换、二维方向和常用尺寸。
void TestFft() {
    std::vector<std::complex<double>> input(8);
    for (int i = 0; i < 8; ++i) {
        input[i] = std::complex<double>(std::sin(i * 0.7), std::cos(i * 0.3));
    }
    std::vector<std::complex<double>> transformed = input;
    FFT1D(transformed, false);
    Check(MaxError(transformed, DirectDft(input)) < 1e-11, "FFT1D versus direct DFT");
    FFT1D(transformed, true);
    Check(MaxError(transformed, input) < 1e-12, "FFT1D inverse normalization");
    std::vector<std::complex<double>> singleton(1, std::complex<double>(2.0, -3.0));
    FFT1D(singleton, false);
    FFT1D(singleton, true);
    Check(std::abs(singleton[0] - std::complex<double>(2.0, -3.0)) < 1e-14, "FFT singleton");

    const int n = 4;
    std::vector<std::complex<double>> square(n * n);
    for (int i = 0; i < n * n; ++i) {
        square[i] = std::complex<double>(i * 0.1, std::sin(i * 0.8));
    }
    std::vector<std::complex<double>> reference(n * n);
    for (int ky = 0; ky < n; ++ky) {
        for (int kx = 0; kx < n; ++kx) {
            for (int y = 0; y < n; ++y) {
                for (int x = 0; x < n; ++x) {
                    const double angle = -2.0 * PI * (kx * x + ky * y) / n;
                    reference[ky * n + kx] += square[y * n + x] * std::polar(1.0, angle);
                }
            }
        }
    }
    FFT2D(square, n, false);
    Check(MaxError(square, reference) < 1e-11, "FFT2D versus direct 2D DFT");
    for (int size = 64; size <= 512; size *= 2) {
        std::vector<std::complex<double>> original(static_cast<std::size_t>(size) * size);
        for (std::size_t i = 0; i < original.size(); ++i) {
            const double index = static_cast<double>(i);
            original[i] = std::complex<double>(std::sin(index * 0.01), std::cos(index * 0.03));
        }
        std::vector<std::complex<double>> result = original;
        FFT2D(result, size, false);
        FFT2D(result, size, true);
        Check(MaxError(original, result) < 1e-10, "FFT2D round trip at 64/128/256/512");
    }
}

// 传播验证：用平面波解析解、能量守恒和正反传播检查 ASM。
void TestAsm() {
    const int n = 16;
    const double pitch = 8e-6;
    const double wavelength = 532e-9;
    const double z = 0.03;
    std::vector<std::complex<double>> field(n * n);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            field[y * n + x] = std::polar(1.0, 2.0 * PI * (3 * x - 2 * y) / n);
        }
    }
    const double fx = 3.0 / (n * pitch);
    const double fy = -2.0 / (n * pitch);
    const double angle = 2.0 * PI * (z / wavelength) *
        std::sqrt(1.0 - wavelength * wavelength * (fx * fx + fy * fy));
    std::vector<std::complex<double>> expected = field;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        expected[i] *= std::polar(1.0, angle);
    }
    const std::vector<std::complex<double>> output = PropagateAsm(field, n, pitch, wavelength, z);
    Check(MaxError(output, expected) < 1e-8, "ASM tilted plane wave analytical solution");
    Check(MaxError(PropagateAsm(field, n, pitch, wavelength, 0.0), field) == 0.0, "ASM zero-distance identity");
    for (int i = 0; i < n * n; ++i) {
        field[i] = std::complex<double>(std::sin(i * 0.17), std::cos(i * 0.21));
    }
    const std::vector<std::complex<double>> forward = PropagateAsm(field, n, pitch, wavelength, z);
    Check(MaxError(PropagateAsm(forward, n, pitch, wavelength, -z), field) < 1e-9,
          "ASM forward/backward recovery");
    double before = 0.0;
    double after = 0.0;
    for (std::size_t i = 0; i < field.size(); ++i) {
        before += std::norm(field[i]);
        after += std::norm(forward[i]);
    }
    Check(std::abs(after - before) / before < 1e-12, "ASM energy conservation");
    const std::vector<std::complex<double>> twice = PropagateAsm(forward, n, pitch, wavelength, z);
    Check(MaxError(twice, PropagateAsm(field, n, pitch, wavelength, 2.0 * z)) < 1e-8,
          "ASM distance composition");
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            field[y * n + x] = (x % 2 == 0) ? 1.0 : -1.0;
        }
    }
    const std::vector<std::complex<double>> zeros(n * n);
    Check(MaxError(PropagateAsm(field, n, wavelength / 4.0, wavelength, z), zeros) < 1e-12,
          "ASM filters evanescent components");
    Check(MaxError(PropagateAsm(field, n, wavelength / 4.0, wavelength, -z), zeros) < 1e-12,
          "ASM reverse evanescent handling remains stable");
    Check(MaxError(PropagateAsm(field, n, wavelength / 4.0, wavelength, 0.0), field) == 0.0,
          "ASM zero distance preserves all components");
}

// 光强和指标验证：使用手算结果，并检查全黑图和完全一致图像。
void TestMetrics() {
    std::vector<std::complex<double>> field(4);
    field[0] = std::complex<double>(3.0, 4.0);
    field[1] = std::complex<double>(0.0, 5.0);
    field[2] = std::complex<double>(0.0, 0.0);
    field[3] = std::complex<double>(1.0, 0.0);
    const GrayImage image = IntensityOf(field, 2);
    Check(image.width == 2 && image.height == 2 && image.data[0] == 1.0 &&
          image.data[1] == 1.0 && image.data[2] == 0.0 &&
          std::abs(image.data[3] - 0.04) < 1e-14, "Intensity is normalized modulus squared");
    const GrayImage black = IntensityOf(std::vector<std::complex<double>>(4), 2);
    Check(black.data == std::vector<double>(4, 0.0), "Black intensity stays zero");
    GrayImage a;
    a.width = 2;
    a.height = 2;
    a.data = {0.0, 0.5, 1.0, 0.0};
    GrayImage b = a;
    b.data = {0.0, 0.0, 1.0, 1.0};
    Check(std::abs(ComputeMse(a, b) - 0.3125) < 1e-14, "MSE hand-calculated reference");
    Check(ComputeMse(a, a) == 0.0, "Identical image MSE");
    Check(std::abs(ComputePsnr(0.01) - 20.0) < 1e-12, "PSNR hand-calculated reference");
    Check(std::isinf(ComputePsnr(0.0)) && ComputePsnr(0.0) > 0.0, "Zero MSE gives positive infinity");
    Check(ComputePsnr(1.0) == 0.0, "Maximum MSE gives zero dB");
}

// 轴向验证：用两束平面波的干涉解析解检查切片位置和整图归一化。
void TestAxial() {
    const int n = 8;
    const int slices = 5;
    const double wavelength = 532e-9;
    const double pitch = 8e-6;
    std::vector<std::complex<double>> field(n * n);
    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            field[y * n + x] = 1.0 + 0.5 * std::polar(1.0, 2.0 * PI * (x + y) / n);
        }
    }
    std::atomic_bool cancel(false);
    lastProgress = -1;
    progressValid = true;
    const GrayImage axial = ComputeAxialSection(field, n, pitch, wavelength, 0.0, 0.002, slices,
                                                cancel, RecordProgress);
    Check(axial.width == n && axial.height == slices && axial.data.size() == n * slices,
          "Axial output dimensions");
    Check(progressValid && lastProgress == 100, "Axial progress is monotonic and complete");
    std::vector<double> reference(n * slices);
    double maximum = 0.0;
    const double f = 1.0 / (n * pitch);
    const double q = std::sqrt(1.0 - 2.0 * wavelength * wavelength * f * f);
    for (int s = 0; s < slices; ++s) {
        const double z = 0.002 * s / (slices - 1);
        for (int x = 0; x < n; ++x) {
            const double phase = 2.0 * PI * (x + n / 2) / n +
                2.0 * PI * (z / wavelength) * (q - 1.0);
            reference[s * n + x] = 1.25 + std::cos(phase);
            maximum = (std::max)(maximum, reference[s * n + x]);
        }
    }
    double error = 0.0;
    for (std::size_t i = 0; i < reference.size(); ++i) {
        error = (std::max)(error, std::abs(axial.data[i] - reference[i] / maximum));
    }
    Check(error < 1e-9, "Axial matches analytical interference and global normalization");
    const GrayImage single = ComputeAxialSection(field, n, pitch, wavelength, 0.0, 0.1, 1, cancel);
    Check(single.height == 1 && std::abs(single.data[0] - 0.25 / 2.25) < 1e-12,
          "Single axial slice uses start distance");
    const GrayImage black = ComputeAxialSection(std::vector<std::complex<double>>(n * n),
        n, pitch, wavelength, 0.0, 0.01, 3, cancel);
    Check(black.data == std::vector<double>(n * 3, 0.0), "Black axial field stays zero");
    cancel.store(true);
    bool cancelled = false;
    try {
        ComputeAxialSection(field, n, pitch, wavelength, 0.0, 0.01, 3, cancel);
    } catch (const std::runtime_error&) {
        cancelled = true;
    }
    Check(cancelled, "Cancellation before axial calculation");
    cancel.store(false);
    cancelFromCallback = &cancel;
    lastProgress = -1;
    cancelled = false;
    try {
        ComputeAxialSection(field, n, pitch, wavelength, 0.0, 0.01, 3, cancel, RecordProgress);
    } catch (const std::runtime_error&) {
        cancelled = true;
    }
    cancelFromCallback = nullptr;
    Check(cancelled && lastProgress < 100, "Cancellation during axial calculation");
}

// 非法输入验证：常见尺寸、单位、数组长度和数值错误必须被明确拒绝。
void TestInvalidInputs() {
    for (int test = 0; test < 18; ++test) {
        bool rejected = false;
        try {
            std::vector<std::complex<double>> field(4, 1.0);
            std::atomic_bool cancel(false);
            GrayImage image;
            image.width = 2;
            image.height = 2;
            image.data.assign(4, 0.5);
            switch (test) {
            case 0: field.clear(); FFT1D(field, false); break;
            case 1: field.resize(3); FFT1D(field, false); break;
            case 2: FFT2D(field, 3, false); break;
            case 3: FFT2D(field, 4, false); break;
            case 4: PropagateAsm(field, 2, 0.0, 532e-9, 0.1); break;
            case 5: PropagateAsm(field, 2, 8e-6, -532e-9, 0.1); break;
            case 6: PropagateAsm(field, 2, 8e-6, 532e-9, std::numeric_limits<double>::infinity()); break;
            case 7: field[0] = std::numeric_limits<double>::quiet_NaN(); FFT1D(field, false); break;
            case 8: IntensityOf(field, 4); break;
            case 9: image.data.pop_back(); ComputeMse(image, image); break;
            case 10: image.data[0] = -0.1; ComputeMse(image, image); break;
            case 11: image.data[0] = 1.1; ComputeMse(image, image); break;
            case 12: image.data[0] = std::numeric_limits<double>::quiet_NaN(); ComputeMse(image, image); break;
            case 13: ComputePsnr(-0.1); break;
            case 14: ComputePsnr(std::numeric_limits<double>::infinity()); break;
            case 15: ComputeAxialSection(field, 2, 8e-6, 532e-9, 0.1, 0.0, 3, cancel); break;
            case 16: ComputeAxialSection(field, 2, 8e-6, 532e-9, 0.0, 0.1, 0, cancel); break;
            case 17: FFT2D(field, 0, false); break;
            }
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected, "Invalid input must throw invalid_argument");
    }
}
}

// 测试入口：任何检查失败或出现意外异常时，返回非零退出码。
int main() {
    try {
        TestFft();
        TestAsm();
        TestMetrics();
        TestAxial();
        TestInvalidInputs();
    } catch (const std::exception& error) {
        std::cerr << "Unexpected exception: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Checks: " << checks << ", passed: " << checks - failures
              << ", failed: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
