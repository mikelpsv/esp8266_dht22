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
#include "gpio_util.h"

#define DEBUG_MODE
#define USER_PROC_TASK_QUEUE_LEN 1

#define DHT_NUMBER_OF_SENSORS 2 // количество датчиков

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static volatile os_timer_t some_timer;

MQTT_Client mqttClient;

// Массив датчиков
dht_sensor dht_sensors[DHT_NUMBER_OF_SENSORS];

inline double pow(double x,double y){
    double z , p = 1;
    int i;
    //y<0 ? z=-y : z=y ;
    if(y < 0)
        z = fabs(y);
    else
        z = y;
    
    for(i = 0; i < z ; ++i){
        p *= x;
    }
    
    if(y<0)
        return 1/p;
    else
        return p;
}

inline double fabs(double x)
{
    return( x<0 ?  -x :  x );
}


// Функция перевода относительной влажности при заданной температуре в абсолютную
// Атрибут "ICACHE_FLASH_ATTR" помещает функцию во FLASH, а не оставляет в ОЗУ
float ICACHE_FLASH_ATTR calc_abs_h(float t, float h){
    float temp;
    temp = pow(2.718281828, (17.67*t) / (t+243.5));
    return (6.112 * temp * h * 2.1674) / (273.15 + t);
} 

void ICACHE_FLASH_ATTR sendData(char* topic, float value, int qos, int retain){
    char str[6];
    int sz = (value < 0) ? 6 : 5;
    os_sprintf(str, "%d", (int)(value*100));
    MQTT_Publish(&mqttClient, topic, str, sz, 1, 1);
}


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


static void ICACHE_FLASH_ATTR read_DHT(void *arg){

    double h_abs1, h_abs2;

    dht_read(&dht_sensors[0]);
    dht_read(&dht_sensors[1]);

    // Отправляем температуру
    sendData("www/qqq/sss/t1", dht_sensors[0].temperature, 0, 1);
    sendData("www/qqq/sss/t2", dht_sensors[1].temperature, 0, 1);
    
    // Отправляем отностительную влажность
    sendData("www/qqq/sss/h1", dht_sensors[0].humidity, 0, 1);
    sendData("www/qqq/sss/h2", dht_sensors[1].humidity, 0, 1);

    // Рассчитываем и отправляем абсолютную влажность
    h_abs1 = calc_abs_h(dht_sensors[0].temperature, dht_sensors[0].humidity);
    h_abs2 = calc_abs_h(dht_sensors[1].temperature, dht_sensors[1].humidity);

    sendData("www/qqq/sss/h11", h_abs1, 0, 1);
    sendData("www/qqq/sss/h21", h_abs1, 0, 1);
}

void ICACHE_FLASH_ATTR motorOn(){
    GPIO_OUTPUT_SET(4, 1);
    // подтверждение, что вентилятор включен
    MQTT_Publish(&mqttClient, "www/qqq/sss/v", "on", 2, 1, 1);
}


void ICACHE_FLASH_ATTR motorOff(){
    GPIO_OUTPUT_SET(4, 0);   
    // подтверждение, что вентилятор выключен
    MQTT_Publish(&mqttClient, "www/qqq/sss/v", "off", 2, 1, 1);    
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

  os_sprintf(str, "Received topic: %s, data: %s [%d]\n", topicBuf, dataBuf, data_len);
  uart0_send_str(str);
  if(strcmp(topicBuf, "www/qqq/sss/v") == 0){
    uart0_send_str("topic ok!\r\n");
    if(strcmp(dataBuf, "on") == 0){
        motorOn();
    }
    if(strcmp(dataBuf, "off") == 0){
        motorOff();
      GPIO_OUTPUT_SET(4, 0);
    }
  }
  os_free(topicBuf);
  os_free(dataBuf);
}

void init_done_cb() {

    //Initialize GPIO
    gpio_init();
  
    set_gpio_mode(2, GPIO_OUTPUT, GPIO_PULLUP);
    GPIO_OUTPUT_SET(2, 0);
    // Чтение настроек
    config_load();

    // Внутренний датчик
    dht_sensors[0].pin = 5;
    dht_sensors[0].type = DHT22;
    dht_sensors[0].enable = 1;

    // Внешний датчик
    dht_sensors[1].pin = 6;
    dht_sensors[1].type = DHT22;
    dht_sensors[1].enable = 1;

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
    //in ms, 0 for once and 1 for repeating
    os_timer_arm(&some_timer, 4000, 1);
}


// Точка входа
void ICACHE_FLASH_ATTR user_init(){
    uart_init(115200, 115200);
    // Запустим процедуру, как будет готово железо
    system_init_done_cb(init_done_cb);

}

