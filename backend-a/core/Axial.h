#pragma once
#include "Asm.h"

// 轴向剖面接口：在指定距离区间采样中心横截线，并上报进度、响应取消。
GrayImage ComputeAxialSection(
    const std::vector<std::complex<double>>& field,
    int size, double pixelPitch_m, double wavelength_m,
    double startDistance_m, double endDistance_m, int slices,
    std::atomic_bool& cancelRequested,
    ProgressCallback onProgress = ProgressCallback());
