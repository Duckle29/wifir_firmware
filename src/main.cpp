#include "main.h"

void setup()
{
    _hostname = get_hostname(base_name);

    // Debug
    Serial.begin(115200);
    Log.begin(LOG_LEVEL, &Serial);
    Log.notice("\nBOOT\n");
    Log.notice("%s version %s\n", base_name, fw_version);

    // WiFi
    wm.begin(_hostname.c_str());

    // SSL
    ssl_wrap.begin(USER_TZ);

    uint16_t mqtts_mfln = ssl_wrap.test_mfln(mqtts_server, mqtts_port);
    uint16_t ota_mfln = ssl_wrap.test_mfln(ota_server, 443);

    if (mqtts_mfln == ota_mfln && mqtts_mfln != 0)
    {
        ssl_wrap.set_mfln(ota_mfln);
    }

    Log.trace("Servers support %d (mqtt) : %d (ota) mfln", mqtts_mfln, ota_mfln);
    client = ssl_wrap.get_client();

    // OTA
    ota = new OtaWrapper(client, ota_server, ota_user, ota_passwd, base_name, fw_version, ota_check_interval);

    // MQTTS
    mqtt = new Adafruit_MQTT_Client(client, mqtts_server, mqtts_port, mqtts_username, mqtts_key);

    for (int i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
    {
        const char *feed_url = get_feed_url(feeds[i].name);
        if (feeds[i].type == PUBLISH)
        {
            feeds[i].pub_obj = new Adafruit_MQTT_Publish(mqtt, feed_url, feeds[i].qos);
        }
        else
        {
            feeds[i].sub_obj = new Adafruit_MQTT_Subscribe(mqtt, feed_url, feeds[i].qos);
            if (feeds[i].cb != nullptr)
            {
                feeds[i].sub_obj->setCallback(feeds[i].cb);
            }
        }
    }

    Sens.set_offset(temp_offset);
    Sens.begin();
}

void loop()
{
    wm.loop();
    ota->loop(); // Check for OTA first. Gives a chance to recover from crashing firmware on boot.

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
        Log.trace("MQTT publish\n");

        Feed *f = get_feed_by_name("temp");
        if (f != nullptr)
        {
            f->pub_obj->publish(Sens.t);
        }
        f = get_feed_by_name("humi");
        if (f != nullptr)
        {
            f->pub_obj->publish(Sens.rh);
        }
        f = get_feed_by_name("eco2");
        if (f != nullptr)
        {
            f->pub_obj->publish(Sens.eco2);
        }
        f = get_feed_by_name("tvoc");
        if (f != nullptr)
        {
            f->pub_obj->publish(Sens.tvoc);
        }
    }
    rate_limit(true); // Mark loop for rate limiter
}

void state_rx_cb(char *data, uint16_t len)
{
}

void config_rx_cb(char *data, uint16_t len)
{
}

Feed *get_feed_by_name(String name)
{
    for (int i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
    {
        if (feeds[i].name == name)
        {
            return &feeds[i];
        }
    }
    return nullptr;
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
