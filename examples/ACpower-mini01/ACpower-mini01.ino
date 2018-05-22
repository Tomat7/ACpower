// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000

#define ACS_RATIO5 0.024414063	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO20 0.048828125	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO30 0.073242188	// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

// подробности http://forum.homedistiller.ru/index.php?topic=166750.0
// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах

#include "ACpower.h"
ACpower TEH(MAXPOWER, 3, 5, A0, A1); 
/*
ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
pinTriac - номер пина который управляет триаком (2-7)
pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*/
// ACpower TEH(MAXPOWER);	// тоже допустимо и эквивалентно ACpower TEH(MAXPOWER, 3, 5, A0, A1); 

uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

void setup()
{
	Serial.begin(SERIALSPEED);
	TEH.init(ACS_RATIO20, 1);
	// TEH.init(0.029, 1);	// вызов с параметром для трансформатора тока
	// в этом случае задаётся не тип ACS712, а конкретный множитель для датчика тока (первый параметр)
	// вторым параметром идет множитель напряжения - полезно если невозможно откалибровать подстроечником
	delay(300);
	Serial.println(F(SKETCHVERSION));
}

void loop()
{
	TEH.control();
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
