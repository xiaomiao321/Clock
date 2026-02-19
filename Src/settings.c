/*
 * @file           : settings.c
 * @brief          : 系统设置管理源文件
 */
#include "settings.h"
#include "TM1637.h"

/* 全局设置变量 */
static SystemSettings g_Settings;

/**
 * @brief  获取设置结构体指针
 */
SystemSettings* Settings_Get(void) {
    return &g_Settings;
}

/**
 * @brief  从 RTC 备份寄存器读取设置
 */
void Settings_ReadBackup(void) {
    uint8_t data[BAK_DATA_SIZE];
    ReadBackData(0, data, BAK_DATA_SIZE);

    g_Settings.isAlarmEnabled = data[BAK_ALARM_ENABLED_INDEX];
    g_Settings.alarmHour = data[BAK_ALARM_HOUR_INDEX];
    g_Settings.alarmMin = data[BAK_ALARM_MINUTE_INDEX];
    g_Settings.tempertureShowTime = data[BAK_TEMP_SHOW_TIME_INDEX];
    g_Settings.tempertureHideTime = data[BAK_TEMP_HIDE_TIME_INDEX];
    g_Settings.isRingOnTimeEnabled = data[BAK_ROT_ENABLED_INDEX];
    g_Settings.ringOnTimeStart = data[BAK_ROT_START_INDEX];
    g_Settings.ringOnTimeStop = data[BAK_ROT_STOP_INDEX];
    g_Settings.savedBrightness = data[BAK_BRIGHTNESS_INDEX];
    g_Settings.strongBrightness = data[BAK_BRIGHTNESS_STRONG_INDEX];
    g_Settings.weakBrightness = data[BAK_BRIGHTNESS_WEAK_INDEX];
}

/**
 * @brief  保存所有设置到 RTC 备份寄存器
 */
void Settings_Save(void) {
    uint8_t data[BAK_DATA_SIZE] = {
        POWER_DOWN_IND_DATA,          /* [0] 掉电标识 */
        POWER_DOWN_IND_DATA,          /* [1] 掉电标识（冗余） */
        g_Settings.isAlarmEnabled,    /* [2] 闹钟使能标志 */
        g_Settings.alarmHour,         /* [3] 闹钟小时 */
        g_Settings.alarmMin,          /* [4] 闹钟分钟 */
        g_Settings.tempertureShowTime,/* [5] 温度显示时长 */
        g_Settings.tempertureHideTime,/* [6] 温度隐藏时长 */
        g_Settings.isRingOnTimeEnabled,/* [7] 整点报时使能标志 */
        g_Settings.ringOnTimeStart,   /* [8] 整点报时开始时间 */
        g_Settings.ringOnTimeStop,    /* [9] 整点报时结束时间 */
        g_Settings.savedBrightness,   /* [10] 亮度设置 */
        g_Settings.strongBrightness,  /* [11] 强光亮度 */
        g_Settings.weakBrightness,    /* [12] 弱光亮度 */
    };
    WriteBackData(0, data, BAK_DATA_SIZE);
}

/**
 * @brief  重置所有设置为默认值
 */
void Settings_ResetToDefault(void) {
    g_Settings.isAlarmEnabled = false;
    g_Settings.alarmHour = 0;
    g_Settings.alarmMin = 0;
    g_Settings.tempertureShowTime = 2;      /* 温度显示 2 秒 */
    g_Settings.tempertureHideTime = 10;     /* 温度隐藏 10 秒 */
    g_Settings.isRingOnTimeEnabled = false;
    g_Settings.ringOnTimeStart = 8;         /* 8:00-20:00 */
    g_Settings.ringOnTimeStop = 20;
    g_Settings.savedBrightness = 8;         /* 默认手动最高亮度 */
    g_Settings.strongBrightness = 8;
    g_Settings.weakBrightness = 1;
}

/**
 * @brief  验证设置数据是否合法
 * @retval true-合法，false-非法
 */
bool Settings_Validate(void) {
    if (g_Settings.alarmHour > 23 || g_Settings.alarmMin > 59) {
        return false;
    }
    if (g_Settings.ringOnTimeStart > 23 || g_Settings.ringOnTimeStop > 23) {
        return false;
    }
    if (g_Settings.savedBrightness > 8 || 
        g_Settings.strongBrightness > 8 || g_Settings.strongBrightness == 0 ||
        g_Settings.weakBrightness > 8 || g_Settings.weakBrightness == 0) {
        return false;
    }
    return true;
}

/**
 * @brief  设置初始化
 */
void Settings_Init(void) {
    uint8_t backupData[BAK_DATA_SIZE];
    ReadBackData(BAK_POWER_DOWN_IND_INDEX, backupData, BAK_DATA_SIZE);

    /* 检查是否为首次上电 */
    if (backupData[0] != POWER_DOWN_IND_DATA &&
        backupData[1] != POWER_DOWN_IND_DATA) {
        /* 首次上电，重置设置并初始化 RTC 时间 */
        DateTime time = {0};
        time.year = 15;
        time.month = 1;
        time.dayOfMonth = 1;
        time.dayOfWeek = 1;
        time.hours = 0;
        time.minutes = 0;
        time.ampm = HOUR24;
        time.seconds = 0;
        SetTime(&time);

        Settings_ResetToDefault();
        Settings_Save();
    } else {
        Settings_ReadBackup();

        /* 验证数据合法性 */
        if (!Settings_Validate()) {
            Settings_ResetToDefault();
            Settings_Save();
        }
    }
}
