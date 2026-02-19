/*
 * @file           : app.h
 * @brief          : 应用层主模块头文件
 *                   整合各功能模块，管理全局应用状态
 */
#ifndef __APP_H
#define __APP_H

#include <stdbool.h>
#include "main.h"
#include "SD3077.h"
#include "settings.h"
#include "display.h"
#include "temperature.h"
#include "alarm.h"
#include "ring.h"

/* ADC 定义 */
#define ADC_CHANNEL_COUNT   2
#define STRONG_BRIGHTNESS_ADC_VALUE   2800
#define WEAK_BRIGHTNESS_ADC_VALUE     2300

/* 亮度控制定时器 */
#define LIGHT_CONTROL_TIMER_HANDLE    htim16
#define LIGHT_CONTROL_TIMER           TIM16

/* 温度显示切换时间 (秒) */
#define TEMP_SHOW_HIDE_TIME_SEC       1000    /* 转换为 ms */

/* 按键时间参数 (ms) */
#define KEY_LONG_PRESS_EFFECT_TIME    800
#define KEY_REPEAT_TIME_INTERVAL      250
#define KEY_CLICK_EFFECT_TIME         50

/* GPIO 引脚定义 */
#define MODE_KEY_GPIO_PORT            MODE_KEY_GPIO_Port
#define MODE_KEY_PIN                  MODE_KEY_Pin
#define SET_KEY_GPIO_PORT             SET_KEY_GPIO_Port
#define SET_KEY_PIN                   SET_KEY_Pin

/* 应用状态结构 */
typedef struct {
    bool isInitCompleted;
    bool isWeakBrightness;
    uint32_t lastDisplayChangeTime;
    uint32_t adcValue[ADC_CHANNEL_COUNT];
    DateTime currentTime;
    DateTime lastTime;
    
    /* 按键时间记录 */
    uint32_t lastModeKeyPressTime;
    uint32_t lastSetKeyPressTime;
    uint32_t lastSetKeyPressReportTime;
    bool setKeyRepeatReported;
} AppState;

/* 函数声明 */
void App_Init(void);
void App_MainLoop(void);
void App_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void App_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* 获取应用状态 */
AppState* App_GetState(void);

/* 全局应用状态外部声明 */
extern AppState g_App;

#endif /* __APP_H */
