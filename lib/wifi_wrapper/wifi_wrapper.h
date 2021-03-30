#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

#include "config.h"
#include "error_types.h"

#include <LittleFS.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

class WifiWrapper
{
public:
    WifiWrapper(uint8_t led_pin = LED_BUILTIN, uint8_t timeout = 10, uint8_t resets = 5);
    error_t begin(const char *hostname);
    void loop();

private:
    const uint8_t m_led_pin;
    const uint8_t m_timeout;
    const uint8_t m_resets;

    bool m_counter_cleared = false;

    AsyncWebServer *m_webServer;
    DNSServer *m_dnsServer;
    void m_blink(uint_fast8_t led_pin, uint_fast8_t times, uint_fast16_t blink_delay = 500);
    int m_check_resets();
    error_t m_reset_wifi(AsyncWiFiManager *wm);
};