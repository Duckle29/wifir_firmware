#pragma once

#include <Arduino.h>

class RateLimiter
{
public:
    RateLimiter(uint32_t limit);
    bool ok(bool ignore = false);

private:
    uint32_t m_interval;
    uint32_t m_last_call;
};