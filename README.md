# ACpower
Arduino Hi-Power regulator for AC devices

__Warning! Hi voltage is dangerous! Be careful!__
=================================================

See schemas and other details/examples on http://forum.homedistiller.ru/index.php?topic=166750.0
### Connections:
* **A0** - Voltage meter 
* **A1** - ACS712
* **D3** - ZeroCross detector 
* **D5** - Triac

#### Original project (c) Sebra. Regulating algorithm (c) Chatterbox. Many thanks to both of them!
#### Converting to library and combining voltage&current - Tomat7

__Внимание! Высокое напряжение опасно для жизни!__
__Будьте аккуратны и внимательны!__
====================================================================================

Схемы, примеры и подробности - http://forum.homedistiller.ru/index.php?topic=166750.0

### Подключения:
* **A0** - подключение "измерителя" напряжения (трансформатор, диодный мост, делитель напряжения)
* **A1** - подключение датчика тока ACS712
* **D3** - детектор нуля
* **D5** - управление триаком

#### Оригинальная идея (c) Sebra, Алгоритм регулирования (c) Chatterbox
#### Вольный перевод в библиотеку и совмещение регулятора тока и напряжения - Tomat7
