#include "BridgeWebServer.h"
#include <SPIFFS.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

BridgeWebServer::BridgeWebServer(EEPROMConfig& s) : settings{ s } {
}


void BridgeWebServer::execute() {
    ws.cleanupClients();
}


void BridgeWebServer::setup(Config &config)
{
    // ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/wifi", [&] (AsyncWebServerRequest *request) {
        bool updated = true;
        if(request->hasParam("apSSID", true) && request->hasParam("apPW", true)) 
        {
            WiFi.softAP(request->arg("apSSID").c_str(), request->arg("apPW").c_str());
        }
        else if(request->hasParam("staSSID", true) && request->hasParam("staPW", true)) 
        {
            WiFi.mode(WIFI_AP_STA);
            WiFi.begin(request->arg("staSSID").c_str(), request->arg("staPW").c_str());
        }
        else
        {
            File file = SPIFFS.open("/wifi.html", "r");
            String html = file.readString();
            file.close();
            html.replace("%staSSID%", WiFi.SSID());
            html.replace("%apSSID%", WiFi.softAPSSID());
            html.replace("%staIP%", WiFi.localIP().toString());
            request->send(200, "text/html", html);
            updated = false;
        }

        if (updated)
        {
            request->send(SPIFFS, "/wifi-updated.html");
        } 
    });

  
    server.on("/config", HTTP_GET, [&] (AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument json(2048);

        config.toJson(settings, json);
        serializeJson(json, *response);
        request->send(response);
    });

    server.on(
        "/config",
        HTTP_POST,
        [](AsyncWebServerRequest * request){},
        NULL,
        [&](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {
        Serial.println("Config POST");
        const size_t JSON_DOC_SIZE = 1024U;
        DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
        
        if (DeserializationError::Ok == deserializeJson(jsonDoc, (const char*)data))
        {
            JsonObject obj = jsonDoc.as<JsonObject>();
            config.fromJson(settings, obj);
            config.save(settings);
            request->send(200, "application/json", "success");

        } else {
            request->send(200, "application/json", "DeserializationError");
        }
    });

    server.on("/edit", 
        HTTP_POST, 
        [](AsyncWebServerRequest * request){},
        [&](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                // open the file on first call and store the file handle in the request object
                request->_tempFile = SPIFFS.open("/" + filename, "w");

            }

            if (len) {
                // stream the incoming chunk to the opened file
                request->_tempFile.write(data, len);
            }

            if (final) {
                // close the file handle as the upload is now done
                request->_tempFile.close();

                if (filename.substring(filename.lastIndexOf(".")).equals("bin")) {
                    Serial.println("Firmware uploaded, restarting");
                    request->send(200, "application/json", "restarting");
                    ESP.restart();   
                }
                request->redirect("/");
            }
        }
        
    );

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  

  // Start server
  Serial.println("Starting Web Server");

  server.begin();
}
