#ifndef __GPIO_UTIL_H__
#define __GPIO_UTIL_H__

#include "gpio.h"

#define GPIO_PIN_NUM 13

#define GPIO_FLOAT 0
#define GPIO_PULLUP 1
#define GPIO_PULLDOWN 2

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
//#define GPIO_INT 2

uint8_t pin_num[GPIO_PIN_NUM];
uint8_t pin_func[GPIO_PIN_NUM];

uint32_t pin_mux[GPIO_PIN_NUM] = {PAD_XPD_DCDC_CONF,  PERIPHS_IO_MUX_GPIO5_U,  PERIPHS_IO_MUX_GPIO4_U, 	 PERIPHS_IO_MUX_GPIO0_U,
								  PERIPHS_IO_MUX_GPIO2_U, PERIPHS_IO_MUX_MTMS_U, PERIPHS_IO_MUX_MTDI_U, PERIPHS_IO_MUX_MTCK_U,
								  PERIPHS_IO_MUX_MTDO_U, PERIPHS_IO_MUX_U0RXD_U, PERIPHS_IO_MUX_U0TXD_U, PERIPHS_IO_MUX_SD_DATA2_U,
								  PERIPHS_IO_MUX_SD_DATA3_U };
uint8_t pin_num[GPIO_PIN_NUM] = {16, 5, 4, 0,
								  2,  14,  12, 13,
								  15,  3,  1, 9,
								  10};
uint8_t pin_func[GPIO_PIN_NUM] = {0, FUNC_GPIO5, FUNC_GPIO4, FUNC_GPIO0,
								  FUNC_GPIO2,  FUNC_GPIO14,  FUNC_GPIO12,  FUNC_GPIO13,
								  FUNC_GPIO15,  FUNC_GPIO3,  FUNC_GPIO1, FUNC_GPIO9,
								  FUNC_GPIO10};
								

int set_gpio_mode(unsigned pin, unsigned mode, unsigned pull);

#endif