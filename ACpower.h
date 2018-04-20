/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
* Version 0.7
* 
* A0 - подключение "измерителя" напряжения (трансформатор, диодный мост, делитель напряжения)
* A1 - подключение датчика тока ACS712
* D5 - управление триаком
* D3 - детектор нуля
*
*/
#ifndef ACpower_h
#define ACpower_h

#include "Arduino.h"

#define LIBVERSION "ACpower_v20180320 zeroI: "
#define ZERO_OFFSET 100			// минимальный угол открытия. *** возможно нужно больше!! ***
#define MAX_OFFSET 18000    	// Максимальный угол открытия триака. (определяет минимально возможную мощность)
#define ACS_RATIO5 0.024414063	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO20 0.048828125	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO30 0.073242188	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ZCROSS 3			// пин подключения детектора нуля.
#define TRIAC 5				// пин управляющий триаком. пока не проверялось! возможно дальше порт прямо указан в программе!
#define PMIN 50				// минимально допустимая устанавливаемая мощность (наверное можно и меньше)
#define HW_COUNT 4			// сколько полуволн (half-wave)собирать ток и напряжение

//#define CALIBRATE_ZERO  // выполнять процедуру калибровки ноля датчика тока
#ifndef CALIBRATE_ZERO
#define _zeroI 512
#endif

//#define U_RATIO 0.2857	// множитель напряжения - теперь он в public и задается при создании объекта
							// при Uratio=1 подсчет напряжения идет как и раньше - АЦП выдает значение "прямо" в вольтах
							// хотя может выдавать до 1023, если немного изменить схему, то можно собирать большие значения
							// увеличив таким образом динамический диапазон и точность измерений
							// но это требует изменения схемы и перекалибровки измерителя напряжения						
							
class ACpower
{
public:
	ACpower(uint16_t Pm);
	ACpower(uint16_t Pm, byte ACStype);
	
	volatile static float Inow;   		// переменная расчета RMS тока
	volatile static float Unow;   		// переменная расчета RMS напряжения
	volatile static float Uratio;
	volatile static float Iratio;
	
	int Angle = MAX_OFFSET;
	uint16_t Pavg;
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;

	void init();
	void init(float Ur);
	
	void control();
	void setpower(uint16_t setP);
	//=== Прерывания
	static void ZeroCross_int() __attribute__((always_inline));
	static void GetADC_int() __attribute__((always_inline));
	static void OpenTriac_int() __attribute__((always_inline));
	static void CloseTriac_int() __attribute__((always_inline));
	// === test
	#ifdef CALIBRATE_ZERO
	int calibrate();
	#endif
	
protected:
	volatile static bool getI;
	volatile static bool takeADC;
	volatile static unsigned int _cntr;
	volatile static unsigned int _zero;
	volatile static unsigned long _Summ;
	volatile static unsigned int _angle;
	#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
	#endif
};
//extern ACpower TEH;

#endif
