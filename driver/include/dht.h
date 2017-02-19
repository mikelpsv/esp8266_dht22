#ifndef __DHT_H__
#define __DHT_H__

#define DHT_MAXTIMINGS	10000
#define DHT_BREAKTIME	20
#define DHT_MAXCOUNT	32000
#define DHT_COUNTER		5


// тип датчика
typedef enum {
	DHT11 = 0,
	DHT22
} dht_type;


// датчик-тип-пин-данные
typedef struct {
  uint8_t pin;       	// используемый вход
  dht_type type;     	// тип датчика
  uint8_t counter;   	// количество считанных данных
  float temperature; 	// данные температуры
  float humidity;    	// данные влажности
  float humidity_a;    	// данные влажности
  uint8_t enable;    	// работает или нет
} dht_sensor;

void dht_init(dht_sensor *sensor);
bool dht_read(dht_sensor *sensor);
//void dataToJSON(dht_sensor *sensor, char* buffer);

#endif