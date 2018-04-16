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
#define ZERO_OFFSET 1			// минимальный угол открытия. *** возможно нужно больше!! ***
#define MAX_OFFSET 2250     	// Максимальный угол открытия триака. (определяет минимально возможную мощность)
#define ACS_RATIO 0.048828125	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ZCROSS 3			// пин подключения детектора нуля.
#define TRIAC 5				// пин управляющий триаком. пока не проверялось! возможно дальше порт прямо указан в программе!
#define PMIN 50				// минимально допустимая устанавливаемая мощность (наверное можно и меньше)

//#define CALIBRATE_ZERO  // выполнять процедуру калибровки ноля датчика тока
#ifndef CALIBRATE_ZERO
#define _zeroI 512
#endif

#define EXTEND_U_RANGE	// увеличение "динамического диапазона" измерений напряжения в 4 раза
						// требуется изменение схемы и перекалибровка измерителя напряжения
						// при Uratio=1 подсчет напряжения идет как и раньше
						
//#define U_RATIO 0.2857	// множитель напряжения - теперь он в public и задается при создании объекта
//#define LAG_FACTOR 10		// Коэффициент интеграции в расчете угла DEBUG! **выпилил за ненадобностью**
//#define AVG_FACTOR 2		// Фактор усреднения измеренного тока DEBUG! **выпилил за ненадобностью**

class ACpower
{
public:
	ACpower(uint16_t Pm);
	//ACpower(uint16_t Pm, float Ur);
	float Inow = 0;   		// переменная расчета RMS тока
	float Unow = 0;   		// переменная расчета RMS напряжения

	int Angle = MAX_OFFSET<<2;
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;
	float Uratio;

	//uint8_t LagFactor = 1;	// DEBUG!! убрать
	uint16_t ADCperiod;		// DEBUG!! убрать

	void init();
	void init(float Ur);
	//void init(uint16_t Pm);
	
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
	volatile static unsigned int _cntr;
	volatile static unsigned long _Summ;
	volatile static unsigned int _angle;
	unsigned long _ADCmillis;
	//unsigned long _ADCmicros;
	#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
	#endif
};
//extern ACpower TEH;

#endif
