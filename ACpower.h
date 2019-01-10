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

#define LIBVERSION "ACpower_v20181119 zeroI: "
#define ZERO_OFFSET 10			// минимальный угол открытия. *** возможно нужно больше!! ***
#define MAX_OFFSET 19000    	// Максимальный угол открытия триака. (определяет минимально возможную мощность)
#define ACS_RATIO5 0.024414063	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO20 0.048828125	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO30 0.073242188	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

#define PMIN 50				// минимально допустимая устанавливаемая мощность (наверное можно и меньше)
#define WAVE_COUNT 4  		// сколько полуволн (half-wave) собирать/считать ток и напряжение

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
	ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712);
	
	float Inow;   		// переменная расчета RMS тока
	float Unow;   		// переменная расчета RMS напряжения

	int Angle;
	//uint16_t Pavg;
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;

	void init();
	void init(float Iratio, float Uratio);
	//void init(float Iratio, float Uratio, bool printConfig);
		
	void control();
	void check();
	void setpower(uint16_t setP);
	void printConfig();
	//String LibVersion;
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
	volatile static bool takeADC;
	volatile static bool newCalc;
	volatile static byte _admuxI;
	volatile static byte _admuxU;
	volatile static byte _zero;
	
	volatile static unsigned int _cntr;
	volatile static unsigned int _Icntr;
	volatile static unsigned int _Ucntr;
	volatile static unsigned long _Summ;
	volatile static unsigned long ACpower::_I2summ;
	volatile static unsigned long ACpower::_U2summ;
	volatile static unsigned int _angle;
	
	volatile static byte _pinTriac;
	byte _pinZCross;
	byte _pinI;
	byte _pinU;
	
	#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
	#endif
};
//extern ACpower TEH;

#endif
