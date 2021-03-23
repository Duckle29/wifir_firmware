#include "ssl_wrapper.h"

SSLWrapper::SSLWrapper()
{
    m_certStore = new BearSSL::CertStore();
    m_client = new BearSSL::WiFiClientSecure();
}

BearSSL::WiFiClientSecure *SSLWrapper::get_client()
{
    return m_client;
}

int SSLWrapper::begin(const char *tz)
{
    m_set_clock(tz);
    LittleFS.begin();

    int numCerts = m_certStore->initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));

    if (numCerts == 0)
    {
        Log.fatal("No certificates found. SSL can't work\n");
        return 0;
    }
    Log.trace("%d certificates loaded\n", numCerts);

    m_client->setCertStore(m_certStore);

    return numCerts;
}

void SSLWrapper::m_set_clock(const char *tz)
{
    configTime(tz, "pool.ntp.org");

    Log.notice("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    Log.notice("Current time: %s\n", ctime(&now));
}