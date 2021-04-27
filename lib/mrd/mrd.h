#pragma once

#include <Arduino.h>

#include <FS.h>

class Mrd{
  public:
        Mrd(FS * fs, const char * file_name, uint_fast32_t reset_time=10000);
        ~Mrd();
        void close();
        int resets();
        int loop();
        int reset_counter();

        bool closed = false;

  private:
    int m_resets;
    FS * m_fs;
    File m_reset_counter_f;
    const char * m_filename;
    const uint_fast32_t m_reset_time;

    int m_open_file();
};

