#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "gpio_util.h"




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



int ICACHE_FLASH_ATTR set_gpio_mode(unsigned pin, unsigned mode, unsigned pull)
{
	if (pin >= GPIO_PIN_NUM)
		return -1;

	switch(pull) {
		case GPIO_PULLUP:
			//PIN_PULLDWN_DIS(pin_mux[pin]);
			PIN_PULLUP_EN(pin_mux[pin]);
			break;
		case GPIO_PULLDOWN:
			PIN_PULLUP_DIS(pin_mux[pin]);
			//PIN_PULLDWN_EN(pin_mux[pin]);
			break;
		case GPIO_FLOAT:
			PIN_PULLUP_DIS(pin_mux[pin]);
			//PIN_PULLDWN_DIS(pin_mux[pin]);
			break;
		default:
			PIN_PULLUP_DIS(pin_mux[pin]);
			//PIN_PULLDWN_DIS(pin_mux[pin]);
			break;
	}

	switch(mode) {
		case GPIO_INPUT:
			GPIO_DIS_OUTPUT(pin_num[pin]);
			break;
		case GPIO_OUTPUT:
			ETS_GPIO_INTR_DISABLE();

			PIN_FUNC_SELECT(pin_mux[pin], pin_func[pin]);
			//disable interrupt
			gpio_pin_intr_state_set(GPIO_ID_PIN(pin_num[pin]), GPIO_PIN_INTR_DISABLE);
			//clear interrupt status
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin_num[pin]));
			GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin_num[pin])), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin_num[pin]))) & (~ GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE))); //disable open drain;
			ETS_GPIO_INTR_ENABLE();
			break;

		default:
			break;
	}
	return 1;
}

int ICACHE_FLASH_ATTR gpio_write(unsigned pin, unsigned level)
{
	if (pin >= GPIO_PIN_NUM)
		return -1;

	GPIO_OUTPUT_SET(GPIO_ID_PIN(pin_num[pin]), level);
}

int ICACHE_FLASH_ATTR gpio_read(unsigned pin)
{
	if (pin >= GPIO_PIN_NUM)
		return -1;

	// GPIO_DIS_OUTPUT(pin_num[pin]);
	return 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin_num[pin]));
}


