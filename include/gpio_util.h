#ifndef __GPIO_UTIL_H__
#define __GPIO_UTIL_H__


#define GPIO_PIN_NUM 13

#define GPIO_FLOAT 		0
#define GPIO_PULLUP 	1
#define GPIO_PULLDOWN	2

#define GPIO_INPUT	0
#define GPIO_OUTPUT 1
#define GPIO_INT	2


uint32_t 	pin_mux[GPIO_PIN_NUM];
uint8_t 	pin_num[GPIO_PIN_NUM];
uint8_t 	pin_func[GPIO_PIN_NUM];

int set_gpio_mode(unsigned pin, unsigned mode, unsigned pull);

#endif