#pragma once
#include "SlmTypes.h"

// 质量评价接口：比较两张范围为 0 到 1 的光强图。
double ComputeMse(const GrayImage& expected, const GrayImage& actual);
double ComputePsnr(double mse);
