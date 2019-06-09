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

volatile bool ACpower::getI;
volatile bool ACpower::takeADC;
volatile bool ACpower::newCalc;
volatile byte ACpower::_zero;
volatile byte ACpower::_admuxI;
volatile byte ACpower::_admuxU;
volatile unsigned int ACpower::_cntr;
volatile unsigned int ACpower::_Icntr;
volatile unsigned int ACpower::_Ucntr;
volatile unsigned long ACpower::_Summ;
volatile unsigned long ACpower::_I2summ;
volatile unsigned long ACpower::_U2summ;
volatile unsigned int ACpower::_angle;
volatile byte ACpower::_pinTriac;


// === Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
// (который сбрасывается в "0" по zero_crosss_int) 

// === Обработка прерывания по совпадению OCR1B для "гашения" триака 

// === Обработка прерывания АЦП для сбора измеренных значений 


ACpower::ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
{
	Pmax = Pm;
	_pinZCross = pinZeroCross;	// пин подключения детектора нуля.
	_pinTriac = pinTriac;		// пин управляющий триаком. 
	_pinI = pinACS712;		// аналоговый пин к которому подключен датчик ACS712
	_pinU = pinVoltage;	// аналоговый пин к которому подключен модуль измерения напряжения
}

void ACpower::init(float Iratio, float Uratio)
{  
	setup_Triac();
	setup_ZeroCross();
	setup_ADC();
	return;
}

void setup_Triac()
{
	DELAYx;
	pinMode(_pinTriac, OUTPUT);
	_angle = 0; // MAX_OFFSET;
	smphTriac = xSemaphoreCreateBinary();
	timerTriac = timerBegin(0, 80, true);
	timerAttachInterrupt(timerTriac, &OpenTriac_int, true);
	timerAlarmWrite(timerTriac, (ANGLE_MAX + ANGLE_DELTA), true);
	timerAlarmEnable(timerTriac);
	timerWrite(timerTriac, _angle);
	PRINTLN("+ TRIAC setup OK");
}

void setup_ZeroCross()
{
	DELAYx;
	takeADC = false;
	_msZCmillis = millis();
	smphRMS = xSemaphoreCreateBinary();
	smphZC = xSemaphoreCreateBinary();
	pinMode(_pinZCross, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, ZC_EDGE);
	PRINTLN("+ ZeroCross setup OK");
}

void setup_ADC()
{
	DELAYx;
	usADCinterval = (uint16_t)(10000 / ADC_RATE);
	ADCperSet = ADC_RATE * WAVES;
	smphADC = xSemaphoreCreateBinary();
	timerADC = timerBegin(1, 80, true);
	timerAttachInterrupt(timerADC, &GetADC_int, true);
	timerAlarmWrite(timerADC, usADCinterval, true);
	timerAlarmEnable(timerADC);
	PRINTLN("+ ADC Inerrupt setup OK");
	PRINTF(".  ADC microSeconds between samples: ", usADCinterval);
	PRINTF(".  ADC samples per half-wave: ", ADC_RATE);
	PRINTF(".  ADC samples per calculation set: ", ADCperSet);
	PRINTF(".  ADC half-waves per calculation set: ", WAVES);
}


void ACpower::check()
{
	control();
}

void ACpower::control()
{	
	if (newCalc)
	{
		newCalc = false;
		
		if (getI) Unow = sqrt((float)_U2summ / (float)_Ucntr) * _Uratio;  // if Uratio !=1 требуется изменение схемы и перекалибровка подстроечником!
		else Inow = sqrt((float)_I2summ / (float)_Icntr) * _Iratio; // одного (float) в числителе или знаменателе достаточно
																	// дважды - это с перепугу после прочтения вумных интернетов.
		Pnow = Inow * Unow;
		
		if (Pset > 0)
		{	
			Angle += Pnow - Pset;
			Angle = constrain(Angle, ZERO_OFFSET, MAX_OFFSET);
		} 
		else Angle = MAX_OFFSET;
		_angle = Angle;
		//OCR1A = int(_angle);
	}
	return;
}


void ACpower::setpower(uint16_t setPower)
{	
	if (setPower > Pmax) Pset = Pmax;
	else if (setPower < PMIN) Pset = 0;
	else Pset = setPower;
	return;
}


void ACpower::ZeroCross_int() //__attribute__((always_inline))
{
	//PORTD &= ~(1 << TRIAC); // установит "0" на выводе D5 - триак закроется
	//cbi(PORTD, TRIAC);
	TCNT1 = 0;
	OCR1A = int(_angle);
	//OCR1B = int(_angle + 1000); // можно и один раз в самом начале.
	_zero++;
	
	if (_zero >= (WAVE_COUNT)) 
	{ 
		takeADC = false;
		if (getI) 
		{
			ADMUX = _admuxU;	// ток уже собрали, теперь начинаем собирать НАПРЯЖЕНИЕ
			getI = false;
			_I2summ = _Summ;
			_Icntr = _cntr;
		}
		else
		{
			ADMUX = _admuxI;	// начинаем собирать ТОК 
			getI = true;
			_U2summ = _Summ;
			_Ucntr = _cntr;
		}
		newCalc = true;
		_Summ = 0;
		_zero = 0;
		_cntr = 0;
	}
	return;
}

void ACpower::GetADC_int() //__attribute__((always_inline))
{
	unsigned long adcData = 0; //мгновенные значения тока
	byte An_pin = ADCL;
	byte An = ADCH;
	if (takeADC)
	{
		adcData = ((An << 8) + An_pin);
		if (getI) adcData -= _zeroI;
		adcData *= adcData;                 // возводим значение в квадрат
		_Summ += adcData;                   // складываем квадраты измерений
		_cntr++;
	}
	else if (_cntr == 0) takeADC = true;
	return;
}

void ACpower::OpenTriac_int() //__attribute__((always_inline))
{
	if (TCNT1 < MAX_OFFSET) sbi(PORTD, _pinTriac);
	//PORTD |= (1 << TRIAC);  - установит "1" и откроет триак
	//PORTD &= ~(1 << TRIAC); - установит "0" и закроет триак
}

void ACpower::CloseTriac_int() //__attribute__((always_inline))
{
	cbi(PORTD, _pinTriac);
}

void ACpower::printConfig()
{
	Serial.print(F(LIBVERSION));
	Serial.print(_zeroI);
	Serial.print(F(", U-meter on A"));
	Serial.print(_pinU);
	Serial.print(F(", I-meter on A"));
	Serial.println(_pinI);
}

#ifdef CALIBRATE_ZERO
int ACpower::calibrate() 
{
	int zero = 0;
	for (int i = 0; i < 10; i++) {
		delay(10);
		zero += analogRead(_pinI + 14);
	}
	zero /= 10;
	return zero;
}
#endif

#endif // ESP32
