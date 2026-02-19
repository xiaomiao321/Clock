/*
 * @file           : alarm.c
 * @brief          : 闹钟控制模块源文件
 */
#include "alarm.h"
#include "gpio.h"

/* 蜂鸣器 GPIO 定义 */
#define BUZZER_GPIO_PORT    BUZZER_GPIO_Port
#define BUZZER_PIN          BUZZER_Pin

/* 闹钟响铃参数 */
#define ALARM_ON_TIME       50      /* 鸣叫时长 (ms) */
#define ALARM_OFF_TIME      50      /* 间隔时长 (ms) */
#define ALARM_REST_TIME     500     /* 一组响铃后的休息时间 (ms) */
#define ALARM_RING_COUNT    4       /* 每组响铃次数 */

/* 全局闹钟状态 */
static AlarmState g_Alarm = {0};
static uint32_t alarmTimestamp = 0;
static uint8_t alarmBeepCount = 0;

/**
 * @brief  闹钟模块初始化
 */
void Alarm_Init(void) {
    g_Alarm.isEnabled = false;
    g_Alarm.isAlarmed = false;
    g_Alarm.isAlarming = false;
    g_Alarm.hour = 0;
    g_Alarm.minute = 0;
    alarmTimestamp = 0;
    alarmBeepCount = 0;
}

/**
 * @brief  启动闹钟响铃
 */
void Alarm_Start(void) {
    g_Alarm.isAlarming = true;
    g_Alarm.isAlarmed = true;
    HAL_TIM_Base_Start_IT(&ALARM_CONTROL_TIMER_HANDLE);
}

/**
 * @brief  停止闹钟响铃
 */
void Alarm_Stop(void) {
    g_Alarm.isAlarming = false;
    HAL_TIM_Base_Stop_IT(&ALARM_CONTROL_TIMER_HANDLE);
    HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_SET);
}

/**
 * @brief  闹钟定时器中断回调（响铃控制）
 */
void Alarm_TimerTick(void) {
    alarmTimestamp++;

    if (HAL_GPIO_ReadPin(BUZZER_GPIO_PORT, BUZZER_PIN) == GPIO_PIN_RESET) {
        /* 蜂鸣器开启中，检查是否关闭 */
        if (alarmTimestamp > ALARM_ON_TIME) {
            HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_SET);
            alarmBeepCount++;
            alarmTimestamp = 0;
        }
    } else {
        /* 蜂鸣器关闭中 */
        if (alarmBeepCount < ALARM_RING_COUNT) {
            /* 继续响铃 */
            if (alarmTimestamp > ALARM_OFF_TIME) {
                HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
                alarmTimestamp = 0;
            }
        } else if (alarmTimestamp > ALARM_REST_TIME) {
            /* 一组响铃结束，重置 */
            HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
            alarmTimestamp = 0;
            alarmBeepCount = 0;
        }
    }
}

/**
 * @brief  检查是否触发闹钟
 * @param  time: 当前时间
 */
void Alarm_Check(DateTime *time) {
    if (g_Alarm.isEnabled && !g_Alarm.isAlarmed &&
        g_Alarm.hour == time->hours && g_Alarm.minute == time->minutes) {
        Alarm_Start();
    }

    /* 时间不匹配时重置已响铃标志 */
    if (g_Alarm.hour != time->hours || g_Alarm.minute != time->minutes) {
        if (g_Alarm.isAlarming) {
            Alarm_Stop();
        }
        g_Alarm.isAlarmed = false;
    }
}

/**
 * @brief  设置闹钟
 * @param  hour: 小时
 * @param  minute: 分钟
 * @param  enable: 是否启用
 */
void Alarm_Set(uint8_t hour, uint8_t minute, bool enable) {
    g_Alarm.hour = hour;
    g_Alarm.minute = minute;
    g_Alarm.isEnabled = enable;
}

/**
 * @brief  获取闹钟使能状态
 */
bool Alarm_IsEnabled(void) {
    return g_Alarm.isEnabled;
}

/**
 * @brief  获取闹钟正在响铃状态
 */
bool Alarm_IsAlarming(void) {
    return g_Alarm.isAlarming;
}

/**
 * @brief  检查闹钟是否设置在指定时间
 */
bool Alarm_IsAlarmSet(uint8_t hour, uint8_t minute) {
    return (g_Alarm.hour == hour && g_Alarm.minute == minute);
}
