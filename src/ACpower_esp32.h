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

#if defined(ESP32)

#define LIBVERSION "ACpower_v20190610 "

#define ZC_CRAZY		// если ZeroCross прерывание выполняется слишком часто :-(
#define ZC_EDGE RISING	// FALLING, RISING

#define ADC_RATE 200    // количество отсчетов АЦП на ПОЛУволну - 200 (для прерываний)
#define ADC_WAVES 10    // количество обсчитываемых ПОЛУволн - 4
#define ADC_NOISE 1000  // попробуем "понизить" шум АЦП

#define U_ZERO 1931     //2113
#define I_ZERO 1942     //1907
//#define U_RATIO 0.2
//#define I_RATIO 0.0129
//#define U_CORRECTION {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0.6,0.7,2.8,8.9,12,14.1,15.2,17.3,18.4}

#define PIN_U 39
#define PIN_I 36
#define PIN_ZCROSS 25
#define PIN_TRIAC 26

#define ANGLE_MIN 1000		// минимальный угол открытия - определяет MIN возможную мощность
#define ANGLE_MAX 10100		// максимальный угол открытия триака - определяет MAX возможную мощность
#define ANGLE_DELTA 100		// запас по времени для открытия триака
#define POWER_MAX 3500		// больше этой мощности установить не получится
#define POWER_MIN 50		// минимально допустимая устанавливаемая мощность (наверное можно и меньше)

#define TIMER_TRIAC 0
#define TIMER_ADC 1

#define DEBUG1
#define DEBUG2

class ACpower
{
public:
	ACpower(uint16_t Pm, uint8_t pinZeroCross, uint8_t pinTriac, uint8_t pinVoltage, uint8_t pinCurrent);
	
	float Inow = 0;   		// переменная расчета RMS тока
	float Unow = 0;   		// переменная расчета RMS напряжения

	int16_t Angle = 0;
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;
	
	volatile static uint32_t CounterZC;
	volatile static uint32_t CounterTR;
	uint32_t CounterRMS = 0;

	volatile static int16_t Xnow;
	volatile static uint32_t X2;
	
	void init(float Iratio, float Uratio);
	
	void control();
	void check();
	void setpower(uint16_t setP);
	void printConfig();
	void calibrate();
	//=== Прерывания
	static void ZeroCross_int(); // __attribute__((always_inline));
	static void GetADC_int(); // __attribute__((always_inline));
	static void OpenTriac_int(); // __attribute__((always_inline));
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
	volatile static uint64_t TRIACtimerOpen, TRIACtimerClose;
	uint32_t RMScore;
	uint16_t RMSprio;
#endif


protected:
	void setup_ZeroCross();
	void setup_Triac();
	void setup_ADC();
	
	float _Uratio;
	float _Iratio;
	
	hw_timer_t* timerADC = NULL;
	static hw_timer_t* timerTriac;
	volatile static SemaphoreHandle_t smphRMS;
	static portMUX_TYPE muxADC;
	//volatile static SemaphoreHandle_t smphTriac;
	//portMUX_TYPE muxTriac = portMUX_INITIALIZER_UNLOCKED;
	
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
	volatile static uint32_t _Icntr;
	volatile static uint32_t _Ucntr;

	volatile static uint16_t _zerolevel;
	static uint16_t _Izerolevel;
	static uint16_t _Uzerolevel;

	volatile static uint32_t _msZCmillis;
    //volatile static bool trOpened;

	//volatile static uint32_t _cntrZC;
	//volatile static uint32_t _tmrTriacNow;

	volatile static uint16_t _angle; 
	
#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
#endif

#ifdef U_CORRECTION
	//static const 
	float Ucorr[25] = U_CORRECTION;
#endif
};

#endif // ESP32

#endif //ACpower_esp32_h
