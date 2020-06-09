#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

#define PIN_GREEN D1
#define PIN_RED D2

#define PIN_SENSOR1 D6
#define PIN_SENSOR2 D7

#define STATE_STARTED 0
#define STATE_CONNECTED 1
#define STATE_RUNNING 2
#define STATE_ALERTING 3

int appState = STATE_STARTED;
unsigned long lastPing = 0;
unsigned long lastReport = 0;

IPAddress gatewayIp;
WebSocketsClient ws;

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len) {
    switch (type) {
        case WStype_DISCONNECTED: {
            appState = STATE_STARTED;
            Serial.println(F("Websocket disconnected! Restarting"));
            ESP.restart();
        }
            break;
        case WStype_CONNECTED: {
            appState = STATE_STARTED;
            Serial.println(F("\nDoing handshake..."));

            char output[50];
            StaticJsonDocument<50> json;

            json["type"] = F("handshake");

            size_t l = serializeJson(json, output);
            ws.sendTXT(output, l);
        }
            break;
        case WStype_PONG: {
            lastPing = millis();
        }
            break;
        case WStype_TEXT: {
            StaticJsonDocument<200> json;
            deserializeJson(json, payload, len);

            if (json["status"] == "ok") {
                digitalWrite(PIN_GREEN, HIGH);

                if (appState == STATE_STARTED) appState = STATE_CONNECTED;
            } else {
                digitalWrite(PIN_RED, HIGH);
                digitalWrite(PIN_GREEN, LOW);
                Serial.println((char *) payload);
                Serial.println(F("Could not properly connect to receiver"));
            }
        }
            break;
        default: {
            Serial.print(F("Unsupported WS event: "));
            Serial.println(type);
        }
    }
}

void reportAlerting(bool alerting) {
    char output[50];
    StaticJsonDocument<50> json;

    const char *status = alerting ? "alert" : "ok";

    Serial.print(F("Reporting status: "));
    Serial.println(status);

    json["type"] = F("alert");
    json["status"] = status;

    size_t l = serializeJson(json, output);
    ws.sendTXT(output, l);
}

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

    Serial.print(F("\nConnecting to wifi "));
    Serial.println(WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(500);
        Serial.print('.');
        digitalWrite(PIN_GREEN, HIGH);
        delay(20);
        digitalWrite(PIN_GREEN, LOW);
    }

    gatewayIp = WiFi.gatewayIP();

    Serial.print(F("\nConnecting to ws://"));
    Serial.print(gatewayIp);
    Serial.println(WS_PATH);

    ws.onEvent(webSocketEvent);
    ws.begin(gatewayIp, 80, WS_PATH);
}

void loop() {
    if (millis() - lastReport > 1000) {
        lastReport = millis();

        Serial.print(F("State: "));

        switch (appState) {
            case STATE_STARTED:
                Serial.println("STARTED");
                break;
            case STATE_CONNECTED:
                Serial.println("CONNECTED");
                break;
            case STATE_RUNNING:
                Serial.println("RUNNING");
                break;
            case STATE_ALERTING:
                Serial.println("ALERTING");
                break;
        }
    }

    if (appState == STATE_CONNECTED) {
        //TODO do rest of setup
        appState = STATE_RUNNING;
    }

    if (appState == STATE_RUNNING || appState == STATE_ALERTING) {
        if (millis() - lastPing > 1000) {
            ws.sendPing();
        }

        // TODO check lastPing

        digitalWrite(PIN_SENSOR1, HIGH);
        digitalWrite(PIN_SENSOR2, LOW);
        delay(20);
        int value1 = analogRead(A0);

        digitalWrite(PIN_SENSOR1, LOW);
        digitalWrite(PIN_SENSOR2, HIGH);
        delay(20);
        int value2 = analogRead(A0);

        bool alerting1 = value1 > 900;
        bool alerting2 = value2 > 900;

        // TODO remove
        alerting1 = !alerting1;
        alerting2 = !alerting2;

        if ((alerting1 || alerting2) && appState == STATE_RUNNING) {
            appState = STATE_ALERTING;
            reportAlerting(true);
        } else if (!alerting1 && !alerting2 && appState == STATE_ALERTING) {
            appState = STATE_RUNNING;
            reportAlerting(false);
        }

//        Serial.print(alerting1);
//        Serial.print(",");
//        Serial.println(alerting2);
    }
}
