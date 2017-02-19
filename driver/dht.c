#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "user_interface.h"
#include "gpio.h"
#include "dht.h"
#include "ugpio.h"





inline double fabs(double x){
    return( x<0 ?  -x :  x );
}

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

// Функция перевода относительной влажности при заданной температуре в абсолютную
// Атрибут "ICACHE_FLASH_ATTR" помещает функцию во FLASH, а не оставляет в ОЗУ
float ICACHE_FLASH_ATTR calc_abs_h(float t, float h){
    float temp;
    temp = pow(2.718281828, (17.67*t) / (t+243.5));
    return (6.112 * temp * h * 2.1674) / (273.15 + t);
} 


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

// данные датчика переводит в json
int dataToJSON(dht_sensor *sensor, char* buffer){

	return os_sprintf(buffer, "{t:%d, h:%d, habs:%d, cnt:%d}\0", 
				(int)(sensor->temperature * 100), 
				(int)(sensor->humidity * 100),
				(int)(sensor->humidity_a * 100),
				sensor->counter
				);
}

// опрос датчика
bool dht_read(dht_sensor *sensor){
	char str[150];
	int counter = 0;
	int laststate = 1;
	int i = 0;
	int j = 0;
	int checksum = 0;
	int data[100];
	data[0] = data[1] = data[2] = data[3] = data[4] = 0;
	uint8_t pin = sensor->pin;
	float _t, _h;
	


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

			// текущие значения датчика
			_t = scale_temperature(sensor->type, data);
			_h = scale_humidity(sensor->type, data);
		
			// усредняем
			sensor->temperature = (sensor->temperature * sensor->counter + _t)/(sensor->counter + 1);
			sensor->humidity 	= (sensor->humidity * sensor->counter + _h)/(sensor->counter + 1);
			sensor->humidity_a 	= calc_abs_h(sensor->temperature, sensor->humidity);

			sensor->counter = sensor->counter + 1;

			os_sprintf(str, "DHT: Temperature*100 =  %d *C, Humidity*100 = %d %% (GPIO%d)\r\n",
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