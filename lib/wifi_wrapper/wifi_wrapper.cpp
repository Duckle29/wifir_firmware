#include "wifi_wrapper.h"

WifiWrapper::WifiWrapper(uint8_t led_pin, bool led_inverted)
    : m_led_pin(led_pin),
      m_led_inv(led_inverted)
{
    m_webServer = new AsyncWebServer(80);
    m_dnsServer = new DNSServer();
}

api_error_t  WifiWrapper::begin(const char *m_hostname, bool reset)
{
    if (m_led_pin >= 0)
    {
        pinMode(m_led_pin, OUTPUT);
    }

    if (m_led_pin >= 0)
    {
        digitalWrite(m_led_pin, !m_led_inv);
    }
    Log.notice(F("Starting configuration portal\n"));

    AsyncWiFiManager m_wifiManager(m_webServer, m_dnsServer);

    if (reset)
    {
        m_reset_wifi(&m_wifiManager);
        m_wifiManager.autoConnect(m_hostname, NULL, 1);
    }
    else
    {
        // 25 connection attempts, no password on fallback hotspot
        m_wifiManager.autoConnect(m_hostname, NULL, 25, 2000);
    }

    if (m_led_pin >= 0)
    {
        digitalWrite(m_led_pin, m_led_inv);
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

api_error_t  WifiWrapper::m_reset_wifi(AsyncWiFiManager *wm)
{
    Log.notice(F("Clearning WiFi credentials\n"));
    wm->resetSettings();
    return I_SUCCESS;
}