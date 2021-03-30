#include "wifi_wrapper.h"

WifiWrapper::WifiWrapper(uint8_t led_pin, uint8_t timeout, uint8_t resets)
    : m_led_pin(led_pin),
      m_timeout(timeout),
      m_resets(resets)
{
    m_webServer = new AsyncWebServer(80);
    m_dnsServer = new DNSServer();
}

error_t WifiWrapper::begin(const char *m_hostname)
{
    LittleFS.begin();
    if (m_led_pin >= 0)
    {
        pinMode(m_led_pin, OUTPUT);
    }

    if (m_led_pin >= 0)
    {
        digitalWrite(m_led_pin, LOW);
    }
    Log.notice(F("Starting configuration portal\n"));

    AsyncWiFiManager m_wifiManager(m_webServer, m_dnsServer);

    // Multiple reset detection
    int num_resets = m_check_resets();
    if (num_resets == E_FILE_ACCESS)
    {
        return (error_t)num_resets;
    }
    m_blink(m_led_pin, num_resets);
    if (num_resets >= m_resets)
    {
        m_reset_wifi(&m_wifiManager);
    }

    m_wifiManager.autoConnect(m_hostname);

    if (m_led_pin >= 0)
    {
        digitalWrite(m_led_pin, HIGH);
        pinMode(m_led_pin, INPUT);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Log.warning(F("Failed to connect to wifi\n"));
        return E_CONNECTION_FAILURE;
    }
    else
    {
        String ipaddr = WiFi.localIP().toString();
        Log.notice(F("Connected with IP: %s\n"), ipaddr.c_str());
        return I_SUCCESS;
    }
}

void WifiWrapper::loop()
{
    if (m_counter_cleared)
    {
        return;
    }
    if (millis() > m_timeout * 1000)
    {
        File reset_counter_f = LittleFS.open(F("reset.counter"), "r+");
        int resets = reset_counter_f.parseInt();

        reset_counter_f.seek(0);
        reset_counter_f.println(0);

        reset_counter_f.close();

        m_counter_cleared = true;
        Log.notice("Cleared reset counter, was %d\n", resets);
    }
}

void WifiWrapper::m_blink(uint_fast8_t led_pin, uint_fast8_t times, uint_fast16_t blink_delay)
{
    for (uint_fast8_t i = 0; i < times * 2; i++)
    {
        digitalWrite(led_pin, !digitalRead(led_pin));
        delay(blink_delay / (i % 2 + 1)); // Be off for half as long as on
    }
}

int WifiWrapper::m_check_resets()
{
    // Detect multiple resets to clear WiFi settings
    File reset_counter_f;
    if (!LittleFS.exists(F("reset.counter")))
    {
        reset_counter_f = LittleFS.open(F("reset.counter"), "w+");
    }
    else
    {
        reset_counter_f = LittleFS.open(F("reset.counter"), "r+");
    }

    if (!reset_counter_f)
    {
        reset_counter_f.close();
        return E_FILE_ACCESS;
    }

    int reset_counter = reset_counter_f.parseInt() + 1;
    reset_counter_f.seek(0);
    reset_counter_f.println(reset_counter);
    reset_counter_f.close();

    Log.notice("%d resets\n", reset_counter);

    return reset_counter;
}

error_t WifiWrapper::m_reset_wifi(AsyncWiFiManager *wm)
{
    File reset_counter_f = LittleFS.open(F("reset.counter"), "w+");
    reset_counter_f.println(0);

    m_blink(m_led_pin, 5, 100);
    Log.notice(F("Clearning WiFi credentials\n"));
    wm->resetSettings();
}