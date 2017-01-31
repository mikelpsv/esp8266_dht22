#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "math.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/dht.h"

#define DEBUG_MODE
#define USER_PROC_TASK_QUEUE_LEN 1

#define DHT_NUMBER_OF_SENSORS 1 // количество датчиков

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static volatile os_timer_t some_timer;


// Массив датчиков
dht_sensor dht_sensors[DHT_NUMBER_OF_SENSORS];
dht_data read_data;

// Атрибут "ICACHE_FLASH_ATTR" помещает функцию во FLASH, а не оставляет в ОЗУ


float __pow(float base, float exponent){
	int j = 1;
	int power = 1;
	for(j = 0;j < exponent; j++){
		power = power * base;
	}
	return power;
}

// Функция перевода относительной влажности при заданной температуре в абсолютную

float ICACHE_FLASH_ATTR calc_abs_h(float t, float h){
	float temp;
	temp = __pow(2.718281828, (17.67*t) / (t+243.5));
	return (6.112 * temp * h * 2.1674) / (273.15 + t);
} 






static void ICACHE_FLASH_ATTR read_DHT(void *arg){
	dht_read(&dht_sensors[0], &read_data);
}

void init_done_cb() {

	//Initialize GPIO
	gpio_init();


    //Set station mode - user_interface.h
    // NULL_MODE       0x00
	// STATION_MODE    0x01
	// SOFTAP_MODE     0x02
	// STATIONAP_MODE  0x03

    wifi_set_opmode( STATION_MODE );

    dht_sensors[0].pin = 5;
	dht_sensors[0].type = DHT22;
	dht_sensors[0].enable = 1;


//  dht_sensors[1].pin = 4;
//	dht_sensors[1].type = DHT22;
//	dht_sensors[1].enable = 1;

	dht_init(&dht_sensors[0]);
	//dht_init(&dht_sensors[1]);


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

