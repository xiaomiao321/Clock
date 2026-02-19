/*
 * @file           : ring.h
 * @brief          : 整点报时模块头文件
 */
#ifndef __RING_H
#define __RING_H

#include <stdbool.h>
#include "main.h"
#include "SD3077.h"

/* 整点报时参数 */
#define RING_ON_TIME_LONG   1000    /* 整点长音时长 (ms) */
#define RING_ON_TIME_SHORT  100     /* 倒计时短音时长 (ms) */

/* 整点报时状态 */
typedef struct {
    bool isEnabled;           /* 整点报时使能 */
    uint8_t startTime;        /* 报时开始时间 */
    uint8_t stopTime;         /* 报时结束时间 */
    bool isChiming;           /* 正在报时标志 */
    uint8_t lastChimeSecond;  /* 上次报时的秒数 */
    uint8_t lastHour;         /* 上次报时的小时 */
    uint32_t ringStartTime;   /* 报时开始时间戳 */
} RingState;

/* 函数声明 */
void Ring_Init(void);
void Ring_Check(DateTime *time);
void Ring_Update(void);
void Ring_Set(uint8_t start, uint8_t stop, bool enable);
void Ring_SetStartTime(uint8_t hour);
bool Ring_IsEnabled(void);
bool Ring_IsChiming(void);
uint32_t Ring_GetStartTime(void);

#endif /* __RING_H */
