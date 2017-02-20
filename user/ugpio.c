/*
    Driver for GPIO
	Original https://github.com/CHERTS/esp8266-gpio16
	    
*/

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "ugpio.h"


#ifdef GPIO_INTERRUPT_ENABLE
GPIO_INT_TYPE pin_int_type[GPIO_PIN_NUM] = {
								GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE,
								GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE,
								GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE,
								GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE, GPIO_PIN_INTR_DISABLE};
#endif


uint8_t ICACHE_FLASH_ATTR is_valid_pin(uint8_t pin_num){
	switch(pin_num) {
		case 6:
		case 7:
		case 8:
		case 11: return 0; break;
		default:
			if(pin_num > GPIO_PIN_NUM) 
				return 0;
			else
				return 1;
			break;
	}
}

uint32_t ICACHE_FLASH_ATTR get_pin_mux(uint8_t pin_num){
	switch(pin_num) {
		case 0: return PERIPHS_IO_MUX_GPIO0_U; break;
		case 1: return PERIPHS_IO_MUX_U0TXD_U; break;
		case 2: return PERIPHS_IO_MUX_GPIO2_U; break;
		case 3: return PERIPHS_IO_MUX_U0RXD_U; break;
		case 4: return PERIPHS_IO_MUX_GPIO4_U; break;
		case 5: return PERIPHS_IO_MUX_GPIO5_U; break;
		case 9: return PERIPHS_IO_MUX_SD_DATA2_U; break;
		case 10: return PERIPHS_IO_MUX_SD_DATA3_U; break;
		case 12: return PERIPHS_IO_MUX_MTDI_U; break;
		case 13: return PERIPHS_IO_MUX_MTCK_U; break;
		case 14: return PERIPHS_IO_MUX_MTMS_U; break;
		case 15: return PERIPHS_IO_MUX_MTDO_U; break;
		case 16: return PAD_XPD_DCDC_CONF; break;
		default:
			// 6, 7,8, 11
			return 0;
			break;
	}
}

uint8_t ICACHE_FLASH_ATTR get_pin_func(uint8_t pin_num){
	switch(pin_num) {
		case 0: return FUNC_GPIO0; break;
		case 1: return FUNC_GPIO1; break;
		case 2: return FUNC_GPIO2; break;
		case 3: return FUNC_GPIO3; break;
		case 4: return FUNC_GPIO4; break;
		case 5: return FUNC_GPIO5; break;
		case 9: return FUNC_GPIO9; break;
		case 10: return FUNC_GPIO10; break;
		case 12: return FUNC_GPIO12; break;
		case 13: return FUNC_GPIO13; break;
		case 14: return FUNC_GPIO14; break;
		case 15: return FUNC_GPIO15; break;
		case 16: return 0; break;
		default:
			// 6, 7,8, 11
			return 0;
			break;
	}
}


void ICACHE_FLASH_ATTR gpio16_output_conf(void) {
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC to output rtc_gpio0

	WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

	WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe) | (uint32)0x1);	//out enable
}

void ICACHE_FLASH_ATTR gpio16_output_set(uint8 value){
	WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32)0xfffffffe) | (uint32)(value & 1));
}

void ICACHE_FLASH_ATTR gpio16_input_conf(void){
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1); 	// mux configuration for XPD_DCDC and rtc_gpio0 connection

	WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0);	//mux configuration for out enable

	WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);	//out disable
}

uint8 ICACHE_FLASH_ATTR gpio16_input_get(void){
	return (uint8)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}




int ICACHE_FLASH_ATTR set_gpio_mode(unsigned pin, unsigned mode, unsigned pull){
	char str[50];
	os_sprintf(str, "set_gpio_mode %d", pin);
	uart0_send_str(str);

	if (!is_valid_pin(pin)) return 0;

	if(pin == 16) {
		if(mode == GPIO_INPUT)
			gpio16_input_conf();
		else
			gpio16_output_conf();
		return 1;
	}
	

	switch(pull) {
		case GPIO_PULLUP:
			// Включить подтяжку
			PIN_PULLUP_EN(get_pin_mux(pin));
			break;
		case GPIO_PULLDOWN:
			// Отключить подтяжку
			PIN_PULLUP_DIS(get_pin_mux(pin));
			break;
		case GPIO_FLOAT:
			// Отключить подтяжку
			PIN_PULLUP_DIS(get_pin_mux(pin));
			break;
		default:
			// Отключить подтяжку
			PIN_PULLUP_DIS(get_pin_mux(pin));
			break;
	}

	switch(mode) {
		case GPIO_INPUT:
			GPIO_DIS_OUTPUT(pin);
			break;
		case GPIO_OUTPUT:
			ETS_GPIO_INTR_DISABLE();

#ifdef GPIO_INTERRUPT_ENABLE
			pin_int_type[pin] = GPIO_PIN_INTR_DISABLE;
#endif
			
			PIN_FUNC_SELECT(get_pin_mux(pin), get_pin_func(pin));
			//disable interrupt
			gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);
			//clear interrupt status
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
			GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(pin))) & (~ GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE))); //disable open drain;
			ETS_GPIO_INTR_ENABLE();
			break;

#ifdef GPIO_INTERRUPT_ENABLE
		case GPIO_INT:
			ETS_GPIO_INTR_DISABLE();
			PIN_FUNC_SELECT(get_pin_mux(pin), get_pin_func(pin));
			GPIO_DIS_OUTPUT(pin);
			gpio_register_set(GPIO_PIN_ADDR(GPIO_ID_PIN(pin)), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                        | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                        | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));
			ETS_GPIO_INTR_ENABLE();
			break;
#endif

		default:
			break;
	}
	return 1;
}

int ICACHE_FLASH_ATTR gpio_write(unsigned pin, unsigned level){
	if (!is_valid_pin(pin)) return 0;

	if(pin == 16){
		gpio16_output_conf();
		gpio16_output_set(level);
		return 1;
	}

	GPIO_OUTPUT_SET(GPIO_ID_PIN(pin), level);
}


int ICACHE_FLASH_ATTR gpio_read(unsigned pin){
	if (!is_valid_pin(pin)) return 0;

	if(pin == 16){
		// gpio16_input_conf();
		return 0x1 & gpio16_input_get();
	}
	// GPIO_DIS_OUTPUT(pin_num[pin]);
	return 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(pin));
}





#ifdef GPIO_INTERRUPT_ENABLE

void ICACHE_FLASH_ATTR gpio_intr_dispatcher(gpio_intr_handler cb){
	uint8 i, level;
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	
	for (i = 0; i < GPIO_PIN_NUM; i++) {
		
		if(is_valid_pin(i) == 0)
			continue;

		if (pin_int_type[i] && (gpio_status & BIT(i)) ) {
			//disable global interrupt
			ETS_GPIO_INTR_DISABLE();
			//disable interrupt
			gpio_pin_intr_state_set(GPIO_ID_PIN(i), GPIO_PIN_INTR_DISABLE);
			//clear interrupt status
			GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(i));
			level = 0x1 & GPIO_INPUT_GET(GPIO_ID_PIN(i));
			if(cb){
				cb(i, level);
			}
			//enable interrupt
			gpio_pin_intr_state_set(GPIO_ID_PIN(i), pin_int_type[i]);
			//enable global interrupt
			ETS_GPIO_INTR_ENABLE();
		}
	}
}

void ICACHE_FLASH_ATTR gpio_intr_attach(gpio_intr_handler cb){
	ETS_GPIO_INTR_ATTACH(gpio_intr_dispatcher, cb);
}

int ICACHE_FLASH_ATTR gpio_intr_deattach(unsigned pin){
	if (!is_valid_pin(pin)) return 0;

	//disable global interrupt
	ETS_GPIO_INTR_DISABLE();
	//clear interrupt status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
	pin_int_type[pin] = GPIO_PIN_INTR_DISABLE;
	//disable interrupt
	gpio_pin_intr_state_set(GPIO_ID_PIN(pin), pin_int_type[pin]);
	//enable global interrupt
	ETS_GPIO_INTR_ENABLE();
	return 1;
}

int ICACHE_FLASH_ATTR gpio_intr_init(unsigned pin, GPIO_INT_TYPE type){
	if (!is_valid_pin(pin)) return 0;

	//disable global interrupt
	ETS_GPIO_INTR_DISABLE();
	//clear interrupt status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(pin));
	pin_int_type[pin] = type;
	//enable interrupt
	gpio_pin_intr_state_set(GPIO_ID_PIN(pin), type);
	//enable global interrupt
	ETS_GPIO_INTR_ENABLE();
	return 1;
}
#endif