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

//ACpower TEH;              // preinstatiate

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

ISR(TIMER1_COMPA_vect) {
	ACpower::OpenTriac_int();
}

// ==== Обработка прерывания по переполнению таймера. необходима для "гашения" триака 
ISR(TIMER1_COMPB_vect) { //timer1 overflow
	ACpower::CloseTriac_int();
}

//================= Обработка прерывания АЦП для расчета среднеквадратичного тока
ISR(ADC_vect) {
	ACpower::GetADC_int();
}

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
}

void ACpower::init(float Iratio, float Uratio)
{
	init(Iratio, Uratio, true);
}

void ACpower::init(float Iratio, float Uratio, bool SerialInfo)
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
	//_Summ=0;
	//Включение АЦП
	ADCSRA = B11101111; 
	ACSR = (1 << ACD);
	
	//- Timer1 - Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCCR1B = (0 << CS12) | (1 << CS11); // Тактирование от CLK. 20000 отсчетов 1 полупериод. (по таблице внизу)
	_angle = MAX_OFFSET;
	OCR1A = int(_angle);				// для открытия триака
	OCR1B = int(MAX_OFFSET + 1000);		// для закрытия триака за ~500 мкс до ZeroCross			
	TIMSK1 |= (1 << OCIE1A);	// Разрешить прерывание по совпадению A
	TIMSK1 |= (1 << OCIE1B);	// Разрешить прерывание по совпадению B
	
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, RISING);	//вызов прерывания при детектировании нуля
	
	if (SerialInfo)
	{
		Serial.print(F(LIBVERSION));
		Serial.print(_zeroI);
		String ACinfo = ", U-meter on A" + String(_pinU, DEC) + ", ACS712 on A" + String(_pinI);
		Serial.println(ACinfo);
	}
	return;
}

void ACpower::control()
{	
	if (_zero == 0)
	{
		uint16_t Pold;
		_zero++;
		Inow = sqrt(_I2summ / _Icntr) * _Iratio;
		Unow = sqrt(_U2summ / _Ucntr) * _Uratio;  // if Uratio !=1 требуется изменение схемы и перекалибровка подстроечником!
		Pold = Pavg;
		Pavg = Pnow;
		Pnow = Inow * Unow;
		Pavg = (Pnow + Pavg + Pold) / 3;
		
		//if (abs(Pnow - Pset) < 10) _zero++;
		//if (((Pset > 0) && (Pnow != Pavg)) || ((_zero == 0) && (Pavg != Pold)))
		
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
	//PORTD &= ~(1 << TRIAC); // установит "0" на выводе D5 - триак закроется
	//cbi(PORTD, TRIAC);
	OCR1A = int(_angle);
	//OCR1B = int(_angle + 1000); // можно и один раз в самом начале.
	TCNT1 = 0;
	_zero++;
	
	if (_zero == (WAVE_COUNT + 1)) 
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
	//PORTD |= (1 << TRIAC);  - установит "1" и откроет триак
	//PORTD &= ~(1 << TRIAC); - установит "0" и закроет триак
}

void ACpower::CloseTriac_int() //__attribute__((always_inline))
{
	cbi(PORTD, _pinTriac);
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

/*
	//===========================================================Настройка АЦП
	
	ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0); //
	
	Биты 7:6 – REFS1:REFS0. Биты выбора опорного напряжения. Если мы будем менять эти биты во время преобразования,
	то изменения вступят в силу только после текущего преобразования. В качестве опорного напряжения может быть выбран AVcc
	(ток источника питания), AREF или внутренний 2.56В источник опорного напряжения.
	Рассмотрим какие же значения можно записать в эти биты:
	REFS1:REFS0
	00    AREF
	01    AVcc, с внешним конденсатором на AREF
	10    Резерв
	11    Внутренний 2.56В  источник, с внешним конденсатором на AREF
	Бит 5 – ADLAR. Определяет как результат преобразования запишется в регистры ADCL и ADCH.
	ADLAR = 0
	Биты 3:0 – MUX3:MUX0 – Биты выбора аналогового канала.
	MUX3:0
	0000      ADC0
	0001      ADC1
	0010      ADC2
	0011      ADC3
	0100      ADC4
	0101      ADC5
	0110      ADC6
	0111      ADC7
	
	
	//--------------------------Включение АЦП
	
	ADCSRA = B11101111;
	Бит 7 – ADEN. Разрешение АЦП.
	0 – АЦП выключен
	1 – АЦП включен
	Бит 6 – ADSC. Запуск преобразования (в режиме однократного
	преобразования)
	0 – преобразование завершено
	1 – начать преобразование
	Бит 5 – ADFR. Выбор режима работы АЦП
	0 – режим однократного преобразования
	1 – режим непрерывного преобразования
	Бит 4 – ADIF. Флаг прерывания от АЦП. Бит устанавливается, когда преобразование закончено.
	Бит 3 – ADIE. Разрешение прерывания от АЦП
	0 – прерывание запрещено
	1 – прерывание разрешено
	Прерывание от АЦП генерируется (если разрешено) по завершении преобразования.
	Биты 2:1 – ADPS2:ADPS0. Тактовая частота АЦП
	ADPS2:ADPS0
	000         СК/2
	001         СК/2
	010         СК/4
	011         СК/8
	100         СК/16
	101         СК/32
	110         СК/64
	111         СК/128
	
	//------ Timer1 ---------- Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	
	TCCR1A = 0x00;  //
	TCCR1B = 0x00;    //
	TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // Тактирование от CLK.
	
	// Если нужен предделитель :
	// TCCR1B |= (1<<CS10);           // CLK/1 = 0,0000000625 * 1 = 0,0000000625, 0,01с / 0,0000000625 = 160000 отсчетов 1 полупериод
	// TCCR1B |= (1<<CS11);           // CLK/8 = 0,0000000625 * 8 = 0,0000005, 0,01с / 0,0000005 = 20000 отсчетов 1 полупериод
	// TCCR1B |= (1<<CS10)|(1<<CS11); // CLK/64 = 0,0000000625 * 64 = 0,000004, 0,01с / 0,000004 = 2500 отсчетов 1 полупериод
	// TCCR1B |= (1<<CS12);           // CLK/256  = 0,0000000625 * 256 = 0,000016, 0,01с / 0,000016 = 625 отсчетов 1 полупериод
	// TCCR1B |= (1<<CS10)|(1<<CS12); // CLK/1024
	// Верхняя граница счета. Диапазон от 0 до 255.
	OCR1A = 0;           // Верхняя граница счета. Диапазон от 0 до 65535.
	
	// Частота прерываний будет = Fclk/(N*(1+OCR1A))
	// где N - коэф. предделителя (1, 8, 64, 256 или 1024)
	
	TIMSK1 |= (1 << OCIE1A) | (1 << TOIE1); // Разрешить прерывание по совпадению и переполнению
	*/
