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
#ifndef ACpower_esp32_h
#define ACpower_esp32_h

#if defined(ESP32)

#define LIBVERSION "ACpower_v20190609 "

#define ZC_CRAZY		// если ZeroCross прерывание выполняется слишком часто :-(
#define ZC_EDGE RISING	// FALLING, RISING

#define ADC_RATE 200    // количество отсчетов АЦП на ПОЛУволну - 200 (для прерываний)
#define WAVES 10        // количество обсчитываемых ПОЛУволн - 4
#define ADC_NOISE 1000  // попробуем "понизить" шум АЦП

#define U_ZERO 1931     //2113
#define I_ZERO 1942     //1907
#define U_RATIO 0.2
#define I_RATIO 0.0129
#define U_CORRECTION {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.5,0.6,0.7,2.8,8.9,12,14.1,15.2,17.3,18.4}

#define PIN_U 39
#define PIN_I 36
#define PIN_ZCROSS 25
#define PIN_TRIAC 26

#define ANGLE_MIN 1000		// минимальный угол открытия - определяет MIN возможную мощность
#define ANGLE_MAX 10100		// максимальный угол открытия триака - определяет MAX возможную мощность
#define ANGLE_DELTA 100		// запас по времени для открытия триака
#define POWER_MAX 3500		// больше этой мощности установить не получится

class ACpower
{
public:
	ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712);
	
	float Inow;   		// переменная расчета RMS тока
	float Unow;   		// переменная расчета RMS напряжения

	int Angle;
	//uint16_t Pavg;
	uint16_t Pnow;
	uint16_t Pset = 0;
	uint16_t Pmax;

	void init(float Iratio, float Uratio);
		
	void control();
	void check();
	void setpower(uint16_t setP);
	void printConfig();
	//=== Прерывания
	static void ZeroCross_int(); // __attribute__((always_inline));
	static void GetADC_int(); // __attribute__((always_inline));
	static void OpenTriac_int(); // __attribute__((always_inline));
	//static void CloseTriac_int(); //__attribute__((always_inline));
	// === test
	#ifdef CALIBRATE_ZERO
	int calibrate();
	#endif
	
protected:

TaskHandle_t taskADC = NULL;
TaskHandle_t taskUPD = NULL;
hw_timer_t * timerADC = NULL;
hw_timer_t * timerTriac = NULL;
volatile SemaphoreHandle_t smphADC, smphZC;
volatile SemaphoreHandle_t smphTriac;
volatile SemaphoreHandle_t smphRMS, smI, smU;
portMUX_TYPE muxTriac = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE muxADC = portMUX_INITIALIZER_UNLOCKED;

volatile bool getI = true;
volatile bool takeADC = false;
volatile bool trOpened = false;
volatile uint8_t _zero = 1;
volatile uint8_t _pin = PIN_I;
volatile int16_t Xnow;
volatile uint32_t X2;

volatile uint64_t _summ = 0;
volatile uint64_t _I2summ = 0;
volatile uint64_t _U2summ = 0;

volatile uint32_t _cntr = 1;
volatile uint32_t _Icntr = 1;
volatile uint32_t _Ucntr = 1;

volatile uint16_t _zerolevel = 0;
volatile uint16_t _Izerolevel = 0;
volatile uint16_t _Uzerolevel = 0;

volatile uint32_t _msZCmillis = 0;
volatile uint16_t _angle = 0; 
volatile uint32_t _cntrZC = 0;
volatile uint32_t _tmrTriacNow = 0;

float Inow = 0, Unow = 0;
int16_t Angle;
uint16_t Pset = 0, Pnow = 0;


	
	#ifdef CALIBRATE_ZERO
	volatile static int _zeroI;
	#endif
};

#endif // ESP32

#endif //ACpower_esp32_h
