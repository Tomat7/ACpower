/*
* Оригинальная идея (c) Sebra
* Базовый алгоритм регулирования (c) Chatterbox
* Алгоритм с привязкой расчетов к детектору нуля, поддержка ESP32 и перевод в библиотеку (c) Tomat7
*/

#include "Arduino.h"
#include "ACpower.h"
#include "ACpower_macros.h"

#if defined(ESP32)

void ACpower::setup_Triac()
{
	pinMode(_pinTriac, OUTPUT);
	digitalWrite(_pinTriac, LOW);
	Angle = 0;
	timerTriac = timerBegin(TIMER_TRIAC, 80, true);
	timerAttachInterrupt(timerTriac, &OpenTriac_int, true);
	timerAlarmWrite(timerTriac, (ANGLE_MAX + ANGLE_DELTA), true);
	timerAlarmEnable(timerTriac);
	timerWrite(timerTriac, Angle);
	if (_ShowLog) PRINTLN(" + TRIAC setup OK");
	return;
}

void ACpower::setup_ZeroCross()
{
	takeADC = false;
	_msZCmillis = millis();
	pinMode(_pinZCross, INPUT_PULLUP);
	smphRMS = xSemaphoreCreateBinary();
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, ZC_EDGE);
	if (_ShowLog) PRINTLN(" + ZeroCross setup OK");
	return;
}

void ACpower::setup_ADC()
{
	uint16_t usADCinterval = (uint16_t)(10000 / ADC_RATE);
	uint16_t ADCperSet = ADC_RATE * ADC_WAVES;
	timerADC = timerBegin(TIMER_ADC, 80, true);
	timerAttachInterrupt(timerADC, &GetADC_int, true);
	timerAlarmWrite(timerADC, usADCinterval, true);
	timerAlarmEnable(timerADC);
	if (_ShowLog) 
	{
		PRINTLN(" + ADC Inerrupt setup OK");
		PRINTF(" . ADC microSeconds between samples: ", usADCinterval);
		PRINTF(" . ADC samples per half-wave: ", ADC_RATE);
		PRINTF(" . ADC samples per calculation set: ", ADCperSet);
		PRINTF(" . ADC half-waves per calculation set: ", ADC_WAVES);
	}
	return;
}


void ACpower::setRMSzerolevel()
{
	setRMSzerolevel(SHIFT_CHECK_SAMPLES);
}

void ACpower::setRMSzerolevel(uint16_t Scntr)
{
	PRINTLN(" + RMS calculating ZERO-shift for U and I...");
	Angle = 0;
	_Izerolevel = get_ZeroLevel(_pinI, Scntr);
	_Uzerolevel = get_ZeroLevel(_pinU, Scntr);
	if (_ShowLog)
	{
		PRINTF(" . RMS ZeroLevel U: ", _Uzerolevel);
		PRINTF(" . RMS ZeroLevel I: ", _Izerolevel);
	}
	return;
}

uint16_t ACpower::get_ZeroLevel(uint8_t z_pin, uint16_t Scntr)
{
	uint32_t ZeroShift = 0;
	adcAttachPin(z_pin);
	DELAYx;
	adcStart(z_pin);
	for (int i = 0; i < Scntr; i++) 
	{
		ZeroShift += adcEnd(z_pin);
		adcStart(z_pin);
		delayMicroseconds(50);
	}
	adcEnd(z_pin);
	return (uint16_t)(ZeroShift / Scntr);
}

void ACpower::setRMSratio(float Iratio, float Uratio)
{  
	_Iratio = Iratio;
	_Uratio = Uratio;
	return;
}

void ACpower::setRMScorrection(float *pIcorr, float *pUcorr)
{
	_pIcorr = pIcorr;
	_pUcorr = pUcorr;
	_corrRMS = true;
}

#endif // ESP32
