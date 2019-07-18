/*
* Оригинальная идея (c) Sebra
* Базовый алгоритм регулирования (c) Chatterbox
* Алгоритм с привязкой расчетов к детектору нуля, поддержка ESP32 и перевод в библиотеку (c) Tomat7
*/

#include "Arduino.h"
#include "ACpower.h"
#include "ACpower_macros.h"

#if defined(ESP32)

portMUX_TYPE ACpower::muxADC = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t *ACpower::timerTriac = NULL;
volatile SemaphoreHandle_t ACpower::smphRMS;

volatile bool ACpower::getI = true;
volatile bool ACpower::takeADC = false;

volatile uint16_t* ACpower::_pAngle;
volatile uint16_t ACpower::Angle;
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
uint16_t ACpower::_Izerolevel = 0;
uint16_t ACpower::_Uzerolevel = 0;

volatile uint32_t ACpower::_msZCmillis;
//volatile bool ACpower::trOpened;

#ifdef DEBUG2
volatile uint32_t ACpower::ZCcore, ACpower::ADCcore, ACpower::TRIACcore;
volatile uint16_t ACpower::ZCprio, ACpower::ADCprio, ACpower::TRIACprio;
volatile uint32_t ACpower::CounterTRopen;
volatile uint32_t ACpower::CounterTRclose;
volatile uint32_t ACpower::TimerTRopen, ACpower::TimerTRclose;
#endif

void IRAM_ATTR ACpower::ZeroCross_int() //__attribute__((always_inline))
{
	if ((millis() - _msZCmillis) > 5)
	{
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
		timerWrite(timerTriac, *_pAngle);
		timerStart(timerTriac);
		D(ZCcore = xPortGetCoreID());
		D(ZCprio = uxTaskPriorityGet(NULL));
		//D(usZCduration = micros() - _usZCmicros);
	}
	return;
}

void IRAM_ATTR ACpower::ZeroCross_3phase_int() //__attribute__((always_inline))
{
	if ((millis() - _msZCmillis) > 5)
	{
		timerStop(timerTriac);
		digitalWrite(_pinTriac, LOW);
		//trOpened = false;
		_msZCmillis = millis();
		//_zero++;
		CounterZC++;
				
		timerWrite(timerTriac, *_pAngle);
		timerStart(timerTriac);
		Angle = *_pAngle;
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
	}
	else if (_cntr == 0)
	{
		adcEnd(_pin);
		takeADC = true;
		adcStart(_pin);
	}
	
	portEXIT_CRITICAL_ISR(&ACpower::muxADC);
	D(ADCcore = xPortGetCoreID());
	D(ADCprio = uxTaskPriorityGet(NULL));
	return;
}

void IRAM_ATTR ACpower::OpenTriac_int() //__attribute__((always_inline))
{
	uint64_t _tmrTriacNow = timerRead(timerTriac);
	if ((_tmrTriacNow > ANGLE_MIN) && (_tmrTriacNow < ANGLE_MAX))
	{
		digitalWrite(_pinTriac, HIGH);
		//trOpened = true;
		D(CounterTRopen++);
		D(TimerTRopen = _tmrTriacNow);
	}
	else
	{
		digitalWrite(_pinTriac, LOW);
		//trOpened = false;
		D(CounterTRclose++);
		D(TimerTRclose = _tmrTriacNow);
	}
	timerStop(timerTriac);
	D(CounterTR++);
	D(TRIACcore = xPortGetCoreID());
	D(TRIACprio = uxTaskPriorityGet(NULL));
}


#endif // ESP32
