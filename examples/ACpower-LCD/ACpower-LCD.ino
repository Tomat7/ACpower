// *** ВНИМАНИЕ ***
// будьте аккуратны - высокое напряжение опасно для жизни!
//
#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2); // 0x3F - адрес на шине I2C, проверяем программой i2c_scanner.ino
// Для подключения ЛСД экрана через I2C необходимо "найти" его адрес программой i2c_scanner.ino
// https://github.com/marcoschwartz/LiquidCrystal_I2C

// Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ACS_RATIO5 0.024414063	
#define ACS_RATIO20 0.048828125
#define ACS_RATIO30 0.073242188

/*
все подробности http://forum.homedistiller.ru/index.php?topic=166750.0
будьте аккуратны - высокое напряжение опасно для жизни!
каждую секунду в COM-порт выдается текущая и установленная мощность
(при отсутствии нагрузки может выдавать ерунду :)
для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
где xxxx мощность в ваттах
*/

#include "ACpower.h"
ACpower TEH(MAXPOWER, 3, 5, A0, A1); 
/*
ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
pinTriac - номер пина который управляет триаком (2-7)
pinVoltage - "имя" вывода к которому подключен "датчик напряжения" (трансформатор с обвязкой) (A0-A7)
pinACS712 - "имя" вывода к которому подключен "датчик тока" ACS712 (A0-A7)
*/
// ACpower TEH(MAXPOWER);		// = ACpower TEH(MAXPOWER, 3, 5, A0, A1); 

uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

void setup()
{
	Serial.begin(SERIALSPEED);
	TEH.init(ACS_RATIO20, 1);
	TEH.printConfig();
	// TEH.init();					//вызов без параметров подразумевает датчик тока ACS712-20A и откалиброванный датчик напряжения
	// TEH.init(ACS_RATIO20, 1);	// можно задать коэффициенты (множители) датчиков тока и напряжения
	// TEH.init(0.029, 1);			// для трансформатора тока
	// если вызов с двумя параметрами - то в этом случае первым параметром задаётся коэффициент ACS712 или трансформатора тока,
	// вторым параметром идет множитель для напряжения - полезно если невозможно откалибровать подстроечником
	// и при изменении схемы позволяет использовать почти весь диапазон АЦП Ардуино
	lcd.init();				// Для подключения ЛСД экрана через I2C
	lcd.backlight();
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
		lcdInfo();
		msShow = millis();
	}
}

void showInfo()
{
	Serial.print("Pnow=");
	Serial.println(TEH.Pnow);

	Serial.print("Pset=");
	Serial.println(TEH.Pset);

	Serial.print("U: ");
	Serial.println(TEH.Unow);
	Serial.print("I: ");
	Serial.println(TEH.Inow);
	Serial.print("Angle: ");
	Serial.println(TEH.Angle);
}

void lcdInfo()
{
	lcd.setCursor(0, 0);
	lcd.print("          ");
	lcd.setCursor(0, 0);
	lcd.print("Pnow:");
	lcd.print(TEH.Pnow);
	if (TEH.Pnow < 1000) lcd.print("w");
	lcd.setCursor(10, 0);
	lcd.print("U:");
	lcd.print((uint16_t)TEH.Unow);
	lcd.print("v");
	if (TEH.Unow < 100) lcd.print(" ");
	
	lcd.setCursor(0, 1);
	lcd.print("          ");
	lcd.setCursor(0, 1);
	lcd.print("Pset:");
	lcd.print(TEH.Pset);
	if (TEH.Pset < 1000) lcd.print("w");
	lcd.setCursor(10, 1);
	lcd.print("I:");
	lcd.print(TEH.Inow);
}

void chkSerial()
{
	while (Serial.available()) //Serial port, пока не конец сообщения, читаем данные и формируем строку
	{
		char ch = Serial.read();
		Var += ch;
		if (ch == '\n')
		{
			Var.toUpperCase();				// ??
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
