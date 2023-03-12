#include "Config.h"
#include <EEPROM.h>

void Config::load(EEPROMConfig& settings) {
  EEPROM.get(0, settings);
  Serial.print("EEPROM VERSION: ");
  Serial.println(settings.version);
  if (settings.version != EEPROM_VERSION)
  {
    Serial.println("Loading Defaults");
    loadDefaults(settings);
  }
}

void Config::save(EEPROMConfig& settings) {
  Serial.print("SAVING EEPROM VERSION: ");
  Serial.println(settings.version);
  EEPROM.put(0, settings);
  EEPROM.commit();
}

void Config::loadDefaults(EEPROMConfig& settings) {
    settings.version = EEPROM_VERSION;
    settings.idOffset = 32;
    settings.isOrangeTopGTE = true;
}

void Config::fromJson(EEPROMConfig& settings, JsonObject &doc) {

    settings.version = doc["version"];
    settings.idOffset = doc["idOffset"];
    settings.isOrangeTopGTE = doc["isOrangeTopGTE"];

}

void Config::toJson(EEPROMConfig& settings, DynamicJsonDocument &root) {
    root["version"] = settings.version;
    root["idOffset"] = settings.idOffset;
    root["isOrangeTopGTE"] = settings.isOrangeTopGTE;

}
