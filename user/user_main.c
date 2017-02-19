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
#define DHT_NUMBER_OF_SENSORS 2 // количество датчиков

#define TIMER_READ_SENSOR_MS 5000   // интервал опроса датчиков
#define SENSOR_PIN_INDOOR 12        // пин внутреннего датчика
#define SENSOR_PIN_OUTDOOR 14       // пин внешнего датчика
#define MOTOR_PIN 4                 // пин вентилятора

#define TOPIC "cellar/device_id/%s"

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static volatile os_timer_t some_timer;

MQTT_Client mqttClient;
char topic_str[20];
int _MAX_HUMIDITY;

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

    // подтверждение, что вентилятор включен
    os_sprintf(topic_str, TOPIC, "motor");
    MQTT_Publish(&mqttClient, topic_str, "on", 2, 1, 1);
}


void ICACHE_FLASH_ATTR motorOff(){
    GPIO_OUTPUT_SET(MOTOR_PIN, 0);   

    // подтверждение, что вентилятор выключен
    os_sprintf(topic_str, TOPIC, "motor");
    MQTT_Publish(&mqttClient, topic_str, "off", 3, 1, 1);    
}

static void ICACHE_FLASH_ATTR read_DHT(void *arg){
    char mqtt_data[50];

    dht_read(&dht_sensors[0]);
    dht_read(&dht_sensors[1]);

    if((dht_sensors[0].counter == DHT_COUNTER) || (dht_sensors[1].counter == DHT_COUNTER)){

        int len;

        len = dataToJSON(&dht_sensors[0], mqtt_data);
        os_sprintf(topic_str, TOPIC, "sensor0");
        MQTT_Publish(&mqttClient, topic_str, mqtt_data, len, 0, 1);
        

        len = dataToJSON(&dht_sensors[1], mqtt_data);
        os_sprintf(topic_str, TOPIC, "sensor1");
        MQTT_Publish(&mqttClient, topic_str, mqtt_data, len, 0, 1);
        

        dht_sensors[0].counter = 0;
        dht_sensors[1].counter = 0;

        if(dht_sensors[1].humidity > _MAX_HUMIDITY && dht_sensors[1].humidity_a < dht_sensors[0].humidity_a){
            motorOn();
        }else{
            motorOff();
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
    char clientid[20] = "TODO!";
    MQTT_Subscribe(client, "#", 0);
}


void mqttDisconnectedCb(uint32_t *args) {
  
    MQTT_Client* client = (MQTT_Client*)args;
    uart0_send_str("MQTT: Disconnected\r\n");

}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len) {

    char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf = (char*) os_zalloc(data_len + 1);
    char str[100];
  
    MQTT_Client* client = (MQTT_Client*) args;
    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;
    char *sp = topicBuf; // string pointer accessing internals of topicBuf

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;

    os_sprintf(str, "Received topic: %s, data: %s [%d]\r\n", topicBuf, dataBuf, data_len);
    uart0_send_str(str);
    if(strcmp(topicBuf, "www/qqq/sss/v") == 0){
        if(strcmp(dataBuf, "on") == 0){
            motorOn();
        }
        if(strcmp(dataBuf, "off") == 0){
            motorOff();
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
    // Чтение настроек
    config_load();


    _MAX_HUMIDITY = 90;

    // Внутренний датчик
    dht_sensors[0].pin = SENSOR_PIN_INDOOR;
    dht_sensors[0].type = DHT22;
    dht_sensors[0].enable = 1;
    dht_sensors[0].counter = 0;

    // Внешний датчик
    dht_sensors[1].pin = SENSOR_PIN_OUTDOOR;
    dht_sensors[1].type = DHT22;
    dht_sensors[1].enable = 1;
    dht_sensors[1].counter = 0;

    // Инициализация датчиков
    dht_init(&dht_sensors[0]);
    dht_init(&dht_sensors[1]);


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

