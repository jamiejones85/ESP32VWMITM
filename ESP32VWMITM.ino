//——————————————————————————————————————————————————————————————————————————————
//  ACAN2515 Demo in loopback mode, for ESP32
//——————————————————————————————————————————————————————————————————————————————

#ifndef ARDUINO_ARCH_ESP32
  #error "Select an ESP32 board" 
#endif

//——————————————————————————————————————————————————————————————————————————————
#include <ACAN_ESP32.h>
#include <ACAN2515.h>
#include <EEPROM.h>
#include "Config.h"
#include "BridgeWebServer.h"
#include <WiFi.h>
#include <SPIFFS.h>

#define LED_BUILTIN 2
#define EEPROM_SIZE 512

static const byte MCP2515_SCK  = 26 ; // SCK input of MCP2517 
static const byte MCP2515_MOSI = 19 ; // SDI input of MCP2517  
static const byte MCP2515_MISO = 18 ; // SDO output of MCP2517 
static const byte MCP2515_CS  = 21 ; // CS input of MCP2515 (adapt to your design) 
static const byte MCP2515_INT =  23 ; // INT output of MCP2515 (adapt to your design)
static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 8 MHz

ACAN2515 bmsCan (MCP2515_CS, SPI, MCP2515_INT) ;
EEPROMConfig eepromConfig;
Config config;
BridgeWebServer bridgeWebServer(eepromConfig);


//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

static void receivedFiltered (const CANMessage & inMessage) {
 if (inMessage.id == 0x0BA) {
    int statuss = ACAN_ESP32::can.tryToSend(inMessage);
  }
  //if modified balance commads
  if (inMessage.id >= 0x1A55542A && inMessage.id <= 0x1A555439) {
    Serial.print("Balance command ");
    Serial.print(inMessage.id, HEX);
    Serial.print(" New ID: ");

    CANMessage frame ;

    if (eepromConfig.isOrangeTopGTE && inMessage.id == 0x1A55542E) {
      frame.id = 0x1A55541E;//funkyness https://github.com/Tom-evnut/VW-bms/pull/2
    } else if (eepromConfig.isOrangeTopGTE && inMessage.id == 0x1A55542F) {
      frame.id = 0x1A55541F;//funkyness https://github.com/Tom-evnut/VW-bms/pull/2
    } else {
      frame.id = inMessage.id - eepromConfig.idOffset;
    }

    frame.ext = inMessage.ext;
    frame.len = inMessage.len;
    frame.data[0] = inMessage.data[0];
    frame.data[1] = inMessage.data[1]; 
    frame.data[2] = inMessage.data[2]; 
    frame.data[3] = inMessage.data[3]; 
    frame.data[4] = inMessage.data[4]; 
    frame.data[5] = inMessage.data[5]; 
    frame.data[6] = inMessage.data[6]; 
    frame.data[7] = inMessage.data[7]; 
    ACAN_ESP32::can.tryToSend(frame);
  }
}

void setup () {
//--- Switch on builtin led
  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
//--- Start serial
  Serial.begin (115200) ;

  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  

  //config on board can
  Serial.println("Initializing CAN...");
  ACAN_ESP32_Settings settings2(500 * 1000);
  settings2.mRxPin = GPIO_NUM_4;
  settings2.mTxPin = GPIO_NUM_5;
  const uint32_t errorCode2 = ACAN_ESP32::can.begin (settings2) ;
  if (errorCode2 == 0) {
    Serial.println ("Configuration ESP32 OK!");
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode2, HEX) ;
  }

  EEPROM.begin(EEPROM_SIZE);
  config.load(eepromConfig);

  SPI.begin (MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI) ;
  Serial.println ("Configure ACAN2515") ;
  
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 500UL * 1000UL) ; 
  settings.mRequestedMode = ACAN2515Settings::NormalMode ;
  const ACAN2515Mask rxm0 = standard2515Mask(0x7FF, 0, 0) ; // For filter #0 and #1
  const ACAN2515Mask rxm1 = extended2515Mask(0x1A555400) ; // For filter #2 to #
  const ACAN2515AcceptanceFilter filters [] = {
    {standard2515Filter(0x0BA, 0, 0), receivedFiltered},
    {standard2515Filter(0x0BA, 0, 0), receivedFiltered},
    {extended2515Filter(0x1A555420), receivedFiltered},
    {extended2515Filter(0x1A555430), receivedFiltered}
  };
  const uint16_t errorCode = bmsCan.begin (settings, [] { bmsCan.isr () ; }, rxm0, rxm1, filters, 4) ;
//    const uint16_t errorCode = bmsCan.begin (settings, [] { bmsCan.isr () ; }) ;

  if (errorCode == 0) {
    Serial.print ("Bit Rate prescaler: ") ;
    Serial.println (settings.mBitRatePrescaler) ;
    Serial.print ("Propagation Segment: ") ;
    Serial.println (settings.mPropagationSegment) ;
    Serial.print ("Phase segment 1: ") ;
    Serial.println (settings.mPhaseSegment1) ;
    Serial.print ("Phase segment 2: ") ;
    Serial.println (settings.mPhaseSegment2) ;
    Serial.print ("SJW: ") ;
    Serial.println (settings.mSJW) ;
    Serial.print ("Triple Sampling: ") ;
    Serial.println (settings.mTripleSampling ? "yes" : "no") ;
    Serial.print ("Actual bit rate: ") ;
    Serial.print (settings.actualBitRate ()) ;
    Serial.println (" bit/s") ;
    Serial.print ("Exact bit rate ? ") ;
    Serial.println (settings.exactBitRate () ? "yes" : "no") ;
    Serial.print ("Sample point: ") ;
    Serial.print (settings.samplePointFromBitStart ()) ;
    Serial.println ("%") ;
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }


  WiFi.mode(WIFI_AP);
  // Connect to Wi-Fi
  WiFi.begin();

  bridgeWebServer.setup(config);

}

//----------------------------------------------------------------------------------------------------------------------



//——————————————————————————————————————————————————————————————————————————————
// Onboard CAN is Battery Side, SPI can BMS
// ---------- Messages from BMS SIDE --------------
// 0x0BA from BMS, repeat to Battery.
// -------- Messages from the Battery side ------------
// Voltage IDS
// 0x1B0 to 0x1CF add 32 and repeat to BMS
// Temperature IDS (not confirmed as a good idea)
// 0x1A555401 to 1A555408 add 32 and send on.
// Balancing requests

bool isBatteryId(uint32_t id) {
  //ID's
  if (id >= 0x1B0 && id <= 0x1CF) {
    return true;
  }
  //Temperatures & balance feedback
  if (id >= 0x1A555401 && id <= 0x1A555408) {
    return true;
  }

  return false;
}

void loop () {
  CANMessage frame ;

  //messages from the battery
  if (ACAN_ESP32::can.receive (frame)) {
      if (isBatteryId(frame.id)) {
        frame.id = frame.id + eepromConfig.idOffset;
        const bool ok = bmsCan.tryToSend(frame);
        if (!ok) {
          Serial.println("Failed to send message to BMS");  
        }
      }
  }

//  if (bmsCan.receive(frame)) {
//    if (frame.id == 0x0BA) {
//      int statuss = ACAN_ESP32::can.tryToSend(frame);
//    }
//  }

  bmsCan.dispatchReceivedMessage() ;
  bridgeWebServer.execute();

}

//——————————————————————————————————————————————————————————————————————————————
