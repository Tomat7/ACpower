# ACpower
Arduino Hi-Power regulator for AC devices

__Warning! Hi voltage is dangerous! Be careful!__
=================================================

See schemas and other details/examples on http://forum.homedistiller.ru/index.php?topic=166750.0

Connections:
D3 - ZeroCross detector
D5 - Triac
A0 - voltage meter
A1 - ACS712

Original project (c) Sebra.
Regulating algorithm (c) Chatterbox.

Many thanks to both of them!


__ Внимание! Высокое напряжение опасно для жизни! Будьте аккуратны и внимательны! __
====================================================================================

* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
* Version 0.7
* 
* A0 - подключение "измерителя" напряжения (трансформатор, диодный мост, делитель напряжения)
* A1 - подключение датчика тока ACS712
* D5 - управление триаком
* D3 - детектор нуля

