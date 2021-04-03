#include "mrd.h"

Mrd::Mrd(FS * fs, const char * file_name, uint_fast32_t reset_time)
: m_reset_time(reset_time),
  m_fs(fs),
  m_filename(file_name)
{}

Mrd::~Mrd()
{
    m_reset_counter_f.close();
    m_fs->end();
}

int Mrd::begin()
{
    m_fs->begin();
    if (!m_fs->exists(m_filename))
    {
        m_reset_counter_f = m_fs->open(m_filename, "w+");
    }
    else
    {
        m_reset_counter_f = m_fs->open(m_filename, "r+");
    }

    if (!m_reset_counter_f)
    {
        m_reset_counter_f.close();
        return -1;
    }

    m_resets = m_reset_counter_f.parseInt() + 1;
    m_reset_counter_f.truncate(0);
    m_reset_counter_f.println(m_reset_counter_f);
    return m_resets;
}



int Mrd::loop()
{
    if(millis() >= m_reset_time && m_resets != 0)
    {
        reset_counter();
        m_resets = 0;
    }
}

int Mrd::reset_counter()
{
    m_reset_counter_f.truncate(0);
    m_reset_counter_f.println(0);
}