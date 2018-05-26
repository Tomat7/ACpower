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
volatile unsigned int ACpower::_cntr;
volatile unsigned long ACpower::_Summ;
volatile unsigned int ACpower::_angle;
volatile static byte ACpower::_pinTriac;
#ifdef CALIBRATE_ZERO
volatile int ACpower::_zeroI;
#endif

//=== Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
//=== устанавливается "1" на _pinTriac, а затем сбрасывается в "0" по CloseTriac_int() 
ISR(TIMER1_COMPA_vect) {
	ACpower::OpenTriac_int();
}

//=== Обработка прерывания по совпадению OCR1B необходима для "гашения" триака 
ISR(TIMER1_COMPB_vect) {
	ACpower::CloseTriac_int();
}

//=== Обработка прерывания АЦП для расчета среднеквадратичного тока
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

/*
void ACpower::init(byte ACS712type)
{
	if 		(ACS712type == 5)	init(ACS_RATIO5, 1);
	else if (ACS712type == 20)	init(ACS_RATIO20, 1);
	else if	(ACS712type == 30)	init(ACS_RATIO30, 1);
	else 
	{
		Serial.println(F("ERROR: ACS712 wrong type!"));
		init(1, 1);
	}
}
*/

void ACpower::init(float Iratio, float Uratio) //__attribute__((always_inline))
{  
	_Iratio = Iratio;
	_Uratio = Uratio;	// обычно Uratio = 1, но при этом диапазон АЦП Ардуино используется от 0 до 220 (до 310)
	// после изменении схемы возможно использовать весь диапазон АЦП (до 1023) но требуется подбор Uratio
	// и возможно перекалибровка если Uratio не удастся подобрать в пределах от 1 до 0.3 
	
	pinMode(_pinZCross, INPUT);	//детектор нуля
	pinMode(_pinTriac, OUTPUT);	//тиристор
	_angle = MAX_OFFSET;
	cbi(PORTD, _pinTriac);		//PORTD &= ~(1 << TRIAC);
	#ifdef CALIBRATE_ZERO
	_zeroI = calibrate();
	#endif
	
	// настойка АЦП
	ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (0 << MUX0); // начинаем с "начала"
	_admuxI = ADMUX | _pinI;	// состояние ADMUX для измерения тока
	_admuxU = ADMUX | _pinU;	// и напряжения
	//Включение АЦП
	ADCSRA = B11101111; 
	ACSR = (1 << ACD);
	//- Timer1 - Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	TCCR1A = 0x00;
	TCCR1B = 0x00;
	TCCR1B = (0 << CS12) | (1 << CS11); // Тактирование от CLK. 20000 отсчетов 1 полупериод. (по таблице внизу)
	OCR1A = MAX_OFFSET;
	OCR1B = MAX_OFFSET + 1000;			// и угол закрытия симистора - при установленных параметрах таймера 500 = 0.25 милисек
	TIMSK1 |= (1 << OCIE1A);	// Разрешить прерывание по совпадению A
	TIMSK1 |= (1 << OCIE1B);	// Разрешить прерывание по совпадению B
	attachInterrupt(digitalPinToInterrupt(_pinZCross), ZeroCross_int, RISING);	//вызов прерывания при детектировании нуля
	
	Serial.print(F(LIBVERSION));
	Serial.print(_zeroI);
	String ACinfo = ", U-meter on A" + String(_pinU, DEC) + ", ACS712 on A" + String(_pinI);
	Serial.println(ACinfo);
	ADMUX = _admuxI;			// начинаем со сбора тока
	getI = true;	
	_Summ=0;		
}

void ACpower::control()
{	
	if (_cntr == 1024)
	{	
		_Summ >>= 10;
		if (getI)
		{	
			ADMUX = _admuxU;	// ток уже собрали, теперь начинаем собирать НАПРЯЖЕНИЕ
			Inow = (_Summ > 2) ? sqrt(_Summ) * _Iratio : 0;
			getI = false;
		}
		else
		{	
			ADMUX = _admuxI;	// начинаем собирать ТОК 
			Unow = (_Summ > 50) ? sqrt(_Summ) * _Uratio : 0;
			getI = true;
		}
		
		//uint16_t Pold;
		//Pold = Pavg;
		//Pavg = Pnow;
		Pnow = Inow * Unow;
		//Pavg = (Pold + Pnow + Pavg) / 3;	// назовём это средней мощностью
		
		if (Pset > 0)	
		{			
			Angle += Pnow - Pset;
			Angle = constrain(Angle, ZERO_OFFSET, MAX_OFFSET);
		} else Angle = MAX_OFFSET;
		
		//cli();			// так в умных интернетах пишут, возможно это лишнее - ** и без этого работает **
		_angle = Angle;
		//_ADCmillis = millis();		// DEBUG!!
		_Summ = 0;
		_cntr = 1025;		// в счетчик установим "кодовое значение", а ZeroCross это проверим
		//sei();
	}
	return;
}

void ACpower::setpower(uint16_t setPower)
{	
	if (setPower > Pmax) Pset = Pmax;
	else if (setPower < PMIN) Pset = 0;		// меньше 50вт установить не получится
	else Pset = setPower;
	return;
}

void ACpower::ZeroCross_int()
{
	TCNT1 = 0;  			
	//cbi(PORTD, _pinTriac);			// PORTD &= ~(1 << TRIAC); установит "0" на выводе D5 - триак закроется
	OCR1A = int(_angle);				// задаём угол открытия симистора
	//OCR1B = int(_angle + 1000);			// и угол закрытия симистора - при установленных параметрах таймера 1000 = 0.5 милисек
	if (_cntr == 1025) _cntr = 1050;	// в счетчик установим "кодовое значение", а в GetADC это проверим
}

void ACpower::GetADC_int()
{
	unsigned long adcData = 0;			//мгновенные значения тока
	byte An_pin = ADCL;
	byte An = ADCH;
	if (_cntr < 1024)
	{
		adcData = ((An << 8) + An_pin);
		if (getI) adcData -= _zeroI;
		adcData *= adcData;                 // возводим значение в квадрат
		_Summ += adcData;                   // складываем квадраты измерений
		_cntr++;
		return;
	}
	if (_cntr == 1050) _cntr = 0;
	return;
}

void ACpower::OpenTriac_int()
{
	if (TCNT1 < MAX_OFFSET) sbi(PORTD, _pinTriac);
	//PORTD |= (1 << TRIAC);  - установит "1" и откроет триак
	//PORTD &= ~(1 << TRIAC); - установит "0" и закроет триак
}

void ACpower::CloseTriac_int()
{
	cbi(PORTD, _pinTriac);
}

#ifdef CALIBRATE_ZERO
	int ACpower::calibrate() 
	{
		int zero = 0;
		for (int i = 0; i < 10; i++) {
			delay(10);
			zero += analogRead();
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
