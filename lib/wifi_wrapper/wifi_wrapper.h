#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

#include "config.h"
#include "error_types.h"
#include <LittleFS.h>
#include <mrd.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

class WifiWrapper
{
  public:
    WifiWrapper(uint8_t led_pin = -1, bool led_inverted = false);
    api_error_t  begin(const char *hostname, bool reset = false);

  private:
    const uint8_t m_led_pin;
    bool m_led_inv;

    AsyncWebServer *m_webServer;
    DNSServer *m_dnsServer;
    api_error_t  m_reset_wifi(AsyncWiFiManager *wm);
};