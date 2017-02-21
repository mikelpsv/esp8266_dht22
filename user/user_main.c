#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "mqtt.h"
#include "wifi.h"
#include "os_type.h"
#include "math.h"
#include "mem.h"
#include "user_interface.h"
#include "uart.h"
#include "config.h"
#include "dht.h"
#include "ugpio.h"

#define DEBUG_MODE
#define USER_PROC_TASK_QUEUE_LEN 1
#define DHT_NUMBER_OF_SENSORS 2       // количество датчиков

#define TIMER_READ_SENSOR_MS 5000     // интервал опроса датчиков
#define DHT_COUNTER          5        // итераций чтения данных для усреднения и отправки на сервер

// SENSOR_PIN_IN, SENSOR_PIN_OUT - пины по физическому размещению датчиков
#define SENSOR_PIN_IN  14             // пин внутреннего датчика
#define SENSOR_PIN_OUT 12             // пин внешнего датчика
#define MOTOR_PIN 4                   // пин вентилятора

// SENSOR_INDEX_INDOOR, SENSOR_INDEX_OUTDOOR - индексы массива датчиков по функциональному использованию
#define SENSOR_INDEX_INDOOR  0        // индекс контрольного датчика в погребе
#define SENSOR_INDEX_OUTDOOR 1        // индекс внешнего датчика

#define DATA_TOPIC "cellar/device_id/%d"
#define SET_MAX_HUM_TOPIC "cellar/device_id/max_hum"
#define SET_DELTA_HUM_TOPIC "cellar/device_id/delta_hum"
#define SET_MIN_TEMP_TOPIC "cellar/device_id/min_temp"
#define SET_MOTOR_ON_TOPIC "cellar/device_id/motor_on"

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static volatile os_timer_t some_timer;

MQTT_Client mqttClient;
char topic_str[20];


int _MAX_HUMIDITY;      // влажность, при которой включится вентилятор
int _DELTA_HAMIDITY;    // гистерезис для отключения вентилятора: _MAX_HUMIDITY - _DELTA_HAMIDITY
int _MIN_TEMP;          // минимальная температура, ниже которой вентилятор не включится при любой влажности
int _MOTOR_ON;          // принудительное включение вентилятора


int motorIsOn;          // текущее состояние вентилятора
uint32_t motorHours;    // счетчик мото-часов вентилятора

// Массив датчиков
dht_sensor dht_sensors[DHT_NUMBER_OF_SENSORS];



void ICACHE_FLASH_ATTR sprint_float(float val, char *buff) {
    char smallBuff[16];
    int val1 = (int) val;
    unsigned int val2;

    if (val < 0) {
        val2 = (int) (-100.0 * val) % 100;
    }else{
        val2 = (int) (100.0 * val) % 100;
    }
   
   if (val2 < 10) {
      os_sprintf(smallBuff, "%i.0%u", val1, val2);
   } else {
      os_sprintf(smallBuff, "%i.%u", val1, val2);
   }

   strcat(buff, smallBuff);
}

void ICACHE_FLASH_ATTR motorOn(){
    GPIO_OUTPUT_SET(MOTOR_PIN, 1);
    motorIsOn = 1;

    // подтверждение, что вентилятор включен
    os_sprintf(topic_str, DATA_TOPIC, MOTOR_PIN);
    MQTT_Publish(&mqttClient, topic_str, "on", 2, 1, 1);
}


void ICACHE_FLASH_ATTR motorOff(){
    GPIO_OUTPUT_SET(MOTOR_PIN, 0);   
    motorIsOn = 0;

    // подтверждение, что вентилятор выключен
    os_sprintf(topic_str, DATA_TOPIC, MOTOR_PIN);
    MQTT_Publish(&mqttClient, topic_str, "off", 3, 1, 1);    
}

static void ICACHE_FLASH_ATTR read_DHT(void *arg){
    char mqtt_data[50];

    if(motorIsOn == 1){
        motorHours++;
    }

    dht_read(&dht_sensors[SENSOR_INDEX_INDOOR]);
    dht_read(&dht_sensors[SENSOR_INDEX_OUTDOOR]);

    // Если хоть один сенсор досчитал, значит пора отправлять данные
    if((dht_sensors[SENSOR_INDEX_INDOOR].counter == DHT_COUNTER) || (dht_sensors[SENSOR_INDEX_OUTDOOR].counter == DHT_COUNTER)){

        int len;

        len = dataToJSON(&dht_sensors[SENSOR_INDEX_INDOOR], mqtt_data);
        os_sprintf(topic_str, DATA_TOPIC, &dht_sensors[SENSOR_INDEX_INDOOR].pin);
        MQTT_Publish(&mqttClient, topic_str, mqtt_data, len, 0, 1);
        

        len = dataToJSON(&dht_sensors[SENSOR_INDEX_OUTDOOR], mqtt_data);
        os_sprintf(topic_str, DATA_TOPIC, &dht_sensors[SENSOR_INDEX_INDOOR].pin);
        MQTT_Publish(&mqttClient, topic_str, mqtt_data, len, 0, 1);
        

        dht_sensors[SENSOR_INDEX_INDOOR].counter = 0;
        dht_sensors[SENSOR_INDEX_OUTDOOR].counter = 0;

        if(_MOTOR_ON == 0) {
            // Включаем, при условии: вентилятор выключен, 
            // влажность внутри выше нормы и внешняя абсолютная влажность меньше чем внутри

            if( (motorOn == 0) && (dht_sensors[SENSOR_INDEX_INDOOR].humidity > _MAX_HUMIDITY)
                    && (dht_sensors[SENSOR_INDEX_OUTDOOR].humidity_a <= dht_sensors[SENSOR_INDEX_INDOOR].humidity_a)
                    && (dht_sensors[SENSOR_INDEX_OUTDOOR].temperature > _MIN_TEMP) ) {
                motorOn();
            }

            // Выключаем, при условии: вентилятор включен, 
            // влажность опустилась ниже (нормы минус гистерезис %) 
            // или температура внутри ниже критической
            if( (motorIsOn == 1)
                && ( (dht_sensors[SENSOR_INDEX_INDOOR].humidity <= (_MAX_HUMIDITY - _DELTA_HAMIDITY))
                    || (dht_sensors[SENSOR_INDEX_OUTDOOR].temperature <= _MIN_TEMP) ) ){

                motorOff();
            }
        }else{
            // уже должен быть включен - на всякий случай
            if(motorOn == 0){
                motorOn();
            }
        }
    }
}


void ICACHE_FLASH_ATTR wifiConnectCb(uint8_t status){
    if(status == STATION_GOT_IP){
        MQTT_Connect(&mqttClient);
    }else{
        MQTT_Disconnect(&mqttClient);
    }
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args) {
  
    MQTT_Client* client = (MQTT_Client*) args;
    MQTT_Subscribe(client, "cellar/device_id/+", 0);
}


void mqttDisconnectedCb(uint32_t *args) {
  
    MQTT_Client* client = (MQTT_Client*)args;
    uart0_send_str("MQTT: Disconnected\r\n");

}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len) {

    char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf = (char*) os_zalloc(data_len + 1);
    char str[100];
    int tempValue = 0;
  
    MQTT_Client* client = (MQTT_Client*) args;
    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    char *sp = topicBuf; // string pointer accessing internals of topicBuf

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;

    os_sprintf(str, "Received topic: %s, data: %s [%d]\r\n", topicBuf, dataBuf, data_len);
    uart0_send_str(str);

    // Команда на установку порога влажности
    if(strcmp(topicBuf, SET_MAX_HUM_TOPIC) == 0){
        tempValue = atoi(dataBuf);
        if(tempValue > 0 && tempValue < 100){
            _MAX_HUMIDITY = tempValue;
            os_sprintf(str, "Set value %s = %d\r\n", "_MAX_HUMIDITY", _MAX_HUMIDITY);
            uart0_send_str(str);
        }
    }

    // Команда на установку гистерезиса
    if(strcmp(topicBuf, SET_DELTA_HUM_TOPIC) == 0){
        tempValue = atoi(dataBuf);
        if(tempValue > 0 && tempValue < 100){
            _DELTA_HAMIDITY = tempValue;
            os_sprintf(str, "Set value %s = %d\r\n", "_DELTA_HAMIDITY", _DELTA_HAMIDITY);
            uart0_send_str(str);
        }
    }

    // Команда на установку минимальной температуры
    if(strcmp(topicBuf, SET_MIN_TEMP_TOPIC) == 0){
        _MIN_TEMP = atoi(dataBuf);
        os_sprintf(str, "Set value %s = %d\r\n", "_MIN_TEMP", _MIN_TEMP);
        uart0_send_str(str);
    }    

    // Команда на включение/отключение принудительной работы вентилятора
    if(strcmp(topicBuf, SET_MOTOR_ON_TOPIC) == 0){
        if(strcmp(dataBuf, "on") == 0){
            _MOTOR_ON = 1;
            motorOn();
            os_sprintf(str, "Set value %s = %d\r\n", "_MOTOR_ON", _MOTOR_ON);
            uart0_send_str(str);
        }

        if(strcmp(dataBuf, "off") == 0){
            _MOTOR_ON = 0;
            motorOff();
            os_sprintf(str, "Set value %s = %d\r\n", "_MOTOR_ON", _MOTOR_ON);            
            uart0_send_str(str);
        }
    }


    os_free(topicBuf);
    os_free(dataBuf);
}

void init_done_cb() {

    //Initialize GPIO
    gpio_init();
  
    set_gpio_mode(MOTOR_PIN, GPIO_OUTPUT, GPIO_PULLUP);
    GPIO_OUTPUT_SET(MOTOR_PIN, 0);
    motorIsOn = 0;

    // Чтение настроек
    config_load();


    _MAX_HUMIDITY   = 90; // включение вентилятора при 90%
    _DELTA_HAMIDITY = 2;  // отключение вентилятора при 88% (90-2)
    _MIN_TEMP       = 1;  // минимальная температура, ниже которой вентилятор не включится при любой влажности
    _MOTOR_ON       = 0;  // принудительное (безусловное) включение вентилятора 

    // Планировалось, что устройство будет внутри, но из-за плохой связи роли датчиков изменились

    // SENSOR_PIN_IN, SENSOR_PIN_OUT - пины по физическому размещению датчиков
    // SENSOR_INDEX_INDOOR, SENSOR_INDEX_OUTDOOR - индексы массива датчиков по функциональному использованию

    // Внутренний датчик: SENSOR_PIN_IN,
    // использование датчика для определения внешней влажности
    dht_sensors[SENSOR_INDEX_OUTDOOR].pin = SENSOR_PIN_IN;
    dht_sensors[SENSOR_INDEX_OUTDOOR].type = DHT22;
    dht_sensors[SENSOR_INDEX_OUTDOOR].enable = 1;
    dht_sensors[SENSOR_INDEX_OUTDOOR].counter = 0;

    // Внешний датчик SENSOR_PIN_OUT,
    // использование датчика: для определения влажности в погребе
    dht_sensors[SENSOR_INDEX_INDOOR].pin = SENSOR_PIN_OUT;
    dht_sensors[SENSOR_INDEX_INDOOR].type = DHT22;
    dht_sensors[SENSOR_INDEX_INDOOR].enable = 1;
    dht_sensors[SENSOR_INDEX_INDOOR].counter = 0;

    // Инициализация датчиков
    dht_init(&dht_sensors[SENSOR_INDEX_INDOOR]);
    dht_init(&dht_sensors[SENSOR_INDEX_OUTDOOR]);


    MQTT_InitConnection(&mqttClient, config.mqtt_host, config.mqtt_port, config.security);

    MQTT_InitClient(&mqttClient, config.device_id, config.mqtt_user, config.mqtt_pass, config.mqtt_keepalive, 1);

    MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
    MQTT_OnConnected(&mqttClient, mqttConnectedCb);
    MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
    MQTT_OnData(&mqttClient, mqttDataCb);


    // Attempt to connect to AP
    WIFI_Connect(config.sta_ssid, config.sta_pwd, wifiConnectCb);

    os_timer_disarm(&some_timer);
    os_timer_setfn(&some_timer, (os_timer_func_t *)read_DHT, NULL);
    
    // запустим таймер опроса датчиков
    os_timer_arm(&some_timer, TIMER_READ_SENSOR_MS, 1);
}


// Точка входа
void ICACHE_FLASH_ATTR user_init(){
    uart_init(115200, 115200);
    // Запустим процедуру, как будет готово железо
    system_init_done_cb(init_done_cb);

}

