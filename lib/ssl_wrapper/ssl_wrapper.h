#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

// SSL
#include <TZ.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <CertStoreBearSSL.h>
#include <FS.h>
#include <LittleFS.h>

#include "config.h"

class SSLWrapper
{
public:
    SSLWrapper();
    int begin(const char *);
    BearSSL::WiFiClientSecure *get_client();

private:
    BearSSL::CertStore *m_certStore;
    BearSSL::WiFiClientSecure *m_client;
    void m_set_clock(const char *);
};