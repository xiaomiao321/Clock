/*
 * @file           : alarm.h
 * @brief          : 闹钟控制模块头文件
 */
#ifndef __ALARM_H
#define __ALARM_H

#include <stdbool.h>
#include "main.h"
#include "SD3077.h"
#include "tim.h"

/* 闹钟控制定时器 */
#define ALARM_CONTROL_TIMER_HANDLE  htim17
#define ALARM_CONTROL_TIMER         TIM17

/* 闹钟状态 */
typedef struct {
    bool isEnabled;
    bool isAlarmed;
    bool isAlarming;
    uint8_t hour;
    uint8_t minute;
} AlarmState;

/* 函数声明 */
void Alarm_Init(void);
void Alarm_Start(void);
void Alarm_Stop(void);
void Alarm_TimerTick(void);
void Alarm_Check(DateTime *time);
void Alarm_Set(uint8_t hour, uint8_t minute, bool enable);
bool Alarm_IsEnabled(void);
bool Alarm_IsAlarming(void);
bool Alarm_IsAlarmSet(uint8_t hour, uint8_t minute);

#endif /* __ALARM_H */
