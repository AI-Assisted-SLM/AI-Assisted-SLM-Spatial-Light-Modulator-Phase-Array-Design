#include "Asm.h"
#include "Fft.h"
#include <cmath>
#include <stdexcept>

// 输入检查：确认光场形状、复数数值和光学参数可以用于计算。
namespace {
const double PI = 3.14159265358979323846;

void CheckField(const std::vector<std::complex<double>>& field, int size) {
    if (size <= 0 || (size & (size - 1)) != 0) {
        throw std::invalid_argument("Field size must be a positive power of two.");
    }
    const std::size_t n = static_cast<std::size_t>(size);
    if (field.size() / n != n || field.size() % n != 0) {
        throw std::invalid_argument("Field length must equal size * size.");
    }
    for (std::size_t i = 0; i < field.size(); ++i) {
        if (!std::isfinite(field[i].real()) || !std::isfinite(field[i].imag())) {
            throw std::invalid_argument("Field values must be finite.");
        }
    }
}
}

// 角谱传播：FFT 后乘传播因子，再逆变换；滤除倏逝波以保持反向传播稳定。
std::vector<std::complex<double>> PropagateAsm(
    std::vector<std::complex<double>> field,
    int size, double pixelPitch_m,
    double wavelength_m, double distance_m) {
    CheckField(field, size);
    if (!std::isfinite(pixelPitch_m) || pixelPitch_m <= 0.0 ||
        !std::isfinite(wavelength_m) || wavelength_m <= 0.0 ||
        !std::isfinite(distance_m)) {
        throw std::invalid_argument("Pitch and wavelength must be positive; distance must be finite.");
    }
    if (distance_m == 0.0) {
        return field;
    }
    const double phaseScale = 2.0 * PI * (distance_m / wavelength_m);
    if (!std::isfinite(phaseScale)) {
        throw std::overflow_error("Propagation phase is too large.");
    }
    FFT2D(field, size, false);
    for (int y = 0; y < size; ++y) {
        const int ky = (y < (size + 1) / 2) ? y : y - size;
        const double fy = (static_cast<double>(ky) / size) / pixelPitch_m;
        for (int x = 0; x < size; ++x) {
            const int kx = (x < (size + 1) / 2) ? x : x - size;
            const double fx = (static_cast<double>(kx) / size) / pixelPitch_m;
            const double u = wavelength_m * fx;
            const double v = wavelength_m * fy;
            const double q = 1.0 - u * u - v * v;
            const std::size_t index = static_cast<std::size_t>(y) * size + x;
            if (q < 0.0) {
                field[index] = std::complex<double>(0.0, 0.0);
            } else {
                const double angle = phaseScale * std::sqrt(q);
                field[index] *= std::complex<double>(std::cos(angle), std::sin(angle));
            }
        }
    }
    FFT2D(field, size, true);
    return field;
}

// 光强计算：取复振幅模平方，再除以整张图的最大值，全黑场保持为零。
GrayImage IntensityOf(const std::vector<std::complex<double>>& field, int size) {
    CheckField(field, size);
    GrayImage image;
    image.width = size;
    image.height = size;
    image.data.resize(field.size());
    double maximum = 0.0;
    for (std::size_t i = 0; i < field.size(); ++i) {
        image.data[i] = std::norm(field[i]);
        if (!std::isfinite(image.data[i])) {
            throw std::overflow_error("Intensity is too large.");
        }
        if (image.data[i] > maximum) {
            maximum = image.data[i];
        }
    }
    if (maximum > 0.0) {
        for (std::size_t i = 0; i < image.data.size(); ++i) {
            image.data[i] /= maximum;
        }
    }
    return image;
}
