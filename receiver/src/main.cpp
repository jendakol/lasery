#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <Arduino.h>

#include "../../common/Constants.h"

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <set>

#include <Tasker.h>

#define DNS_PORT 53
#define STATIC_FILES_PREFIX "/w"

AsyncWebServer webServer(80);
AsyncWebSocket wsSensors(SENSOR_WS_PATH);
AsyncWebSocket wsFrontend(FRONTEND_WS_PATH);
DNSServer dnsServer;

#define FileSystem SPIFFS

#define STATE_UNARMED 0
#define STATE_ARMED 1
#define STATE_ALERTING 2

#define CLIENT_PONG_TIMEOUT (PING_EVERY + 500)

// TODO change back to STATE_UNARMED;
int appState = STATE_ARMED;
unsigned long lastReport = 0;

unsigned long pingNo = 0;

std::set<uint32_t> alerting_ids;
std::map<uint32_t, uint32_t> lastPongs;

void reportStateToFrontend();

void updateStatus() {
    if (appState != STATE_UNARMED) appState = alerting_ids.empty() ? STATE_ARMED : STATE_ALERTING;
}

void clearClients() {
    wsSensors.cleanupClients();

    const unsigned long now = millis();

    for (auto client: wsSensors.getClients()) {
        const uint32_t lastPong = lastPongs[client->id()];

        if (duration(now, lastPong) > CLIENT_PONG_TIMEOUT) {
            Serial.printf("Kicking unresponsive client %d\n", client->id());
            client->client()->close(true);
            lastPongs.erase(client->id());
        }
    }
}

void onTextMessage(AsyncWebSocketClient *client, uint8_t *data, const size_t len) {
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

        updateStatus();

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
                     AwsEventType type, __unused void *arg, uint8_t *data, const size_t len) {
    const unsigned long now = millis();

    if (type == WS_EVT_CONNECT) {
        client->keepAlivePeriod(30000);
        Serial.printf("Sensor client %d connection received\n", client->id());
        lastPongs.insert(std::pair<uint32_t, uint32_t>(client->id(), now));
        const String &string = String(pingNo);
        client->ping((uint8_t *) string.c_str(), string.length());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("Sensor client %d disconnected\n", client->id());
        alerting_ids.erase(client->id());
        lastPongs.erase(client->id());
    } else if (type == WS_EVT_PONG) {
        lastPongs[client->id()] = now;
    } else if (type == WS_EVT_DATA) {
        lastPongs[client->id()] = now;
        onTextMessage(client, data, len);
        reportStateToFrontend();
    }
}

void onFrontendWsEvent(__unused AsyncWebSocket *s, __unused AsyncWebSocketClient *client,
                       AwsEventType type, __unused void *arg, uint8_t *data, const size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println(F("Frontend client connection received"));
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println(F("Frontend client disconnected"));
    } else if (type == WS_EVT_DATA) {
        Serial.print(F("Frontend data received: "));

        for (uint i = 0; i < len; i++) {
            Serial.print((char) data[i]);
        }
        Serial.println();

        StaticJsonDocument<100> requestJson;
        deserializeJson(requestJson, data, len);

        if (requestJson["op"] == "init") {
            reportStateToFrontend();
            return;
        }

        if (requestJson["op"] == "arm") {
            if (appState == STATE_UNARMED)appState = STATE_ARMED;
            reportStateToFrontend();

            return;
        }

        if (requestJson["op"] == "unarm") {
            if (appState == STATE_ARMED || appState == STATE_ALERTING)appState = STATE_UNARMED;
            reportStateToFrontend();

            return;
        }

        if (requestJson["op"] == "alert") {
            appState = STATE_ALERTING;
            Serial.println(F("Manually alerting!"));
            reportStateToFrontend();

            return;
        }
    }
}

void reportStateToFrontend() {
    StaticJsonDocument<50> responseJson;

    responseJson["type"] = "state";
    responseJson["state"] = appState;
    responseJson["clients"] = wsSensors.getClients().length();

    char output[50];
    size_t l = serializeJson(responseJson, output);

    wsFrontend.textAll(output, l);
}

String getContentType(const String &filename) {
    if (filename.endsWith(F(".htm"))) return F("text/html");
    if (filename.endsWith(F(".html"))) return F("text/html");
    if (filename.endsWith(F(".css"))) return F("text/css");
    if (filename.endsWith(F(".js"))) return F("application/javascript");
    if (filename.endsWith(F(".png"))) return F("image/png");
    if (filename.endsWith(F(".gif"))) return F("image/gif");
    if (filename.endsWith(F(".jpg"))) return F("image/jpeg");
    if (filename.endsWith(F(".ico"))) return F("image/x-icon");
    if (filename.endsWith(F(".xml"))) return F("text/xml");
    if (filename.endsWith(F(".pdf"))) return F("application/x-pdf");
    if (filename.endsWith(F(".zip"))) return F("application/x-zip");
    if (filename.endsWith(F(".gz"))) return F("application/x-gzip");

    return F("text/plain");
}

bool handleStaticFile(AsyncWebServerRequest *request) {
    String path = STATIC_FILES_PREFIX + request->url();

    if (path.endsWith("/")) path += F("index.html");

    const String contentType = getContentType(path);
    String pathWithGz = path + ".gz";

    if (FileSystem.exists(pathWithGz) || FileSystem.exists(path)) {
        bool gzipped = false;

        if (FileSystem.exists(pathWithGz)) {
            gzipped = true;
            path += ".gz";
        }

        File file = FileSystem.open(path, "r");

        Serial.println("Serving static file, path=" + path + " size=" + file.size() + " content-type=" + contentType);

        Tasker::sleep(5);

        AsyncWebServerResponse *response = request->beginResponse(
                contentType,
                file.size(),
                [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t {
                    const int bytes = file.read(buffer, max(maxLen, (size_t) 512));

                    Tasker::sleep(1);

                    // close file at the end
                    if (bytes + total == file.size()) file.close();

                    return max(0, bytes); // return 0 even when no bytes were loaded
                }
        );

        if (gzipped) {
            response->addHeader(F("Content-Encoding"), F("gzip"));
        }

        Tasker::sleep(5);

        request->send(response);

        return true;
    }

    return false;
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

void setup() {
    Serial.begin(115200);
    FileSystem.begin();

    createAP();

    DefaultTasker.once("network-setup", [] {

        webServer.onNotFound([](AsyncWebServerRequest *request) {
            if (handleStaticFile(request)) return;
            request->send(404);
        });

        webServer.addHandler(&wsSensors);
        webServer.addHandler(&wsFrontend);
        wsSensors.onEvent(onSensorWsEvent);
        wsFrontend.onEvent(onFrontendWsEvent);

        webServer.begin();
        Serial.println(F("HTTP server started"));
    });

    DefaultTasker.loopEvery("SendPing", PING_EVERY, [] {
        clearClients();

        const String &string = String(++pingNo);
        const char *data = string.c_str();
        wsSensors.pingAll((uint8_t *) data, string.length());
    });

    DefaultTasker.loopEvery("PrintStatus", 10, [] {
        updateStatus();

        const unsigned long now = millis();

        if (appState == STATE_ALERTING && (duration(now, lastReport) > 500)) {
            lastReport = now;
            Serial.print(F("Connected clients: "));
            Serial.print(wsSensors.getClients().length());
            Serial.println(F(" State: ALERTING"));
        } else if (duration(now, lastReport) > 2000) {
            lastReport = now;
            Serial.print(F("Connected clients: "));
            Serial.print(wsSensors.getClients().length());
            Serial.print(F(" State: "));

            switch (appState) {
                case STATE_UNARMED:
                    Serial.println("UNARMED");
                    break;
                case STATE_ARMED:
                    Serial.println("ARMED");
                    break;
            }
        }
    });
}

void loop() {
    // no-op, handling by Tasker

    // TODO self-check, restart if in weird state
}

#pragma clang diagnostic pop
