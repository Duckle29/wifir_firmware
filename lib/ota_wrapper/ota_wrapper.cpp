#include "ota_wrapper.h"

OtaWrapper::OtaWrapper(BearSSL::WiFiClientSecure *client, const char *ota_server, const char *ota_user, const char *ota_pass,
                       const char *base_name, const char *version,
                       uint32_t check_interval)
{
    m_client = client;
    m_ota_server = ota_server;
    m_ota_user = ota_user;
    m_ota_pass = ota_pass;
    m_base_name = base_name;
    m_version = version;
    m_check_interval = check_interval;
    m_last_check -= check_interval;
}

void OtaWrapper::loop(bool check_now)
{
    if (millis() - m_last_check > m_check_interval)
    {
        m_last_check = millis();
        Log.trace("[OTA] Checking for update\n");

        // Capitalize first character of device name and lowercase the rest
        String device_name = String(&m_base_name[1]);
        device_name.toLowerCase();
        device_name = (char)toupper(m_base_name[0]) + device_name;

        String url = m_ota_server;
        url += ":443/update?ver=";
        url += m_version;
        url += "&dev=";
        url += device_name;

        ESPhttpUpdate.setAuthorization(m_ota_user, m_ota_pass);
        ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
        HTTPUpdateResult ret = ESPhttpUpdate.update(*m_client, url);

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Log.error("[OTA] Update failed\n");
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Log.trace("[OTA] No update\n");
            break;
        case HTTP_UPDATE_OK:
            Log.notice("[OTA] Update successfull\n");
            break;
        }
    }
}