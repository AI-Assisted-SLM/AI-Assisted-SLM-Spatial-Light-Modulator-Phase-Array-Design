#include "Axial.h"
#include <cmath>
#include <limits>
#include <stdexcept>

// 轴向剖面：每次从原始光场传播，取 y=size/2 一行，最后统一归一化。
GrayImage ComputeAxialSection(
    const std::vector<std::complex<double>>& field,
    int size, double pixelPitch_m, double wavelength_m,
    double startDistance_m, double endDistance_m, int slices,
    std::atomic_bool& cancelRequested, ProgressCallback onProgress) {
    if (slices <= 0 || size <= 0 || !std::isfinite(startDistance_m) ||
        !std::isfinite(endDistance_m) || startDistance_m > endDistance_m) {
        throw std::invalid_argument("Invalid axial dimensions or distance interval.");
    }
    if (cancelRequested.load()) {
        throw std::runtime_error("Axial calculation cancelled.");
    }
    PropagateAsm(field, size, pixelPitch_m, wavelength_m, 0.0);
    const std::size_t width = static_cast<std::size_t>(size);
    if (static_cast<std::size_t>(slices) > (std::numeric_limits<std::size_t>::max)() / width) {
        throw std::length_error("Axial image is too large.");
    }
    GrayImage axial;
    axial.width = size;
    axial.height = slices;
    axial.data.resize(width * slices);
    ProgressInfo progress;
    progress.stage = "计算轴向剖面";
    if (onProgress) {
        onProgress(progress);
    }
    double maximum = 0.0;
    for (int s = 0; s < slices; ++s) {
        if (cancelRequested.load()) {
            throw std::runtime_error("Axial calculation cancelled.");
        }
        const double t = slices == 1 ? 0.0 : static_cast<double>(s) / (slices - 1);
        const double distance = (1.0 - t) * startDistance_m + t * endDistance_m;
        const std::vector<std::complex<double>> propagated =
            PropagateAsm(field, size, pixelPitch_m, wavelength_m, distance);
        if (cancelRequested.load()) {
            throw std::runtime_error("Axial calculation cancelled.");
        }
        for (int x = 0; x < size; ++x) {
            const double intensity = std::norm(propagated[width * (size / 2) + x]);
            if (!std::isfinite(intensity)) {
                throw std::overflow_error("Axial intensity is too large.");
            }
            axial.data[static_cast<std::size_t>(s) * size + x] = intensity;
            if (intensity > maximum) {
                maximum = intensity;
            }
        }
        progress.percent = static_cast<int>(99.0 * (s + 1) / slices);
        if (onProgress) {
            onProgress(progress);
        }
    }
    if (maximum > 0.0) {
        for (std::size_t i = 0; i < axial.data.size(); ++i) {
            axial.data[i] /= maximum;
        }
    }
    if (cancelRequested.load()) {
        throw std::runtime_error("Axial calculation cancelled.");
    }
    progress.percent = 100;
    if (onProgress) {
        onProgress(progress);
    }
    return axial;
}
