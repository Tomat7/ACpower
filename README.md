# ACpower
Arduino Hi-Power regulator for AC devices

* **19.11.2018** Updated with new algorithm. See https://github.com/Tomat7/ACpower/tree/base for old version.

__Warning! Hi voltage is dangerous! Be careful!__
=================================================

See schemas and other details/examples on http://forum.homedistiller.ru/index.php?topic=166750.0
### Connections:
* **any A0-A7** - Voltage meter (originally A0)
* **any A0-A7** - ACS712 (originally A1)
* **D2 or D3** - ZeroCross detector (originally D3)
* **any D0-D7** - Triac (originally D5)


#### Original project (c) Sebra. Regulating algorithm (c) Chatterbox. Many thanks to both of them!
#### Converting to library and combining voltage&current - Tomat7.

* **19.11.2018** Обновлен алгоритм. Старый версия https://github.com/Tomat7/ACpower/tree/base.

__Внимание! Высокое напряжение опасно для жизни!__
__Будьте аккуратны и внимательны!__
====================================================================================

Схемы, примеры и подробности - http://forum.homedistiller.ru/index.php?topic=166750.0

### Подключения:
* **A0** - подключение "измерителя" напряжения - трансформатор, диодный мост, делитель напряжения
* **A1** - подключение датчика тока ACS712
* **D3** - детектор нуля
* **D5** - управление триаком

#### Оригинальная идея (c) Sebra, Алгоритм регулирования (c) Chatterbox
#### Вольный перевод в библиотеку и совмещение регулятора тока и напряжения - Tomat7.
