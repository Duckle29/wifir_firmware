#include "mrd.h"

Mrd::Mrd(FS * fs, const char * file_name, uint_fast32_t reset_time)
: m_fs(fs),
  m_filename(file_name),
  m_reset_time(reset_time)
{
    m_fs->begin();
}

Mrd::~Mrd()
{
    m_reset_counter_f.close();
    m_fs->end();
}

int Mrd::resets()
{
    if (m_open_file() != 0)
    {
        return -1;
    }
    m_resets = m_reset_counter_f.parseInt() + 1;
    m_reset_counter_f.truncate(0);
    m_reset_counter_f.println(m_resets);

    m_reset_counter_f.close();

    return m_resets;
}

int Mrd::m_open_file()
{
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
    return 0;
}



int Mrd::loop()
{
    int cnt = 0;
    if(millis() >= m_reset_time && m_resets != 0)
    {
        cnt = reset_counter();
        m_resets = 0;
    }
    return cnt;
}

int Mrd::reset_counter()
{
    if (m_open_file() != 0)
    {
        return -1;
    }
    m_reset_counter_f.truncate(0);
    m_reset_counter_f.println(0);
    m_reset_counter_f.close();
    return m_resets;
}