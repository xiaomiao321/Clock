/*
 * @file           : temperature.c
 * @brief          : 温度检测模块源文件
 *                   NTC 温度传感器 ADC 值转换为温度
 */
#include "temperature.h"

/* 温度映射表：ADC 值到温度值 (0-125℃) */
static const uint16_t tempertureMap[] = {
    1054, 1091, 1128, 1165, 1203, 1242, 1280, 1320, 1359, 1399, 1439, 1479,
    1520, 1560, 1601, 1642, 1683, 1724, 1765, 1805, 1846, 1887, 1927, 1968,
    2008, 2048, 2087, 2126, 2165, 2204, 2242, 2280, 2318, 2355, 2391, 2427,
    2463, 2498, 2533, 2567, 2601, 2634, 2667, 2699, 2730, 2761, 2791, 2821,
    2850, 2879, 2907, 2935, 2962, 2988, 3014, 3040, 3064, 3089, 3113, 3136,
    3158, 3181, 3202, 3224, 3244, 3264, 3284, 3303, 3322, 3341, 3359, 3376,
    3393, 3410, 3426, 3442, 3457, 3472, 3487, 3501, 3515, 3529, 3542, 3555,
    3568, 3580, 3592, 3603, 3615, 3626, 3637, 3647, 3657, 3667, 3677, 3687,
    3696, 3705, 3714, 3722, 3730, 3739, 3747, 3754, 3762, 3769, 3776, 3783,
    3790, 3797, 3803, 3809, 3815, 3821, 3827, 3833, 3839, 3844, 3849, 3854,
    3859, 3864, 3869, 3874, 3878, 3883,
};

/* 温度缓存 */
static uint16_t tempBuffer[TEMP_BUFFER_SIZE + 1];
static uint8_t tempBuffered = 0;
static uint8_t currentTemp = 25;

/**
 * @brief  温度模块初始化
 */
void Temperature_Init(void) {
    tempBuffered = 0;
    currentTemp = 25;
    for (uint8_t i = 0; i <= TEMP_BUFFER_SIZE; i++) {
        tempBuffer[i] = 0;
    }
}

/**
 * @brief  更新温度值（基于 ADC 采样值）
 * @param  adcValue: ADC 采样值
 */
void Temperature_Update(uint32_t adcValue) {
    uint32_t avgValue;

    if (tempBuffered < TEMP_BUFFER_SIZE) {
        tempBuffer[tempBuffered++] = (uint16_t)adcValue;
        avgValue = adcValue;
    } else {
        tempBuffer[TEMP_BUFFER_SIZE] = (uint16_t)adcValue;
        avgValue = 0;
        /* 滑动窗口平均滤波 */
        for (uint8_t i = 0; i < TEMP_BUFFER_SIZE; i++) {
            tempBuffer[i] = tempBuffer[i + 1];
            avgValue += tempBuffer[i];
        }
        avgValue = avgValue / TEMP_BUFFER_SIZE;
    }

    /* 查表法获取温度值 */
    for (uint8_t i = 0; i < TEMP_MAP_SIZE; i++) {
        if (avgValue <= tempertureMap[i]) {
            currentTemp = i;
            return;
        }
    }
    currentTemp = 0;
}

/**
 * @brief  获取当前温度值
 * @retval 温度值 (0-99℃)
 */
uint8_t Temperature_Get(void) {
    if (currentTemp > 99) {
        return 99;
    }
    return currentTemp;
}
