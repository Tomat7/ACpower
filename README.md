# ACpower
Arduino Hi-Power regulator for AC devices

* **19.11.2018** Updated with new algorithm. See https://github.com/Tomat7/ACpower/tree/base for old version.

__Warning! Hi voltage is dangerous! Be careful!__
=================================================

See schemas and other details/examples on http://forum.homedistiller.ru/index.php?topic=166750.0
### Connections:
* **any of A0-A7** - Voltage meter (originally A0)
* **any of A0-A7** - ACS712 (originally A1)
* **D2 or D3** - ZeroCross detector (originally D3)
* **any of D0-D7** - Triac (originally D5)


#### Original project (c) Sebra. 
#### Regulating algorithm (c) Chatterbox. 
#### Many thanks to both of them!
#### Converting to library, combining voltage&current meters, updated algorithm (c) Tomat7.
  
  
  
* **19.11.2018** Обновлен алгоритм. Старый версия https://github.com/Tomat7/ACpower/tree/base.

__Внимание! Высокое напряжение опасно для жизни!__
__Будьте аккуратны и внимательны!__
====================================================================================

Схемы, примеры и подробности - http://forum.homedistiller.ru/index.php?topic=166750.0

### Подключения:
* **любой из A0-A7** - подключение "измерителя" напряжения - трансформатор, диодный мост, делитель напряжения
* **любой из A0-A7** - подключение датчика тока ACS712
* **D2 или D3** - детектор нуля
* **любой из D0-D7** - управление триаком

#### Оригинальная идея (c) Sebra.
#### Алгоритм регулирования (c) Chatterbox.
#### Перевод в библиотеку, совмещение регулятора тока и напряжения, обновленный алгоритм (c) Tomat7.
