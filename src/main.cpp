#include "../include/main.h"

void setup()
{
    _hostname = get_hostname(base_name);

    pinMode(LED_BUILTIN, OUTPUT);

    // Debug
    Serial.begin(115200);
    Log.begin(LOG_LEVEL, &Serial);
    Log.notice("\nBOOT\n");
    Log.notice("%s version %s\n", base_name, fw_version);

    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff), "BOOT: %s Version %s\n", base_name, fw_version);

    // Reset detection
    bool clear_settings = false;
    int resets = mrd.begin();

    blink(LED_BUILTIN, resets, 500);

    if (resets >= mrd_resets)
    {
        blink(LED_BUILTIN, 10, 200);
        mrd.reset_counter();
        clear_settings = true;
        ir.reset_protocol();
        Log.notice("Cleared reset counter, was %d\n", resets);
    }

    // WiFi
    wm.begin(_hostname.c_str(), clear_settings);


    p_mqtt_log_buff +=
        snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff), "Connected with IP: %s\n", WiFi.localIP().toString().c_str());

    // SSL
    ssl_wrap.begin(USER_TZ);
    Log.notice("Testing MFLN server capabilites");
    uint16_t mqtts_mfln = ssl_wrap.test_mfln(mqtts_server, mqtts_port);
    uint16_t ota_mfln = ssl_wrap.test_mfln(ota_server, 443);

    p_mqtt_log_buff +=
        snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff), "MFLN support: OTA=%d, MQTTS=%d\n", ota_mfln, mqtts_mfln);

    Log.notice("MFLN support: MQTTS=%d | OTA=%d", mqtts_mfln, ota_mfln);

    if (mqtts_mfln == ota_mfln && mqtts_mfln != 0)
    {
        ssl_wrap.set_mfln(ota_mfln);
    }

    client = ssl_wrap.get_client();

    // OTA
    ota = new OtaWrapper(client, ota_server, ota_user, ota_passwd, base_name, fw_version, ota_check_interval);

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

    Sens.set_offset(temp_offset);
    Sens.begin();
    p_mqtt_log_buff += snprintf(p_mqtt_log_buff, sizeof(mqtt_log_buff), "Exiting setup, device operational");
}

void loop()
{
    Log.verbose(F("WM\n"));
    mrd.loop();
    ota->loop(); // Check for OTA first. Gives a chance to recover from crashing firmware on boot.

    Log.verbose(F("Sens\n"));
    error_t rc = Sens.loop();
    if (rc != I_SUCCESS && rc != W_RATE_LIMIT)
    {
        Log.error("%s\n", get_error_desc(rc));
    }

    // Print sensor readings
    if (limiter_debug.ok())
    {
        char base_buff[35];
        snprintf(base_buff, sizeof(base_buff), " baseline age: %.2f h |", (float)Sens.baseline_age / (float)(60 * 60));
        Log.notice("Sensor readings: | %F°C | %F%% RH | eCO2: %d ppm | TVOC: %d ppb |%s\n", Sens.t, Sens.rh, Sens.eco2,
                   Sens.tvoc, base_buff);
    }

    Log.verbose(F("IR\n"));
    rc = ir.loop();

    ir.send_state();

    Log.verbose(F("MQTT:PUB\n"));
    if (limiter_mqtt.ok(true))
    {

        MQTT_connect();
        Log.trace("MQTT publish\n");

        for (uint_fast8_t i = 0; i < sizeof(feeds) / sizeof(feeds[0]); i++)
        {
            delay(0);
            if (feeds[i].f_type == PUBLISH)
            {
                switch (feeds[i].d_type)
                {
                case FLOAT:
                    feeds[i].pub_obj->publish(*(float *)feeds[i].data);
                    break;

                case BYTES:
                    char *src;
                    src = (char *)feeds[i].data;
                    while (String(src).length() && src != nullptr)
                    {
                        char dest_buff[mqtt_max_len];
                        split_message(dest_buff, src, sizeof(dest_buff), sizeof(dest_buff));

                        uint_fast8_t retries = 0;
                        while (!feeds[i].pub_obj->publish(dest_buff))
                        {
                            if (retries++ >= mqtt_max_retries)
                            {
                                Log.error("[MQTT:PUB] Faild to send log");
                            }
                        }
                    }
                    p_mqtt_log_buff = mqtt_log_buff;
                    break;

                case IRRX:

                    if (ir.state_changed)
                    {
                        decode_results res = *(decode_results *)feeds[i].data;
                        char buff[sizeof(res) + 1];

                        stdAc::state_t state;
                        stdAc::state_t prev;
                        IRAcUtils::decodeToState(&res, &state, &prev);

                        memcpy(buff, &state, sizeof(state));
                        buff[sizeof(state)] = '\0';

                        if (String(buff).length())
                        {
                            char dest_buff[mqtt_max_len];
                            split_message(dest_buff, buff, sizeof(dest_buff), sizeof(dest_buff));

                            uint_fast8_t retries = 0;
                            while (!feeds[i].pub_obj->publish(dest_buff))
                            {
                                if (retries++ >= mqtt_max_retries)
                                {
                                    Log.error("[MQTT:PUB] Faild to send state");
                                }
                            }
                        }
                        ir.state_changed = false;
                    }
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
    limiter_debug.ok(true);

    Log.verbose(F("MQTT:SUB\n"));
    mqtt->processPackets(250);
    if (limiter_mqtt_ping.ok(true))
    {
        mqtt->ping();
    }
}

void state_rx_cb(char *data, uint16_t len)
{
    // Log.trace("%s\n", data);
    ir.set_state(data, len);
}

void config_rx_cb(char *data, uint16_t len)
{
    if (String(data) == "reboot")
    {
        ESP.restart();
    }
    else if (String(data) == "ota")
    {
        ota->loop(true);
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

void split_message(char *dest, char *src, uint16_t segment_len, uint16_t dest_max_len)
{
    if (String(src).length() <= segment_len)
    {
        strncpy(dest, src, dest_max_len);
        src[0] = '\0';
        return;
    }

    String s = String(src);
    int next_newline = s.indexOf('\n');
    int next_space = s.indexOf('\0');
    if (next_newline <= segment_len)
    {
        strncpy(dest, s.substring(0, next_newline).c_str(), dest_max_len);
        strcpy(src, s.substring(next_newline + 1).c_str());
    }
    else if (next_space <= segment_len)
    {
        strncpy(dest, s.substring(0, next_newline).c_str(), dest_max_len);
        strcpy(src, s.substring(next_newline + 1).c_str());
    }
    else
    {
        strncpy(dest, s.substring(0, next_newline).c_str(), dest_max_len);
        strcpy(src, s.substring(next_newline + 1).c_str());
    }
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



void blink(uint_fast8_t led_pin, uint_fast8_t times, uint_fast16_t blink_delay)
{
    for (uint_fast8_t i = 0; i < times * 2; i++)
    {
        digitalWrite(led_pin, !digitalRead(led_pin));
        delay(blink_delay / (i % 2 + 1)); // Be off for half as long as on
    }
}