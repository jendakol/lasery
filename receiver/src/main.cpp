#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <set>

#include <Tasker.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

AsyncWebServer webServer(80);
AsyncWebSocket wsSensors(SENSOR_WS_PATH);

#define PIN_SWITCH_ARMED 32
#define PIN_SWITCH_SIREN 33

#define PIN_SIREN 33

typedef u8 AppState;

#define STATE_UNARMED 0
#define STATE_ARMED 1
#define STATE_ALERTING 2

typedef u8 SwitchState;

#define SWITCH_UNARMED 10
#define SWITCH_ARMED 11
#define SWITCH_SIREN 12

#define CLIENT_PONG_TIMEOUT (PING_EVERY + 500)

#define HOLD_ALERT_FOR 1000

AppState appState = STATE_ARMED;
u64 lastReport = 0;
u64 alertSince = 0;

u32 pingNo = 0;

std::set<u32> alerting_ids;
std::map<u32, u64> lastPongs;

bool shouldAlert() { return appState >= STATE_ARMED && !alerting_ids.empty(); }

void displayState() {
    // TODO display
}

void switchSiren(bool on) {
    // TODO switch
    if (on) {
        Serial.println("SIREN!!!");
    }
}

SwitchState readSwitchState() {
    // TODO adjust to physical switch

    if (digitalRead(PIN_SWITCH_ARMED) == HIGH) {
        return SWITCH_ARMED;
    }

    if (digitalRead(PIN_SWITCH_SIREN) == HIGH) {
        return SWITCH_SIREN;
    }

    return SWITCH_UNARMED;
}

void clearClients() {
    wsSensors.cleanupClients();

    const u64 now = millis();

    for (auto client: wsSensors.getClients()) {
        const u64 lastPong = lastPongs[client->id()];

        if (durationBetween(now, lastPong) > CLIENT_PONG_TIMEOUT) {
            Serial.printf("Kicking unresponsive client %d\n", client->id());
            client->client()->close(true);
            lastPongs.erase(client->id());
        }
    }
}

void onTextMessage(AsyncWebSocketClient *client, u8 *data, const size_t len) {
    StaticJsonDocument<50> requestJson;
    deserializeJson(requestJson, data, len);

    const String type = requestJson["type"];

    if (type == "alert") {
        const String status = requestJson["status"];

        Serial.printf("Received alert info from sensor %d: %s\n", client->id(), status.c_str());

        if (status == "alert") {
            alerting_ids.insert(client->id());
        } else if (status == "alert-ok") {
            alerting_ids.erase(client->id());
        }

        StaticJsonDocument<30> responseJson;

        responseJson["status"] = status;

        char output[30];
        size_t l = serializeJson(responseJson, output);

        client->text(output, l);
        return;
    }

    Serial.printf("Received unknown type: '%s'\n", type.c_str());
}

void onSensorWsEvent(__unused AsyncWebSocket *server, AsyncWebSocketClient *client,
                     AwsEventType type, __unused void *arg, u8 *data, const size_t len) {
    const u64 now = millis();

    if (type == WS_EVT_CONNECT) {
        client->keepAlivePeriod(30000);
        Serial.printf("Sensor client %d connection received\n", client->id());
        lastPongs.insert(std::pair<u32, u32>(client->id(), now));
        const String &string = String(pingNo);
        client->ping((u8 *) string.c_str(), string.length());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Sensor client %d disconnected\n", client->id());
        alerting_ids.erase(client->id());
        lastPongs.erase(client->id());
    } else if (type == WS_EVT_PONG) {
        lastPongs[client->id()] = now;
    } else if (type == WS_EVT_DATA) {
        lastPongs[client->id()] = now;
        onTextMessage(client, data, len);
    }
}

void createAP() {
    IPAddress apIP(8, 8, 8, 8);
    IPAddress netMsk(255, 255, 255, 0);

    WiFi.disconnect(true);

    Serial.print(F("Configuring access point with SSID "));
    Serial.println(WIFI_SSID);

    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    Tasker::sleep(100);
    WiFi.softAPConfig(apIP, apIP, netMsk);

    Tasker::sleep(500);
    Serial.print(F("AP IP address: "));
    Serial.println(WiFi.softAPIP());
}

void printState(const u64 now) {
    lastReport = now;

    String appStateName = "UNARMED";

    switch (appState) {
        case STATE_ARMED:
            appStateName = "ARMED";
            break;
        case STATE_ALERTING:
            appStateName = "ALERTING";
            break;
    }

    String switchStateName = "UNARMED";

    switch (readSwitchState()) {
        case SWITCH_ARMED:
            switchStateName = "ARMED";
            break;
        case SWITCH_SIREN:
            switchStateName = "SIREN";
            break;
    }

    Serial.printf("Connected sensors: %d Switch state: %s App state: %s\n",
                  wsSensors.getClients().length(),
                  switchStateName.c_str(),
                  appStateName.c_str());
}

void setup() {
    Serial.begin(115200);

    pinMode(PIN_SWITCH_ARMED, INPUT_PULLDOWN);
    pinMode(PIN_SWITCH_SIREN, INPUT_PULLDOWN);
    pinMode(PIN_SIREN, OUTPUT);

    createAP();

    DefaultTasker.once("network-setup", [] {
        webServer.addHandler(&wsSensors);
        wsSensors.onEvent(onSensorWsEvent);

        webServer.begin();
        Serial.println(F("HTTP server started"));
    });

    DefaultTasker.loopEvery("SendPing", PING_EVERY, [] {
        clearClients();

        const String &string = String(++pingNo);
        const char *data = string.c_str();
        wsSensors.pingAll((u8 *) data, string.length());
    });

    DefaultTasker.loopEvery("SwitchState", 50, [] {
        switch (readSwitchState()) {
            case SWITCH_UNARMED:
                appState = STATE_UNARMED;
                break;
            case SWITCH_ARMED:
            case SWITCH_SIREN:
                if (appState != STATE_ALERTING) {
                    appState = STATE_ARMED;
                }
                break;
        }
    });

    DefaultTasker.loopEvery("DisplayState", 10, [] {
        const u64 now = millis();

        if (appState >= STATE_ARMED) {
            if (shouldAlert()) {
                if (appState == STATE_ARMED) {
                    appState = STATE_ALERTING;
                    alertSince = now;
                }
            } else {
                // `now > HOLD_ALERT_FOR` == enough long after start
                if (now > HOLD_ALERT_FOR && durationBetween(now, alertSince) >= HOLD_ALERT_FOR) {
                    appState = STATE_ARMED;
                }
            }
        }

        displayState();

        switchSiren(appState == STATE_ALERTING && readSwitchState() == SWITCH_SIREN);

        if ((appState == STATE_ALERTING && (durationBetween(now, lastReport) > 500)) || (durationBetween(now, lastReport) > 2000)) {
            printState(now);
        }
    });
}

void loop() {
    // no-op, handling by Tasker

    // TODO self-check, restart if in weird state
}

#pragma clang diagnostic pop
