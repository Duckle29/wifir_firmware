#include "sensor_wrapper.h"

Sensors::Sensors(uint32_t poll_interval)
{
    m_polling_interval = poll_interval;
    m_aht = new Adafruit_AHTX0();
    m_sgp = new Adafruit_SGP30();
}

void Sensors::begin()
{
    // AHT20
    if (!m_aht->begin())
    {
        Serial.println("Failed to find AHT10/AHT20 chip");
        while (true)
        {
            delay(10);
        }
    }

    m_sens_temperature = m_aht->getTemperatureSensor();
    m_sens_humidity = m_aht->getHumiditySensor();

    m_sens_temperature->printSensorDetails();
    m_sens_humidity->printSensorDetails();

    // SGP30
    if (!m_sgp->begin())
    {
        Serial.println("SGP30 not found :(");
        while (true)
        {
            delay(10);
        }
    }

    m_sgp_start = time(nullptr);

    uint16_t eco2_base, tvoc_base;
    error_t rc = m_sgp_read_baseline(&eco2_base, &tvoc_base);
    switch (rc)
    {
    case I_SUCCESS:
        m_old_baseline = false;
        m_sgp->setIAQBaseline(eco2_base, tvoc_base);
        break;

    case W_FILE_NOT_FOUND:
    case W_OLD_BASELINE:
        m_old_baseline = true;
        m_sgp->setIAQBaseline(eco2_base, tvoc_base);
        break;

    default:
        Log.fatal("%s\n", get_error_desc(rc));
        break;
    }

    delay(50);
    m_poll_temp_humi();

    m_sgp->setHumidity(m_get_absolute_humidity(t, rh));
}

error_t Sensors::loop()
{
    error_t rc = m_sgp_getter(&eco2, &tvoc);
    m_poll_temp_humi();

    return rc;
}

void Sensors::set_offset(float temp_offset)
{
    m_t_offset = temp_offset;
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

error_t Sensors::m_sgp_read_baseline(uint16_t *eco2_base, uint16_t *tvoc_base)
{
    if (!LittleFS.exists(F("sgp_baseline")))
    {
        return W_FILE_NOT_FOUND;
    }

    File baseline = LittleFS.open(F("sgp_baseline"), "r");
    if (!baseline)
    {
        return E_FILE_ACCESS;
    }

    *eco2_base = baseline.parseInt();
    *tvoc_base = baseline.parseInt();
    baseline.close();

    time_t now = time(nullptr);
    // Baseline with sensor off is valid for 7 days
    if (now - baseline.getLastWrite() > (7 * 24 * 60 * 60))
    {
        return W_OLD_BASELINE;
    }
    return I_SUCCESS;
}

error_t Sensors::m_sgp_save_baseline(uint16_t eco2_base, uint16_t tvoc_base)
{
    File baseline = LittleFS.open(F("sgp_baseline"), "w");
    if (!baseline)
    {
        return E_FILE_ACCESS;
    }

    baseline.println(eco2_base);
    baseline.println(tvoc_base);
    baseline.close();
    return I_SUCCESS;
}

error_t Sensors::m_sgp_getter(uint16_t *eco2, uint16_t *tvoc)
{
    if (millis() - m_last_poll < m_polling_interval)
    {
        return W_RATE_LIMIT;
    }
    m_last_poll = millis();

    error_t rc = I_SUCCESS;
    if (!m_old_baseline)
    {
        // If the baseline isn't old, save it every hour
        if ((time(nullptr) - m_sgp_start) % (60 * 60))
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
            m_old_baseline = false;
        }
    }

    if (!m_sgp->IAQmeasure())
    {
        return E_SENSOR;
    }
    *eco2 = m_sgp->eCO2;
    *tvoc = m_sgp->TVOC;
    return rc;
}

void Sensors::m_poll_temp_humi()
{
    m_sens_temperature->getEvent(&m_temperature);
    m_sens_humidity->getEvent(&m_humidity);
    t = m_temperature.temperature + m_t_offset;
    rh = m_get_compensated_humidity(m_humidity.relative_humidity, m_temperature.temperature, m_t_offset);
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
