#include "wifi_wrapper.h"

WifiWrapper::WifiWrapper()
{
    m_webServer = new AsyncWebServer(80);
    m_dnsServer = new DNSServer();
    m_drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
}

bool WifiWrapper::begin(const char *hostname, uint8_t led_pin)
{
    if (led_pin >= 0)
    {
        pinMode(LED_BUILTIN, OUTPUT);
    }

    AsyncWiFiManager m_wifiManager(m_webServer, m_dnsServer);

    if (m_drd->detectDoubleReset())
    {
        Log.notice(F("Clearning WiFi credentials\n"));
        m_wifiManager.resetSettings();
    }

    if (led_pin >= 0)
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
    Log.notice(F("Starting configuration portal\n"));

    m_wifiManager.autoConnect(hostname);

    if (led_pin >= 0)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        pinMode(LED_BUILTIN, INPUT);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        Log.warning(F("Failed to connect\n"));
        return false;
    }
    else
    {
        String ipaddr = WiFi.localIP().toString();
        Log.notice(F("Connected with IP: %s\n"), ipaddr.c_str());
        return true;
    }
}

void WifiWrapper::loop()
{
    m_drd->loop();
}