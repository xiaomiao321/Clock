/*
 * @file           : display.c
 * @brief          : 显示控制模块源文件
 */
#include "display.h"
#include "TM1637.h"
#include "settings.h"

/* 全局显示状态 */
static DisplayMode g_CurrentMode = MODE_SHOW_TIME;
static bool g_BlinkControl = false;

/**
 * @brief  显示初始化
 */
void Display_Init(void) {
    TM1637Init();
    SystemSettings *settings = Settings_Get();
    uint8_t brightness = settings->savedBrightness != 0 ? 
                         settings->savedBrightness : STRONG_BRIGHTNESS_VALUE;
    TM1637SetBrightness(brightness);
}

/**
 * @brief  显示时间（时:分）
 * @param  time: 当前时间
 * @param  blink: 是否闪烁
 */
void Display_ShowTime(DateTime *time, bool blink) {
    uint16_t number = time->hours * 100 + time->minutes;
    uint8_t colon = blink ? 2 : 0xFF;
    TM1637ShowNumberRight(3, number, colon, 1);
}

/**
 * @brief  显示秒数
 * @param  time: 当前时间
 */
void Display_ShowSecond(DateTime *time) {
    TM1637SetChar(0, ' ', false);
    TM1637SetChar(1, ' ', true);
    TM1637SetChar(2, time->seconds / 10 + '0', false);
    TM1637SetChar(3, time->seconds % 10 + '0', false);
}

/**
 * @brief  显示温度
 * @param  temp: 温度值
 */
void Display_ShowTemperature(uint8_t temp) {
    if (temp > 99) {
        temp = 99;
    }
    TM1637SetChar(0, ' ', false);
    TM1637SetChar(1, temp / 10 + '0', false);
    TM1637SetChar(2, temp % 10 + '0', false);
    TM1637SetChar(3, 'C', false);
}

/**
 * @brief  显示设置界面
 * @param  mode: 当前设置模式
 * @param  time: 当前时间
 * @param  blink: 是否闪烁
 */
void Display_ShowSettings(DisplayMode mode, DateTime *time, bool blink) {
    SystemSettings *settings = Settings_Get();
    char disp[5] = {0};

    switch (mode) {
    case MODE_SET_HOUR:
        disp[0] = blink ? ' ' : (time->hours / 10 + '0');
        disp[1] = blink ? ' ' : (time->hours % 10 + '0');
        disp[2] = time->minutes / 10 + '0';
        disp[3] = time->minutes % 10 + '0';
        break;

    case MODE_SET_MINUTE:
        disp[0] = time->hours / 10 + '0';
        disp[1] = time->hours % 10 + '0';
        disp[2] = blink ? ' ' : time->minutes / 10 + '0';
        disp[3] = blink ? ' ' : time->minutes % 10 + '0';
        break;

    case MODE_SET_ALARM_ENABLE:
        disp[0] = 'A';
        disp[1] = 'L';
        disp[2] = blink ? ' ' : 'o';
        disp[3] = blink ? ' ' : (settings->isAlarmEnabled ? 'n' : 'F');
        break;

    case MODE_SET_ALARM_HOUR:
        disp[0] = blink ? ' ' : (settings->alarmHour / 10 + '0');
        disp[1] = blink ? ' ' : (settings->alarmHour % 10 + '0');
        disp[2] = settings->alarmMin / 10 + '0';
        disp[3] = settings->alarmMin % 10 + '0';
        break;

    case MODE_SET_ALARM_MINUTE:
        disp[0] = settings->alarmHour / 10 + '0';
        disp[1] = settings->alarmHour % 10 + '0';
        disp[2] = blink ? ' ' : settings->alarmMin / 10 + '0';
        disp[3] = blink ? ' ' : settings->alarmMin % 10 + '0';
        break;

    case MODE_SET_TEMP_SHOW:
        disp[0] = 'T';
        disp[1] = 'S';
        disp[2] = blink ? ' ' : settings->tempertureShowTime / 10 + '0';
        disp[3] = blink ? ' ' : settings->tempertureShowTime % 10 + '0';
        break;

    case MODE_SET_TEMP_HIDE:
        disp[0] = 'T';
        disp[1] = 'H';
        disp[2] = blink ? ' ' : settings->tempertureHideTime / 10 + '0';
        disp[3] = blink ? ' ' : settings->tempertureHideTime % 10 + '0';
        break;

    case MODE_SET_BRIGHTNESS:
        disp[0] = 'b';
        disp[1] = 'r';
        if (settings->savedBrightness) {
            disp[2] = blink ? ' ' : '0';
            disp[3] = blink ? ' ' : (settings->savedBrightness + '0');
        } else {
            disp[2] = 'A';
            disp[3] = 'U';
        }
        break;

    case MODE_SET_BRIGHTNESS_STRONG:
        disp[0] = 'b';
        disp[1] = '1';
        disp[2] = blink ? ' ' : '0';
        disp[3] = blink ? ' ' : (settings->strongBrightness + '0');
        break;

    case MODE_SET_BRIGHTNESS_WEAK:
        disp[0] = 'b';
        disp[1] = '2';
        disp[2] = blink ? ' ' : '0';
        disp[3] = blink ? ' ' : (settings->weakBrightness + '0');
        break;

    case MODE_SET_ROT_ENABLE:
        disp[0] = 'r';
        disp[1] = 'o';
        disp[2] = blink ? ' ' : 'o';
        disp[3] = blink ? ' ' : (settings->isRingOnTimeEnabled ? 'n' : 'F');
        break;

    case MODE_SET_ROT_START:
        disp[0] = 'r';
        disp[1] = 'S';
        disp[2] = blink ? ' ' : settings->ringOnTimeStart / 10 + '0';
        disp[3] = blink ? ' ' : settings->ringOnTimeStart % 10 + '0';
        break;

    case MODE_SET_ROT_STOP:
        disp[0] = 'r';
        disp[1] = 'e';
        disp[2] = blink ? ' ' : settings->ringOnTimeStop / 10 + '0';
        disp[3] = blink ? ' ' : settings->ringOnTimeStop % 10 + '0';
        break;

    default:
        break;
    }

    TM1637SetChar(0, disp[0], false);
    TM1637SetChar(1, disp[1], true);
    TM1637SetChar(2, disp[2], false);
    TM1637SetChar(3, disp[3], false);
}

/**
 * @brief  设置亮度
 */
void Display_SetBrightness(uint8_t level) {
    TM1637SetBrightness(level);
}

/**
 * @brief  清空显示
 */
void Display_Clear(void) {
    for (uint8_t i = 0; i < 4; i++) {
        TM1637SetChar(i, ' ', false);
    }
}

/* 模式管理 */
DisplayMode Display_GetMode(void) {
    return g_CurrentMode;
}

void Display_SetMode(DisplayMode mode) {
    g_CurrentMode = mode;
}

void Display_SetBlink(bool blink) {
    g_BlinkControl = blink;
}

bool Display_GetBlink(void) {
    return g_BlinkControl;
}

/* 全局显示时间（用于设置模式） */
static DateTime g_DisplayTime = {0};

/**
 * @brief  获取显示时间
 */
void Display_GetTime(DateTime *time) {
    *time = g_DisplayTime;
}

/**
 * @brief  设置显示时间
 */
void Display_SetTime(DateTime *time) {
    g_DisplayTime = *time;
}
