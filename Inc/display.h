/*
 * @file           : display.h
 * @brief          : 显示控制模块头文件
 */
#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdbool.h>
#include "main.h"
#include "SD3077.h"

/* 显示模式枚举 */
typedef enum {
    MODE_SHOW_TIME = 0,        /* 显示时间（时:分） */
    MODE_SHOW_SECOND,          /* 显示秒数 */
    MODE_SHOW_TEMPERTURE,      /* 显示温度 */
    MODE_SET_HOUR,             /* 设置小时 */
    MODE_SET_MINUTE,           /* 设置分钟 */
    MODE_SET_ALARM_ENABLE,     /* 设置闹钟开关 */
    MODE_SET_ALARM_HOUR,       /* 设置闹钟小时 */
    MODE_SET_ALARM_MINUTE,     /* 设置闹钟分钟 */
    MODE_SET_TEMP_SHOW,        /* 设置温度显示时长 */
    MODE_SET_TEMP_HIDE,        /* 设置温度隐藏时长 */
    MODE_SET_BRIGHTNESS,       /* 设置亮度（手动/自动） */
    MODE_SET_BRIGHTNESS_STRONG,/* 设置强光亮度 */
    MODE_SET_BRIGHTNESS_WEAK,  /* 设置弱光亮度 */
    MODE_SET_ROT_ENABLE,       /* 设置整点报时开关 */
    MODE_SET_ROT_START,        /* 设置整点报时开始时间 */
    MODE_SET_ROT_STOP,         /* 设置整点报时结束时间 */
} DisplayMode;

/* 显示控制函数 */
void Display_Init(void);
void Display_ShowTime(DateTime *time, bool blink);
void Display_ShowSecond(DateTime *time);
void Display_ShowTemperature(uint8_t temp);
void Display_ShowSettings(DisplayMode mode, DateTime *time, bool blink);
void Display_SetBrightness(uint8_t level);
void Display_Clear(void);

/* 获取/设置当前显示模式 */
DisplayMode Display_GetMode(void);
void Display_SetMode(DisplayMode mode);

/* 闪烁控制 */
void Display_SetBlink(bool blink);
bool Display_GetBlink(void);

/* 时间获取/设置（用于设置模式） */
void Display_GetTime(DateTime *time);
void Display_SetTime(DateTime *time);

#endif /* __DISPLAY_H */
