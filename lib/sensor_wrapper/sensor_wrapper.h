#pragma once
#include <Arduino.h>
#include <ArduinoLog.h>

#include <time.h>
#include <LittleFS.h>
#include "error_types.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_SGP30.h>

class Sensors
{
public:
    Sensors(uint32_t poll_interval = 500); // Limit to 2Hz

    void begin(void);
    error_t loop();
    void set_offset(float temp_offset);

    float t, rh;
    uint16_t eco2, tvoc;
    time_t baseline_age;

private:
    uint32_t m_polling_interval;
    uint32_t m_last_poll;

    // AHT20
    Adafruit_AHTX0 *m_aht = nullptr;
    Adafruit_Sensor *m_sens_humidity, *m_sens_temperature;
    sensors_event_t m_humidity, m_temperature;
    double m_t_offset;

    // SGP30
    Adafruit_SGP30 *m_sgp = nullptr;

    time_t m_sgp_start;
    boolean m_old_baseline = true;

    // Prototypes
    void m_poll_temp_humi();

    uint32_t m_get_absolute_humidity(float temperature, float humidity);
    float m_get_compensated_humidity(double humi_meas, double temp_meas, double temp_offset);
    double m_get_vapor_density_saturation(double temp);

    error_t m_sgp_read_baseline(uint16_t *, uint16_t *);
    error_t m_sgp_save_baseline(uint16_t, uint16_t);
    error_t m_sgp_getter(uint16_t *eco2, uint16_t *tvoc);
};