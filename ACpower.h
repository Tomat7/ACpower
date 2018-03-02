/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
* Version 0.5
*/
#ifndef ACpower_h
#define ACpower_h

#include "Arduino.h"

#define LIBVERSION "ACpower_v20180302 zeroI: "
#define ZEROOFFSET 40         // Сдвиг срабатывания детектора нуля относительно реального нуля в тиках таймера
#define C_TIMER 2500-ZEROOFFSET          // Максимальное количество срабатываний таймера за полупериод синусоиды
#define ACS_RATIO 0.048828125 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ZCROSS 3
#define TRIAC PORTD5	// пока не проверялось! возможно дальше порт прямо указан в программе!

#define LAG_FACTOR 10	// Коэффициент интеграции в расчете угла (defaul 300)
#define AVG_FACTOR 2	// Фактор усреднения измеренного тока (defaul 5)
#define CALIBRATE_ZERO  // выполнять процедуру калибровки ноля датчика тока

class ACpower
{
public:
	ACpower();
	//float resist = 0;
	float Inow = 0;   		// переменная расчета RMS тока
	//float Iset = 0;   		// установленный ток
	float Unow = 0;   		// переменная расчета RMS тока
	//float Uset = 0;   		// установленный ток
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;

	void init(uint16_t Pmax);
	void control();
	void setpower(int setP);
	int calibrate();
	//================= Прерывания
	static void ZeroCross_int() __attribute__((always_inline));
	static void GetADC_int() __attribute__((always_inline));
	static void SetTriac_int() __attribute__((always_inline));
	// === test
	uint16_t LagFactor;
	//uint16_t ADCperiod;
	uint16_t ADCperiod;
		
protected:
	volatile static int _zeroI;
	volatile static bool getI;
	volatile static int _cntr;
	volatile static unsigned long _Summ;
	volatile static float _angle = C_TIMER;
	unsigned long _ADCmillis;
	unsigned long _ADCmicros;
};
extern ACpower TEH;

#endif
