/*
	* Оригинальная идея и алгорим регулирования напряжения (c) Sebra
	* Алгоритм регулирования тока (c) Chatterbox
	* 
	* Вольный перевод в библиотеку мелкие доработки алгоритма - Tomat7
*/

#include "Arduino.h"
#include "ACpower.h"
// defines for setting and clearing register bits

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

volatile bool ACpower::getI;
volatile bool ACpower::takeADC;
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

#ifdef CALIBRATE_ZERO
volatile int ACpower::_zeroI;
#endif
//=== Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
// (который сбрасывается в "0" по zero_crosss_int) 

ISR(TIMER1_COMPA_vect) { ACpower::OpenTriac_int(); }

// ==== Обработка прерывания по переполнению таймера. необходима для "гашения" триака 
ISR(TIMER1_COMPB_vect) { ACpower::CloseTriac_int(); }		//timer1 overflow

//================= Обработка прерывания АЦП для расчета среднеквадратичного тока
ISR(ADC_vect) { ACpower::GetADC_int(); }


ACpower::ACpower(uint16_t Pm)
{
	Pmax = Pm;
	_pinZCross = 3;
	_pinTriac = 5;
	_pinI = A1 - 14;
	_pinU = A0 - 14;
}

ACpower::ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
{
	Pmax = Pm;
	_pinZCross = pinZeroCross;	// пин подключения детектора нуля.
	_pinTriac = pinTriac;		// пин управляющий триаком. 
	_pinI = pinACS712 - 14;		// аналоговый пин к которому подключен датчик ACS712
	_pinU = pinVoltage - 14;	// аналоговый пин к которому подключен модуль измерения напряжения
}

void ACpower::init()
{
	init(ACS_RATIO20, 1);
	printConfig();
}

void ACpower::init(float Iratio, float Uratio)
{  
	_Iratio = Iratio;
	_Uratio = Uratio;	
	// обычно Uratio = 1, но при этом диапазон АЦП Ардуино используется от 0 до 220 (до 310)
	// после изменении схемы возможно использовать весь диапазон АЦП (до 1023) но требуется подбор Uratio
	// и возможно перекалибровка если Uratio не удастся подобрать в пределах от 1 до 0.3 
	
	pinMode(_pinZCross, INPUT);	//детектор нуля
	pinMode(_pinTriac, OUTPUT);	//тиристор
	cbi(PORTD, _pinTriac);		//PORTD &= ~(1 << TRIAC);
	#ifdef CALIBRATE_ZERO
	_zeroI = calibrate();
	#endif
	
	// настойка АЦП
	ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0); // начинаем с "начала"
	_admuxI = ADMUX | _pinI;	// состояние ADMUX для измерения тока
	_admuxU = ADMUX | _pinU;	// и напряжения
	ADMUX = _admuxI;			// начинаем со сбора тока
	getI = true;
	takeADC = false;

	//Включение АЦП
	ADCSRA = B11101111; 
	ACSR = (1 << ACD);
	
	//- Timer1 - Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCCR1B = (0 << CS12) | (1 << CS11); // Тактирование от CLK. 20000 отсчетов 1 полупериод. (по таблице внизу)
	_angle = MAX_OFFSET;
	OCR1A = int(_angle);				// для открытия триака
	OCR1B = int(MAX_OFFSET + 500);		// для закрытия триака за ~250 мкс до ZeroCross			
	TIMSK1 |= (1 << OCIE1A);	// Разрешить прерывание по совпадению A
	TIMSK1 |= (1 << OCIE1B);	// Разрешить прерывание по совпадению B
	
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, RISING);	//вызов прерывания при детектировании нуля
	
	return;
}

void ACpower::check()
{
	control();
}

void ACpower::control()
{	
	if (_zero == 0)
	{
		_zero++;
		Unow = sqrt((float)_U2summ / (float)_Ucntr) * _Uratio;  // if Uratio !=1 требуется изменение схемы и перекалибровка подстроечником!
		Inow = sqrt((float)_I2summ / (float)_Icntr) * _Iratio;  // одного (float) в числителе или знаменателе достаточно
		Pnow = Inow * Unow;
		
		if (Pset > 0)
		{	
			Angle += Pnow - Pset;
			Angle = constrain(Angle, ZERO_OFFSET, MAX_OFFSET);
		} else Angle = MAX_OFFSET;
		_angle = Angle;
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
	TCNT1 = 0;
	OCR1A = int(_angle);
	_zero++;
	
	if (_zero >= (WAVE_COUNT + 1)) 
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
}

void ACpower::CloseTriac_int() //__attribute__((always_inline))
{
	cbi(PORTD, _pinTriac);
}

/*
void ACpower::printConfig()
{
	Serial.print(F(LIBVERSION));
	Serial.print(_zeroI);
	String ACinfo = ", U-meter on A" + String(_pinU, DEC) + ", ACS712 on A" + String(_pinI);
	Serial.println(ACinfo);
}
*/

void ACpower::printConfig()
{
	Serial.print(F(LIBVERSION));
	Serial.print(_zeroI);
	Serial.print(F(", U-meter on A"));
	Serial.print(_pinU);
	Serial.print(F(", ACS712 on A"));
	Serial.println(_pinI);
}


#ifdef CALIBRATE_ZERO
int ACpower::calibrate() 
{
	int zero = 0;
	for (int i = 0; i < 10; i++) {
		delay(10);
		zero += analogRead(A1);
	}
	zero /= 10;
	return zero;
}
#endif
