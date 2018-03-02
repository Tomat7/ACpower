#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#include "ACpower.h"
/*
   Вход с детектора нуля - D3
   Выход на триак - D5
   Аналоговый вход с датчика тока - A1
*/
uint16_t inst_P = 0;
unsigned long msShow = 0;
String T1, Var;

ACpower TEH; 

void setup()
{
  Serial.begin(SERIALSPEED);
  TEH.init(MAXPOWER);
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
}

void chkSerial() {
  while (Serial.available()) //Serial port, пока не конец сообщения, читаем данные и формируем строку
  {
    char ch = Serial.read();
    Var += ch;
    if (ch == '\n')
    {
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
