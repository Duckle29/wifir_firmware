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
    bool loop(bool check_now = false);

private:
    BearSSL::WiFiClientSecure *m_client;

    const char *m_ota_server;
    const char *m_ota_user;
    const char *m_ota_pass;
    const char *m_base_name;
    const char *m_version;
    unsigned long m_check_interval;
    unsigned long m_last_check;
};