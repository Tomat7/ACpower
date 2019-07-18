// ТОЛЬКО для ESP32!!

// ** ВНИМАНИЕ ** будьте аккуратны - высокое напряжение опасно для жизни!

// подробности http://forum.homedistiller.ru/index.php?topic=166750.0
// каждую секунду в COM-порт выдается текущая и установленная мощность
// (при отсутствии нагрузки может выдавать ерунду :)
// для установки необходимой мощности нужно в COM-порт "дать команду" SPxxxx,
// где xxxx мощность в ваттах

#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define MAXPOWER 3000
#define SERIALSPEED 115200
#define SHOWINTERVAL 1000
#define PRINTF(a, ...) Serial.print(a); Serial.println(__VA_ARGS__)

// Коэффициенты ниже справедливы только для Arduino c 10-битным АЦП и 5-вольтовым питанием!
// Коэффициенты датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |

#include "ACpower.h"

#if defined(ESP32)
#define PIN_Z0 25  // детектор нуля
#define PIN_T0 26  // триак 
#define PIN_U0 39  // датчик напряжения
#define PIN_I0 36  // датчик тока
#else
#error "Chip not supported. Use ESP32."
#endif

/*
  ACpower(uint16_t Pm, byte pinZeroCross, byte pinTriac, byte pinVoltage, byte pinACS712)
  Pm - максимальная мощность. регулятор не позволит установить мощность больше чем MAXPOWER
  pinZeroCross - номер пина к которому подключен детектор нуля (2 или 3)
  pinTriac - номер пина который управляет триаком (2-7)
  pinVoltage - "имя" вывода к которому подключен "датчик напряжения" - трансформатор с обвязкой (A0-A7)
  pinACS712 - "имя" вывода к которому подключен "датчик тока" - ACS712 (A0-A7)
*/

ACpower TEH0(PIN_Z0, PIN_T0);

uint16_t inst_P = 0;
unsigned long msShow = 0;
String Stext, Svar;
uint16_t Angle = 0;

void setup()
{
  Serial.begin(SERIALSPEED);
  delay(300);
  Serial.println(F(SKETCHVERSION));
  /*
    вызов с двумя параметрами - в этом случае задаётся коэффициент ACS712 или трансформатора тока,
    вторым параметром идет множитель для напряжения - полезно если невозможно откалибровать подстроечником
    и при изменении схемы позволяет использовать почти весь диапазон АЦП Ардуино
  */

  TEH0.init(&Angle, 0);        // трансформатора тока, датчик напряжения с выпрямителем
}

void loop()
{
  //TEH.control();  // нужно вызывать регулярно для пересчета мощности и угла открытия триака
  if ((millis() - msShow) > SHOWINTERVAL)
  {
    chkSerial();
    showInfo();
    Angle = int(millis() / 10);
    msShow = millis();
  }
}

void showInfo()
{
  /*
    Serial.print("Pnow=");
    Serial.println(TEH.Pnow);
    Serial.print("Pset=");
    Serial.println(TEH.Pset);
    Serial.print("Unow=");
    Serial.println(TEH.Unow);
    Serial.print("Inow=");
    Serial.println(TEH.Inow);
  */
  Serial.print("Angle: ");
  Serial.println(Angle);
  //PRINTF("&Angle=", (uint32_t)&Angle, HEX);
  Serial.print("TEH0.Angle: ");
  Serial.println(TEH0.Angle);
  Serial.print("TEH0.CounterZC: ");
  Serial.println(TEH0.CounterZC);
  TEH0.CounterZC = 0;
  Serial.print("+++");
  Serial.println(millis());

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
        //TEH.setpower(inst_P);
      }
      Svar = "";
    }
  }
}
