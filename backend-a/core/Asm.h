#pragma once
#include "SlmTypes.h"
#include <complex>
#include <vector>

// 光场接口：按米制参数传播复振幅，并生成最大值归一化的光强图。
std::vector<std::complex<double>> PropagateAsm(
    std::vector<std::complex<double>> field,
    int size, double pixelPitch_m,
    double wavelength_m, double distance_m);

GrayImage IntensityOf(
    const std::vector<std::complex<double>>& field, int size);
