/*
* Оригинальная идея (c) Sebra
* Базовый алгоритм регулирования (c) Chatterbox
* Алгоритм с привязкой расчетов к детектору нуля, поддержка ESP32 и перевод в библиотеку (c) Tomat7
*/

#include "Arduino.h"
#include "ACpower.h"
#include "ACpower_macros.h"

#if defined(ESP32)

ACpower::ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinCurrent)
{
	Pmax = Pm;
	_pinZCross = pinZeroCross;	// пин подключения детектора нуля.
	_pinTriac = pinTriac;		// пин управляющий триаком. 
	_pinU = pinVoltage;		// аналоговый пин к которому подключен модуль измерения напряжения
	_pinI = pinCurrent;		// аналоговый пин к которому подключен датчик ACS712 или траснформатор тока
	_pin = _pinI;
	_ShowLog = false;
	return;
}

ACpower::ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinCurrent, bool ShowLog)
{
	Pmax = Pm;
	_pinZCross = pinZeroCross;	// пин подключения детектора нуля.
	_pinTriac = pinTriac;		// пин управляющий триаком. 
	_pinU = pinVoltage;		// аналоговый пин к которому подключен модуль измерения напряжения
	_pinI = pinCurrent;		// аналоговый пин к которому подключен датчик ACS712 или траснформатор тока
	_pin = _pinI;
	_ShowLog = ShowLog;
	return;
}

void ACpower::init()
{  
	init(ACPOWER_ADC_I_RATIO, ACPOWER_ADC_U_RATIO, true);
	return;
}

void ACpower::init(float Iratio, float Uratio)
{  
	init(Iratio, Uratio, true);
	return;
}

void ACpower::init(float Iratio, float Uratio, bool NeedCalibrate)
{  
	//_Iratio = Iratio;
	//_Uratio = Uratio;
	//_pAngle = (uint16_t*) malloc(sizeof(uint16_t));
	if (_ShowLog) printConfig();
	DELAYx;
	setup_Triac();
	DELAYx;
	setup_ZeroCross();
	DELAYx;

	setRMSratio(Iratio, Uratio);
	
	if (NeedCalibrate) setRMSzerolevel();
	setup_ADC();
	DELAYx;

	return;
}

void ACpower::control()
{
	if ((millis() - _zcCheckMillis) > 1000) check_ZC();
		
	if (xSemaphoreTake(smphRMS, 0) == pdTRUE)
	{ 
		CounterRMS++;
		MillisRMS = millis();
		
		if (getI) Unow = sqrt(_U2summ / _Ucntr) * _Uratio;
		else Inow = sqrt(_I2summ / _Icntr) * _Iratio;
		
		correctRMS();
		Pnow = (uint16_t)(Inow * Unow);
		Pavg = Pnow;
		
		if (Pset > 0)
		{
			_angle += Pset - Pnow;
			_angle = constrain(_angle, ACPOWER_ANGLE_MIN, ACPOWER_ANGLE_MAX - ACPOWER_ANGLE_DELTA);
		}
		else _angle = ACPOWER_ANGLE_MIN - 500;
		
		Angle = _angle;
		D(RMScore = xPortGetCoreID());
		D(RMSprio = uxTaskPriorityGet(NULL));
	}
	return;
}

void ACpower::control(uint16_t setAngle)
{
	control();
	_angle = setAngle;
	Angle = _angle;
}


void ACpower::correctRMS()
{
	if (_corrRMS)
	{
		int n;
		float X_head, X_tail;
		
		if ((getI) && (_pUcorr) && (Unow < 240))
		{
			X_head = Unow / 10;
			n = (int)X_head;
			X_tail = X_head - n;
			float Ushift = *(_pUcorr + n) + (*(_pUcorr + n + 1) - *(_pUcorr + n)) * X_tail;
			Unow += Ushift;
		}

		if ((!getI) && (_pIcorr) && (Inow < 16))
		{
			X_head = Inow;
			n = (int)X_head;
			X_tail = X_head - n;
			float Ishift = *(_pIcorr + n) + (*(_pIcorr + n + 1) - *(_pIcorr + n)) * X_tail;
			Inow += Ishift;
		}
	}
}


void ACpower::check_ZC()
{
	_zcCheckMillis = millis();

	if (_zcCounter > 5) 
	{ 
		ZC = true; 
	}
	else
	{ 
		ZC = false;
		timerStop(timerTriac);
		digitalWrite(_pinTriac, LOW);
	}
	
	_zcCounter = 0;
}


void ACpower::stop()
{
	Angle = 0;
	delay(20);
	timerStop(timerADC);
	timerDetachInterrupt(timerADC);
	timerStop(timerTriac);
	timerDetachInterrupt(timerTriac);
	detachInterrupt(digitalPinToInterrupt(_pinZCross));
	digitalWrite(_pinTriac, LOW);
}

void ACpower::setpower(uint16_t setPower)
{	
	if (setPower > Pmax) Pset = Pmax;
	else if (setPower < ACPOWER_POWER_MIN) Pset = 0;
	else Pset = setPower;
	return;
}


void ACpower::printConfig()
{
	Serial.println(F(ACPOWER_LIBVERSION));
	PRINTF("Pmax: ", Pmax);
	Serial.print(F(" . ZeroCross on pin "));
	Serial.print(_pinZCross);
	Serial.print(F(", Triac on pin "));
	Serial.println(_pinTriac);
	Serial.print(F(" . U-meter on pin "));
	Serial.print(_pinU);
	Serial.print(F(", I-meter on pin "));
	Serial.println(_pinI);
}


void ACpower::check()
{
	control();
}

#endif // ESP32
