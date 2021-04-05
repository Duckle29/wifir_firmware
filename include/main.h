#pragma once

#include "config.h"
#include "secrets.h"
#include <Arduino.h>
String _hostname;
String get_hostname(const char *);
void blink(uint_fast8_t led_pin, uint_fast8_t times, uint_fast16_t blink_delay);

// --- Debug printing ---
#include "error_types.h"
#include "rate_limiter.h"
#include <ArduinoLog.h>
char mqtt_log_buff[500];
char *p_mqtt_log_buff = mqtt_log_buff;

RateLimiter limiter_debug(debug_interval);
int segment_data(void *dest, const void *src, uint_fast32_t segment_length, uint_fast32_t src_length = 0,
                 const char *preffered_split_characters = "\n \0");

// --- Reset detection ---
#include <LittleFS.h>
#include <mrd.h>
Mrd mrd(&LittleFS, "reset_counter", mrd_timeout);

// --- WiFi ---
#include "wifi_wrapper.h"
#include <ESP8266WiFi.h>
WifiWrapper wm(led_pin, true);

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
const uint_fast16_t mqtt_max_len = 90;
const uint_fast8_t mqtt_max_retries = 3;

RateLimiter limiter_mqtt(mqtt_interval);
RateLimiter limiter_mqtt_ping(mqtt_keepalive - 60 * 1000);
error_t mqtt_handle();

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
                       {"ir-state", PUBLISH, MQTT_QOS_1, IRRX, ir.state_str},
                       {"log", PUBLISH, MQTT_QOS_0, BYTES, mqtt_log_buff},
                       {"set-state", SUBSCRIBE, MQTT_QOS_1, CB, nullptr, &state_rx_cb},
                       {"config", SUBSCRIBE, MQTT_QOS_1, CB, nullptr, &config_rx_cb}};

// --- OTA ---
#include "ota_wrapper.h"
OtaWrapper *ota;
