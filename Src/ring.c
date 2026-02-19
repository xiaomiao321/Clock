/*
 * @file           : ring.c
 * @brief          : 整点报时模块源文件
 * @note           新逻辑：从每小时的 59 分 55 秒开始，每秒响一次短音（100ms）
 *                   一直到下个小时 00 分 00 秒，共 6 次
 *                   最后一次（00:00）响长音（1000ms）
 */
#include "ring.h"
#include "gpio.h"
#include "alarm.h"

/* 蜂鸣器 GPIO 定义 */
#define BUZZER_GPIO_PORT    BUZZER_GPIO_Port
#define BUZZER_PIN          BUZZER_Pin

/* 全局整点报时状态 */
static RingState g_Ring = {0};

/**
 * @brief  整点报时模块初始化
 */
void Ring_Init(void) {
    g_Ring.isEnabled = false;
    g_Ring.startTime = 8;
    g_Ring.stopTime = 20;
    g_Ring.isChiming = false;
    g_Ring.lastChimeSecond = 0xFF;
    g_Ring.lastHour = 0;
    g_Ring.ringStartTime = 0;
}

/**
 * @brief  设置上次报时的小时
 */
void Ring_SetStartTime(uint8_t hour) {
    g_Ring.lastHour = hour;
}

/**
 * @brief  检查是否在报时时间范围内
 * @param  hour: 要检查的小时
 * @retval true-在范围内，false-不在范围内
 */
static bool IsInTimeRange(uint8_t hour) {
    if (g_Ring.startTime <= g_Ring.stopTime) {
        /* 正常时间段（如 8:00-20:00） */
        return (hour >= g_Ring.startTime && hour <= g_Ring.stopTime);
    } else {
        /* 跨天时间段（如 22:00-6:00） */
        return (hour >= g_Ring.startTime || hour <= g_Ring.stopTime);
    }
}

/**
 * @brief  检查并触发整点报时
 * @param  time: 当前时间
 */
void Ring_Check(DateTime *time) {
    if (!g_Ring.isEnabled) {
        return;
    }

    /* 整点报时逻辑：59 分 55 秒到 00 分 00 秒，共 6 次 */
    if (time->minutes == 59 && time->seconds >= 55) {
        /* 倒计时阶段：55 秒~59 秒（短音） */
        uint8_t nextHour = (time->hours + 1) % 24;

        /* 检查下一个小时是否在报时范围内 */
        if (!IsInTimeRange(nextHour)) {
            return;
        }

        /* 避免与闹钟冲突 */
        if (Alarm_IsEnabled() && Alarm_IsAlarmSet(nextHour, 0)) {
            return;
        }

        if (g_Ring.lastChimeSecond != time->seconds) {
            /* 短音：100ms */
            g_Ring.ringStartTime = HAL_GetTick();
            HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
            g_Ring.isChiming = true;
            g_Ring.lastChimeSecond = time->seconds;
        }
    } else if (time->minutes == 0 && time->seconds == 0) {
        /* 整点：00 分 00 秒（长音） */
        if (!IsInTimeRange(time->hours)) {
            return;
        }

        /* 避免与闹钟冲突 */
        if (Alarm_IsEnabled() && Alarm_IsAlarmSet(time->hours, 0)) {
            return;
        }

        if (g_Ring.lastChimeSecond != 0) {
            /* 长音：1000ms */
            g_Ring.ringStartTime = HAL_GetTick();
            HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
            g_Ring.isChiming = true;
            g_Ring.lastChimeSecond = 0;
            g_Ring.lastHour = time->hours;
        }
    } else {
        /* 其他时间，重置秒数标记 */
        if (time->seconds > 0) {
            g_Ring.lastChimeSecond = 0xFF;
        }
    }
}

/**
 * @brief  更新整点报时状态（蜂鸣器控制）
 */
void Ring_Update(void) {
    if (!g_Ring.isChiming) {
        return;
    }

    if (HAL_GPIO_ReadPin(BUZZER_GPIO_PORT, BUZZER_PIN) == GPIO_PIN_RESET) {
        uint32_t chimeDuration;
        DateTime time;
        TimeNow(&time);

        /* 判断是短音还是长音 */
        if (time.minutes == 0 && time.seconds == 0) {
            chimeDuration = RING_ON_TIME_LONG;    /* 整点长音 */
        } else {
            chimeDuration = RING_ON_TIME_SHORT;   /* 倒计时短音 */
        }

        uint32_t now = HAL_GetTick();
        if (now < g_Ring.ringStartTime || 
            (now - g_Ring.ringStartTime >= chimeDuration)) {
            HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_SET);
            g_Ring.isChiming = false;
        }
    }
}

/**
 * @brief  设置整点报时
 * @param  start: 开始时间
 * @param  stop: 结束时间
 * @param  enable: 是否启用
 */
void Ring_Set(uint8_t start, uint8_t stop, bool enable) {
    g_Ring.startTime = start;
    g_Ring.stopTime = stop;
    g_Ring.isEnabled = enable;
}

/**
 * @brief  获取整点报时使能状态
 */
bool Ring_IsEnabled(void) {
    return g_Ring.isEnabled;
}

/**
 * @brief  获取正在报时状态
 */
bool Ring_IsChiming(void) {
    return g_Ring.isChiming;
}

/**
 * @brief  获取报时开始时间戳
 */
uint32_t Ring_GetStartTime(void) {
    return g_Ring.ringStartTime;
}
