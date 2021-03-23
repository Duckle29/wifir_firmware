#include "main.h"

void setup()
{
    _hostname = get_hostname(base_name);

    // Debug
    Serial.begin(115200);
    Log.begin(LOG_LEVEL, &Serial);

    // WiFi
    wm.begin(_hostname.c_str());

    // SSL
    ssl_wrap.begin(USER_TZ);
    client = ssl_wrap.get_client();

    // MQTTS
    mqtt = new Adafruit_MQTT_Client(client, mqtts_server, mqtts_port, mqtts_username, mqtts_key);

    const char *feed_url_c = get_feed_url("temp");
    temp_feed = new Adafruit_MQTT_Publish(mqtt, feed_url_c, MQTT_QOS_0);
    feed_url_c = get_feed_url("humi");
    humi_feed = new Adafruit_MQTT_Publish(mqtt, feed_url_c, MQTT_QOS_0);

    Sens.set_offset(temp_offset);
    Sens.begin();
}

void loop()
{
    wm.loop();

    error_t rc = Sens.loop();
    if (rc != I_SUCCESS && rc != W_RATE_LIMIT)
    {
        Log.error("%s\n", get_error_desc(rc));
    }

    // Print sensor readings
    if (rate_limit() == I_SUCCESS)
    {
        Log.notice("Sensor readings: | %F°C | %F%% RH | eCO2: %d ppm | TVOC: %d ppb | baseline age: %F h |\n",
                   Sens.t,
                   Sens.rh,
                   Sens.eco2,
                   Sens.tvoc,
                   Sens.baseline_age / (float)(60 * 60));
    }

    rc = ir.loop();
    // Print IR readings
    if (ir.state_changed && rate_limit() == I_SUCCESS)
    {
        ir.state_changed = false;
        Log.notice("New IR state(local): %s\n%s\n\n",
                   ir.results_as_string().c_str(),
                   ir.results_as_decoded_string().c_str());
    }

    if (rate_limit() == I_SUCCESS)
    {
        MQTT_connect();
        Log.notice("MQTT publish\n");
        temp_feed->publish(Sens.t);
        humi_feed->publish(Sens.rh);
    }

    rate_limit(true); // Mark loop for rate limiter
}

const char *get_feed_url(String feed_name)
{
    String name = _hostname + "-" + feed_name;
    name.toLowerCase();
    String feed_url = String(mqtts_username) + "/feeds/" + name;

    char *cstr = (char *)malloc(feed_url.length() + 1);
    strcpy(cstr, feed_url.c_str());
    return cstr;
}

error_t rate_limit(bool loop_marker)
{
    if (loop_marker && was_called)
    {
        //Serial.println("2");
        last_call = millis();
        was_called = false;
        return I_SUCCESS;
    }

    if (millis() - last_call < call_interval)
    {
        return W_RATE_LIMIT;
    }
    was_called = true;
    return I_SUCCESS;
}

String get_hostname(const char *base_name)
{
    String chipID = String(ESP.getChipId(), HEX);
    chipID.toUpperCase();

    return String(base_name) + "-" + chipID;
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect()
{
    int8_t ret;

    // Stop if already connected.
    if (mqtt->connected())
    {
        return;
    }

    Log.trace("Connecting to MQTT\n");

    uint8_t retries = 3;
    while ((ret = mqtt->connect()) != 0)
    { // connect will return 0 for connected
        Log.warning("%s\n", mqtt->connectErrorString(ret));
        Log.warning("Retrying MQTT connection in 5 seconds...\n");
        mqtt->disconnect();
        delay(5000); // wait 5 seconds
        retries--;
        if (retries == 0)
        {
            // basically die and wait for WDT to reset me
            while (1)
                ;
        }
    }

    Log.trace("MQTT Connected!\n");
}
