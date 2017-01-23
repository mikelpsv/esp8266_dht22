#include "ets_sys.h"
#include "osapi.h"
#include "gpio_util.h"

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


