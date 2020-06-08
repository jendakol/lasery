#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESP8266WiFi.h>
#include <WebSocketClient.h>

#define PIN_GREEN D1
#define PIN_RED D2

#define PIN_SENSOR1 D6
#define PIN_SENSOR2 D7

IPAddress gatewayIp;
WebSocketClient ws(true);

void setup() {
    Serial.begin(9600);

    pinMode(A0, INPUT);

    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);

    pinMode(PIN_SENSOR1, OUTPUT);
    pinMode(PIN_SENSOR2, OUTPUT);

    digitalWrite(PIN_GREEN, LOW);
    digitalWrite(PIN_RED, LOW);

    digitalWrite(PIN_SENSOR1, HIGH);
    digitalWrite(PIN_SENSOR2, HIGH);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
        Serial.print('.');
        digitalWrite(PIN_GREEN, HIGH);
        delay(20);
        digitalWrite(PIN_GREEN, LOW);
    }

    while (!ws.isConnected()) {
        ws.connect(gatewayIp.toString(), "/ws", 80);
        Serial.print('.');
        digitalWrite(PIN_GREEN, HIGH);
        delay(200);
        digitalWrite(PIN_GREEN, LOW);
    }

    digitalWrite(PIN_GREEN, HIGH);

    Serial.println('\n');
    Serial.println(F("Connection established!"));
    gatewayIp = WiFi.gatewayIP();
}

void loop() {
    digitalWrite(PIN_SENSOR1, HIGH);
    digitalWrite(PIN_SENSOR2, LOW);
    delay(50);
    int value1 = analogRead(A0);

    digitalWrite(PIN_SENSOR1, LOW);
    digitalWrite(PIN_SENSOR2, HIGH);
    delay(50);
    int value2 = analogRead(A0);

    bool status1 = value1 > 900;
    bool status2 = value2 > 900;

    Serial.print(status1);
    Serial.print(",");
    Serial.println(status2);
}
