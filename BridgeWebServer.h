#ifndef BRIDGE_WEB_SERVER_H
#define BRIDGE_WEB_SERVER_H
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "Config.h"

class BridgeWebServer
{
  public:
    BridgeWebServer(EEPROMConfig& settings);
    void setup(Config &config);
    void execute();
  private:
    EEPROMConfig& settings;
};
#endif
