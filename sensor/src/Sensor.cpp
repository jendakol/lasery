#include "Sensor.h"

Sensor::Sensor(const u8 address) : pcf(address) {
    pcf.begin();
    pcf.write8(0xff);
    this->address = address;
}

void Sensor::loop(const u64 now) {
    const u8 phase = now / 250 % 2;

    switch (ledStatus) {
        case GreenBlink:
            pcf.write(SENSOR_PIN_LED_RED, LOW);
            pcf.write(SENSOR_PIN_LED_GREEN, phase == 0 ? HIGH : LOW);
            pcf.write(SENSOR_PIN_BUZZER, HIGH);
            break;
        case GreenStill:
            pcf.write(SENSOR_PIN_LED_RED, LOW);
            pcf.write(SENSOR_PIN_LED_GREEN, HIGH);
            pcf.write(SENSOR_PIN_BUZZER, LOW);
            break;
        case RedBlink:
            pcf.write(SENSOR_PIN_LED_RED, phase == 0 ? HIGH : LOW);
            pcf.write(SENSOR_PIN_LED_GREEN, LOW);
            pcf.write(SENSOR_PIN_BUZZER, LOW);
            break;
        case RedStill:
            pcf.write(SENSOR_PIN_LED_RED, HIGH);
            pcf.write(SENSOR_PIN_LED_GREEN, LOW);
            pcf.write(SENSOR_PIN_BUZZER, HIGH);
            break;
    }
}

u8 Sensor::measure() {
    return pcf.read(SENSOR_PIN_SIGNAL) == LOW ? HIGH : LOW; // return HIGH on a signal, LOW on no signal
}

u8 Sensor::getAddress() const {
    return address;
}
