#include "Fft.h"
#include <cmath>
#include <stdexcept>
#include <utility>

// 数据检查：拒绝非有限复数，避免无效数据进入计算。
namespace {
const double PI = 3.14159265358979323846;

void CheckValues(const std::vector<std::complex<double>>& values) {
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i].real()) ||
            !std::isfinite(values[i].imag())) {
            throw std::invalid_argument("FFT values must be finite.");
        }
    }
}
}

// 一维 FFT：先做二进制倒序，再逐层完成蝶形运算。
void FFT1D(std::vector<std::complex<double>>& values, bool inverse) {
    const std::size_t n = values.size();
    if (n == 0 || (n & (n - 1)) != 0) {
        throw std::invalid_argument("FFT length must be a positive power of two.");
    }
    CheckValues(values);

    std::size_t j = 0;
    for (std::size_t i = 1; i < n; ++i) {
        std::size_t bit = n >> 1;
        while ((j & bit) != 0) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            std::swap(values[i], values[j]);
        }
    }

    for (std::size_t length = 2; length <= n; length *= 2) {
        const double angle = (inverse ? 2.0 : -2.0) * PI /
            static_cast<double>(length);
        const std::complex<double> step(std::cos(angle), std::sin(angle));
        for (std::size_t start = 0; start < n; start += length) {
            std::complex<double> factor(1.0, 0.0);
            for (std::size_t k = 0; k < length / 2; ++k) {
                const std::complex<double> a = values[start + k];
                const std::complex<double> b = values[start + k + length / 2] * factor;
                values[start + k] = a + b;
                values[start + k + length / 2] = a - b;
                factor *= step;
            }
        }
        if (length == n) {
            break;
        }
    }

    if (inverse) {
        for (std::size_t i = 0; i < n; ++i) {
            values[i] /= static_cast<double>(n);
        }
    }
    CheckValues(values);
}

// 二维 FFT：依次对所有行和所有列执行一维变换。
void FFT2D(std::vector<std::complex<double>>& field, int size, bool inverse) {
    if (size <= 0 || (size & (size - 1)) != 0) {
        throw std::invalid_argument("FFT size must be a positive power of two.");
    }
    const std::size_t n = static_cast<std::size_t>(size);
    if (field.size() / n != n || field.size() % n != 0) {
        throw std::invalid_argument("Field length must equal size * size.");
    }
    CheckValues(field);
    std::vector<std::complex<double>> line(n);
    for (std::size_t y = 0; y < n; ++y) {
        for (std::size_t x = 0; x < n; ++x) {
            line[x] = field[y * n + x];
        }
        FFT1D(line, inverse);
        for (std::size_t x = 0; x < n; ++x) {
            field[y * n + x] = line[x];
        }
    }
    for (std::size_t x = 0; x < n; ++x) {
        for (std::size_t y = 0; y < n; ++y) {
            line[y] = field[y * n + x];
        }
        FFT1D(line, inverse);
        for (std::size_t y = 0; y < n; ++y) {
            field[y * n + x] = line[y];
        }
    }
}
