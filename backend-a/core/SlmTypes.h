#pragma once
#include <atomic>
#include <functional>
#include <string>
#include <vector>

// 公共数据结构：与分工文档中的图像、参数、结果和进度接口保持一致。
struct GrayImage {
    int width = 0;
    int height = 0;
    std::vector<double> data;
};

enum class SolverMode {
    GS,
    AiFastSearch
};

struct SimulationConfig {
    int size = 256;
    int iterations = 80;
    int axialSlices = 61;
    double wavelength_m = 532e-9;
    double pixelPitch_m = 8e-6;
    double distance_m = 0.30;
    SolverMode mode = SolverMode::GS;
};

struct SimulationResult {
    GrayImage phase;
    GrayImage reconstruction;
    GrayImage axial;
    double mse = 0.0;
    double psnr = 0.0;
};

struct ProgressInfo {
    int percent = 0;
    int iteration = 0;
    std::string stage;
};

using ProgressCallback = std::function<void(const ProgressInfo&)>;

SimulationResult RunSimulation(
    const GrayImage& target,
    const SimulationConfig& config,
    std::atomic_bool& cancelRequested,
    ProgressCallback onProgress);
