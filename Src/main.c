/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "TM1637.h"
#include "SD3077.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// 显示模式枚举定义
typedef enum{
	MODE_SHOW_TIME = 0,           // 显示时间模式（时:分）
	MODE_SHOW_SECOND,             // 显示秒数模式
	MODE_SHOW_TEMPERTURE,         // 显示温度模式
	MODE_SET_HOUR,                // 设置小时模式
	MODE_SET_MINUTE,              // 设置分钟模式
	MODE_SET_ALARM_ENABLE,        // 设置闹钟开关模式
	MODE_SET_ALARM_HOUR,          // 设置闹钟小时模式
	MODE_SET_ALARM_MINUTE,        // 设置闹钟分钟模式
	MODE_SET_TEMP_SHOW,           // 设置温度显示时长模式
	MODE_SET_TEMP_HIDE,           // 设置温度隐藏时长模式
	MODE_SET_BRIGHTNESS,          // 设置亮度模式（手动/自动）
	MODE_SET_BRIGHTNESS_STRONG,   // 设置强光亮度模式
	MODE_SET_BRIGHTNESS_WEAK,     // 设置弱光亮度模式
	MODE_SET_ROT_ENABLE,          // 设置整点报时开关模式
	MODE_SET_ROT_START,           // 设置整点报时开始时间模式
	MODE_SET_ROT_STOP,            // 设置整点报时结束时间模式
}DisplayMode;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// GPIO引脚定义
#define SECOND_INT_PIN               SEC_INT_Pin             // 秒中断管脚编号（RTC每秒产生中断）
#define SECOND_INT_EXTI_IRQN         SEC_INT_EXTI_IRQn       // 秒中断IRQN
#define KEY_EXTI_IRQN                MODE_KEY_EXTI_IRQn      // 按键中断IRQN

#define MODE_KEY_GPIO_PORT           MODE_KEY_GPIO_Port      // 模式按钮GPIO端口
#define MODE_KEY_PIN                 MODE_KEY_Pin            // 模式按钮GPIO引脚

#define SET_KEY_GPIO_PORT            SET_KEY_GPIO_Port       // 设置按钮GPIO端口
#define SET_KEY_PIN                  SET_KEY_Pin             // 设置按钮GPIO引脚

#define BUZZER_GPIO_PORT             BUZZER_GPIO_Port        // 蜂鸣器GPIO端口
#define BUZZER_PIN                   BUZZER_Pin              // 蜂鸣器GPIO引脚

// 按键相关时间参数（单位：毫秒）
#define KEY_LONG_PRESS_EFFECT_TIME   800       // 按键长按生效时间（超过此时间视为长按）
#define KEY_REPEAT_TIME_INTERVAL     250       // 按键长按重复触发间隔
#define KEY_CLICK_EFFECT_TIME        50        // 按键单击生效时间（消抖时间）

// 时间设置范围
#define YEAR_MAX_SET                 38        // 最大年份设置（20xx年）
#define YEAR_MIN_SET                 15        // 最小年份设置（20xx年）

// 温度显示时间范围（单位：秒）
#define TEMPERTURE_MAX_SHOW_TIME     15        // 温度最大显示时长
#define TEMPERTURE_MAX_HIDE_TIME     30        // 温度最大隐藏时长

// 整点报时参数
#define RING_ON_TIME_LONG            1000      // 整点报时蜂鸣器鸣叫时长（毫秒）

// 自动亮度调节ADC阈值
#define STRONG_BRIGHTNESS_ADC_VALUE  2800      // 强光ADC阈值（超过此值切换到强光模式）
#define WEAK_BRIGHTNESS_ADC_VALUE    2300      // 弱光ADC阈值（低于此值切换到弱光模式）
#define STRONG_BRIGHTNESS_VALUE      8         // 强光默认亮度值（1-8级）
#define WEAK_BRIGHTNESS_VALUE        1         // 弱光默认亮度值（1-8级）

// 定时器句柄定义
#define ALARM_CONTROL_TIMER_HANDLE   htim17    // 闹钟控制定时器句柄
#define ALARM_CONTROL_TIMER          TIM17     // 闹钟控制定时器
#define LIGHT_CONTROL_TIMER_HANDLE   htim16    // 亮度控制定时器句柄
#define LIGHT_CONTROL_TIMER          TIM16     // 亮度控制定时器
#define TEMPERTURE_ADC_HANDLE        hadc      // 温度ADC句柄

// 备份数据存储索引（保存在RTC备份寄存器中）
#define BAK_DATA_SIZE                13        // 备份数据总大小
#define BAK_POWER_DOWN_IND_INDEX     0x00      // 掉电标识索引（用于判断是否首次上电）
#define BAK_ALARM_ENABLED_INDEX      0x02      // 闹钟使能标志索引
#define BAK_ALARM_HOUR_INDEX         0x03      // 闹钟小时索引
#define BAK_ALARM_MINUTE_INDEX       0x04      // 闹钟分钟索引
#define BAK_TEMP_SHOW_TIME_INDEX     0x05      // 温度显示时长索引
#define BAK_TEMP_HIDE_TIME_INDEX     0x06      // 温度隐藏时长索引
#define BAK_ROT_ENABLED_INDEX        0x07      // 整点报时使能标志索引
#define BAK_ROT_START_INDEX          0x08      // 整点报时开始时间索引
#define BAK_ROT_STOP_INDEX           0x09      // 整点报时结束时间索引
#define BAK_BRIGHTNESS_INDEX         0x0A      // 亮度设置索引（0为自动，1-8为手动）
#define BAK_BRIGHTNESS_STRONG_INDEX  0x0B      // 强光亮度索引
#define BAK_BRIGHTNESS_WEAK_INDEX    0x0C      // 弱光亮度索引

#define POWER_DOWN_IND_DATA          0xFA      // 掉电标识数据（用于判断是否首次上电）

// NTC温度传感器相关
#define TEMP_BUFFER_SIZE   8     // 温度读取缓存长度（用于滤波平均）
#define TEMP_MAP_SIZE      126   // 温度映射表长度（ADC值到温度的映射）
// 温度映射表：将ADC值映射到温度值（0-125℃）
const uint16_t tempertureMap[] = {1054, 1091, 1128, 1165, 1203, 1242, 1280, 1320, 1359, 1399, 1439, 1479, 1520, 1560, 1601, 1642, 1683, 1724, 1765, 1805, 1846, 1887, 1927, 1968, 2008, 2048, 2087, 2126, 2165, 2204, 2242, 2280, 2318, 2355, 2391, 2427, 2463, 2498, 2533, 2567, 2601, 2634, 2667, 2699, 2730, 2761, 2791, 2821, 2850, 2879, 2907, 2935, 2962, 2988, 3014, 3040, 3064, 3089, 3113, 3136, 3158, 3181, 3202, 3224, 3244, 3264, 3284, 3303, 3322, 3341, 3359, 3376, 3393, 3410, 3426, 3442, 3457, 3472, 3487, 3501, 3515, 3529, 3542, 3555, 3568, 3580, 3592, 3603, 3615, 3626, 3637, 3647, 3657, 3667, 3677, 3687, 3696, 3705, 3714, 3722, 3730, 3739, 3747, 3754, 3762, 3769, 3776, 3783, 3790, 3797, 3803, 3809, 3815, 3821, 3827, 3833, 3839, 3844, 3849, 3854, 3859, 3864, 3869, 3874, 3878, 3883, };

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 当前显示模式
DisplayMode currentMode = MODE_SHOW_TIME;
bool isInitCompleted = false;           // 初始化完成标志
uint32_t lastDisplayChangeTime;         // 上次显示切换时间（用于温度/时间自动切换）

// ADC采样值（0:光敏电阻，1:温度传感器）
uint32_t adcValue[2];

// 亮度控制相关
uint8_t savedBrightness = 0;            // 保存的亮度值（0=自动，1-8=手动）
bool isWeakBrightness = true;           // 当前是否为弱光模式
uint8_t strongBrightness, weakBrightness; // 强光/弱光亮度值

// 时间相关
DateTime time, lastTime;                // 当前时间和上次设置的时间
uint8_t blinkControl;                   // 闪烁控制标志（用于设置模式下的闪烁显示）

// 闹钟相关
bool isAlarmEnabled = false;            // 闹钟使能标志
bool isAlarmed = false, isAlarming = false; // 已触发标志和正在响铃标志
uint8_t alarmHour = 0, alarmMin = 0;    // 闹钟时间

// 整点报时相关
bool isRingOnTimeEnabled ;              // 整点报时使能标志
uint8_t ringOnTimeStart, ringOnTimeStop; // 整点报时起止时间
uint8_t lastRingOnTimeHour;             // 上次整点报时的小时（防止重复触发）
uint32_t ringStartTime;                 // 整点报时开始时间戳
uint8_t lastChimeSecond = 0xFF;         // 上次整点报时的秒数（防止同一秒重复触发）
bool isChiming = false;                 // 正在整点报时标志

// 按键相关
uint32_t lastModeKeyPressTime, lastSetKeyPressTime, lastSetKeyPressReportTime; // 按键按下时间戳
uint32_t alarmTimestamp, alarmBeepCount; // 闹钟定时器计数
bool setKeyRepeatReported = false;      // 设置键重复触发标志

// 温度相关
uint8_t temperture = 25;                // 当前温度值
uint8_t tempertureShowTime, tempertureHideTime; // 温度显示/隐藏时长（秒）
uint8_t tempBuffered = 0;               // 温度缓存计数
uint16_t tempBuffer[TEMP_BUFFER_SIZE + 1]; // 温度采样缓存（用于滤波）
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
 * @brief  从RTC备份寄存器读取闹钟设置
 * @note   读取闹钟时间和使能状态，并进行合法性检查
 * @param  None
 * @retval None
 */
void readBackupSettings() {
	uint8_t data[3];
	ReadBackData(0, data, 3);  // 从备份寄存器读取3字节数据

	alarmHour = data[0];       // 闹钟小时
	alarmMin = data[1];        // 闹钟分钟
	isAlarmEnabled = data[2];  // 闹钟使能标志

	// 检查读取的数据是否合法，不合法则重置为0
	if(alarmHour > 23) alarmHour = 0;
	if(alarmMin > 59) alarmMin = 0;
}

/**
 * @brief  保存所有设置到RTC备份寄存器
 * @note   将闹钟、温度显示、整点报时、亮度等设置保存到备份寄存器
 *         掉电后数据不会丢失
 * @param  None
 * @retval None
 */
void saveSettings(){
	uint8_t data[BAK_DATA_SIZE] = {
			POWER_DOWN_IND_DATA,        // [0] 掉电标识（用于判断是否首次上电）
			POWER_DOWN_IND_DATA,        // [1] 掉电标识（冗余）
			isAlarmEnabled,             // [2] 闹钟使能标志
			alarmHour,                  // [3] 闹钟小时
			alarmMin,                   // [4] 闹钟分钟
			tempertureShowTime,         // [5] 温度显示时长
			tempertureHideTime,         // [6] 温度隐藏时长
			isRingOnTimeEnabled,        // [7] 整点报时使能标志
			ringOnTimeStart,            // [8] 整点报时开始时间
			ringOnTimeStop,             // [9] 整点报时结束时间
			savedBrightness,            // [10] 亮度设置（0=自动，1-8=手动）
			strongBrightness,           // [11] 强光亮度值
			weakBrightness,             // [12] 弱光亮度值
	};
	WriteBackData(0, data, BAK_DATA_SIZE);  // 写入备份寄存器
}

/**
 * @brief  重置所有设置为默认值
 * @note   首次上电或数据异常时调用，恢复出厂设置
 * @param  None
 * @retval None
 */
void resetSettings(){
	  isAlarmEnabled = false;        // 闹钟默认关闭
	  alarmHour = 0;                 // 闹钟时间 00:00
	  alarmMin = 0;
	  tempertureShowTime = 2;        // 温度显示2秒
	  tempertureHideTime = 10;       // 温度隐藏10秒
	  isRingOnTimeEnabled = false;   // 整点报时默认关闭
	  ringOnTimeStart = 8;           // 整点报时时间段 8:00-20:00
	  ringOnTimeStop = 20;
	  savedBrightness = 8;           // 默认手动最高亮度
	  strongBrightness = 8;          // 强光亮度8级
	  weakBrightness = 1;            // 弱光亮度1级
}

/**
 * @brief  检查并触发整点报时
 * @note   新逻辑：从每小时的59分55秒开始，每秒响一次短音（100ms）
 *         一直到下个小时的00分00秒，共6次
 *         最后一次（00:00）响长音（1000ms）
 *         避免与闹钟冲突
 * @param  None
 * @retval None
 */
void checkRingOnTime(){
	// 检查整点报时是否开启
	if(!isRingOnTimeEnabled){
		return;
	}

	// 检查是否在整点报时时间段内
	bool inTimeRange = false;
	if(ringOnTimeStart <= ringOnTimeStop){
		// 正常时间段（如8:00-20:00）
		inTimeRange = (time.hours >= ringOnTimeStart && time.hours <= ringOnTimeStop);
	}
	else{
		// 跨天时间段（如22:00-6:00）
		inTimeRange = (time.hours >= ringOnTimeStart || time.hours <= ringOnTimeStop);
	}

	if(!inTimeRange){
		return;
	}

	// 避免与闹钟冲突：如果闹钟开启且设置在整点，则不报时
	if(isAlarmEnabled && alarmMin == 0 && alarmHour == time.hours){
		return;
	}

	// 整点报时逻辑：59分55秒到00分00秒，共6次
	if(time.minutes == 59 && time.seconds >= 55){
		// 倒计时阶段：55秒、56秒、57秒、58秒、59秒（短音）
		if(lastChimeSecond != time.seconds){
			// 短音：100ms
			ringStartTime = HAL_GetTick();
			HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
			isChiming = true;
			lastChimeSecond = time.seconds;
		}
	}
	else if(time.minutes == 0 && time.seconds == 0){
		// 整点：00分00秒（长音）
		if(lastChimeSecond != 0){
			// 长音：1000ms
			ringStartTime = HAL_GetTick();
			HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_RESET);
			isChiming = true;
			lastChimeSecond = 0;
			lastRingOnTimeHour = time.hours;  // 记录已报时的小时
		}
	}
	else{
		// 其他时间，重置秒数标志
		if(time.seconds > 0){
			lastChimeSecond = 0xFF;
		}
	}
}

void alarmStart(){
	isAlarming = true;
	isAlarmed = true;
	HAL_TIM_Base_Start_IT(&ALARM_CONTROL_TIMER_HANDLE);
}

void alarmTimerTick()
{
    uint16_t onTime = 50, offTime = 50, restTime = 500;
    uint8_t ringCounts = 4;
	alarmTimestamp ++;
	if(HAL_GPIO_ReadPin(BUZZER_GPIO_PORT, BUZZER_PIN) == GPIO_PIN_RESET)
	{
		// ����XX����
		if(alarmTimestamp > onTime){
			HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, SET);
			alarmBeepCount ++;
			alarmTimestamp = 0;
		}
	}
	else
	{
		// ��X+1��
		if(alarmBeepCount < ringCounts)
		{
			// ���ζ���ʱ����
			if(alarmTimestamp > offTime)
			{
				HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, RESET);
				alarmTimestamp = 0;
			}

		}
		// ��X+1�κ�ֹͣ����һ��ʱ��
		else if(alarmTimestamp > restTime)
		{
			HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, RESET);
			alarmTimestamp = 0;
			alarmBeepCount = 0;
		}
	}
}

void alarmStop(){
	isAlarming = false;
	HAL_TIM_Base_Stop_IT(&ALARM_CONTROL_TIMER_HANDLE);
	HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, SET);
}

void checkAlarm(){
	if(alarmHour == time.hours && alarmMin == time.minutes && isAlarmed == false && isAlarmEnabled){
		alarmStart();
	}

	if(alarmHour != time.hours || alarmMin != time.minutes){
		if(isAlarming){
			alarmStop();
		}
		isAlarmed = false;
	}
}

void refreshTemperture(){
	// �����¶�ת��ADC

	uint32_t value;
	uint32_t srcValue = adcValue[1];

	if(tempBuffered < TEMP_BUFFER_SIZE)
	{
		tempBuffer[tempBuffered++] = (uint16_t)srcValue;
		value = srcValue;
	}
	else{
		tempBuffer[TEMP_BUFFER_SIZE] = (uint16_t)srcValue;
		value = 0;
		for(uint8_t i = 0; i < TEMP_BUFFER_SIZE; i++)
		{
			tempBuffer[i] = tempBuffer[i+1];
			value += tempBuffer[i];
		}
		value = value / TEMP_BUFFER_SIZE;
	}

	for(uint8_t i = 0; i < TEMP_MAP_SIZE; i++)
	{
		if(value <= tempertureMap[i]){
			temperture = i;
			return;
		}
	}

	temperture = 0;
	return;
}

void refreshTimeDisplay() {
	TimeNow(&time);
	lastTime = time;
	if (currentMode == MODE_SHOW_TIME) {
		uint16_t number = time.hours * 100 + time.minutes;
		TM1637ShowNumberRight(3, number, blinkControl ? 2 : 0xFF, 1);
	}
	else if (currentMode == MODE_SHOW_SECOND) {
		TM1637SetChar(0, ' ', false);
		TM1637SetChar(1, ' ', true);
		TM1637SetChar(2, time.seconds / 10 + '0', false);
		TM1637SetChar(3, time.seconds % 10 + '0', false);
	}
	else if (currentMode == MODE_SHOW_TEMPERTURE){
		refreshTemperture();
		if(temperture > 99){
			temperture = 99;
		}
		TM1637SetChar(0, ' ', false);
		TM1637SetChar(1, temperture / 10 + '0', false);
		TM1637SetChar(2, temperture % 10 + '0', false);
		TM1637SetChar(3, 'C', false);
	}

	checkAlarm();
	checkRingOnTime();
}

void refreshSettingsDisplay(){
	char disp[5] = {0};
	if(currentMode == MODE_SET_HOUR){
		disp[0] = blinkControl ? ' ' : (lastTime.hours / 10 + '0');
		disp[1] = blinkControl ? ' ' : (lastTime.hours % 10 + '0');
		disp[2] = lastTime.minutes / 10  + '0';
		disp[3] = lastTime.minutes % 10  + '0';
	}
	else if(currentMode == MODE_SET_MINUTE){
		disp[0] = lastTime.hours / 10 + '0';
		disp[1] = lastTime.hours % 10 + '0';
		disp[2] = blinkControl ? ' ' : lastTime.minutes / 10  + '0';
		disp[3] = blinkControl ? ' ' : lastTime.minutes % 10  + '0';
	}
	else if(currentMode == MODE_SET_ALARM_ENABLE){
		disp[0] = 'A';
		disp[1] = 'L';
		disp[2] = blinkControl ? ' ' : 'o' ;
		disp[3] = blinkControl ? ' ' : isAlarmEnabled ? 'n' : 'F';
	}
	else if(currentMode == MODE_SET_ALARM_HOUR){
		disp[0] = blinkControl ? ' ' : (alarmHour / 10 + '0');
		disp[1] = blinkControl ? ' ' : (alarmHour % 10 + '0');
		disp[2] = alarmMin / 10  + '0';
		disp[3] = alarmMin % 10  + '0';
	}
	else if(currentMode == MODE_SET_ALARM_MINUTE){
		disp[0] = alarmHour / 10 + '0';
		disp[1] = alarmHour % 10 + '0';
		disp[2] = blinkControl ? ' ' : alarmMin / 10  + '0';
		disp[3] = blinkControl ? ' ' : alarmMin % 10  + '0';
	}
	else if(currentMode == MODE_SET_TEMP_SHOW){
		disp[0] = 'T';
		disp[1] = 'S';
		disp[2] = blinkControl ? ' ' : tempertureShowTime / 10  + '0';
		disp[3] = blinkControl ? ' ' : tempertureShowTime % 10  + '0';
	}
	else if(currentMode == MODE_SET_TEMP_HIDE){
		disp[0] = 'T';
		disp[1] = 'H';
		disp[2] = blinkControl ? ' ' : tempertureHideTime / 10  + '0';
		disp[3] = blinkControl ? ' ' : tempertureHideTime % 10  + '0';
	}
	else if (currentMode == MODE_SET_BRIGHTNESS) {
		disp[0] = 'b';
		disp[1] = 'r';
		if (savedBrightness) {
			disp[2] = blinkControl ? ' ' : '0';
			disp[3] = blinkControl ? ' ' : savedBrightness + '0';
		}
		else {
			disp[2] = 'A';
			disp[3] = 'U';
		}
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_STRONG) {
		disp[0] = 'b';
		disp[1] = '1';
		disp[2] = blinkControl ? ' ' : '0';
		disp[3] = blinkControl ? ' ' : strongBrightness + '0';
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_WEAK) {
		disp[0] = 'b';
		disp[1] = '2';
		disp[2] = blinkControl ? ' ' : '0';
		disp[3] = blinkControl ? ' ' : weakBrightness + '0';
	}
	else if(currentMode == MODE_SET_ROT_ENABLE){
		disp[0] = 'r';
		disp[1] = 'o';
		disp[2] = blinkControl ? ' ' : 'o' ;
		disp[3] = blinkControl ? ' ' : isRingOnTimeEnabled ? 'n' : 'F';
	}
	else if(currentMode == MODE_SET_ROT_START){
		disp[0] = 'r';
		disp[1] = 'S';
		disp[2] = blinkControl ? ' ' : ringOnTimeStart / 10  + '0';
		disp[3] = blinkControl ? ' ' : ringOnTimeStart % 10  + '0';
	}
	else if(currentMode == MODE_SET_ROT_STOP){
		disp[0] = 'r';
		disp[1] = 'e';
		disp[2] = blinkControl ? ' ' : ringOnTimeStop / 10  + '0';
		disp[3] = blinkControl ? ' ' : ringOnTimeStop % 10  + '0';
	}

	TM1637SetChar(0, disp[0], false);
	TM1637SetChar(1, disp[1], true);
	TM1637SetChar(2, disp[2], false);
	TM1637SetChar(3, disp[3], false);
}


void modeKeyClicked() {

	if(isAlarming){
		alarmStop();
		return;
	}

	if (currentMode == MODE_SHOW_TIME || currentMode == MODE_SHOW_TEMPERTURE) {
		currentMode = MODE_SET_HOUR;
		blinkControl = 0xFF;
		refreshSettingsDisplay();
		SetInterruptOuput(F_2_HZ);
	}
	else if (currentMode == MODE_SHOW_SECOND) {
		lastTime.seconds = 0;
		SetTime(&lastTime);
		refreshTimeDisplay();
	}
	else if (currentMode == MODE_SET_HOUR) {
		currentMode = MODE_SET_MINUTE;
		blinkControl = 0xFF;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_MINUTE) {
		currentMode = MODE_SET_ALARM_ENABLE;
		blinkControl = 0x00;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_ENABLE) {
		currentMode = isAlarmEnabled ? MODE_SET_ALARM_HOUR : MODE_SET_TEMP_SHOW;
		blinkControl = 0xFF;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_HOUR) {
		currentMode = MODE_SET_ALARM_MINUTE;
		blinkControl = 0xFF;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_MINUTE) {
		currentMode = MODE_SET_TEMP_SHOW;
		blinkControl = 0x00;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_TEMP_SHOW) {
		if(tempertureShowTime != 0){
			currentMode = MODE_SET_TEMP_HIDE;
		}
		else{
			currentMode = MODE_SET_BRIGHTNESS;
		}
		blinkControl = 0x00;
		refreshSettingsDisplay();

	}
	else if (currentMode == MODE_SET_TEMP_HIDE) {
		currentMode = MODE_SET_BRIGHTNESS;
		blinkControl = 0x00;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS) {
		if(savedBrightness == 0){
			currentMode = MODE_SET_BRIGHTNESS_STRONG;
		}
		else{
			currentMode = MODE_SET_ROT_ENABLE;
		}
		blinkControl = 0x00;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_STRONG) {
		currentMode = MODE_SET_BRIGHTNESS_WEAK;
		blinkControl = 0x00;
		TM1637SetBrightness(weakBrightness);
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_WEAK) {
		currentMode = MODE_SET_ROT_ENABLE;
		blinkControl = 0x00;
		TM1637SetBrightness(strongBrightness);
		isWeakBrightness = false;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_ENABLE) {
		blinkControl = 0x00;
		if(!isRingOnTimeEnabled){
			currentMode = MODE_SHOW_TIME;
			// ����ʱ��
			TimeNow(&time);
			lastTime.seconds = time.seconds;
			SetTime(&lastTime);
			lastRingOnTimeHour = lastTime.hours;

			saveSettings();

			refreshTimeDisplay();
			EnableSencodInterruptOuput();
			lastDisplayChangeTime = HAL_GetTick();
		}
		else{
			currentMode = MODE_SET_ROT_START;
			refreshSettingsDisplay();
		}
	}
	else if (currentMode == MODE_SET_ROT_START) {
		currentMode = MODE_SET_ROT_STOP;
		blinkControl = 0x00;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_STOP) {
		currentMode = MODE_SHOW_TIME;
		// ����ʱ��
		TimeNow(&time);
		lastTime.seconds = time.seconds;
		SetTime(&lastTime);
		lastRingOnTimeHour = lastTime.hours;
		saveSettings();
		refreshTimeDisplay();
		EnableSencodInterruptOuput();
		lastDisplayChangeTime = HAL_GetTick();

	}
}


void modeKeyLongPressed(){
	if(isAlarming){
		alarmStop();
		return;
	}

	if(currentMode >= MODE_SET_HOUR && currentMode <= MODE_SET_ROT_STOP){
		currentMode = MODE_SHOW_TIME;
		refreshTimeDisplay();
		lastDisplayChangeTime = HAL_GetTick();
	}
}

void setKeyClicked() {
	if(isAlarming){
		alarmStop();
		return;
	}

	if (currentMode == MODE_SHOW_TIME || currentMode == MODE_SHOW_TEMPERTURE) {
		currentMode = MODE_SHOW_SECOND;
		refreshTimeDisplay();
	}
	else if (currentMode == MODE_SHOW_SECOND) {
		currentMode = MODE_SHOW_TIME;
		refreshTimeDisplay();
		lastDisplayChangeTime = HAL_GetTick();
	}
	else if (currentMode == MODE_SET_HOUR) {
		lastTime.hours++;
		if (lastTime.hours > 23) {
			lastTime.hours = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_MINUTE) {
		lastTime.minutes++;
		if (lastTime.minutes > 59) {
			lastTime.minutes = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_ENABLE) {
		isAlarmEnabled = isAlarmEnabled ? false : true;
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_HOUR) {
		alarmHour++;
		if (alarmHour > 23) {
			alarmHour = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_MINUTE) {
		alarmMin++;
		if (alarmMin > 59) {
			alarmMin = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_TEMP_SHOW) {
		tempertureShowTime++;
		if (tempertureShowTime > TEMPERTURE_MAX_SHOW_TIME) {
			tempertureShowTime = tempertureHideTime == 0 ? 1 : 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_TEMP_HIDE) {
		tempertureHideTime++;
		if (tempertureHideTime > TEMPERTURE_MAX_HIDE_TIME) {
			tempertureHideTime = tempertureShowTime == 0 ? 1 : 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS) {
		savedBrightness++;
		if (savedBrightness > 8) {
			savedBrightness = 0;
		}
		TM1637SetBrightness(savedBrightness == 0 ? 1 : savedBrightness);
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_STRONG) {
		strongBrightness++;
		if (strongBrightness > 8) {
			strongBrightness = 1;
		}
		blinkControl = 0;
		TM1637SetBrightness(strongBrightness);
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_BRIGHTNESS_WEAK) {
		weakBrightness++;
		if (weakBrightness > 8) {
			weakBrightness = 1;
		}
		blinkControl = 0;
		TM1637SetBrightness(weakBrightness);
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_ENABLE) {
		isRingOnTimeEnabled = isRingOnTimeEnabled ? false : true;
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_START) {
		ringOnTimeStart++;
		if (ringOnTimeStart > 23) {
			ringOnTimeStart = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_STOP) {
		ringOnTimeStop++;
		if (ringOnTimeStop > 23) {
			ringOnTimeStop = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
}

void setKeyPresseRepeatReport() {
	setKeyRepeatReported = true;

	if (currentMode == MODE_SET_HOUR) {
		lastTime.hours++;
		if (lastTime.hours > 23) {
			lastTime.hours = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_MINUTE) {
		lastTime.minutes++;
		if (lastTime.minutes > 59) {
			lastTime.minutes = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_HOUR) {
		alarmHour++;
		if (alarmHour > 23) {
			alarmHour = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ALARM_MINUTE) {
		alarmMin++;
		if (alarmMin > 59) {
			alarmMin = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_TEMP_SHOW) {
		tempertureShowTime++;
		if (tempertureShowTime > TEMPERTURE_MAX_SHOW_TIME) {
			tempertureShowTime = tempertureHideTime == 0 ? 1 : 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_TEMP_HIDE) {
		tempertureHideTime++;
		if (tempertureHideTime > TEMPERTURE_MAX_HIDE_TIME) {
			tempertureHideTime = tempertureShowTime == 0 ? 1 : 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_START) {
		ringOnTimeStart++;
		if (ringOnTimeStart > 23) {
			ringOnTimeStart = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
	else if (currentMode == MODE_SET_ROT_STOP) {
		ringOnTimeStop++;
		if (ringOnTimeStop > 23) {
			ringOnTimeStop = 0;
		}
		blinkControl = 0;
		refreshSettingsDisplay();
	}
}

void setKeyPressed(){
	lastSetKeyPressTime = HAL_GetTick();
}

void setKeyReleased() {
	uint32_t currentVal = HAL_GetTick();
	if (lastSetKeyPressTime > currentVal) {
		setKeyClicked();
	}
	else if (currentVal - lastSetKeyPressTime > KEY_CLICK_EFFECT_TIME && !setKeyRepeatReported) {
		setKeyClicked();
	}
	setKeyRepeatReported = false;
}

void modeKeyPressed(){
	lastModeKeyPressTime = HAL_GetTick();
}

void modeKeyReleased(){
	uint32_t currentVal = HAL_GetTick();
	if(lastModeKeyPressTime > currentVal)
	{
		modeKeyClicked();
	}
	else if(currentVal - lastModeKeyPressTime > KEY_LONG_PRESS_EFFECT_TIME)
	{
		modeKeyLongPressed();
	}
	else if(currentVal - lastModeKeyPressTime > KEY_CLICK_EFFECT_TIME)
	{
		modeKeyClicked();
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC_Init();
  MX_TIM17_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  // ����Ƿ���粢��ȡ���б��������
  uint8_t backupData[BAK_DATA_SIZE];
  ReadBackData(BAK_POWER_DOWN_IND_INDEX, backupData, BAK_DATA_SIZE);
  if(backupData[0] != POWER_DOWN_IND_DATA && backupData[1] != POWER_DOWN_IND_DATA){
	  // ����ʱ��
	  time.year = YEAR_MIN_SET;
	  time.month = 1;
	  time.dayOfMonth = 1;
	  time.dayOfWeek = 1;
	  time.hours = 0;
	  time.minutes = 0;
	  time.ampm = HOUR24;
	  time.seconds = 0;
	  SetTime(&time);

	  // ��������
	  resetSettings();
	  saveSettings();
  }else{
	  isAlarmEnabled = backupData[BAK_ALARM_ENABLED_INDEX];
	  alarmHour = backupData[BAK_ALARM_HOUR_INDEX];
	  alarmMin = backupData[BAK_ALARM_MINUTE_INDEX];
	  tempertureShowTime = backupData[BAK_TEMP_SHOW_TIME_INDEX];
	  tempertureHideTime = backupData[BAK_TEMP_HIDE_TIME_INDEX];
	  isRingOnTimeEnabled = backupData[BAK_ROT_ENABLED_INDEX];
	  ringOnTimeStart = backupData[BAK_ROT_START_INDEX];
	  ringOnTimeStop = backupData[BAK_ROT_STOP_INDEX];
	  savedBrightness = backupData[BAK_BRIGHTNESS_INDEX];
	  strongBrightness = backupData[BAK_BRIGHTNESS_STRONG_INDEX];
	  weakBrightness = backupData[BAK_BRIGHTNESS_WEAK_INDEX];
  }

  // ���ݼ��
  bool hasError = false;
  if(alarmHour > (uint8_t)23 || alarmMin > (uint8_t)59){
	  hasError = true;
  }
//  if(tempertureShowTime == 0 && tempertureHideTime == 0){
//	  hasError = true;
//  }
  if(ringOnTimeStart > 23 || ringOnTimeStop > 23){
	  hasError = true;
  }
  if(savedBrightness > 8 || strongBrightness > 8 || strongBrightness == 0 || weakBrightness > 8 || weakBrightness == 0){
	  hasError = true;
  }

  // ��������ڴ������������
  if(hasError){
	  resetSettings();
	  saveSettings();
  }

  TM1637Init();
  if(savedBrightness != 0){
	  TM1637SetBrightness(savedBrightness);
  }
  else{
	  TM1637SetBrightness(STRONG_BRIGHTNESS_VALUE);
	  isWeakBrightness = false;
  }

  HAL_ADCEx_Calibration_Start(&hadc);
  HAL_ADC_Start_DMA(&hadc, adcValue, 2);
  HAL_TIM_Base_Start(&htim3);

  HAL_TIM_Base_Start_IT(&LIGHT_CONTROL_TIMER_HANDLE);

  TimeNow(&time);
  lastRingOnTimeHour = time.hours;

  EnableSencodInterruptOuput();

  lastDisplayChangeTime = HAL_GetTick();

  isInitCompleted = true;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t now = 0, passedTime;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		// 蜂鸣器控制逻辑（整点报时和闹钟）
		if (HAL_GPIO_ReadPin(BUZZER_GPIO_PORT, BUZZER_PIN) == GPIO_PIN_RESET) {
			now = HAL_GetTick();

			// 如果是闹钟响铃，由定时器中断控制，这里不处理
			if(isAlarming){
				// 闹钟响铃由alarmTimerTick()控制
			}
			// 如果是整点报时
			else if(isChiming){
				uint32_t chimeDuration;
				// 判断是短音还是长音
				if(time.minutes == 0 && time.seconds == 0){
					// 整点长音：1000ms
					chimeDuration = 1000;
				}
				else{
					// 倒计时短音：100ms
					chimeDuration = 100;
				}

				// 检查是否到达鸣叫时长
				if (now < ringStartTime || (now - ringStartTime >= chimeDuration)) {
					HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_SET);
					isChiming = false;
				}
			}
			// 其他情况（兼容旧代码）
			else{
				if (now < ringStartTime || (now - ringStartTime >= RING_ON_TIME_LONG)) {
					HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_PIN, GPIO_PIN_SET);
				}
			}
		}

		// 温度显示自动切换逻辑
		if (tempertureShowTime > 0) {
			if (currentMode == MODE_SHOW_TIME) {
				now = HAL_GetTick();
				passedTime = now - lastDisplayChangeTime;
				if (now < lastDisplayChangeTime || passedTime >= (tempertureHideTime * 1000)) {
					currentMode = MODE_SHOW_TEMPERTURE;
					lastDisplayChangeTime = now;
				}

			}
			else if (currentMode == MODE_SHOW_TEMPERTURE && tempertureHideTime > 0) {
				now = HAL_GetTick();
				passedTime = now - lastDisplayChangeTime;
				if (now < lastDisplayChangeTime || passedTime >= (tempertureShowTime * 1000)) {
					currentMode = MODE_SHOW_TIME;
					lastDisplayChangeTime = now;
				}
			}
		} else if(currentMode == MODE_SHOW_TEMPERTURE){
			currentMode = MODE_SHOW_TIME;
			lastDisplayChangeTime = now;
		}

		// 设置键长按重复触发逻辑
		if (HAL_GPIO_ReadPin(SET_KEY_GPIO_PORT, SET_KEY_PIN) == GPIO_PIN_RESET
				&& currentMode >= MODE_SET_HOUR && currentMode <= MODE_SET_ROT_STOP) {
				uint32_t curVal = HAL_GetTick();
				uint32_t timePassed = curVal - lastSetKeyPressTime;
				if (timePassed > KEY_LONG_PRESS_EFFECT_TIME) {
					if (curVal - lastSetKeyPressReportTime > KEY_REPEAT_TIME_INTERVAL) {
						setKeyPresseRepeatReport();
						lastSetKeyPressReportTime = curVal;
					}
				}
			}

	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){

	if (GPIO_Pin == SECOND_INT_PIN && isInitCompleted) {
		if(currentMode == MODE_SHOW_TIME || currentMode == MODE_SHOW_SECOND || currentMode == MODE_SHOW_TEMPERTURE){
			refreshTimeDisplay();
		}
		else if(currentMode >= MODE_SET_HOUR && currentMode <= MODE_SET_ROT_STOP){
			refreshSettingsDisplay();
		}
		blinkControl = ~blinkControl;
	}
	else if (GPIO_Pin == MODE_KEY_PIN) {

		if (HAL_GPIO_ReadPin(MODE_KEY_GPIO_PORT, MODE_KEY_PIN) == GPIO_PIN_RESET) {
			modeKeyPressed();
		}
		else {
			modeKeyReleased();
		}
	}
	else if (GPIO_Pin == SET_KEY_PIN) {
		if (HAL_GPIO_ReadPin(SET_KEY_GPIO_PORT, SET_KEY_PIN) == GPIO_PIN_RESET) {
			setKeyPressed();
		}
		else {
			setKeyReleased();
		}
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == ALARM_CONTROL_TIMER) {
		alarmTimerTick();
	}
	else if (htim->Instance == LIGHT_CONTROL_TIMER) {
		if (savedBrightness == 0) {
			if (isWeakBrightness && adcValue[0] > STRONG_BRIGHTNESS_ADC_VALUE) {
				isWeakBrightness = false;
				if(strongBrightness > 0){
					TM1637SetBrightness(strongBrightness);
				}
			}
			else if (!isWeakBrightness && adcValue[0] < WEAK_BRIGHTNESS_ADC_VALUE) {
				isWeakBrightness = true;
				if(weakBrightness > 0){
					TM1637SetBrightness(weakBrightness);
				}
			}
		}
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
