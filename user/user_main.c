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






static void ICACHE_FLASH_ATTR read_DHT(void *arg){
	dht_read(&dht_sensors[0]);
	dht_read(&dht_sensors[1]);
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
  
  //INFO("MQTT: Connected! will use %s as MQTT topic \n", clientid);
  char *buf = "                            ";
  int i = 0;

  for (i=0; i<6; i++){
    os_sprintf(buf, "/lcd%1d", i);
    MQTT_Subscribe(client, buf, 0);
    os_sprintf(buf, "%s/lcd%1d", clientid, i);
    MQTT_Subscribe(client, buf, 0);
  }

  MQTT_Subscribe(client, "/lcd/clearscreen", 0);
  os_sprintf(buf, "%s/clearscreen", clientid);
  MQTT_Subscribe(client, buf, 0);
  MQTT_Subscribe(client, "/lcd/contrast", 0);
  os_sprintf(buf, "%s/contrast", clientid);
  MQTT_Subscribe(client, buf, 0);
}


void mqttDisconnectedCb(uint32_t *args) {
  
  MQTT_Client* client = (MQTT_Client*)args;
  uart0_send_str("MQTT: Disconnected\r\n");

}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len) {

  char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf = (char*) os_zalloc(data_len + 1);

  MQTT_Client* client = (MQTT_Client*) args;
  os_memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;
  char *sp = topicBuf; // string pointer accessing internals of topicBuf

  os_memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  //INFO("Received topic: %s, data: %s [%d]\n", topicBuf, dataBuf, data_len);

  os_free(topicBuf);
  os_free(dataBuf);
}
void init_done_cb() {

	//Initialize GPIO
	gpio_init();

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

