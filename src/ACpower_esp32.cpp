/*
	* Оригинальная идея и алгорим регулирования напряжения (c) Sebra
	* Алгоритм регулирования тока (c) Chatterbox
	* 
	* Вольный перевод в библиотеку мелкие доработки алгоритма - Tomat7
*/

#include "Arduino.h"
#include "ACpower.h"
// defines for setting and clearing register bits

#if defined(ESP32)

#define DELAYx vTaskDelay(300 / portTICK_PERIOD_MS)

#define PRINTF(a, ...) Serial.print(a); Serial.println(__VA_ARGS__)
#define PRINTLN(...) (Serial.println(__VA_ARGS__))
#define PRINT(...) (Serial.print(__VA_ARGS__))

#ifdef DEBUG1
#define DPRINTLN(...) (Serial.println(__VA_ARGS__))
#define DPRINT(...) (Serial.print(__VA_ARGS__))
#define DPRINTF(a, ...) Serial.print(a); Serial.println(__VA_ARGS__)
#else
#define DPRINTLN(...)
#define DPRINT(...)
#define DPRINTF(a, ...)
#endif

#ifdef DEBUG2
#define D(a) a
#else
#define D(a)
#endif

//volatile SemaphoreHandle_t ACpower::smphTriac;
volatile SemaphoreHandle_t ACpower::smphRMS;
portMUX_TYPE ACpower::muxADC = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *ACpower::timerTriac = NULL;

volatile bool ACpower::getI = true;
volatile bool ACpower::takeADC = false;

volatile int16_t ACpower::Xnow;
volatile uint32_t ACpower::X2;

volatile uint8_t ACpower::_zero = 1;
volatile uint32_t ACpower::CounterZC;
volatile uint32_t ACpower::CounterTR;

volatile uint8_t ACpower::_pin;
uint8_t ACpower::_pinI;
uint8_t ACpower::_pinU;
uint8_t ACpower::_pinTriac;

volatile uint32_t ACpower::_cntr = 1;
volatile uint32_t ACpower::_Icntr = 1;
volatile uint32_t ACpower::_Ucntr = 1;

volatile uint64_t ACpower::_summ = 0;
volatile uint64_t ACpower::_I2summ = 0;
volatile uint64_t ACpower::_U2summ = 0;

volatile uint16_t ACpower::_zerolevel = 0;
uint16_t ACpower::_Izerolevel = I_ZERO;
uint16_t ACpower::_Uzerolevel = U_ZERO;

volatile uint16_t ACpower::_angle;

volatile uint32_t ACpower::_msZCmillis;
//volatile bool ACpower::trOpened;

//float _Uratio;
//float _Iratio;

#ifdef DEBUG2
volatile uint32_t ACpower::ZCcore, ACpower::ADCcore, ACpower::TRIACcore;
volatile uint16_t ACpower::ZCprio, ACpower::ADCprio, ACpower::TRIACprio;
volatile uint32_t ACpower::CounterTRopen;
volatile uint32_t ACpower::CounterTRclose;
volatile uint64_t ACpower::TRIACtimerOpen, ACpower::TRIACtimerClose;
#endif

ACpower::ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinCurrent)
{
	Pmax = Pm;
	_pinZCross = pinZeroCross;	// пин подключения детектора нуля.
	_pinTriac = pinTriac;		// пин управляющий триаком. 
	_pinU = pinVoltage;		// аналоговый пин к которому подключен модуль измерения напряжения
	_pinI = pinCurrent;		// аналоговый пин к которому подключен датчик ACS712 или траснформатор тока
	_pin = _pinI;
	return;
}

void ACpower::init(float Iratio, float Uratio)
{  
	_Iratio = Iratio;
	_Uratio = Uratio;
	DELAYx;
	setup_Triac();
	DELAYx;
	setup_ZeroCross();
	DELAYx;
	setup_ADC();
	DELAYx;
	return;
}

void ACpower::setup_Triac()
{
	pinMode(_pinTriac, OUTPUT);
	_angle = 0; // MAX_OFFSET;
	//smphTriac = xSemaphoreCreateBinary();
	timerTriac = timerBegin(TIMER_TRIAC, 80, true);
	timerAttachInterrupt(timerTriac, &OpenTriac_int, true);
	timerAlarmWrite(timerTriac, (ANGLE_MAX + ANGLE_DELTA), true);
	timerAlarmEnable(timerTriac);
	timerWrite(timerTriac, _angle);
	PRINTLN("+ TRIAC setup OK");
	return;
}

void ACpower::setup_ZeroCross()
{
	takeADC = false;
	_msZCmillis = millis();
	smphRMS = xSemaphoreCreateBinary();
	//smphZC = xSemaphoreCreateBinary();
	pinMode(_pinZCross, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, ZC_EDGE);
	PRINTLN("+ ZeroCross setup OK");
	return;
}

void ACpower::setup_ADC()
{
	uint16_t usADCinterval = (uint16_t)(10000 / ADC_RATE);
	uint16_t ADCperSet = ADC_RATE * ADC_WAVES;
	//smphADC = xSemaphoreCreateBinary();
	timerADC = timerBegin(TIMER_ADC, 80, true);
	timerAttachInterrupt(timerADC, &GetADC_int, true);
	timerAlarmWrite(timerADC, usADCinterval, true);
	timerAlarmEnable(timerADC);
	PRINTLN("+ ADC Inerrupt setup OK");
	PRINTF(". ADC microSeconds between samples: ", usADCinterval);
	PRINTF(". ADC samples per half-wave: ", ADC_RATE);
	PRINTF(". ADC samples per calculation set: ", ADCperSet);
	PRINTF(". ADC half-waves per calculation set: ", ADC_WAVES);
	return;
}

void ACpower::control()
{	
	if (xSemaphoreTake(smphRMS, 0) == pdTRUE)
	{ 
		CounterRMS++;
		if (getI) Unow = sqrt(_U2summ / _Ucntr) * _Uratio;
		else Inow = sqrt(_I2summ / _Icntr) * _Iratio;
		
		#ifdef U_CORRECTION
		if ((getI) && (rmsCalibrated) && (Unow < 240))
		{
			float U_head = Unow / 10;
			int n = (int)U_head;
			float U_tail = U_head - n;
			float Ushift = Ucorr[n] + (Ucorr[n + 1] - Ucorr[n]) * U_tail;
			Unow += Ushift;
		}
		#endif
		
		Pnow = (uint16_t)(Inow * Unow);
		
		if (Pset > 0)
		{
			Angle += Pset - Pnow;
			Angle = constrain(Angle, ANGLE_MIN, ANGLE_MAX - ANGLE_DELTA);
		}
		else Angle = ANGLE_MIN - 500;
		_angle = Angle;
		D(RMScore = xPortGetCoreID());
		D(RMSprio = uxTaskPriorityGet(NULL));
	}
	return;
}


void IRAM_ATTR ACpower::ZeroCross_int() //__attribute__((always_inline))
{
	
	if ((millis() - _msZCmillis) > 5)
	{
		//xSemaphoreGiveFromISR(smphZC, NULL);
		//D(_usZCmicros = micros());
		
		timerStop(timerTriac);
		digitalWrite(_pinTriac, LOW);
		//trOpened = false;
		_msZCmillis = millis();
		_zero++;
		CounterZC++;
		
		if (_zero >= ADC_WAVES)
		{
			portENTER_CRITICAL_ISR(&muxADC);
			takeADC = false;
			
			if (getI)
			{
				_pin = _pinU;
				getI = false;
				_zerolevel = _Uzerolevel;
				_I2summ = _summ;
				_Icntr = _cntr;
			}
			else
			{
				_pin = _pinI;
				getI = true;
				_zerolevel = _Izerolevel;
				_U2summ = _summ;
				_Ucntr = _cntr;
			}
			
			adcAttachPin(_pin);
			_summ = 0;
			_cntr = 0;
			_zero = 0;
			adcStart(_pin);
			portEXIT_CRITICAL_ISR(&muxADC);
			xSemaphoreGiveFromISR(smphRMS, NULL);
		}
		timerWrite(timerTriac, _angle);
		timerStart(timerTriac);
		D(ZCcore = xPortGetCoreID());
		D(ZCprio = uxTaskPriorityGet(NULL));
		//D(usZCduration = micros() - _usZCmicros);
	}
	return;
}

void IRAM_ATTR ACpower::GetADC_int() //__attribute__((always_inline))
{
	portENTER_CRITICAL_ISR(&muxADC);
	
	if (takeADC)
	{
		Xnow = adcEnd(_pin) - _zerolevel;
		X2 = Xnow * Xnow;
		if (X2 < ADC_NOISE) X2 = 0;
		_summ += X2;
		_cntr++;
		adcStart(_pin);
		//D(if (getI) { if (Xnow > adcImax) adcImax = Xnow; } else { if (Xnow > adcUmax) adcUmax = Xnow; });
	}
	else if (_cntr == 0)
	{
		adcEnd(_pin);
		takeADC = true;
		adcStart(_pin);
	}
	D(ADCcore = xPortGetCoreID());
	D(ADCprio = uxTaskPriorityGet(NULL));
	
	portEXIT_CRITICAL_ISR(&ACpower::muxADC);
	return;
}

void IRAM_ATTR ACpower::OpenTriac_int() //__attribute__((always_inline))
{
	uint64_t _tmrTriacNow = timerRead(timerTriac);
	if ((_tmrTriacNow > ANGLE_MIN) && (_tmrTriacNow < ANGLE_MAX))
	{
		digitalWrite(_pinTriac, HIGH);
		//trOpened = true;
		//xSemaphoreGiveFromISR(smphTriac, NULL);

		//D(usZCtoTRIAC = micros() - _usZCmicros);
		D(TRIACtimerOpen = _tmrTriacNow);
		//D(_usTRmicros = micros());
		D(CounterTRopen++);
	}
	else
	{
		digitalWrite(_pinTriac, LOW);
		//trOpened = false;

		D(TRIACtimerClose = _tmrTriacNow);
		//D(usTRopen = micros() - _usTRmicros);
		D(CounterTRclose++);
	}
	timerStop(timerTriac);
	D(CounterTR++);
	D(TRIACcore = xPortGetCoreID());
	D(TRIACprio = uxTaskPriorityGet(NULL));
}

	
void ACpower::printConfig()
{
	Serial.print(F(LIBVERSION));
	//Serial.print(_zeroI);
	Serial.print(F(", U-meter on A"));
	Serial.print(_pinU);
	Serial.print(F(", I-meter on A"));
	Serial.println(_pinI);
}

#ifdef CALIBRATE_ZERO

#endif

void ACpower::setpower(uint16_t setPower)
{	
	if (setPower > Pmax) Pset = Pmax;
	else if (setPower < POWER_MIN) Pset = 0;
	else Pset = setPower;
	return;
}


void ACpower::check()
{
	control();
}


#endif // ESP32
