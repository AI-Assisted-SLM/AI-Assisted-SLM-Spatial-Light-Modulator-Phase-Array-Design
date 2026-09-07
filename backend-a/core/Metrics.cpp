#include "Metrics.h"
#include <cmath>
#include <limits>
#include <stdexcept>

// 图像检查：评价前确认图像非空、数组长度正确且光强在约定范围内。
namespace {
void CheckImage(const GrayImage& image) {
    if (image.width <= 0 || image.height <= 0) {
        throw std::invalid_argument("Image dimensions must be positive.");
    }
    const std::size_t width = static_cast<std::size_t>(image.width);
    if (image.data.size() / width != static_cast<std::size_t>(image.height) ||
        image.data.size() % width != 0) {
        throw std::invalid_argument("Image length does not match its dimensions.");
    }
    for (std::size_t i = 0; i < image.data.size(); ++i) {
        if (!std::isfinite(image.data[i]) || image.data[i] < 0.0 || image.data[i] > 1.0) {
            throw std::invalid_argument("Image intensities must be finite and in [0, 1].");
        }
    }
}
}

// 均方误差：累加对应像素差的平方，再除以像素总数。
double ComputeMse(const GrayImage& expected, const GrayImage& actual) {
    CheckImage(expected);
    CheckImage(actual);
    if (expected.width != actual.width || expected.height != actual.height) {
        throw std::invalid_argument("Images must have the same dimensions.");
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < expected.data.size(); ++i) {
        const double difference = expected.data[i] - actual.data[i];
        sum += difference * difference;
    }
    return sum / static_cast<double>(expected.data.size());
}

// 峰值信噪比：以 1 为峰值，完全相同的图像返回正无穷。
double ComputePsnr(double mse) {
    if (!std::isfinite(mse) || mse < 0.0 || mse > 1.0) {
        throw std::invalid_argument("MSE must be finite and in [0, 1].");
    }
    if (mse == 0.0) {
        return std::numeric_limits<double>::infinity();
    }
    return -10.0 * std::log10(mse);
}
