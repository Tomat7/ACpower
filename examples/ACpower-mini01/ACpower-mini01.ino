// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000

// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

// подробности http://forum.homedistiller.ru/index.php?topic=166750.0
// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах

#include "ACpower.h"
ACpower TEH(MAXPOWER);	// эквивалентно ACpower TEH(MAXPOWER, 3, 5, A0, A1); 

// ACpower TEH(MAXPOWER, 3, 5, A0, A1); 
// "длинный" вариант возволяет сконфигурировать пины внешней обвязки
// 3 - детектор нуля, 5 - триак, A0 - датчик напряжения, A1 - датчик тока
/*
ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
pinTriac - номер пина который управляет триаком (2-7)
pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*/
// 

uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

void setup()
{
	Serial.begin(SERIALSPEED);
	TEH.init(ACS712_20);  // Возможны варианты: ACS712_5, ACS712_20, ACS712_30. Датчик напряжения должен быть откалиброван!
	// TEH.init();                // вызов без параметров = датчик тока ACS712-20A и откалиброванный датчик напряжения
	// TEH.init(0.048828125, 1);	// можно задать коэффициенты (множители) датчиков тока и напряжения
	// TEH.init(0.029, 1);	  		// для трансформатора тока
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
		Var += ch;
		if (ch == '\n')
		{
			Var.toUpperCase();
			if (Var.substring(0, 2) == "SP")
			{
				T1 = Var.substring(Var.indexOf("SP", 2) + 3); //команда
				inst_P = T1.toFloat();          //Выставленная мощность с Serial
				TEH.setpower(inst_P);
			}
				Var = "";
		}
	}
}
