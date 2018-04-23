// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#include "ACpower.h"

/*
Вход с детектора нуля - D3
Выход на триак - D5
Аналоговый вход с датчика тока - A1 (ACS712)
Аналоговый вход с датчика напряжения - A0 (трансформатор с обвязкой подлюченный к ВЫХОДУ триака)
*/

// все подробности http://forum.homedistiller.ru/index.php?topic=166750.0

// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах


uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

ACpower TEH(MAXPOWER, 3, 5, A0, A1); 
/*
ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
pinZeroCross - номер пина к которому подключен детектор нуля
pinTriac - номер пина к которому вывод управления триаком
pinVoltage - "имя" вывода к которому подключен "датчик напряжения" (трансформатор с обвязкой)
pinACS712 - "имя" вывода к которому подключен "датчик тока" ACS712
*/
// ACpower TEH(MAXPOWER);	// тоже допустимо и эквивалентно ACpower TEH(MAXPOWER, 3, 5, A0, A1); 

void setup()
{
	Serial.begin(SERIALSPEED);
	TEH.init(20);			// вызов с одним параметром - допустимы только три значения: 20 - ACS712-20A, 30 - ACS712-30A, 5 - ACS712-5A
	// TEH.init(0.029, 1);	// вызов с двумя параметрами
	// в этом случае задаётся не тип ACS712, а конкретный множитель для датчика тока (первый параметр)
	// вторым параметром идет множитель напряжения - полезно если невозможно откалибровать подстроечником
	lcd.init();				// Для подключения ЛСД экрана через I2C
	delay(3000);
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
				Var = "";
			}
		}
	}
}
