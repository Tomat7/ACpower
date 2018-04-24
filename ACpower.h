/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
* Version 0.7
* 
* A0 - подключение "измерителя" напряжения (трансформатор, диодный мост, делитель напряжения)
* A1 - подключение "выхода" датчика тока ACS712
* D5 - управление триаком
* D3 - детектор нуля
* это условный "стандарт", потому как все эти  входы-выходы можно менять
* детектор нуля может быть на D2 или D3
* управление триаком почти на любом цифровом выходе порта D, то есть D2-D7
* эти входы-выходы могут (или должны) задаваться при инициализации объекта ACpower
*
*	ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712);
	Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
	pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
	pinTriac - номер пина который управляет триаком (2-7)
	pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
	pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*	
* 	ACpower(uint16_t Pm) = ACpower(MAXPOWER, 3, 5, A0, A1) - так тоже можно
*/
#ifndef ACpower_h
#define ACpower_h

#include "Arduino.h"

#define LIBVERSION "ACpower_v20180424 zeroI: "
#define ZERO_OFFSET 100			// минимальный угол открытия. *** возможно нужно больше!! ***
#define MAX_OFFSET 18000     	// Максимальный угол открытия триака. (определяет минимально возможную мощность)
#define PMIN 50				// минимально допустимая устанавливаемая мощность (наверное можно и меньше)
#define ACS_RATIO5 0.024414063	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO20 0.048828125	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO30 0.073242188	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

//#define CALIBRATE_ZERO  // выполнять процедуру калибровки ноля датчика тока, бывает нужно если ACS712 заметно врёт
#ifndef CALIBRATE_ZERO
#define _zeroI 512
#endif

#define EXTEND_U_RANGE
/*
увеличение в 2-3 раза "динамического диапазона" АЦП при измерении напряжения 
требуется изменение схемы и подбора Uratio (возможно перекалибровка измерителя напряжения)
при Uratio=1 подсчет напряжения идет как и раньше
*/
//#define U_RATIO 0.3	// множитель напряжения - теперь он в public и задается при создании объекта


class ACpower
{
public:
	ACpower(uint16_t Pm);
	ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712);

	float Inow = 0;   		// переменная расчета RMS тока
	float Unow = 0;   		// переменная расчета RMS напряжения

	int Angle = MAX_OFFSET;
	uint16_t Pnow;
	uint16_t Pavg;			// грубо говоря, это средняя мощность за 3 измерения
	uint16_t Pset = 0;
	uint16_t Pmax;

	void init();
	void init(byte ACS712type);
	void init(float Iratio, float Uratio);
	
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
	float _Uratio;
	float _Iratio;
	volatile static bool getI;
	volatile static unsigned int _cntr;
	volatile static unsigned long _Summ;
	volatile static unsigned int _angle;
	unsigned long _ADCmillis;
	volatile static byte _pinTriac;
	byte _pinZCross;
	byte _pinI;
	byte _pinU;
	byte _admuxI;
	byte _admuxU;
	#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
	#endif
};

#endif
