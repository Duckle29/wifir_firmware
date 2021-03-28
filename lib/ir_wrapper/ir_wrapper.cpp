#include "ir_wrapper.h"

Ir::Ir(uint8_t RX_PIN, uint8_t TX_PIN)
{
    m_ir_rx = new IRrecv(RX_PIN, m_rx_buffer_size, m_ir_timeout, false);
    m_ir_ac = new IRac(TX_PIN);

    m_ir_rx->enableIRIn();
}

error_t Ir::loop()
{
    if (m_ir_rx->decode(&m_results, nullptr, 0, 10))
    {
        m_protocol = m_results.decode_type;

        if (hasACState(m_protocol))
        {
            rx_results = m_results;
            state_changed = true;
        }
        m_ir_rx->resume();
    }
    return I_SUCCESS;
}

void Ir::send_state()
{

    if (m_ir_ac->hasStateChanged())
    {
        if (m_ir_ac->isProtocolSupported(m_protocol))
        {
            m_ir_ac->next.protocol = m_protocol;
            m_ir_rx->disableIRIn(); // Disable IR in, or sending might keep triggering interrupts
            m_ir_ac->sendAc();
            m_ir_rx->enableIRIn();
            Serial.println("I'm blaaastin");
        }
        else
        {
            Serial.println("No protocol");
            // State hasn't changed or protocol hasn't been set (by using OEM remote once)
        }
    }
}

void Ir::set_state(char *state, uint_fast16_t len)
{
    // Ensure the data is null terminated
    // char *buff = (char *)malloc(len + 1);
    // strncpy(buff, state, len);
    // buff[len] = '\0';

    set_state(String(state));
}

void Ir::set_state(String state)
{
    String sub;
    while (state.indexOf(":") != -1)
    {
        delay(0);
        int idx = state.indexOf(":");
        sub = state.substring(2, idx);
        Serial.println(sub);

        if (state.startsWith("T")) // Temperature
        {
            m_ir_ac->next.celsius = sub.endsWith("C");
            if (sub.endsWith("C") || sub.endsWith("F"))
            {
                sub.remove(sub.length() - 1);
            }
            m_ir_ac->next.degrees = atof(sub.c_str());
        }
        else if (state.startsWith("P")) // Power
        {
            m_ir_ac->next.power = IRac::strToBool(sub.c_str());
        }
        else if (state.startsWith("M")) // Mode
        {
            m_ir_ac->next.mode = IRac::strToOpmode(sub.c_str());
        }
        else if (state.startsWith("H")) // Horizontal swing
        {
            m_ir_ac->next.swingh = IRac::strToSwingH(sub.c_str());
        }
        else if (state.startsWith("V")) // Vertical swing
        {
            m_ir_ac->next.swingv = IRac::strToSwingV(sub.c_str());
        }
        else if (state.startsWith("S")) // Fan speed
        {
            m_ir_ac->next.fanspeed = IRac::strToFanspeed(sub.c_str());
        }
        state.remove(0, idx + 1);
    }
}

String Ir::results_as_string()
{
    return resultToHumanReadableBasic(&rx_results);
}

String Ir::results_as_decoded_string()
{
    return IRAcUtils::resultAcToString(&rx_results);
}