/*
 * @file           : temperature.h
 * @brief          : 温度检测模块头文件
 */
#ifndef __TEMPERATURE_H
#define __TEMPERATURE_H

#include "main.h"

/* 温度相关常量 */
#define TEMP_BUFFER_SIZE    8
#define TEMP_MAP_SIZE       126

/* 函数声明 */
void Temperature_Init(void);
void Temperature_Update(uint32_t adcValue);
uint8_t Temperature_Get(void);

#endif /* __TEMPERATURE_H */
