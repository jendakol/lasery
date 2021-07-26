#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <Sensor.h>
#include <Wire.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"

#define STATE_STARTED 0
#define STATE_CONNECTED 1
#define STATE_RUNNING 2
#define STATE_ALERTING 3

#define STATE_REPORT_EVERY 2000

#define REPORT_INTERVAL_MIN 50
#define REPORT_INTERVAL_MAX 5000

#define CONFIRMATION_TIMEOUT 100
#define RECONNECTION_TIMEOUT 50
#define PING_TIMEOUT (PING_EVERY + 500)

#define MAX_RECONNECTIONS 5
#define MIN_RECONNECTIONS_INTERVAL 5000

int appState = STATE_STARTED;
u64 lastPing = 0;
u64 lastReport = 0;
u64 lastDebugReport = 0;
u64 lastReportSent = 0;
bool lastReceivedStateWasAlert = false;
bool unconfirmedMessage = false;

u8 totalReconnections = 0;
u64 lastReconnection = 0;

// TODO init dynamically - support less than 2 sensors
#define SENSORS_COUNT 2
Sensor *sensors[] = {new Sensor(0x20), new Sensor(0x21)};

IPAddress gatewayIp;
WebSocketsClient ws;

void printState();

void displayState(u64 now);

u8 measureAndUpdateStates();

void webSocketEvent(WStype_t type, uint8_t *payload, size_t len) {
    u64 now = millis();

    switch (type) {
        case WStype_DISCONNECTED: {
            appState = STATE_STARTED;
            for (int i = 0; i < SENSORS_COUNT; ++i) {
                Sensor *sensor = sensors[i];
                sensor->ledStatus = LedStatus::RedStill;
            }
            displayState(now);
            Serial.println(F("Websocket disconnected!"));

            if (durationBetween(now, lastReconnection) < MIN_RECONNECTIONS_INTERVAL) {
                Serial.println(F("Too frequent reconnections, restarting..."));
                ESP.restart();
                return;
            }

            if (totalReconnections++ > MAX_RECONNECTIONS) {
                Serial.println(F("Too many reconnections, restarting..."));
                ESP.restart();
                return;
            }

            Serial.println(F("Reconnecting..."));

            const u64 start = now;

            ws.begin(gatewayIp, 80, SENSOR_WS_PATH);

            for (int i = 0; i < SENSORS_COUNT; ++i) {
                Sensor *sensor = sensors[i];
                sensor->ledStatus = LedStatus::RedBlink;
            }

            while (!ws.isConnected() && ((now = millis()) - start) < RECONNECTION_TIMEOUT) {
                delay(0);
                displayState(now);
                ws.loop();
            }

            displayState(now);

            if (!ws.isConnected()) {
                Serial.println(F("Could not reconnect, restarting"));
                ESP.restart();
                return;
            }

            totalReconnections++;
            lastReconnection = millis();
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

            Serial.printf("Confirmed status after %llu ms: %s\n", durationBetween(now, lastReportSent), status.c_str());

            if (status == "alert" || status == "alert-ok") {
                unconfirmedMessage = false;
                lastReceivedStateWasAlert = (status == "alert");
            } else {
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
            lastPing = now;
        }
            break;
        default: {
            Serial.print(F("Unsupported WS event: "));
            Serial.println(type);
        }
    }
}

void wsLoop() {
    const u64 start = millis();

    ws.loop();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "LoopDoesntUseConditionVariableInspection"
    while ((millis() - start) < 1) {
        delay(0);
        ws.loop();
    }
#pragma clang diagnostic pop
}

void reportAlerting(bool alerting) {
    char output[40];
    StaticJsonDocument<40> json;

    const char *status = alerting ? "alert" : "alert-ok";

    Serial.print(F("Reporting status: "));
    Serial.println(status);

    json["type"] = F("alert");
    json["status"] = status;

    wsLoop();

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

    for (int i = 0; i < SENSORS_COUNT; ++i) {
        Sensor *sensor = sensors[i];
        sensor->ledStatus = LedStatus::RedStill;
    }

    Serial.println("Initializing...");

    displayState(millis());

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print(F("\nConnecting to wifi "));
    Serial.println(WIFI_SSID);

    for (int i = 0; i < SENSORS_COUNT; ++i) {
        Sensor *sensor = sensors[i];
        sensor->ledStatus = LedStatus::RedBlink;
    }

    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        const u64 start = millis();
        u64 now = start;
        while ((now = millis()) - start < 500) {
            displayState(now);
            delay(5);
        }
        Serial.print('.');

    }
    delay(20);

    displayState(millis());

    Serial.printf("\nConnected to wifi with IP %s\n", WiFi.localIP().toString().c_str());

    gatewayIp = WiFi.gatewayIP();

    Serial.print(F("Connecting to ws://"));
    Serial.print(gatewayIp);
    Serial.println(SENSOR_WS_PATH);

    ws.onEvent(webSocketEvent);
    ws.begin(gatewayIp, 80, SENSOR_WS_PATH);
}

void loop() {
    wsLoop();
    bool isDebugReporting = false;

    u64 now = millis();

    if (now - lastDebugReport > 500) {
        isDebugReporting = true;
        lastDebugReport = now;
    }

    displayState(now);

    if (durationBetween(now, lastReport) > STATE_REPORT_EVERY) {
        printState();
        wsLoop();
    }

    if (appState == STATE_CONNECTED) {
        //TODO do rest of setup
        appState = STATE_RUNNING;
    }

    wsLoop();

    if (appState >= STATE_RUNNING) {
        const bool alerting = measureAndUpdateStates() == LOW;

        if (isDebugReporting) {
            for (auto & sensor : sensors) {
                Wire.beginTransmission(sensor->getAddress());
                if (Wire.endTransmission() != 0) {
                    Serial.printf("Sensor on address 0x%x not found\n", sensor->getAddress());
                }
            }

            if (alerting) Serial.println("Alert!!");
        }

        wsLoop();

        now = millis();

        // following two values are basically a snapshot; this is to prevent race-condition with WS events -.|.-
        const u32 sinceLastReport = durationBetween(now, lastReportSent);
        const bool isUnconfirmedMessage = unconfirmedMessage;

        const bool alertingChange = alerting != lastReceivedStateWasAlert;

        // at least once per REPORT_INTERVAL_MAX but not more often than REPORT_INTERVAL_MIN
        if (sinceLastReport > REPORT_INTERVAL_MAX || (alertingChange && (sinceLastReport > REPORT_INTERVAL_MIN))) {
            if (!isUnconfirmedMessage) {
                reportAlerting(alerting);
            } else {
                Serial.println("There is an unconfirmed message, skipping!");
            }
            wsLoop();
        }

        if (isUnconfirmedMessage && (sinceLastReport > CONFIRMATION_TIMEOUT)) {
            Serial.printf("Message confirmation has timed-out (%u ms), disconnecting\n", sinceLastReport);
            ws.disconnect();
            return;
        }

        wsLoop();

        const u32 sinceLastPing = durationBetween(now, lastPing);
        if (sinceLastPing > PING_TIMEOUT) {
            Serial.printf("Didn't receive ping for too long (%u ms), disconnecting\n", sinceLastPing);
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

void displayState(u64 now) {
    for (int i = 0; i < SENSORS_COUNT; ++i) {
        sensors[i]->loop(now);
    }
}

u8 measureAndUpdateStates() {
    u8 result = HIGH;
    for (auto sensor : sensors) {
        const u8 measured = sensor->measure();
//        if (measured == LOW) Serial.printf("Sensor 0x%x alerting!\n", sensor->getAddress());
        result &= measured;
        sensor->ledStatus = (measured == HIGH ? LedStatus::GreenStill : LedStatus::GreenBlink);
    }

    return result;
}

#pragma clang diagnostic pop
