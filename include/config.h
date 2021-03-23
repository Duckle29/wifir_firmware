#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>
#include <TZ.h>

#define USER_TZ TZ_Europe_Copenhagen

// Debug printing
#define LOG_LEVEL LOG_LEVEL_NOTICE
const uint32_t call_interval = 10000;

// HostName
const char *base_name = "WiFIR";

// Double Reset Detection
const int drd_timeout = 10;
const int drd_address = 0;

// MQTT server
const char *mqtts_server = "io.adafruit.com";
const uint16_t mqtts_port = 8883;

const uint8_t ir_pins[] = {
    D3, // IR input
    D6  // IR output
};

// MQTT
const char *mqtt_server = "io.adafruit.com";
const uint16_t mqtt_port = 8883;

// Sensors
const float temp_offset = -9;