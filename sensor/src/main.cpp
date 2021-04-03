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

#define STATE_REPORT_EVERY 2000

#define REPORT_INTERVAL_MIN 1000
#define REPORT_INTERVAL_MAX 5000

#define CONFIRMATION_TIMEOUT 100
#define RECONNECTION_TIMEOUT 50
#define PING_TIMEOUT (PING_EVERY + 500)

#define MEASURE_SAMPLES 3
#define MEASURE_THRESHOLD 80

int appState = STATE_STARTED;
unsigned long lastPing = 0;
unsigned long lastReport = 0;
unsigned long lastReportSent = 0;
bool lastReceivedStateWasAlert = false;
bool unconfirmedMessage = false;

IPAddress gatewayIp;
WebSocketsClient ws;

typedef struct SensorConfig {
    String wifiSsid;
    String wifiPass;
    byte sensors;
} SensorConfig;

// TODO load from eeprom ?
SensorConfig config = SensorConfig{
        .wifiSsid = WIFI_SSID,
        .wifiPass = WIFI_PASSWORD,
        .sensors = 1
};

void printState();

byte measure(byte sensorId) {
    switch (sensorId) {
        case 0: {
            digitalWrite(PIN_SENSOR1, HIGH);
            digitalWrite(PIN_SENSOR2, LOW);
        }
            break;
        case 1: {
            digitalWrite(PIN_SENSOR1, LOW);
            digitalWrite(PIN_SENSOR2, HIGH);
        }
            break;
        default:
            Serial.println("Invalid sensorId!");
            return -1;
    }

    int sum = 0;

    for (int i = 0; i < MEASURE_SAMPLES; ++i) {
        sum += analogRead(A0);
        delay(1);
    }

    return sum / MEASURE_SAMPLES;
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len) {
    unsigned long now = millis();

    switch (type) {
        case WStype_DISCONNECTED: {
            appState = STATE_STARTED;
            Serial.println(F("Websocket disconnected! Reconnecting.."));

            const unsigned long start = now;

            ws.begin(gatewayIp, 80, SENSOR_WS_PATH);

            while (!ws.isConnected() && (millis() - start) < RECONNECTION_TIMEOUT) {
                delay(0);
                ws.loop();
            }

            if (!ws.isConnected()) {
                Serial.println(F("Could not reconnect, restarting"));
                ESP.restart();
            }
        }
            break;
        case WStype_CONNECTED: {
            Serial.println("Connected!");
            lastReportSent = lastPing = now;
            appState = STATE_CONNECTED;
        }
            break;
        case WStype_TEXT: {
            StaticJsonDocument<100> json;
            deserializeJson(json, payload, len);

            const String status = json["status"];

            Serial.printf("Confirmed status after %lu ms: %s\n", now - lastReportSent, status.c_str());

            if (status == "alert" || status == "alert-ok") {
                unconfirmedMessage = false;
                lastReceivedStateWasAlert = (status == "alert");
            } else {
                digitalWrite(PIN_RED, HIGH);
                digitalWrite(PIN_GREEN, LOW);
                Serial.println((char *) payload);
                Serial.println(F("Unknown message received:"));

                for (uint i = 0; i < len; i++) {
                    Serial.print((char) payload[i]);
                }
                Serial.println();
            }
        }
            break;
        case WStype_PING: {
//            Serial.print("PING ");
//
//            for (uint i = 0; i < len; i++) {
//                Serial.print((char) payload[i]);
//            }
//            Serial.println();
            lastPing = now;
        }
            break;
        default: {
            Serial.print(F("Unsupported WS event: "));
            Serial.println(type);
        }
    }
}

void loopFor(unsigned int mill) {
    const unsigned long start = millis();

    ws.loop();

    while ((millis() - start) < mill) {
        delay(0);
        ws.loop();
    }
}

void reportAlerting(bool alerting) {
    char output[40];
    StaticJsonDocument<40> json;

    const char *status = alerting ? "alert" : "alert-ok";

    Serial.print(F("Reporting status: "));
    Serial.println(status);

    json["type"] = F("alert");
    json["status"] = status;

    loopFor(5);

    const size_t l = serializeJson(json, output);
    if (!ws.sendTXT(output, l)) {
        Serial.println("Could not send 'alert' message!");
        ws.disconnect();
        return;
    }

    lastReportSent = millis();
    unconfirmedMessage = true;
}

void setup() {
    Serial.begin(115200);

    Serial.println("Initializing...");

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

    Serial.printf("\nConnected to wifi with IP %s\n", WiFi.localIP().toString().c_str());

    gatewayIp = WiFi.gatewayIP();

    Serial.print(F("Connecting to ws://"));
    Serial.print(gatewayIp);
    Serial.println(SENSOR_WS_PATH);

    ws.onEvent(webSocketEvent);
    ws.begin(gatewayIp, 80, SENSOR_WS_PATH);
}

void loop() {
    loopFor(5);

    unsigned long now = millis();

    if (duration(now, lastReport) > STATE_REPORT_EVERY) {
        printState();
    }

    if (appState == STATE_CONNECTED) {
        //TODO do rest of setup
        appState = STATE_RUNNING;
    }

    loopFor(5);

    if (appState >= STATE_RUNNING) {
        bool alerting = false;

        for (int i = 0; i < config.sensors; ++i) {
            loopFor(5);
            alerting |= (measure(i) < MEASURE_THRESHOLD);
        }

        loopFor(5);

        now = millis();

        // following two values are basically a snapshot; this is to prevent race-condition with WS events -.|.-
        const unsigned long sinceLastReport = duration(now, lastReportSent);
        const bool isUnconfirmedMessage = unconfirmedMessage;

        const bool alertingChange = alerting != lastReceivedStateWasAlert;

        // at least once per REPORT_INTERVAL_MAX but not more often than REPORT_INTERVAL_MIN
        if (sinceLastReport > REPORT_INTERVAL_MAX || (alertingChange && (sinceLastReport > REPORT_INTERVAL_MIN))) {
            if (!isUnconfirmedMessage) {
                reportAlerting(alerting);
            } else {
                Serial.println("There is an unconfirmed message, skipping!");
            }
            loopFor(5);
        }

        if (isUnconfirmedMessage && (sinceLastReport > CONFIRMATION_TIMEOUT)) {
            Serial.printf("Message confirmation has timed-out (%lu ms), disconnecting\n", sinceLastReport);
            ws.disconnect();
            return;
        }

        loopFor(5);

        const unsigned long sinceLastPing = duration(now, lastPing);
        if (sinceLastPing > PING_TIMEOUT) {
            Serial.printf("Didn't receive ping for too long (%lu ms), disconnecting\n", sinceLastPing);
            ws.disconnect();
            return;
        }

        appState = alerting ? STATE_ALERTING : STATE_RUNNING;
    }
}

void printState() {
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
