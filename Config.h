#ifndef CONFIG_H
#define CONFIG_H

#include <ArduinoJson.h>
#define EEPROM_VERSION 1

typedef struct {
  uint8_t version;
  uint8_t idOffset;
  bool isOrangeTopGTE;
} EEPROMConfig;

class Config
{
  public:
    void load(EEPROMConfig& settings);
    void save(EEPROMConfig& settings);
    void toJson(EEPROMConfig& settings, DynamicJsonDocument &root);
    void fromJson(EEPROMConfig& settings, JsonObject &doc);
    void loadDefaults(EEPROMConfig& settings);
};

#endif
