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

uint16_t SSLWrapper::test_mfln(const char *server, uint16_t port)
{
    for (uint16_t fragment_length = 512; fragment_length <= 4096; fragment_length *= 2)
    {
        if (m_client->probeMaxFragmentLength(server, port, fragment_length))
        {
            return fragment_length;
        }
    }
    return 0;
}

void SSLWrapper::set_mfln(uint16_t fragment_length)
{
    m_client->setBufferSizes(fragment_length, fragment_length);
}

void SSLWrapper::clear_mfln()
{
    m_client->setBufferSizes(16384, 512); // Minimum safe
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