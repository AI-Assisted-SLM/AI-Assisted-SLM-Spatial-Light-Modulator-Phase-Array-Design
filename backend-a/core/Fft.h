#pragma once
#include <complex>
#include <vector>

// 傅里叶变换接口：支持一维和正方形二维数据，逆变换自动归一化。
void FFT1D(std::vector<std::complex<double>>& values, bool inverse);
void FFT2D(std::vector<std::complex<double>>& field, int size, bool inverse);
