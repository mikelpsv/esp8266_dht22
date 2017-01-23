#ifndef __DHT_H__
#define __DHT_H__

#define DHT_MAXTIMINGS	10000
#define DHT_BREAKTIME	20
#define DHT_MAXCOUNT	32000


// тип датчика
typedef enum {
	DHT11 = 0,
	DHT22
} dht_type;

// данные датчика
typedef struct {
  float temperature;
  float humidity;
} dht_data;

// датчик-тип-пин
typedef struct {
  uint8_t pin;
  dht_type type;
} dht_sensor;

bool dht_init(dht_sensor *sensor);
bool dht_read(dht_sensor *sensor, dht_data* output);

#endif