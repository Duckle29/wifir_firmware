#include "rate_limiter.h"

RateLimiter::RateLimiter(uint32_t interval)
{
    m_interval = interval;
    m_last_call = millis() - interval;
}

bool RateLimiter::ok(bool limit)
{
    if (millis() - m_last_call > m_interval)
    {
        if (limit)
        {
            m_last_call = millis();
        }
        return true;
    }
    return false;
}