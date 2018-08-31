# ESP8266_DHT22

Прошивка для ESP8266 (07) для управления принудительной вентиляцией в погребе.

Принцип работы: производится несколько замеров абсолютной влажности внутри вентилируемого помещения и снаружи, которые потом усредняются и принимается решение включить вентилятор или выключить если он включен. 
Использование внешнего датчика обусловлено тем, что если влажность снаружи выше, то включать вентиляцию не надо.

Датчики подключены к 12 и 14 пинам, назначение датчиков (внешний или внутренний задается константами).
Реле управления вентилятором подключено на 4 пин.

В устройстве не заложена индикация работы, вся информация передается на MQTT сервер, с него же устройство и получает настройки.

Передаваемые значения на MQTT:
1. Значения температуры, относительной и абсолютной влажности, количество считанных значений для каждого датчика в формате JSON
2. Информация о включенном (выключенном) вентиляторе 

Получаемые настройки с MQTT:
1. Команда принудительного включения вентилятора
2. Пороговое значение влажности, % (отностительная), при которой будет включен вентилятор
3. Гистерезис влажности, % (относительная) - дельта, при снижении на заданное значение вентилятор будет выключен
4. Минимальная температура внутри, ниже которой вентиляция не будет включена

Значения, установленные по умолчанию:
1. Принудительное включение отключено
2. Вентиляция выключена
3. Пороговое значение влажности 90%
4. Гистерезис 1%
5. Минимальная температура внутри 1 градус цельсия.
6. Количество замеров для принятия решения и отправки данных 5.
7. Интервал опроса датчиков 5 сек.

##Building

[esp8266-wiki](https://github.com/esp8266/esp8266-wiki/wiki/Toolchain)

Add to .profile
PATH="/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin/:$PATH"

-------------------------------------------------------

Firmware for ESP8266 (07) for controlling forced ventilation in the cellar.

Principle of operation: several absolute humidity measurements are made inside the ventilated room and outside, which are then averaged and a decision is made to turn on the fan or turn it off if it is on. The use of an external sensor is due to the fact that if the humidity outside is higher, then you do not need to turn on the ventilation.

The sensors are connected to 12 and 14 pins, the purpose of the sensors (external or internal is given by constants). The fan control relay is connected to 4 pin.

The device does not have a work indication, all information is transferred to the MQTT server, and the device receives settings from it.

Transmitted values on MQTT:

1. The values of temperature, relative and absolute humidity, the number of readings for each sensor in the JSON format
2. Information about the fan on (off)

Received settings with MQTT:

1. Command forced fan on
2. Threshold humidity value,% (inaccurate), at which the fan will be switched on
3. Humidity hysteresis,% (relative) - delta, with a decrease by a set value, the fan will be off
4. Minimum temperature inside, below which ventilation will not be switched on

The default values are:

1. Force Enable Disabled
2. Ventilation off
3. Humidity threshold value 90%
4. Hysteresis 1%
5. The minimum temperature is 1 degree Celsius.
6. Number of measurements for decision making and data sending 5.
7. Polling interval of sensors 5 sec.

## Building

esp8266-wiki

Add to .profile PATH = "/ opt / Espressif / crosstool-NG / builds / xtensa-lx106-elf / bin /: $ PATH"

Make!





