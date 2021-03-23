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
void MQTT_connect();
const char *get_feed_url(String feed_name);
Adafruit_MQTT_Client *mqtt;
Adafruit_MQTT_Publish *temp_feed, *humi_feed, *eco2_feed, *tvoc_feed, *state_tx_feed, *state_rx_feed;

// --- Sensors ---
#include "sensor_wrapper.h"
Sensors Sens;

// --- IR ---
#include "ir_wrapper.h"
Ir ir(ir_pins[0], ir_pins[1]);
