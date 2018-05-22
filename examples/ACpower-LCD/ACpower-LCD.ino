// *** ВНИМАНИЕ ***
// будьте аккуратны - высокое напряжение опасно для жизни!
//
#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#include "ACpower.h"
#include <LiquidCrystal_I2C.h>
// https://github.com/marcoschwartz/LiquidCrystal_I2C
// Для подключения ЛСД экрана через I2C необходимо "найти" его адрес программой i2c_scanner.ino

/*
Вход с детектора нуля - D3
Выход на триак - D5
Аналоговый вход с датчика тока - A1 (ACS712)
Аналоговый вход с датчика напряжения - A0 (трансформатор с обвязкой подлюченный к ВЫХОДУ триака)
*/

// все подробности http://forum.homedistiller.ru/index.php?topic=166750.0
// будьте аккуратны - высокое напряжение опасно для жизни!

// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах


uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

LiquidCrystal_I2C lcd(0x3F, 16, 2); // 0x3F - адрес на шине I2C, проверяем программой i2c_scanner.ino

ACpower TEH(MAXPOWER);		// регулятор не позволит установить мощность больше чем MAXPOWER

void setup()
{
	Serial.begin(SERIALSPEED);
	TEH.init();
	lcd.init();       // Для подключения ЛСД экрана через I2C
	lcd.backlight();      // Для подключения ЛСД экрана через I2C
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
	Serial.println(TEH.Angle / 4);
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
				Var = "";
			}
		}
	}
}
