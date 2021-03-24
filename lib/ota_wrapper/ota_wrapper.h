#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

#include <ESP8266WiFi.h>
#include <CertStoreBearSSL.h>
#include <ESP8266httpUpdate.h>

#include <config.h>

class OtaWrapper
{
public:
    OtaWrapper(BearSSL::WiFiClientSecure *client, const char *ota_server, const char *ota_user, const char *ota_pass,
               const char *base_name, const char *version,
               uint32_t check_interval = 1 * 60 * 1000);
    void loop();

private:
    BearSSL::WiFiClientSecure *m_client;

    const char *m_ota_server;
    const char *m_ota_user;
    const char *m_ota_pass;
    const char *m_base_name;
    const char *m_version;
    uint32_t m_check_interval;
    uint32_t m_last_check;
};