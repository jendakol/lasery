#ifndef SENSOR_SENSOR_H
#define SENSOR_SENSOR_H

#include <Arduino.h>
#include <PCF8574.h>

#define SENSOR_PIN_SIGNAL 0
#define SENSOR_PIN_LED_RED 1
#define SENSOR_PIN_LED_GREEN 2
#define SENSOR_PIN_BUZZER 3

enum LedStatus {
    GreenBlink, GreenStill, RedBlink, RedStill
};

class Sensor {
public:
    Sensor(u8 address);

    void loop(u64 now);

    u8 measure();

    u8 getAddress() const;

    LedStatus ledStatus = RedStill;
private:
    PCF8574 pcf;
    u8 address;
};


#endif //SENSOR_SENSOR_H
