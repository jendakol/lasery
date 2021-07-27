#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <set>
#include <TelnetPrint.h>

#include <Tasker.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

AsyncWebServer webServer(80);
AsyncWebSocket wsSensors(SENSOR_WS_PATH);

#define TFT_GREY 0x5AEB

TFT_eSPI tft;

#define PIN_SWITCH_SIREN 32
#define PIN_SWITCH_LASERS 33

#define PIN_RELAY_SIREN 27
#define PIN_RELAY_LASERS 26

enum SwitchState {
    SwOff, SwLasersOn, SwSirenEnabled
};

SwitchState switchState = SwOff;

boolean sirenOn = false;

#define CLIENT_PONG_TIMEOUT (PING_EVERY + 500)

#define HOLD_ALERT_FOR 1000

bool started = false;
bool redrawDisplay = true;

u64 lastReport = 0;
u64 alertSince = 0;

u32 pingNo = 0;

std::set<u32> alertingIds;
std::map<u32, u8> clientIds;
std::map<u32, u64> lastPongs;

void redraw() {
    redrawDisplay = true;
}

bool shouldAlert() { return switchState >= SwLasersOn && !alertingIds.empty(); }

void switchRelays() {
    if (switchState >= SwLasersOn) {
        digitalWrite(PIN_RELAY_LASERS, LOW);
    } else {
        digitalWrite(PIN_RELAY_LASERS, HIGH);
    }

    if (sirenOn) {
        Serial.println("SIREN!!!");
        digitalWrite(PIN_RELAY_SIREN, LOW);
    } else {
        digitalWrite(PIN_RELAY_SIREN, HIGH);
    }
}

void printState(const u64 now) {
    lastReport = now;

    const String sirenSettingName = switchState == SwSirenEnabled ? "ON" : "OFF";
    const String lasersSettingName = switchState >= SwLasersOn ? "ON" : "OFF";

    if (!alertingIds.empty()) {
        auto it = alertingIds.begin();

        Serial.print("Alerting IDs: ");
        TelnetPrint.print("Alerting IDs: ");
        while (it != alertingIds.end()) {
            Serial.printf("%d, ", clientIds[*it]);
            TelnetPrint.printf("%d, ", clientIds[*it]);
            it++;
        }
        Serial.println();
        TelnetPrint.println();
    }

    TelnetPrint.printf("Sensors: %d Siren en.: %s Lasers en.: %s Should alert: %s\r\n",
                       wsSensors.getClients().length(),
                       sirenSettingName.c_str(),
                       lasersSettingName.c_str(),
                       shouldAlert() ? "YES" : "NO");

    Serial.printf("Connected sensors: %d Siren enabled: %s Lasers enabled: %s Should alert: %s\n",
                  wsSensors.getClients().length(),
                  sirenSettingName.c_str(),
                  lasersSettingName.c_str(),
                  shouldAlert() ? "YES" : "NO");
}

void displayState() {
    if (!redrawDisplay) return;
    redrawDisplay = false;

    if (!started) {
        tft.fillScreen(TFT_GREY);
        tft.fillRoundRect(10, 10, TFT_HEIGHT - 20, TFT_WIDTH - 20, 10, TFT_GREEN);
        tft.setTextSize(1);
        tft.setTextColor(TFT_BLACK);
        tft.drawCentreString("Starting...", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 10, 4);
        return;
    }

    if (sirenOn) {
        tft.fillScreen(TFT_RED);
        return;
    }

    const bool possibleAlert = shouldAlert();

    uint bgColor = switchState >= SwLasersOn ? (possibleAlert ? TFT_ORANGE : TFT_GREEN) : TFT_GREY;
    uint textColor = switchState >= SwLasersOn ? TFT_BLACK : TFT_WHITE;

    const String sirenSettingText = switchState == SwSirenEnabled ? "Siren: ENABLED" : "Siren: DISABLED";
    const String lasersSettingText = switchState >= SwLasersOn ? "Lasers: ON" : "Lasers: OFF";

    tft.fillScreen(bgColor);
    tft.setTextColor(textColor);
    tft.setTextSize(1);
    tft.setTextFont(2);

    tft.drawString(sirenSettingText, 2, 0, 2);
    tft.drawRightString(lasersSettingText, TFT_HEIGHT - 2, 0, 2);

    tft.setCursor(2, TFT_WIDTH - 16, 2);
    const u8 clientsCount = wsSensors.getClients().length();
    tft.printf("Clients: %d", clientsCount);

    if (switchState == SwSirenEnabled) {
        tft.setTextColor(TFT_ORANGE);
        tft.setTextSize(4);
        tft.drawCentreString(clientsCount > 0 ? "!!!" : "!", TFT_HEIGHT / 2, TFT_WIDTH / 2 - 40, 4);
    }
}

void clearClients() {
    const uint oldClientsCount = wsSensors.getClients().length();

    wsSensors.cleanupClients();

    const u64 now = millis();

    for (auto client: wsSensors.getClients()) {
        const u64 lastPong = lastPongs[client->id()];

        if (durationBetween(now, lastPong) > CLIENT_PONG_TIMEOUT) {
            Serial.printf("Kicking unresponsive client ID %d (client %d)!\n", clientIds[client->id()], client->id());
            TelnetPrint.printf("Kicking unresponsive client ID %d!\r\n", clientIds[client->id()]);
            client->client()->close(true);
            lastPongs.erase(client->id());
        }
    }

    if (wsSensors.getClients().length() != oldClientsCount) { redraw(); }
}

void onTextMessage(AsyncWebSocketClient *client, u8 *data, const size_t len) {
    StaticJsonDocument<60> requestJson;
    deserializeJson(requestJson, data, len);

    const String type = requestJson["type"];

    if (type == "alert") {
        const String status = requestJson["status"];
        const u8 clientId = requestJson["clientId"];

        Serial.printf("Received alert info from sensor ID %d (client %d): %s\n", clientId, client->id(), status.c_str());
        TelnetPrint.printf("Alert info from sensor ID %d: %s\r\n", clientId, status.c_str());

        const bool oldAlerting = !alertingIds.empty();

        clientIds.insert(std::pair<u32, u8>(client->id(), clientId));

        if (status == "alert") {
            alertingIds.insert(client->id());
        } else if (status == "alert-ok") {
            alertingIds.erase(client->id());
        }

        if (!alertingIds.empty() != oldAlerting) {
            redraw();
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
        TelnetPrint.printf("Sensor client connection received\r\n");
        lastPongs.insert(std::pair<u32, u32>(client->id(), now));
        redraw();
        const String &string = String(pingNo);
        client->ping((u8 *) string.c_str(), string.length());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Sensor client %d disconnected\n", client->id());
        TelnetPrint.printf("Sensor ID %d disconnected\r\n", clientIds[client->id()]);
        clientIds.erase(client->id());
        alertingIds.erase(client->id());
        lastPongs.erase(client->id());
        redraw();
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

    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, 1, 0, 10);
    Tasker::sleep(100);
    WiFi.softAPConfig(apIP, apIP, netMsk);

    Tasker::sleep(500);
    Serial.print(F("AP IP address: "));
    Serial.println(WiFi.softAPIP());
}

void setup() {
    Serial.begin(115200);

    tft.init();
    tft.setRotation(1);

    displayState();

    pinMode(PIN_SWITCH_LASERS, INPUT_PULLDOWN);
    pinMode(PIN_SWITCH_SIREN, INPUT_PULLDOWN);
    pinMode(PIN_RELAY_LASERS, OUTPUT);
    pinMode(PIN_RELAY_SIREN, OUTPUT);
    digitalWrite(PIN_RELAY_LASERS, HIGH);
    digitalWrite(PIN_RELAY_SIREN, HIGH);

    createAP();

    Serial.println("Starting telnet server...");
    TelnetPrint.begin();

    DefaultTasker.loopEvery("DisplayState", 50, displayState);
    DefaultTasker.loopEvery("SwitchRelays", 50, switchRelays);

    DefaultTasker.once("NetworkSetup", [] {
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

    DefaultTasker.loopEvery("ReadSwitches", 50, [] {
        const u64 now = millis();

        const bool oldSirenState = sirenOn;
        const SwitchState oldSwitchState = switchState;

        if (digitalRead(PIN_SWITCH_LASERS) == HIGH) {
            switchState = SwOff;
        } else if (digitalRead(PIN_SWITCH_LASERS) == LOW && digitalRead(PIN_SWITCH_SIREN) == LOW) {
            switchState = SwLasersOn;
        } else if (digitalRead(PIN_SWITCH_SIREN) == HIGH) {
            switchState = SwSirenEnabled;
        } else {
            Serial.println("Weird switch state!!!");
        }

        // siren is ON but disabled...
        if (oldSirenState && switchState <= SwLasersOn) {
            // to stop immediately after switch change!
            sirenOn = false;
            alertSince = 0;
        } else {
            if (shouldAlert()) {
                if (switchState == SwSirenEnabled) {
                    sirenOn = true;
                    if (sirenOn != oldSirenState) alertSince = now;
                }
            } else {
                // `now > HOLD_ALERT_FOR` == enough long after start
                if (now > HOLD_ALERT_FOR && durationBetween(now, alertSince) >= HOLD_ALERT_FOR) {
                    sirenOn = false;
                    alertSince = 0;
                }
            }
        }

        if (switchState != oldSwitchState || sirenOn != oldSirenState) {
            redraw();
        }

        if ((sirenOn && (durationBetween(now, lastReport) > 500)) || (durationBetween(now, lastReport) > 2000)) {
            printState(now);
        }
    });

    started = true;
    redraw();
}

void loop() {
    // no-op, handling by Tasker

    // TODO self-check, restart if in weird state
}

#pragma clang diagnostic pop
