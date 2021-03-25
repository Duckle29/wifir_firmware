#pragma once

#include <Arduino.h>
#include "config.h"
#include "secrets.h"
String _hostname;
String get_hostname(const char *);

// --- Debug printing ---
#include "error_types.h"
#include <ArduinoLog.h>

bool was_called = false;
uint32_t last_call;
error_t rate_limit(bool loop_marker = false);

// --- WiFi ---
#include <ESP8266WiFi.h>
#include "wifi_wrapper.h"
WifiWrapper wm;

// --- SSL ---
#include "ssl_wrapper.h"
SSLWrapper ssl_wrap;
BearSSL::WiFiClientSecure *client;

// --- MQTT ---
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
// Client
void MQTT_connect();
const char *get_feed_url(String feed_name);
Adafruit_MQTT_Client *mqtt;

// Feeds
void state_rx_cb(char *data, uint16_t len);
void config_rx_cb(char *data, uint16_t len);
enum feed_type
{
    PUBLISH,
    SUBSCRIBE
};

struct Feed
{
    String name;
    feed_type type;
    uint8_t qos;
    void (*cb)(char *data, uint16_t len);

    union
    {
        Adafruit_MQTT_Publish *pub_obj;
        Adafruit_MQTT_Subscribe *sub_obj;
    };
};

Feed *get_feed_by_name(String name);

struct Feed feeds[] = {
    {.name = "temp", .type = PUBLISH},
    {.name = "humi", .type = PUBLISH},
    {.name = "eco2", .type = PUBLISH},
    {.name = "tvoc", .type = PUBLISH},
    {.name = "state_tx", .type = PUBLISH, .qos = MQTT_QOS_1},
    {.name = "state_rx", .type = SUBSCRIBE, .qos = MQTT_QOS_1, .cb = &state_rx_cb},
    {.name = "config_rx", .type = SUBSCRIBE, .qos = MQTT_QOS_1, .cb = &config_rx_cb}};

Adafruit_MQTT_Publish *
    pub_feeds[5];
String pub_feed_names[] = {"temp", "humi", "eco2", "tvoc", "state_tx"};

Adafruit_MQTT_Subscribe *sub_feeds[1];
String sub_feed_names[] = {"state_rx", "config"};

// --- OTA ---
#include "ota_wrapper.h"
OtaWrapper *ota;

// --- Sensors ---
#include "sensor_wrapper.h"
Sensors Sens;

// --- IR ---
#include "ir_wrapper.h"
Ir ir(ir_pins[0], ir_pins[1]);
