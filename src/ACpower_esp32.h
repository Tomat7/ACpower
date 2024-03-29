/*
* Оригинальная идея (c) Sebra
* Базовый алгоритм регулирования (c) Chatterbox
* Алгоритм с привязкой расчетов к детектору нуля, поддержка ESP32 и перевод в библиотеку (c) Tomat7
* Version 3.1 (ESP32 support added starting v3)
* http://forum.homedistiller.ru/index.php?topic=166750.0
* https://tomat.visualstudio.com/ESP32-AC-power
*	
* ESP32 pin connections (ESP32 Wemos Lolin32):
* 39 - Voltage meter (https://learn.openenergymonitor.org/electricity-monitoring/voltage-sensing/measuring-voltage-with-an-acac-power-adapter )
* 36 - Current transformer (https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/interface-with-arduino )
* 25 - ZeroCross detector ()
* 26 - Triac ()
*
* ACpower(uint16_t Pm) = ACpower(MAXPOWER, 25, 26, 39, 36)
*/
#ifndef ACpower_esp32_h
#define ACpower_esp32_h

#include "esp32-adc-nowait.h"

#if defined(ESP32)

#define ACPOWER_LIBVERSION "ACpower_v20190718 "

//#define ZC_CRAZY		// если ZeroCross прерывание выполняется слишком часто :-(
#define ACPOWER_ZCEDGE RISING	// FALLING, RISING

#define ACPOWER_ADC_RATE 200    // количество отсчетов АЦП на ПОЛУволну - 200 (для прерываний)
#define ACPOWER_ADC_WAVES 10    // количество обсчитываемых ПОЛУволн - 4
#define ACPOWER_ADC_NOISE 1000  // попробуем "понизить" шум АЦП
#define ACPOWER_ADC_I_RATIO 0.02	// значение по умолчанию
#define ACPOWER_ADC_U_RATIO 0.2 	// значение по умолчанию

//#define I_ZERO 1942     //1907
//#define U_ZERO 1931     //2113

#define ACPOWER_PIN_U 39
#define ACPOWER_PIN_I 36
#define ACPOWER_PIN_ZC 25
#define ACPOWER_PIN_TR 26

#define ACPOWER_ANGLE_MIN 1000		// минимальный угол открытия - определяет MIN возможную мощность
#define ACPOWER_ANGLE_MAX 10100		// максимальный угол открытия триака - определяет MAX возможную мощность
#define ACPOWER_ANGLE_DELTA 100		// запас по времени для открытия триака
#define ACPOWER_POWER_MAX 3000		// больше этой мощности установить не получится
#define ACPOWER_POWER_MIN 50		// минимально допустимая устанавливаемая мощность (наверное можно и меньше)

#define TIMER_TRIAC 0
#define TIMER_ADC 3
#define SHIFT_CHECK_SAMPLES 10000	// количество отсчетов для определения "нулевого" уровня

#define DEBUG0
#define DEBUG1
#define DEBUG2

class ACpower
{
public:
	//ACpower();
	//ACpower(uint16_t Pm, uint8_t pinZeroCross, uint8_t pinTriac, uint8_t pinVoltage, uint8_t pinCurrent);
	ACpower(uint16_t Pm, uint8_t pinZeroCross, uint8_t pinTriac, uint8_t pinVoltage, uint8_t pinCurrent, bool ShowLog = true);
	
	float Inow = 0;   		// переменная расчета RMS тока
	float Unow = 0;   		// переменная расчета RMS напряжения

	bool ZC;
	uint16_t Pnow, Pavg;
	uint16_t Pset = 0;
	uint16_t Pmax = 0;
	
	String LibVersion = ACPOWER_LIBVERSION;
	String LibConfig;
	
	volatile static uint32_t CounterZC;
	volatile static uint32_t CounterTR;

	uint32_t CounterRMS = 0;
	uint32_t MillisRMS = 0;
	
	volatile static int16_t Xnow;
	volatile static uint32_t X2;
	volatile static uint16_t Angle; 

	void init();		// ACPOWER_ADC_I_RATIO, ACPOWER_ADC_U_RATIO  по умолчанию
	void init(float Iratio, float Uratio);
	void init(float Iratio, float Uratio, bool NeedCalibrate);
	
	void control();
	void control(uint16_t setAngle);
	void check();
	void stop();
	void setpower(uint16_t setP);
	void printConfig();
	void setRMSzerolevel();
	void setRMSzerolevel(uint16_t Scntr);
	void setRMSratio(float Iratio, float Uratio);
	void setRMScorrection(float *pIcorr, float *pUcorr);
	//=== Прерывания
	static void ZeroCross_int();
	static void GetADC_int();
	static void OpenTriac_int(); 
	//static void CloseTriac_int(); //__attribute__((always_inline));
	// === test
#ifdef DEBUG2
	volatile static uint32_t ZCcore;
	volatile static uint16_t ZCprio;
	volatile static uint32_t ADCcore;
	volatile static uint16_t ADCprio;
	volatile static uint32_t TRIACcore;
	volatile static uint16_t TRIACprio;
	volatile static uint32_t CounterTRopen;
	volatile static uint32_t CounterTRclose;
	volatile static uint32_t TimerTRopen, TimerTRclose;
	uint32_t RMScore;
	uint16_t RMSprio;
#endif

	volatile static uint32_t _Icntr;
	volatile static uint32_t _Ucntr;

protected:
	
	void setup_ZeroCross();
	void setup_Triac();
	void setup_ADC();
	void correctRMS();
	void check_ZC();
	uint16_t get_ZeroLevel(uint8_t z_pin, uint16_t Scntr);
	
	int16_t _angle = 0;
	float _Uratio;
	float _Iratio;
	
	bool _ShowLog;
	bool _corrRMS = false;
	float *_pUcorr = NULL, *_pIcorr = NULL;
	
	hw_timer_t* timerADC = NULL;
	static hw_timer_t* timerTriac;
	volatile static SemaphoreHandle_t smphRMS;
	static portMUX_TYPE muxADC;
	
	volatile static bool getI;
	volatile static bool takeADC;

	volatile static uint8_t _zero;
	volatile static uint8_t _pin;
	static uint8_t _pinI;
	static uint8_t _pinU;
	static uint8_t _pinTriac;
	uint8_t _pinZCross;

	volatile static uint64_t _summ;
	volatile static uint64_t _I2summ;
	volatile static uint64_t _U2summ;

	volatile static uint32_t _cntr;
	volatile static uint32_t _zcCounter;

	volatile static uint16_t _zerolevel;
	static uint16_t _Izerolevel;
	static uint16_t _Uzerolevel;

	volatile static uint32_t _msZCmillis;
	uint32_t _zcCheckMillis;
    //volatile static bool trOpened;
	
	void log_cfg(String str0);
	void log_cfg(String str0, uint16_t num1);
	void log_cfg_ln(String str0);
	void log_cfg_f(String str0, String str1);
	void log_cfg_f(String str0, uint16_t num1);
/*
#ifdef U_CORRECTION
	float Ucorr[25] = U_CORRECTION;
#endif
*/
};

#endif // ESP32

#endif //ACpower_esp32_h
