#include "../include/main.h"

void setup()
{
    Log.trace("LITTLE_FS BEGIN: %s\n", LittleFS.begin() ? "true" : "false");
    _hostname = get_hostname(base_name);

    pinMode(LED_BUILTIN, OUTPUT);

    // Debug
    Serial.begin(115200);
    Log.begin(LOG_LEVEL, &Serial);
    Log.notice("\nBOOT\n");
    Log.notice("%s version %s\n", base_name, fw_version);

    Log.notice("Test: Blinking IR LEDs\n");
    pinMode(ir_pins[1], OUTPUT);
    blink(ir_pins[1], 10, 150);

    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                "BOOT: %s Version %s\n", base_name, fw_version);

    // Reset detection
    bool clear_settings = false;
    int resets = mrd.resets();

    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                "Resets: %d \n", resets);

    blink(LED_BUILTIN, resets, 300);

    if (resets >= mrd_resets)
    {
        blink(LED_BUILTIN, 10, 100);
        mrd.reset_counter();
        clear_settings = true;
        ir.reset_protocol();
        Log.notice("Cleared reset counter, was %d\n", resets);
    }

    // WiFi
    wm.begin(_hostname.c_str(), clear_settings);
    Log.info("clear %d\n", clear_settings);
    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                "Connected with IP: %s\n", WiFi.localIP().toString().c_str());

    // SSL
    ssl_wrap.begin(USER_TZ);

    Log.notice("Testing MFLN server capabilites. OTA server is hard-coded to 512B, so only test MQTTS\n");
    uint16_t mqtts_mfln = ssl_wrap.test_mfln(mqtts_server, mqtts_port);
    //uint16_t ota_mfln = ssl_wrap.test_mfln(ota_server, 443);
    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                "MFLN support: MQTTS=%d\n", mqtts_mfln);

    Log.notice("MFLN support: MQTTS=%d\n", mqtts_mfln);

    if (mqtts_mfln != 0)
    {
        ssl_wrap.set_mfln(mqtts_mfln);
    }

    client = ssl_wrap.get_client();

    // OTA
    ota = new OtaWrapper(client, ota_server, ota_user, ota_passwd, base_name, fw_version, ota_check_interval);

    ota->loop(true);

    // MQTTS
    mqtt = new Adafruit_MQTT_Client(client, mqtts_server, mqtts_port, mqtts_username, mqtts_key);

    for (uint_fast8_t i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
    {
        const char *feed_url = get_feed_url(feeds[i].name);
        if (feeds[i].f_type == PUBLISH)
        {
            feeds[i].pub_obj = new Adafruit_MQTT_Publish(mqtt, feed_url, feeds[i].qos);
        }
        else
        {
            feeds[i].sub_obj = new Adafruit_MQTT_Subscribe(mqtt, feed_url, feeds[i].qos);
            if (feeds[i].cb != nullptr)
            {
                Serial.println(feed_url);
                feeds[i].sub_obj->setCallback(feeds[i].cb);
                mqtt->subscribe(feeds[i].sub_obj);
            }
        }
    }

    MQTT_connect();

    Sens.begin();
    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff), "Exiting setup, device operational\n");
}

void loop()
{
    Log.verbose(F("WM\n"));

    if (!mrd.closed)
    {
        int resets_were = mrd.loop();
        if(resets_were)
        {
            p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                    "Reset counter cleared. Resets were %d\n", resets_were);
            mrd.close();
        }
    }

    api_error_t  rc;
    // Reading sensors right after ota seems to result in spikes
    if (ota->loop())
    {
        sensor_hold_timestamp = millis();
    }

    if (millis() - sensor_hold_timestamp > sensor_hold_time)
    {
        Log.verbose(F("Sens\n"));
        rc = Sens.loop();
        if (rc != I_SUCCESS && rc != W_RATE_LIMIT)
        {
            Log.error("%s\n", get_error_desc(rc));
        }
    }

    // Print sensor readings
    if (limiter_debug.ok())
    {
        char base_buff[35];
        snprintf(base_buff, sizeof(base_buff), "baseline age: %.2f h |", (float)Sens.baseline_age / (float)(60 * 60));
        Log.notice("Sensor readings: | %F°C | %F%% RH | eCO2: %d ppm | TVOC: %d ppb | %s\n", Sens.t, Sens.rh, Sens.eco2,
                   Sens.tvoc, base_buff);
    }

    Log.verbose(F("IR\n"));
    rc = ir.loop();

    if(ir.send_state())
    {
        sensor_hold_timestamp = millis();
    }

    Log.verbose(F("MQTT:PUB\n"));
    if (limiter_mqtt.ok(true))
    {
        mqtt_handle();
    }
    limiter_debug.ok(true);

    Log.verbose(F("MQTT:SUB\n"));
    mqtt->processPackets(250);
    if (limiter_mqtt_ping.ok(true))
    {
        mqtt->ping();
    }
}

api_error_t  mqtt_handle()
{
    api_error_t  last_return_code = I_SUCCESS;
    {
        MQTT_connect();
        Log.notice("MQTT publish\n");

        for (uint_fast8_t i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
        {
            delay(0);
            if (feeds[i].warmup > millis())
            {
                continue;
            }
            if (feeds[i].f_type == PUBLISH)
            {
                switch (feeds[i].d_type)
                {
                case FLOAT:
                    feeds[i].pub_obj->publish(*(float *)feeds[i].data);
                    break;

                case IRRX:
                    if (ir.state_changed)
                    {
                        ir.state_changed = false;
                    }
                    else
                    {
                        break;
                    }

                case BYTES:
                    // //Temp log of free heap
                    // p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                    //             "Free heap: %d\n", ESP.getFreeHeap());
                    char *src_p;
                    char *src_end;

                    src_p = (char *)feeds[i].data;
                    src_end = src_p + strlen(src_p);

                    while (src_p < src_end && src_p != nullptr)
                    {
                        char dest_buff[mqtt_max_len];
                        int bytes = segment_data(dest_buff, src_p, mqtt_max_len - 1);
                        src_p += bytes;
                        dest_buff[bytes] = '\0';

                        uint_fast8_t retries = 0;
                        while (!feeds[i].pub_obj->publish(dest_buff))
                        {
                            if (retries++ >= mqtt_max_retries)
                            {
                                Log.error("[MQTT:PUB] Faild to send log\n");
                                break;
                            }
                        }
                    }
                    p_mqtt_log_buff = mqtt_log_buff;
                    mqtt_log_buff[0] = '\0';
                    break;

                case UINT16:
                    feeds[i].pub_obj->publish(*(uint16_t *)feeds[i].data);
                    break;
                default:
                    break;
                }
            }
        }
    }
    return last_return_code;
}

void state_rx_cb(char *data, uint16_t len)
{
    // Log.trace("%s\n", data);
    ir.set_state(data, len);
}

void config_rx_cb(char *data, uint16_t len)
{
    String data_str = String(data);
    data_str.toLowerCase();
    if (data_str == "reboot")
    {
        p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff), "Rebooting");
        mqtt_handle();
        ESP.restart();
    }
    else if (data_str == "ota")
    {
        p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff), "Checking for update");
        mqtt_handle();
        ota->loop(true);
    }
    else if (data_str.startsWith("caltemp:"))
    {
        Log.trace("%s\n", data_str.c_str());
        data_str.remove(0, 8);
        Log.trace("%s\n", data_str.c_str());
        double cal_temp = data_str.toDouble();
        double new_offset = cal_temp - (Sens.t - Sens.get_offset());
        Sens.set_offset(new_offset);
        p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff),
                                "Temp offset set to: %f °C\n", new_offset);
    }
    else if (data_str == "get_sensors")
    {
        p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff) - (p_mqtt_log_buff - mqtt_log_buff), "Sensors requested");
        mqtt_handle();
    }
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

String get_hostname(const char *base_name)
{
    String chipID = String(ESP.getChipId(), HEX);
    chipID.toUpperCase();

    return String(base_name) + "-" + chipID;
}

/**
 * @brief Splits a source buffer into segments and places the segments in dest array.
 *
 * @param dest The buffer to place the segments in. Ensure this is at least *segment_length* large. This function
 * doesn't add null termination.
 * @param src  The input buffer.
 * @param segment_length The max length of a segment
 * @param src_length The length of the input src. If set to 0 will try to find it with strlen(). Default: 0
 * @param preffered_split_characters Null terminated string of characters to prefer to split on, in order of decreasing
 * preference. Defaults to newline -> space -> null (for splitting strings)
 * @return int The amount of bytes placed in the dest buffer
 */
int segment_data(void *dest, const void *src, uint_fast32_t segment_length, uint_fast32_t src_length,
                 const char *preffered_split_characters)
{
    if (src_length == 0)
    {
        src_length = strlen((const char *)src);
    }

    // If the src data is already short enough, just move it to the destination and return it's length
    if (src_length <= segment_length)
    {
        memmove(dest, src, src_length);
        return src_length;
    }

    // Go through list of preferred split characters, returning on the first one that's short enough
    char segment_buff[segment_length];
    memmove(segment_buff, src, segment_length);
    for (uint_fast16_t i = 0; i < strlen(preffered_split_characters); i++)
    {
        char * split_char_p = strrchr(segment_buff, preffered_split_characters[i]);

        if(split_char_p == 0)
        {
            continue;
        }

        uint_fast64_t split_char_idx = split_char_p - (const char *)segment_buff;

        Log.verbose("Found %d at idx %d\n", preffered_split_characters[i], split_char_idx);

        if (split_char_idx+1 <= segment_length)
        {
            memmove(dest, src, split_char_idx);
            return split_char_idx+1;
        }
    }

    // If no preferred split points have been found, just segment it into *segment_length* chunks
    memmove(dest, src, segment_length);
    return segment_length;
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
        Log.warning("Retrying MQTT connection in 15 seconds...\n");
        mqtt->disconnect();
        delay(15000); // wait 5 seconds
        retries--;
        if (retries == 0)
        {
            mrd.reset_counter();
            // basically die and wait for WDT to reset me
            while (1)
                ;
        }
    }

    Log.trace("MQTT Connected!\n");
}

void blink(uint_fast8_t led_pin, uint_fast8_t times, uint_fast16_t blink_delay)
{
    for (uint_fast8_t i = 0; i < times * 2; i++)
    {
        digitalWrite(led_pin, !digitalRead(led_pin));
        delay(blink_delay / (i % 2 + 1)); // Be off for half as long as on
    }
}

