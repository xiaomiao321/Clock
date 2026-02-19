/*
 * @file           : app.c
 * @brief          : 应用层主模块源文件
 */
#include "app.h"
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* 外部变量声明 */
extern ADC_HandleTypeDef hadc;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim16;

/* 全局应用状态 */
AppState g_App = {0};

/* 函数声明 */
static void ModeKey_Clicked(void);
static void ModeKey_LongPressed(void);
static void SetKey_Clicked(void);
static void SetKey_Repeat(void);

/**
 * @brief  应用初始化
 */
void App_Init(void) {
    /* 初始化各模块 */
    Settings_Init();
    Display_Init();
    Temperature_Init();
    Alarm_Init();
    Ring_Init();

    /* 初始化应用状态 */
    g_App.isInitCompleted = false;
    g_App.isWeakBrightness = true;
    g_App.lastDisplayChangeTime = 0;
    g_App.lastModeKeyPressTime = 0;
    g_App.lastSetKeyPressTime = 0;
    g_App.lastSetKeyPressReportTime = 0;
    g_App.setKeyRepeatReported = false;

    /* ADC 和定时器初始化 */
    HAL_ADCEx_Calibration_Start(&hadc);
    HAL_ADC_Start_DMA(&hadc, g_App.adcValue, 2);
    HAL_TIM_Base_Start(&htim3);
    HAL_TIM_Base_Start_IT(&LIGHT_CONTROL_TIMER_HANDLE);

    /* 初始化 RTC 和时间 */
    DateTime time = {0};
    TimeNow(&time);
    g_App.lastTime = time;
    Ring_SetStartTime(time.hours);

    /* 启用秒中断 */
    EnableSencodInterruptOuput();

    g_App.lastDisplayChangeTime = HAL_GetTick();
    g_App.isInitCompleted = true;
}

/**
 * @brief  模式键单击处理
 */
static void ModeKey_Clicked(void) {
    if (Alarm_IsAlarming()) {
        Alarm_Stop();
        return;
    }

    DisplayMode mode = Display_GetMode();
    DateTime time;
    TimeNow(&time);

    switch (mode) {
    case MODE_SHOW_TIME:
    case MODE_SHOW_TEMPERTURE:
        Display_SetMode(MODE_SET_HOUR);
        Display_SetBlink(true);
        /* 保存当前 RTC 时间到 g_App.lastTime 用于设置 */
        g_App.lastTime = time;
        Display_SetTime(&g_App.lastTime);
        Display_ShowSettings(MODE_SET_HOUR, &g_App.lastTime, true);
        SetInterruptOuput(F_2_HZ);
        break;

    case MODE_SHOW_SECOND:
        time.seconds = 0;
        SetTime(&time);
        Display_ShowTime(&time, false);
        break;

    case MODE_SET_HOUR:
        Display_SetMode(MODE_SET_MINUTE);
        Display_SetBlink(true);
        Display_ShowSettings(MODE_SET_MINUTE, &g_App.lastTime, true);
        break;

    case MODE_SET_MINUTE:
        /* 保存设置的时间到 RTC */
        g_App.lastTime.seconds = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        SetTime(&g_App.lastTime);
        Display_SetMode(MODE_SET_ALARM_ENABLE);
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_ENABLE, &g_App.lastTime, false);
        break;

    case MODE_SET_ALARM_ENABLE:
        Display_SetMode(Alarm_IsEnabled() ? MODE_SET_ALARM_HOUR : MODE_SET_TEMP_SHOW);
        Display_SetBlink(true);
        Display_ShowSettings(Display_GetMode(), &g_App.lastTime, true);
        break;

    case MODE_SET_ALARM_HOUR:
        Display_SetMode(MODE_SET_ALARM_MINUTE);
        Display_SetBlink(true);
        Display_ShowSettings(MODE_SET_ALARM_MINUTE, &g_App.lastTime, true);
        break;

    case MODE_SET_ALARM_MINUTE:
        Display_SetMode(MODE_SET_TEMP_SHOW);
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_TEMP_SHOW, &g_App.lastTime, false);
        break;

    case MODE_SET_TEMP_SHOW:
        Display_SetMode(Settings_Get()->tempertureShowTime != 0 ? 
                        MODE_SET_TEMP_HIDE : MODE_SET_BRIGHTNESS);
        Display_SetBlink(false);
        Display_ShowSettings(Display_GetMode(), &g_App.lastTime, false);
        break;

    case MODE_SET_TEMP_HIDE:
        Display_SetMode(MODE_SET_BRIGHTNESS);
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_BRIGHTNESS, &g_App.lastTime, false);
        break;

    case MODE_SET_BRIGHTNESS:
        Display_SetMode(Settings_Get()->savedBrightness == 0 ? 
                        MODE_SET_BRIGHTNESS_STRONG : MODE_SET_ROT_ENABLE);
        Display_SetBlink(false);
        Display_ShowSettings(Display_GetMode(), &g_App.lastTime, false);
        break;

    case MODE_SET_BRIGHTNESS_STRONG:
        Display_SetMode(MODE_SET_BRIGHTNESS_WEAK);
        Display_SetBlink(false);
        Display_SetBrightness(Settings_Get()->weakBrightness);
        Display_ShowSettings(MODE_SET_BRIGHTNESS_WEAK, &g_App.lastTime, false);
        break;

    case MODE_SET_BRIGHTNESS_WEAK:
        Display_SetMode(MODE_SET_ROT_ENABLE);
        Display_SetBlink(false);
        Display_SetBrightness(Settings_Get()->strongBrightness);
        Display_ShowSettings(MODE_SET_ROT_ENABLE, &g_App.lastTime, false);
        break;

    case MODE_SET_ROT_ENABLE:
        Display_SetBlink(false);
        if (!Ring_IsEnabled()) {
            /* 保存设置的时间到 RTC */
            g_App.lastTime.seconds = 0;
            g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
            SetTime(&g_App.lastTime);
            Ring_SetStartTime(g_App.lastTime.hours);
            Settings_Save();
            Display_SetMode(MODE_SHOW_TIME);
            Display_ShowTime(&g_App.lastTime, false);
            EnableSencodInterruptOuput();
            g_App.lastDisplayChangeTime = HAL_GetTick();
        } else {
            Display_SetMode(MODE_SET_ROT_START);
            Display_ShowSettings(MODE_SET_ROT_START, &g_App.lastTime, false);
        }
        break;

    case MODE_SET_ROT_START:
        Display_SetMode(MODE_SET_ROT_STOP);
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_STOP, &g_App.lastTime, false);
        break;

    case MODE_SET_ROT_STOP:
        /* 保存设置的时间到 RTC */
        g_App.lastTime.seconds = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        SetTime(&g_App.lastTime);
        Ring_SetStartTime(g_App.lastTime.hours);
        Settings_Save();
        Display_SetMode(MODE_SHOW_TIME);
        Display_ShowTime(&g_App.lastTime, false);
        EnableSencodInterruptOuput();
        g_App.lastDisplayChangeTime = HAL_GetTick();
        break;

    default:
        break;
    }
}

/**
 * @brief  模式键长按处理
 */
static void ModeKey_LongPressed(void) {
    if (Alarm_IsAlarming()) {
        Alarm_Stop();
        return;
    }

    DisplayMode mode = Display_GetMode();
    DateTime time;

    if (mode >= MODE_SET_HOUR && mode <= MODE_SET_ROT_STOP) {
        if (mode == MODE_SET_HOUR || mode == MODE_SET_MINUTE) {
            /* 保存设置的时间到 RTC */
            g_App.lastTime.seconds = 0;
            g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
            SetTime(&g_App.lastTime);
        }
        Display_SetMode(MODE_SHOW_TIME);
        /* 从 RTC 读取最新时间并显示 */
        TimeNow(&time);
        Display_ShowTime(&time, false);
        g_App.lastDisplayChangeTime = HAL_GetTick();
    }
}

/**
 * @brief  设置键单击处理
 */
static void SetKey_Clicked(void) {
    if (Alarm_IsAlarming()) {
        Alarm_Stop();
        return;
    }

    DisplayMode mode = Display_GetMode();
    DateTime time;
    SystemSettings *settings = Settings_Get();

    if (mode == MODE_SHOW_TIME || mode == MODE_SHOW_TEMPERTURE) {
        Display_SetMode(MODE_SHOW_SECOND);
        TimeNow(&time);
        Display_ShowSecond(&time);
    } else if (mode == MODE_SHOW_SECOND) {
        Display_SetMode(MODE_SHOW_TIME);
        TimeNow(&time);
        Display_ShowTime(&time, false);
        g_App.lastDisplayChangeTime = HAL_GetTick();
    } else if (mode == MODE_SET_HOUR) {
        g_App.lastTime.hours++;
        if (g_App.lastTime.hours > 23) g_App.lastTime.hours = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        Display_SetBlink(false);
        /* 设置模式下只修改内存，不写入 RTC */
        Display_ShowSettings(MODE_SET_HOUR, &g_App.lastTime, false);
    } else if (mode == MODE_SET_MINUTE) {
        g_App.lastTime.minutes++;
        if (g_App.lastTime.minutes > 59) g_App.lastTime.minutes = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        Display_SetBlink(false);
        /* 设置模式下只修改内存，不写入 RTC */
        Display_ShowSettings(MODE_SET_MINUTE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ALARM_ENABLE) {
        settings->isAlarmEnabled = !settings->isAlarmEnabled;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_ENABLE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ALARM_HOUR) {
        settings->alarmHour++;
        if (settings->alarmHour > 23) settings->alarmHour = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_HOUR, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ALARM_MINUTE) {
        settings->alarmMin++;
        if (settings->alarmMin > 59) settings->alarmMin = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_MINUTE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_TEMP_SHOW) {
        settings->tempertureShowTime++;
        if (settings->tempertureShowTime > TEMPERTURE_MAX_SHOW_TIME) {
            settings->tempertureShowTime = settings->tempertureHideTime == 0 ? 1 : 0;
        }
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_TEMP_SHOW, &g_App.lastTime, false);
    } else if (mode == MODE_SET_TEMP_HIDE) {
        settings->tempertureHideTime++;
        if (settings->tempertureHideTime > TEMPERTURE_MAX_HIDE_TIME) {
            settings->tempertureHideTime = settings->tempertureShowTime == 0 ? 1 : 0;
        }
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_TEMP_HIDE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_BRIGHTNESS) {
        settings->savedBrightness++;
        if (settings->savedBrightness > 8) settings->savedBrightness = 0;
        Display_SetBrightness(settings->savedBrightness == 0 ? 1 : settings->savedBrightness);
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_BRIGHTNESS, &g_App.lastTime, false);
    } else if (mode == MODE_SET_BRIGHTNESS_STRONG) {
        settings->strongBrightness++;
        if (settings->strongBrightness > 8) settings->strongBrightness = 1;
        Display_SetBlink(false);
        Display_SetBrightness(settings->strongBrightness);
        Display_ShowSettings(MODE_SET_BRIGHTNESS_STRONG, &g_App.lastTime, false);
    } else if (mode == MODE_SET_BRIGHTNESS_WEAK) {
        settings->weakBrightness++;
        if (settings->weakBrightness > 8) settings->weakBrightness = 1;
        Display_SetBlink(false);
        Display_SetBrightness(settings->weakBrightness);
        Display_ShowSettings(MODE_SET_BRIGHTNESS_WEAK, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ROT_ENABLE) {
        settings->isRingOnTimeEnabled = !settings->isRingOnTimeEnabled;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_ENABLE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ROT_START) {
        settings->ringOnTimeStart++;
        if (settings->ringOnTimeStart > 23) settings->ringOnTimeStart = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_START, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ROT_STOP) {
        settings->ringOnTimeStop++;
        if (settings->ringOnTimeStop > 23) settings->ringOnTimeStop = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_STOP, &g_App.lastTime, false);
    }
}

/**
 * @brief  设置键重复触发处理
 */
static void SetKey_Repeat(void) {
    DisplayMode mode = Display_GetMode();
    SystemSettings *settings = Settings_Get();

    if (mode == MODE_SET_HOUR) {
        g_App.lastTime.hours++;
        if (g_App.lastTime.hours > 23) g_App.lastTime.hours = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        Display_SetBlink(false);
        /* 设置模式下只修改内存，不写入 RTC */
        Display_ShowSettings(MODE_SET_HOUR, &g_App.lastTime, false);
    } else if (mode == MODE_SET_MINUTE) {
        g_App.lastTime.minutes++;
        if (g_App.lastTime.minutes > 59) g_App.lastTime.minutes = 0;
        g_App.lastTime.ampm = HOUR24;  /* 确保 24 小时制 */
        Display_SetBlink(false);
        /* 设置模式下只修改内存，不写入 RTC */
        Display_ShowSettings(MODE_SET_MINUTE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ALARM_HOUR) {
        settings->alarmHour++;
        if (settings->alarmHour > 23) settings->alarmHour = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_HOUR, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ALARM_MINUTE) {
        settings->alarmMin++;
        if (settings->alarmMin > 59) settings->alarmMin = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ALARM_MINUTE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_TEMP_SHOW) {
        settings->tempertureShowTime++;
        if (settings->tempertureShowTime > TEMPERTURE_MAX_SHOW_TIME) {
            settings->tempertureShowTime = settings->tempertureHideTime == 0 ? 1 : 0;
        }
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_TEMP_SHOW, &g_App.lastTime, false);
    } else if (mode == MODE_SET_TEMP_HIDE) {
        settings->tempertureHideTime++;
        if (settings->tempertureHideTime > TEMPERTURE_MAX_HIDE_TIME) {
            settings->tempertureHideTime = settings->tempertureShowTime == 0 ? 1 : 0;
        }
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_TEMP_HIDE, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ROT_START) {
        settings->ringOnTimeStart++;
        if (settings->ringOnTimeStart > 23) settings->ringOnTimeStart = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_START, &g_App.lastTime, false);
    } else if (mode == MODE_SET_ROT_STOP) {
        settings->ringOnTimeStop++;
        if (settings->ringOnTimeStop > 23) settings->ringOnTimeStop = 0;
        Display_SetBlink(false);
        Display_ShowSettings(MODE_SET_ROT_STOP, &g_App.lastTime, false);
    }
}

/**
 * @brief  主循环处理
 */
void App_MainLoop(void) {
    SystemSettings *settings = Settings_Get();
    DisplayMode mode = Display_GetMode();
    uint32_t now = HAL_GetTick();
    uint32_t passedTime;

    /* 温度显示自动切换逻辑 */
    if (settings->tempertureShowTime > 0) {
        if (mode == MODE_SHOW_TIME) {
            passedTime = now - g_App.lastDisplayChangeTime;
            if (now < g_App.lastDisplayChangeTime ||
                passedTime >= (settings->tempertureHideTime * 1000)) {
                Display_SetMode(MODE_SHOW_TEMPERTURE);
                g_App.lastDisplayChangeTime = now;
            }
        } else if (mode == MODE_SHOW_TEMPERTURE && settings->tempertureHideTime > 0) {
            passedTime = now - g_App.lastDisplayChangeTime;
            if (now < g_App.lastDisplayChangeTime ||
                passedTime >= (settings->tempertureShowTime * 1000)) {
                Display_SetMode(MODE_SHOW_TIME);
                g_App.lastDisplayChangeTime = now;
            }
        }
    } else if (mode == MODE_SHOW_TEMPERTURE) {
        Display_SetMode(MODE_SHOW_TIME);
        g_App.lastDisplayChangeTime = now;
    }

    /* 设置键长按重复触发逻辑 */
    if (HAL_GPIO_ReadPin(SET_KEY_GPIO_PORT, SET_KEY_PIN) == GPIO_PIN_RESET &&
        mode >= MODE_SET_HOUR && mode <= MODE_SET_ROT_STOP) {
        uint32_t curVal = HAL_GetTick();
        uint32_t timePassed = curVal - g_App.lastSetKeyPressTime;
        if (timePassed > KEY_LONG_PRESS_EFFECT_TIME) {
            if (curVal - g_App.lastSetKeyPressReportTime > KEY_REPEAT_TIME_INTERVAL) {
                SetKey_Repeat();
                g_App.lastSetKeyPressReportTime = curVal;
            }
        }
    }
}

/**
 * @brief  定时器周期回调
 */
void App_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == ALARM_CONTROL_TIMER) {
        Alarm_TimerTick();
    } else if (htim->Instance == LIGHT_CONTROL_TIMER) {
        /* 亮度控制定时器中断（1ms） */
        SystemSettings *settings = Settings_Get();
        
        /* 更新温度（使用最新的 ADC 值） */
        Temperature_Update(g_App.adcValue[1]);
        
        /* 自动亮度调节 */
        if (settings->savedBrightness == 0) {
            if (g_App.isWeakBrightness && g_App.adcValue[0] > STRONG_BRIGHTNESS_ADC_VALUE) {
                g_App.isWeakBrightness = false;
                if (settings->strongBrightness > 0) {
                    Display_SetBrightness(settings->strongBrightness);
                }
            } else if (!g_App.isWeakBrightness && g_App.adcValue[0] < WEAK_BRIGHTNESS_ADC_VALUE) {
                g_App.isWeakBrightness = true;
                if (settings->weakBrightness > 0) {
                    Display_SetBrightness(settings->weakBrightness);
                }
            }
        }
    }
}

/**
 * @brief  GPIO 外部中断回调
 */
void App_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    DisplayMode mode = Display_GetMode();
    bool blink = Display_GetBlink();

    if (GPIO_Pin == SEC_INT_Pin && g_App.isInitCompleted) {
        if (mode == MODE_SHOW_TIME || mode == MODE_SHOW_SECOND ||
            mode == MODE_SHOW_TEMPERTURE) {
            TimeNow(&g_App.currentTime);
            g_App.lastTime = g_App.currentTime;

            if (mode == MODE_SHOW_TIME) {
                Display_ShowTime(&g_App.lastTime, blink);
            } else if (mode == MODE_SHOW_SECOND) {
                Display_ShowSecond(&g_App.lastTime);
            } else if (mode == MODE_SHOW_TEMPERTURE) {
                Display_ShowTemperature(Temperature_Get());
            }

            Alarm_Check(&g_App.currentTime);
            Ring_Check(&g_App.currentTime);
        } else if (mode >= MODE_SET_HOUR && mode <= MODE_SET_ROT_STOP) {
            /* 设置模式下，不更新 g_App.lastTime，只刷新显示 */
            Display_ShowSettings(mode, &g_App.lastTime, blink);
        }
        Display_SetBlink(!blink);
    } else if (GPIO_Pin == MODE_KEY_PIN) {
        if (HAL_GPIO_ReadPin(MODE_KEY_GPIO_PORT, MODE_KEY_PIN) == GPIO_PIN_RESET) {
            g_App.lastModeKeyPressTime = HAL_GetTick();
        } else {
            uint32_t currentVal = HAL_GetTick();
            if (g_App.lastModeKeyPressTime > currentVal) {
                ModeKey_Clicked();
            } else if (currentVal - g_App.lastModeKeyPressTime > KEY_LONG_PRESS_EFFECT_TIME) {
                ModeKey_LongPressed();
            } else if (currentVal - g_App.lastModeKeyPressTime > KEY_CLICK_EFFECT_TIME) {
                ModeKey_Clicked();
            }
        }
    } else if (GPIO_Pin == SET_KEY_PIN) {
        if (HAL_GPIO_ReadPin(SET_KEY_GPIO_PORT, SET_KEY_PIN) == GPIO_PIN_RESET) {
            g_App.lastSetKeyPressTime = HAL_GetTick();
            g_App.lastSetKeyPressReportTime = 0;
            g_App.setKeyRepeatReported = false;
        } else {
            uint32_t currentVal = HAL_GetTick();
            if (g_App.lastSetKeyPressTime > currentVal) {
                SetKey_Clicked();
            } else if (currentVal - g_App.lastSetKeyPressTime > KEY_CLICK_EFFECT_TIME &&
                       !g_App.setKeyRepeatReported) {
                SetKey_Clicked();
            }
            g_App.setKeyRepeatReported = false;
        }
    }
}

/**
 * @brief  获取应用状态指针
 */
AppState* App_GetState(void) {
    return &g_App;
}
