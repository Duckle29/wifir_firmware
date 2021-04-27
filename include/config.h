#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>
#include <TZ.h>

#define USER_TZ TZ_Europe_Copenhagen

// Warmup time
const uint_fast32_t warmup = 0 * 60 * 1000;

// Debuggin
#define LOG_LEVEL LOG_LEVEL_TRACE
const uint8_t led_pin = LED_BUILTIN;
const uint32_t debug_interval = 1000;


// HostName
const char *base_name = "WiFIR";
const char *fw_version = "v0.2.0";

// Multiple reset detection
const int mrd_resets = 5;   // How many resets to do to cause a settings reset
const int mrd_timeout = 10; // Time to clear counter [s]

// OTA
const unsigned long ota_check_interval = 60 * 60 * 1000; // Check for updates every 60 minutes

const uint8_t ir_pins[] = {
    D3, // IR input
    D6  // IR output
};

// Sensors
const float temp_offset = -5;

// --- MQTT ---
// Server
const char *mqtts_server = "io.adafruit.com";
const uint16_t mqtts_port = 8883;
const uint32_t mqtt_interval = 30000;
const uint32_t mqtt_keepalive = 5 * 60 * 1000;

// Callback prototypes
void state_rx_cb(char *data, uint16_t len);
void config_rx_cb(char *data, uint16_t len);

// Feeds are set up in main.h, to ensure the required objects are available