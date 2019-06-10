// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#define ACS_RATIO5 0.024414063  // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO20 0.048828125 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO30 0.073242188 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

// подробности http://forum.homedistiller.ru/index.php?topic=166750.0
// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах

#include "ACpower.h"
/*
ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
pinTriac - номер пина который управляет триаком (2-7)
pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*/
#if defined(__AVR__)      // Arduino Nano/Mini - Atmel328p
#define PIN_ZEROCROSS 3   // 3 - детектор нуля
#define PIN_TRIAC 5       // 5 - триак 
#define PIN_U A0          // A0 - датчик напряжения
#define PIN_I A1          // A1 - датчик тока

#elif defined(ESP32)      // ESP32 (Wemos Lolin32)
#define PIN_ZEROCROSS 25
#define PIN_TRIAC 26
#define PIN_U 39
#define PIN_I 36

#else
#error "Chip not supported. Use AVR or ESP32."
#endif

//ACpower TEH(MAXPOWER);  
// эквивалентно ACpower TEH(MAXPOWER, 3, 5, A0, A1); // только для AVR 

ACpower TEH(MAXPOWER, PIN_ZEROCROSS, PIN_TRIAC, PIN_U, PIN_I); 
// "длинный" вариант возволяет сконфигурировать пины внешней обвязки

uint16_t inst_P = 0;
unsigned long msShow = 0;
String Stext, Svar;

void setup()
{
	Serial.begin(SERIALSPEED);
	// TEH.init(ACS712_20);  // Atmel328p: ACS712_5, ACS712_20, ACS712_30. Датчик напряжения должен быть откалиброван!
	// TEH.init();                // вызов без параметров = датчик тока ACS712-20A и откалиброванный датчик напряжения
#if defined(__AVR__)
	TEH.init(0.048828125, 1);	// можно задать коэффициенты (множители) датчиков тока и напряжения
#elif defined(ESP32)
	TEH.init(0.02, 0.1);	  	// для трансформатора тока, датчик тока с выпрямителем
#endif
	// вызов с двумя параметрами - в этом случае задаётся коэффициент ACS712 или трансформатора тока,
	// вторым параметром идет множитель для напряжения - полезно если невозможно откалибровать подстроечником
	// и при изменении схемы позволяет использовать почти весь диапазон АЦП Ардуино
	delay(300);
	Serial.println(F(SKETCHVERSION));
}

void loop()
{
	TEH.control();	// нужно вызывать регулярно для пересчета мощности и угла открытия триака
	if ((millis() - msShow) > SHOWINTERVAL)
	{
		chkSerial();
		showInfo();
		msShow = millis();
	}
}

void showInfo()
{
	Serial.print("Pnow=");
	Serial.println(TEH.Pnow);
	Serial.print("Pset=");
	Serial.println(TEH.Pset);
	Serial.print("Unow=");
	Serial.println(TEH.Unow);
	Serial.print("Inow=");
	Serial.println(TEH.Inow);
}

void chkSerial() {
	while (Serial.available()) //Serial port, пока не конец сообщения, читаем данные и формируем строку
	{
		char ch = Serial.read();
		Svar += ch;
		if (ch == '\n')
		{
			Svar.toUpperCase();
			if (Svar.substring(0, 2) == "SP")
			{
				Stext = Svar.substring(Svar.indexOf("SP", 2) + 3); //команда
				inst_P = Stext.toFloat();          //Выставленная мощность с Serial
				TEH.setpower(inst_P);
			}
				Svar = "";
		}
	}
}
