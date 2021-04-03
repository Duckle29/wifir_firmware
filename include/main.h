#pragma once

#include "config.h"
#include "secrets.h"
#include <Arduino.h>
String _hostname;
String get_hostname(const char *);

// --- Debug printing ---
#include "error_types.h"
#include "rate_limiter.h"
#include <ArduinoLog.h>
RateLimiter limiter_debug(debug_interval);

// --- WiFi ---
#include "wifi_wrapper.h"
#include <ESP8266WiFi.h>
WifiWrapper wm(led_pin, mrd_timeout, mrd_resets);

// --- SSL ---
#include "ssl_wrapper.h"
SSLWrapper ssl_wrap;
BearSSL::WiFiClientSecure *client;

// --- Sensors ---
#include "sensor_wrapper.h"
Sensors Sens;

// --- IR ---
#include "ir_wrapper.h"

Ir ir(ir_pins[0], ir_pins[1], LED_BUILTIN, true);

// --- MQTT ---
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

RateLimiter limiter_mqtt(mqtt_interval);
RateLimiter limiter_mqtt_ping(mqtt_keepalive - 60 * 1000);

// Client
void MQTT_connect();
const char *get_feed_url(String feed_name);
Adafruit_MQTT_Client *mqtt;

// Feeds
#include <mqtt_structures.h>
struct Feed feeds[] = {{"temp", PUBLISH, MQTT_QOS_0, FLOAT, &Sens.t},
                       {"humi", PUBLISH, MQTT_QOS_0, FLOAT, &Sens.rh},
                       {"eco2", PUBLISH, MQTT_QOS_0, UINT16, &Sens.eco2},
                       {"tvoc", PUBLISH, MQTT_QOS_0, UINT16, &Sens.tvoc},
                       {"state-tx", PUBLISH, MQTT_QOS_1, RXRES, &ir.rx_results},
                       {"state-rx", SUBSCRIBE, MQTT_QOS_1, CB, nullptr, &state_rx_cb},
                       {"config-rx", SUBSCRIBE, MQTT_QOS_1, CB, nullptr, &config_rx_cb}};

// --- OTA ---
#include "ota_wrapper.h"
OtaWrapper *ota;
