#include "ir_wrapper.h"

Ir::Ir(uint8_t RX_PIN, uint8_t TX_PIN)
{
    m_ir_rx = new IRrecv(RX_PIN, m_rx_buffer_size, m_ir_timeout, false);
    m_ir_tx = new IRsend(TX_PIN);

    m_ir_rx->enableIRIn();
    m_ir_tx->begin();
}

error_t Ir::loop()
{
    if (m_ir_rx->decode(&m_results))
    {
        decode_type_t protocol = m_results.decode_type;

        if (hasACState(protocol))
        {
            memcpy(state, m_results.state, kStateSizeMax);
            state_changed = true;
        }
        m_ir_rx->resume();
    }
    return I_SUCCESS;
}

bool Ir::send_state(uint8_t *state, uint16_t size)
{
    m_ir_rx->disableIRIn(); // Disable IR in, or sending might keep triggering interrupts
    return m_ir_tx->send(m_protocol, state, size);
    m_ir_rx->enableIRIn();
}

void Ir::set_protocol(decode_type_t protocol)
{
    m_protocol = protocol;
}

String Ir::results_as_string()
{
    return resultToHumanReadableBasic(&m_results);
}

String Ir::results_as_decoded_string()
{
    return IRAcUtils::resultAcToString(&m_results);
}