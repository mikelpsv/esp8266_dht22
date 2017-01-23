#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "driver/uart.h"

#define DEBUG_MODE
#define USER_PROC_TASK_QUEUE_LEN 1

os_event_t user_procTaskQueue[USER_PROC_TASK_QUEUE_LEN];
static void main_loop(os_event_t *events);


// Основной цикл
static void ICACHE_FLASH_ATTR main_loop(os_event_t *events){

	os_delay_us(10000);

	system_os_post(USER_TASK_PRIO_0, 0, 0);
}


// Точка входа
void ICACHE_FLASH_ATTR user_init(){
	
	uart_init(115200, 115200);

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

	system_os_task(main_loop, USER_TASK_PRIO_0, user_procTaskQueue, USER_PROC_TASK_QUEUE_LEN);
	system_os_post(USER_TASK_PRIO_0, 0, 0);
}

