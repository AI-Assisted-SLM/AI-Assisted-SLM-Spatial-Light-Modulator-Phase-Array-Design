#include "Asm.h"
#include "Axial.h"
#include "Metrics.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

// 示例辅助：将光强图存成 CSV，并在控制台显示轴向计算进度。
void SaveCsv(const GrayImage& image, const char* path) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Cannot open CSV output.");
    }
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            if (x > 0) {
                output << ',';
            }
            output << image.data[static_cast<std::size_t>(y) * image.width + x];
        }
        output << '\n';
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Cannot finish CSV output.");
    }
}

void ShowProgress(const ProgressInfo& progress) {
    std::cout << "\rAxial progress: " << progress.percent << "%   " << std::flush;
}

// 示例流程：高斯光束叠加透镜相位，计算焦平面与轴向剖面。
int main() {
    try {
        const double pi = 3.14159265358979323846;
        SimulationConfig config;
        config.distance_m = 0.10;
        config.axialSlices = 31;
        const int size = config.size;
        std::vector<std::complex<double>> field(static_cast<std::size_t>(size) * size);
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                const double px = (x - size / 2) * config.pixelPitch_m;
                const double py = (y - size / 2) * config.pixelPitch_m;
                const double radiusSquared = px * px + py * py;
                const double amplitude = std::exp(-radiusSquared / (0.00030 * 0.00030));
                const double phase = -pi * radiusSquared /
                    (config.wavelength_m * config.distance_m);
                field[static_cast<std::size_t>(y) * size + x] =
                    std::polar(amplitude, phase);
            }
        }
        const GrayImage input = IntensityOf(field, size);
        const GrayImage reconstruction = IntensityOf(
            PropagateAsm(field, size, config.pixelPitch_m,
                config.wavelength_m, config.distance_m), size);
        const GrayImage zeroDistance = IntensityOf(
            PropagateAsm(field, size, config.pixelPitch_m,
                config.wavelength_m, 0.0), size);
        const double mse = ComputeMse(input, zeroDistance);
        std::cout << "Zero-distance identity: MSE = " << mse
                  << ", PSNR = " << ComputePsnr(mse) << " dB\n";
        std::atomic_bool cancelRequested(false);
        const GrayImage axial = ComputeAxialSection(
            field, size, config.pixelPitch_m, config.wavelength_m,
            0.0, 0.20, config.axialSlices, cancelRequested, ShowProgress);
        SaveCsv(input, "input_intensity.csv");
        SaveCsv(reconstruction, "reconstruction.csv");
        SaveCsv(axial, "axial.csv");
        std::cout << "\nSaved input_intensity.csv, reconstruction.csv and axial.csv\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Demo failed: " << error.what() << '\n';
        return 1;
    }
}
