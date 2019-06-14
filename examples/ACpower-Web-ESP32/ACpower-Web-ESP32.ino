// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!
/*
   http://forum.homedistiller.ru/index.php?topic=166750.0
   http://forum.homedistiller.ru/index.php?topic=331296.0
   https://tomat.visualstudio.com/ESP32-AC-power
   каждую секунду в COM-порт выдается текущая и установленная мощность (при отсутствии нагрузки может выдавать ерунду :)
   для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx, где xxxx мощность в ваттах
*/
#define WIFI_NAME "Tomat1"
#define WIFI_PASS "filimon7"
#define WIFI_TIMEOUT 50
#define USE_DHCP  // закоментировать если нужно назначать адреса вруную (см. network.ino)

#if defined(ESP32)      // ESP32 (Wemos Lolin32)
#define MAXPOWER 3000
#define PIN_ZEROCROSS 25
#define PIN_TRIAC 26
#define PIN_U 39
#define PIN_I 36
#else
#error "Chip not supported. Use AVR or ESP32."
#endif

#include "ACpower.h"
ACpower TEH(MAXPOWER, PIN_ZEROCROSS, PIN_TRIAC, PIN_U, PIN_I);
// "длинный" вариант возволяет сконфигурировать пины внешней обвязки
/*
  ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
  Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
  pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
  pinTriac - номер пина который управляет триаком (2-7)
  pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
  pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*/

#include <WiFi.h>

#ifndef USE_DHCP
IPAddress local_IP(ETHERNET_IP);
IPAddress gateway(IP_NETWORK + 254);
IPAddress subnet(255, 255, 255, 0);
#endif

#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
WebServer server(80);

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000

uint16_t inst_P = 0, wifiErrors=0;
unsigned long msShow = 0;
String Stext, Svar;

// корректировка RMS - пробуем бороться с нелинейностью измерений.
// в массиве задается сколько вольт прибавить к измеренному напряжению 
// для каждого значения напряжения кратного 10 
// все числа поправить под свои измерения. (экпериментальная фича, надо проверять!)
static float Ucorr[25] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* до 140 вольт коррекцию не делаем */
                          0.5,  /* если намеряли выше 150 вольт, то прибавим 0.5 вольта */
                          0.6,  /* если намеряли выше 160 вольт, то прибавим 0.6 вольта */
                          0.7,  /* если намеряли выше 170 вольт, то прибавим 0.7 вольта */
                          2.8,  /* если намеряли выше 180 вольт, то прибавим 2.8 вольта */
                          8.9,  /* если намеряли выше 190 вольт, то прибавим 8.9 вольта */
                          12.0, /* если намеряли выше 200 вольт, то прибавим 12.0 вольта */
                          14.1, /* если намеряли выше 210 вольт, то прибавим 14.1 вольта */
                          15.2, /* если намеряли выше 220 вольт, то прибавим 15.2 вольта */
                          17.3, /* если намеряли выше 230 вольт, то прибавим 17.3 вольта */
                          18.4  /* если намеряли выше 240 вольт, то прибавим 18.4 вольта */
                          };    /* поправить под свои измерения!! */

void setup()
{
  Serial.begin(SERIALSPEED);
  delay(300);
  Serial.println(F(SKETCHVERSION));
  setup_Network();
  setup_Web();
  /*
    вызов с двумя параметрами - в этом случае задаётся коэффициент ACS712 или трансформатора тока,
    вторым параметром идет множитель для напряжения - полезно если невозможно откалибровать подстроечником
    и при изменении схемы позволяет использовать почти весь диапазон АЦП Ардуино
  */
  TEH.init(0.02, 0.2);        // трансформатора тока, датчик напряжения с выпрямителем
  TEH.adjustRMS(NULL, Ucorr); // передаём данные для коррестирвки ихмеренных значений
  
  delay(3000);
}

void loop()
{
  TEH.control();  // нужно вызывать регулярно для пересчета мощности и угла открытия триака
  server.handleClient();
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

  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  char UpTimeStr[20];
  snprintf(UpTimeStr, 50, "+ Uptime: %02d:%02d:%02d", hr, min % 60, sec % 60);
  Serial.println(UpTimeStr);
}

void chkSerial()
{
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
