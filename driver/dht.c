#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "dht.h"
#include "gpio_util.h"

static inline float scale_humidity(dht_type sensor_type, int *data){
	
	if(sensor_type == DHT11) {
		return (float) data[0];
	} else {
		float humidity = data[0] * 256 + data[1];
		return humidity /= 10;
	}
}

static inline float scale_temperature(dht_type sensor_type, int *data){
	if(sensor_type == DHT11) {
		return (float) data[2];
	} else {
		float temperature = data[2] & 0x7f;
		temperature *= 256;
		temperature += data[3];
		temperature /= 10;
		if (data[2] & 0x80)
			temperature *= -1;
		return temperature;
	}
}

/*
char* float2string(char* buffer, float value)
{
  os_sprintf(buffer, "%d.%d", (int)(value),(int)((value - (int)value)*100));
  return buffer;
}
*/
bool dht_read(dht_sensor *sensor){
	char str[150];
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int j = 0;
	int checksum = 0;
	int data[100];
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	uint8_t pin = pin_num[sensor->pin];

	


    uart0_send_str("start read\r\n");

	// Wake up device, 250ms of high
	GPIO_OUTPUT_SET(pin, 1);
	os_delay_us(250*1000);
	
	// Hold low for 20ms
	GPIO_OUTPUT_SET(pin, 0);
	os_delay_us(20*1000);

	// High for 40ns
	GPIO_OUTPUT_SET(pin, 1);
	os_delay_us(40);

	// Set DHT_PIN pin as an input
	GPIO_DIS_OUTPUT(pin);
	//PIN_PULLUP_EN(pin_mux[pin]);

	// wait for pin to drop?
	while (GPIO_INPUT_GET(pin) == 1 && i < DHT_MAXCOUNT) {
		os_delay_us(1);
		i++;
	}

	if(i == DHT_MAXCOUNT)
	{
		os_sprintf(str, "DHT: ошибка получения данных с GPIO%d, dying\r\n", pin);
		uart0_send_str(str);
	    return false;
	}

	// read data
	for (i = 0; i < DHT_MAXTIMINGS; i++){

		// Count high time (in approx us)
		counter = 0;
		while (GPIO_INPUT_GET(pin) == laststate){

			//os_sprintf(str, "laststate %d\r\n", laststate);
			//uart0_send_str(str);
			counter++;
			os_delay_us(1);
			if (counter == 1000)
				break;
		}

		laststate = GPIO_INPUT_GET(pin);
		if (counter == 1000)
			break;


		// storagere data after 3 reads
		if ((i>3) && (i%2 == 0)) {
			// shove each bit into the storage bytes
			data[j/8] <<= 1;
			if (counter > DHT_BREAKTIME)
				data[j/8] |= 1;
			j++;
		}
	}

	if (j >= 39) {
		checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
		os_sprintf(str, "DHT%s: %02x %02x %02x %02x [%02x] CS: %02x (GPIO%d)\r\n",
			sensor->type==DHT11?"11":"22",
			data[0], data[1], data[2], data[3], data[4], checksum, pin
			);
		uart0_send_str(str);

		if (data[4] == checksum) {
			// checksum is valid
			sensor->temperature = scale_temperature(sensor->type, data);
			sensor->humidity = scale_humidity(sensor->type, data);

			os_sprintf(str, "DHT: Temperature*100 =  %d *C, Humidity*100 = %d %% (GPIO%d)\n",
		          (int) (sensor->temperature * 100), (int) (sensor->humidity * 100), pin);
			uart0_send_str(str);
		} else {
			os_sprintf(str, "DHT: Checksum was incorrect after %d bits. Expected %d but got %d (GPIO%d)\r\n",
		                j, data[4], checksum, pin);
		    uart0_send_str(str);
		    return false;
		}
	} else {
	    os_sprintf(str, "DHT: Got too few bits: %d should be at least 40 (GPIO%d)\r\n", j, pin);
	    uart0_send_str(str);
	    return false;
	}
	return true;
}

void dht_init(dht_sensor *sensor){

	if (set_gpio_mode(sensor->pin, GPIO_OUTPUT, GPIO_PULLUP)) {
		uart0_send_str("dht_init true\r\n");
		sensor->enable = 1;
	} else {
		uart0_send_str("dht_init false\r\n");
		sensor->enable = 0;
	}
	return;
}