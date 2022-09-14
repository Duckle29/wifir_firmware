#include "sensor_wrapper.h"

Sensors::Sensors(uint32_t poll_interval, float offset, int median_window)
{
    m_polling_interval = poll_interval;
    m_t_offset = offset;

    m_aht = new Adafruit_AHTX0();
    m_sgp = new Adafruit_SGP30();
    
    m_filt_temp = new MedianFilter<float>(median_window);
    m_filt_humi = new MedianFilter<float>(median_window);
    m_filt_eco2 = new MedianFilter<float>(median_window);
    m_filt_tvoc = new MedianFilter<float>(median_window);
}

void Sensors::begin()
{
    // AHT20
    if (!m_aht->begin())
    {
        Serial.println("Failed to find AHT10/AHT20 chip");
        while (!m_aht->begin())
        {
            delay(500);
        }
    }
    Serial.println("Found AHT10/AHT20 chip");

    m_sens_temperature = m_aht->getTemperatureSensor();
    m_sens_humidity = m_aht->getHumiditySensor();

    m_sens_temperature->printSensorDetails();
    m_sens_humidity->printSensorDetails();

    // SGP30
    if (!m_sgp->begin())
    {
        Serial.println("SGP30 not found :(");
        while (!m_sgp->begin())
        {
            delay(500);
        }
    }
    Serial.println("Found SGP30");

    m_sgp_start = time(nullptr);

    uint16_t eco2_base, tvoc_base;
    api_error_t  rc = m_sgp_read_baseline(&eco2_base, &tvoc_base);
    switch (rc)
    {
    case I_SUCCESS:
        old_baseline = false;
        m_sgp->setIAQBaseline(eco2_base, tvoc_base);
        break;

    case W_FILE_NOT_FOUND:
    case W_OLD_BASELINE:
        old_baseline = true;
        m_sgp->setIAQBaseline(eco2_base, tvoc_base);
        break;

    default:
        Log.fatal("%s\n", get_error_desc(rc));
        break;
    }

    float cal_t_offset;
    if (m_read_calibration(&cal_t_offset) == I_SUCCESS)
    {
        m_t_offset = cal_t_offset;
    }

    delay(50);

    // Take 10 samples to start with
    for (int i = 0; i<10; i++)
    {
        m_poll_temp_humi();
        delay(100);
    }

    m_sgp->setHumidity(m_get_absolute_humidity(t, rh));
}

api_error_t  Sensors::loop()
{
    api_error_t  rc = m_sgp_getter(&eco2, &tvoc);
    m_poll_temp_humi();

    return rc;
}

void Sensors::set_offset(double temp_offset)
{
    m_t_offset = temp_offset;
    api_error_t  rc = m_save_calibration(m_t_offset);
    if (rc != I_SUCCESS)
    {
        Log.error("[Sensors] %s", get_error_desc(rc));
    }
}

double Sensors::get_offset()
{
    return m_t_offset;
}


/* return absolute humidity [mg/m^3] with approximation formula
* @param temperature [°C]
* @param humidity [%RH]
*/
uint32_t Sensors::m_get_absolute_humidity(float temperature, float humidity)
{
    // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
    const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
    const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
    return absoluteHumidityScaled;
}

api_error_t  Sensors::m_open_file(const char * filename, File * fp, const char * mode, bool override)
{
    if (LittleFS.exists(filename))
    {
        if(mode != "r" && mode != "r+" && !override)
        {
            return W_FILE_EXISTS;
        }
    }
    else
    {
        if(mode == "r" || mode == "r+")
        {
            return W_FILE_NOT_FOUND;
        }
    }

    *fp = LittleFS.open(filename, mode);

    if (!*fp)
    {
        return E_FILE_ACCESS;
    }
    return I_SUCCESS;
}

api_error_t  Sensors::m_value_from_file(const char * filename, uint16_t * values, size_t len)
{
    api_error_t  rc = I_SUCCESS;
    File f;
    
    rc = m_open_file(filename, &f, "r");

    if (rc != I_SUCCESS)
    {
        return rc;
    }

    for(unsigned int i=0; i<len; i++)
    {
        *values++ = f.parseInt();
    }
    f.close();
    return I_SUCCESS;
}

api_error_t  Sensors::m_value_from_file(const char * filename, float * values, size_t len)
{
    api_error_t  rc = I_SUCCESS;
    File f;
    
    rc = m_open_file(filename, &f, "r");

    if (rc != I_SUCCESS)
    {
        return rc;
    }

    for(unsigned int i=0; i<len; i++)
    {
        *values++ = f.parseFloat();
    }
    f.close();
    return I_SUCCESS;
}

api_error_t  Sensors::m_value_to_file(const char * filename, uint16_t * values, size_t len, bool override)
{
    api_error_t  rc = I_SUCCESS;
    File f;
    
    rc = m_open_file(filename, &f, "w", override);

    if (rc != I_SUCCESS)
    {
        return rc;
    }

    for(unsigned int i=0; i<len; i++)
    {
        f.println(*values++);
    }
    f.close();
    return I_SUCCESS;
}

api_error_t  Sensors::m_value_to_file(const char * filename, float * values, size_t len, bool override)
{
    api_error_t  rc = I_SUCCESS;
    File f;
    
    rc = m_open_file(filename, &f, "w", override);

    if (rc != I_SUCCESS)
    {
        return rc;
    }

    for(unsigned int i=0; i<len; i++)
    {
        f.println(*values++);
    }
    f.close();
    return I_SUCCESS;
}

time_t Sensors::m_get_last_write(const char * filename)
{
    File f;
    if(m_open_file(filename, &f, "r") != I_SUCCESS)
    {
        return 0;
    }
    
    time_t last_write = f.getLastWrite();
    f.close();
    return last_write;
}

api_error_t  Sensors::m_read_calibration(float * cal_t_offset)
{
    return m_value_from_file(calibration_filename, cal_t_offset);
}

api_error_t  Sensors::m_save_calibration(float cal_t_offset)
{
    char buff[10];
    snprintf(buff, sizeof(buff), "%.2f", cal_t_offset);
    Log.trace("offset set to %s\n", buff);
    return m_value_to_file(calibration_filename, &cal_t_offset, 1, true);
}

api_error_t  Sensors::m_sgp_read_baseline(uint16_t *eco2_base, uint16_t *tvoc_base)
{
    api_error_t  rc = I_SUCCESS;
    uint16_t buff[2];
    

    rc = m_value_from_file(sgp_baseline_filename, buff, 2);

    if (rc != I_SUCCESS)
    {
        return rc;
    }

    *eco2_base = buff[0];
    *tvoc_base = buff[1];

    time_t now = time(nullptr);
    // Baseline with sensor off is valid for 7 days
    if (now - m_get_last_write(sgp_baseline_filename) > (7 * 24 * 60 * 60))
    {
        return W_OLD_BASELINE;
    }
    return I_SUCCESS;
}

api_error_t  Sensors::m_sgp_save_baseline(uint16_t eco2_base, uint16_t tvoc_base)
{
    uint16_t buff[] = {eco2_base, tvoc_base};

    return m_value_to_file(sgp_baseline_filename, buff, sizeof(buff), true);
}

api_error_t  Sensors::m_sgp_getter(uint16_t *eco2, uint16_t *tvoc)
{
    if (millis() - m_last_poll < m_polling_interval)
    {
        return W_RATE_LIMIT;
    }
    m_last_poll = millis();

    api_error_t  rc = I_SUCCESS;
    if (!old_baseline)
    {
        // If the baseline isn't old, save it every hour
        if ((time(nullptr) - m_sgp_start) % (60 * 60) == 0)
        {
            uint16_t eco2_base, tvoc_base;
            if (m_sgp->getIAQBaseline(&eco2_base, &tvoc_base))
            {
                rc = m_sgp_save_baseline(eco2_base, tvoc_base);
                if (rc == I_SUCCESS)
                {
                    Serial.printf("Saved baseline:\n\teco2: %X\n\ttvoc: %X", eco2_base, tvoc_base);
                }
            }
        }
    }
    else // Old sample, wait 12h before saving
    {
        baseline_age = time(nullptr) - m_sgp_start;
        if (baseline_age > 12 * 60 * 60)
        {
            // If the sensor has had 12h to get a baseline, it's no-longer old
            old_baseline = false;
        }
    }

    if (!m_sgp->IAQmeasure())
    {
        return E_SENSOR;
    }
    m_filt_tvoc->AddValue(m_sgp->TVOC);
    m_filt_eco2->AddValue(m_sgp->eCO2);
    *tvoc = m_filt_tvoc->GetFiltered();
    *eco2 = m_filt_eco2->GetFiltered();
    return rc;
}

void Sensors::m_poll_temp_humi()
{
    m_sens_temperature->getEvent(&m_temperature);
    m_sens_humidity->getEvent(&m_humidity);

    m_filt_temp->AddValue(m_temperature.temperature);
    m_filt_humi->AddValue(m_humidity.relative_humidity);

    t = m_filt_temp->GetFiltered() + m_t_offset;
    rh = m_get_compensated_humidity(m_filt_humi->GetFiltered() , m_filt_temp->GetFiltered(), m_t_offset);
}

// Thanks to help from aheid in home assistant DIY channel for pointing me in the right direction :)
// Based on the equations at http://hyperphysics.phy-astr.gsu.edu/hbase/Kinetic/relhum.html
float Sensors::m_get_compensated_humidity(double humi_meas, double temp_meas, double temp_offset)
{
    return humi_meas * (m_get_vapor_density_saturation(temp_meas) / m_get_vapor_density_saturation(temp_meas + temp_offset));
}

// Based on the reduced fit http://hyperphysics.phy-astr.gsu.edu/hbase/Kinetic/relhum.html#C3
double Sensors::m_get_vapor_density_saturation(double temp)
{
    double VD = 5.018;
    VD += 0.32321 * temp;
    VD += 0.0081847 * temp * temp;
    VD += 0.00031243 * temp * temp * temp;
    return VD;
}
