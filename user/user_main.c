#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/dht.h"

#define DEBUG_MODE
#define USER_PROC_TASK_QUEUE_LEN 1

#define DHT_NUMBER_OF_SENSORS 2 // количество датчиков

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static void main_loop(os_event_t *events);

// Массив датчиков
dht_sensor dht_sensors[DHT_NUMBER_OF_SENSORS];

// Функция перевода относительной влажности при заданной температуре в абсолютную
float ICACHE_FLASH_ATTR calc_abs_h(float t, float h){
	float temp;
 	temp = pow(2.718281828, (17.67*t) / (t+243.5));
 	return (6.112 * temp * h * 2.1674) / (273.15 + t);
}

// Основной цикл
// Атрибут "ICACHE_FLASH_ATTR" помещает функцию во FLASH, а не оставляет в ОЗУ
static void ICACHE_FLASH_ATTR main_loop(os_event_t *events){

	//t1, h1 - температура и влажность датчиков внутри
	//t2, h2 - температура и влажность датчиков снаружи

	//tn1_min, hn1_min, tn1_max, hn1_max - минимальные и максимальные пороги нормы температуры и влажности внутри

	// vent 1 включен вентилятор, 0 выключен

	p1 = t1 >= tn1_min; // 1 попадаем в норму, 0 не попадаем (по нижней границе)
	p2 = t1 <= tn1_max; // 1 попадаем в норму, 0 не попадаем (по верхней границе)
	
	p3 = h1 >= hn1_min; // 1 попадаем в норму, 0 не попадаем (по нижней границе)
	p4 = h1 <= hn1_max; // 1 попадаем в норму, 0 не попадаем (по верхней границе)

	p5 = t1 >= t2;      // 1 температура внутри выше улицы, 0 температура внутри ниже улицы
	p6 = h1 >= h2;      // 1 влажность внутри выше улицы, 0 влажность внутри ниже улицы

	if(p1 && p2 && p3 && p4){
		// отключаем
		vent = 0;
	}else{
		// по температуре

		if(!p1 && !p5){
			// если температура внутри ниже нормы и температура внутри ниже улицы, то можно включить, чтобы поднять температуру в подвале
			vent_min_t = 1; // признак включения по температуре по нижней границе
		}else{
			vent_min_t = 0;
		}

		if(!p2 && !p5){
			// если температура внутри выше нормы и температура внутри выше улицы, то можно включить, чтобы опустить температуру в подвале
			vent_max_t = 1; // признак включения по температуре по верхней границе
		}else{
			vent_max_t = 0;			
		}		

		// по влажности
		if(!p3 && !p6){
			// если влажность внутри ниже нормы и влажность внутри ниже улицы, то можно включить, чтобы поднять влажность в подвале
			vent_min_h = 1; // признак включения по влажности по нижней границе
		}else{
			vent_max_h = 0;
		}

		if(!p4 && !p6){
			// если влажность внутри выше нормы и влажность внутри выше улицы, то можно включить, чтобы опустить влажность в подвале
			vent_max_h = 1; // признак включения по влажности по верхней границе
		}else{
			vent_max_h = 0;			
		}			

		// Однозначная необходимость включения
		if(vent_min_t && vent_min_h){
			vent = 1;
		}

		// Однозначная необходимость включения
		if(vent_max_t && vent_max_h){
			vent = 1;
		}




	}





	os_delay_us(10000);

	system_os_post(USER_TASK_PRIO_0, 0, 0);
}

void init_done_cb() {
	#ifdef DEBUG_MODE
		uart0_send_str("Hello world!\r\n");
	#endif

	//Initialize GPIO
	gpio_init();

    //Set station mode - user_interface.h
    // NULL_MODE       0x00
	// STATION_MODE    0x01
	// SOFTAP_MODE     0x02
	// STATIONAP_MODE  0x03

    wifi_set_opmode( STATION_MODE );


	// user task's prio must be 0/1/2 - user_interface.h
    // USER_TASK_PRIO_0 = 0,
    // USER_TASK_PRIO_1,
    // USER_TASK_PRIO_2,
    // USER_TASK_PRIO_MAX


    dht_sensors[0].pin = 3;
	dht_sensors[0].type = DHT22;
	dht_sensors[0].enable = 1;

    dht_sensors[1].pin = 4;
	dht_sensors[1].type = DHT22;
	dht_sensors[1].enable = 1;

	if (!dht_init(&dht_sensors[0])){
		dht_sensors[0].enable = 0;
	}

	if (dht_init(&dht_sensors[1])){
		dht_sensors[1].enable = 0;	
	}

	system_os_task(main_loop, USER_TASK_PRIO_0, user_procTaskQueue, USER_PROC_TASK_QUEUE_LEN);
	system_os_post(USER_TASK_PRIO_0, 0, 0);
}

// Точка входа
void ICACHE_FLASH_ATTR user_init(){
	
	uart_init(115200, 115200);
	
	// Запустим процедуру, как будет готово железо
	system_init_done_cb(init_done_cb);

}

