**Warning! Сurrent version working with ESP32 Arduino core v1.0.4!**
Plan to update soon.

**Внимание! Текущая версия работает с ESP32 Arduino core не выше версии 1.0.4!**
Обновления будут скоро.

# ACpower
Arduino Hi-Power regulator for AC devices

* **19.11.2018** Updated with new algorithm. See https://github.com/Tomat7/ACpower/tree/base for old version.
* **8.5.2019** Cleanup code. Possible, it is final for AVR. See https://tomat.visualstudio.com/ESP32-AC-power
* **21.6.2019** ESP32 test code on experimental branch https://github.com/Tomat7/ACpower/tree/experimental
* **23.6.2019** ESP32 code merged to Master branch. Experimental still for development.
* **22.7.2019** 3-phase development separated to https://github.com/Tomat7/ACpower3

__Warning! Hi voltage is dangerous! Be careful!__
=================================================

See schemas and other details/examples on:   
http://forum.homedistiller.ru/index.php?topic=166750.0  
http://forum.homedistiller.ru/index.php?topic=331296.0  
https://github.com/Tomat7/ESP32-AC-power

### Connections for Arduino Nano (see .jpg for ESP32):
* **any of A0-A7** - Voltage meter (originally A0)
* **any of A0-A7** - ACS712 or current transformer (originally A1)
* **D2 or D3** - ZeroCross detector (originally D3)
* **any of D0-D7** - Triac (originally D5)

**Original project of voltage regulator (c) Sebra.**  
**Modified regulating algorithm (c) Chatterbox.**  
**Many thanks to both of them!**  
#### Converting to library, combining voltage&current meters, updated algorithm (c) Tomat7.
=============================================================
   
* **19.11.2018** Обновлен алгоритм. Старая версия https://github.com/Tomat7/ACpower/tree/base.
* **8.5.2019** Видимо это финальная версия для AVR, ухожу на ESP32 https://tomat.visualstudio.com/ESP32-AC-power
* **21.6.2019** Тестовый код для ESP32 в experimental https://github.com/Tomat7/ACpower/tree/experimental
* **23.6.2019** Код для ESP32 добавлен в ветку Master. Experimental остаётся для разработки.
* **22.7.2019** Разработка 3-х фазного регулятора отделена. https://github.com/Tomat7/ACpower3

__Внимание! Высокое напряжение опасно для жизни!__
__Будьте аккуратны и внимательны!__
====================================================================================

Схемы, примеры и подробности:  
[Регулятор напряжения и тока на Arduino Pro Mini](http://forum.homedistiller.ru/index.php?topic=166750.0)  
[Регулятор мощности на ESP32 (Arduino)](http://forum.homedistiller.ru/index.php?topic=331296.0)  
[ACpower regulator on ESP32](https://github.com/Tomat7/ESP32-AC-power)

### Подключения Arduino Nano (для ESP32 по соответствующему .jpg):
* **любой из A0-A7** - "измеритель" напряжения - трансформатор, диодный мост, делитель напряжения
* **любой из A0-A7** - датчика тока (ACS712, трансформатор тока)
* **D2 или D3** - детектор нуля
* **любой из D0-D7** - управление триаком

**Оригинальная идея регулятора напряжения(c) Sebra.**  
**Модифицированный алгоритм регулирования (c) Chatterbox.**  
**Огромное им СПАСИБО!**  
#### Перевод в библиотеку, совмещение измерителя тока и напряжения, обновленный алгоритм (c) Tomat7.
