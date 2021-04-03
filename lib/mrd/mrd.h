#pragma once

#include <Arduino.h>

#include <FS.h>

class Mrd{
  public:
        Mrd(FS * fs, const char * file_name, uint_fast32_t reset_time=10000);
        ~Mrd();
        int begin();
        int loop();
        int reset_counter();

  private:
    int m_resets;
    FS * m_fs;
    File m_reset_counter_f;
    const char * m_filename;
    const uint_fast32_t m_reset_time;
};

