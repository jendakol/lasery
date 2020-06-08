#include <Arduino.h>

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#include "../../common/Constants.h"

#include <ESPAsyncWebServer.h> // this is universal over ESP32/ESP8266
#include <DNSServer.h>
#include <FS.h>

#include <ArduinoJson.h>

#define STATIC_FILES_PREFIX "/w"

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

const byte DNS_PORT = 53;
DNSServer dnsServer;

AsyncWebSocketClient *wsClient = nullptr;

static FS FileSystem = SPIFFS;

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println(F("Websocket client connection received"));
        wsClient = client;
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println(F("Websocket client disconnected"));
        wsClient = nullptr;
    } else if (type == WS_EVT_DATA) {
        Serial.print(F("Websocket data received: "));

        for (uint i = 0; i < len; i++) {
            Serial.print((char) data[i]);
        }

        // TODO
        client->text("{'message': 'ahoj'}");

        Serial.println();
    }
}

String getContentType(const String &filename) {
    if (filename.endsWith(F(".htm"))) return F("text/html");
    else if (filename.endsWith(F(".html"))) return F("text/html");
    else if (filename.endsWith(F(".css"))) return F("text/css");
    else if (filename.endsWith(F(".js"))) return F("application/javascript");
    else if (filename.endsWith(F(".png"))) return F("image/png");
    else if (filename.endsWith(F(".gif"))) return F("image/gif");
    else if (filename.endsWith(F(".jpg"))) return F("image/jpeg");
    else if (filename.endsWith(F(".ico"))) return F("image/x-icon");
    else if (filename.endsWith(F(".xml"))) return F("text/xml");
    else if (filename.endsWith(F(".pdf"))) return F("application/x-pdf");
    else if (filename.endsWith(F(".zip"))) return F("application/x-zip");
    else if (filename.endsWith(F(".gz"))) return F("application/x-gzip");

    return F("text/plain");
}

bool handleStaticFile(AsyncWebServerRequest *request) {
    String path = STATIC_FILES_PREFIX + request->url();

    if (path.endsWith("/")) path += F("index.html");

    String contentType = getContentType(path);
    String pathWithGz = path + ".gz";

    if (FileSystem.exists(pathWithGz) || FileSystem.exists(path)) {
        bool gzipped = false;

        if (FileSystem.exists(pathWithGz)) {
            gzipped = true;
            path += ".gz";
        }

        File file = FileSystem.open(path, "r");

        Serial.println("Serving static file, path=" + path + " size=" + file.size() + " content-type=" + contentType);

        AsyncWebServerResponse *response = request->beginResponse(
                contentType,
                file.size(),
                [file](uint8_t *buffer, size_t maxLen, size_t total) mutable -> size_t {
                    int bytes = file.read(buffer, maxLen);

                    // close file at the end
                    if (bytes + total == file.size()) file.close();

                    return max(0, bytes); // return 0 even when no bytes were loaded
                }
        );

        if (gzipped) {
            response->addHeader(F("Content-Encoding"), F("gzip"));
        }

        request->send(response);

        return true;
    }

    return false;
}

void installCaptivePortalRedirects() {
    webServer.addRewrite(new AsyncWebRewrite("/generate_204", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/gen_204", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/fwlink", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/connecttest.txt", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/hotspot-detect.html", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/library/test/success.html", "/index.html"));
    webServer.addRewrite(new AsyncWebRewrite("/kindle-wifi/wifistub.html", "/index.html"));
}

void createAP() {
    IPAddress apIP(8, 8, 8, 8);
    IPAddress netMsk(255, 255, 255, 0);

    WiFi.disconnect(true);

    Serial.print(F("Configuring access point…"));
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);

    delay(500);
    Serial.print(F("AP IP address: "));
    Serial.println(WiFi.softAPIP());

    installCaptivePortalRedirects();
}

void setup() {
    Serial.begin(9600);
    FileSystem.begin();

    createAP();

    webServer.onNotFound([](AsyncWebServerRequest *request) {
        if (handleStaticFile(request)) return;

        request->send(404);
    });

    webServer.addHandler(&ws);
    ws.onEvent(onWsEvent);

    webServer.begin();
    Serial.println(F("HTTP server started"));
}

void loop() {};
