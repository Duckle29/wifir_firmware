#pragma once
#include <Arduino.h>
#include <ArduinoLog.h>

#include <time.h>
#include <LittleFS.h>
#include "error_types.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_SGP30.h>

#include <MedianFilterLib.h>

class Sensors
{
public:
    Sensors(uint32_t poll_interval = 500, float offset = 0, int median_window = 5); // Limit to 2Hz

    void begin(void);
    error_t loop();
    void set_offset(double temp_offset);
    double get_offset();

    float t, rh;
    uint16_t eco2, tvoc;
    time_t baseline_age;
    bool old_baseline = true;

private:
    uint32_t m_polling_interval;
    uint32_t m_last_poll;

    // Sensor filtering
    MedianFilter<float> * m_filt_temp;
    MedianFilter<float> * m_filt_humi; 
    MedianFilter<float> * m_filt_eco2;
    MedianFilter<float> * m_filt_tvoc;  

    // AHT20
    Adafruit_AHTX0 *m_aht = nullptr;
    Adafruit_Sensor *m_sens_humidity, *m_sens_temperature;
    sensors_event_t m_humidity, m_temperature;
    double m_t_offset;

    // SGP30
    Adafruit_SGP30 *m_sgp = nullptr;

    time_t m_sgp_start;

    // Prototypes
    void m_poll_temp_humi();

    uint32_t m_get_absolute_humidity(float temperature, float humidity);
    float m_get_compensated_humidity(double humi_meas, double temp_meas, double temp_offset);
    double m_get_vapor_density_saturation(double temp);
    error_t m_sgp_getter(uint16_t *eco2, uint16_t *tvoc);

    /** File handling **/
    error_t m_open_file(const char * filename, File * fp, const char * mode, bool override=false);
    time_t m_get_last_write(const char * filename);

    error_t m_value_from_file(const char * filename, uint16_t * values, size_t len=1);
    error_t m_value_from_file(const char * filename, float * values, size_t len=1);

    error_t m_value_to_file(const char * filename, uint16_t * values, size_t len=1, bool override=false);
    error_t m_value_to_file(const char * filename, float * values, size_t len=1, bool override=false);
    
    
    error_t m_sgp_read_baseline(uint16_t *eco2_base, uint16_t *tvoc_base);
    error_t m_sgp_save_baseline(uint16_t eco2_base, uint16_t tvoc_base);

    error_t m_read_calibration(float * cal_t_offset);
    error_t m_save_calibration(float cal_t_offset);
};

const char * const sgp_baseline_filename = "sgp_baseline";
const char * const calibration_filename = "calibration";