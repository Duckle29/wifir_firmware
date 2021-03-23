#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

#include "config.h"
#include "error_types.h"

#define USE_LittleFS
#ifdef USE_LittleFS
#include <FS.h>
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#define DRD_TIMEOUT 2
#define DRD_ADDRESS 0

#ifdef ESP8266
#define ESP8266_DRD_USE_RTC false //true
#define ESP_DRD_USE_LITTLEFS true //false
#endif

#define ESP_DRD_USE_EEPROM false
#define ESP_DRD_USE_SPIFFS false
#define DOUBLERESETDETECTOR_DEBUG true
#include <ESP_DoubleResetDetector.h>

class WifiWrapper
{
public:
    WifiWrapper();
    bool begin(const char *hostname, uint8_t led_pin = LED_BUILTIN);
    void loop();

private:
    AsyncWebServer *m_webServer;
    DNSServer *m_dnsServer;
    DoubleResetDetector *m_drd;
};