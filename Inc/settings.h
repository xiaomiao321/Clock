/*
 * @file           : settings.h
 * @brief          : 系统设置管理头文件
 *                   管理闹钟、温度显示、整点报时、亮度等设置
 */
#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <stdbool.h>
#include "main.h"
#include "SD3077.h"

/* 备份数据存储索引 */
#define BAK_DATA_SIZE              13
#define BAK_POWER_DOWN_IND_INDEX   0x00
#define BAK_ALARM_ENABLED_INDEX    0x02
#define BAK_ALARM_HOUR_INDEX       0x03
#define BAK_ALARM_MINUTE_INDEX     0x04
#define BAK_TEMP_SHOW_TIME_INDEX   0x05
#define BAK_TEMP_HIDE_TIME_INDEX   0x06
#define BAK_ROT_ENABLED_INDEX      0x07
#define BAK_ROT_START_INDEX        0x08
#define BAK_ROT_STOP_INDEX         0x09
#define BAK_BRIGHTNESS_INDEX       0x0A
#define BAK_BRIGHTNESS_STRONG_INDEX 0x0B
#define BAK_BRIGHTNESS_WEAK_INDEX  0x0C

#define POWER_DOWN_IND_DATA        0xFA

/* 设置范围定义 */
#define TEMPERTURE_MAX_SHOW_TIME   15
#define TEMPERTURE_MAX_HIDE_TIME   30

/* 亮度设置 */
#define STRONG_BRIGHTNESS_VALUE    8
#define WEAK_BRIGHTNESS_VALUE      1

/* 设置数据结构 */
typedef struct {
    bool isAlarmEnabled;
    uint8_t alarmHour;
    uint8_t alarmMin;
    uint8_t tempertureShowTime;
    uint8_t tempertureHideTime;
    bool isRingOnTimeEnabled;
    uint8_t ringOnTimeStart;
    uint8_t ringOnTimeStop;
    uint8_t savedBrightness;
    uint8_t strongBrightness;
    uint8_t weakBrightness;
} SystemSettings;

/* 函数声明 */
void Settings_Init(void);
void Settings_ReadBackup(void);
void Settings_Save(void);
void Settings_ResetToDefault(void);
bool Settings_Validate(void);

/* 获取/设置函数 */
SystemSettings* Settings_Get(void);

#endif /* __SETTINGS_H */
