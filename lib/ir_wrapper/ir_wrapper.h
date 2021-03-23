#pragma once

#include <Arduino.h>

#include "error_types.h"
#include "config.h"

#include <IRsend.h>
#include <IRrecv.h>
#include <IRac.h>
#include <IRutils.h>
#include <IRremoteESP8266.h>
// #include <ir_Panasonic.h>

class Ir
{
public:
    Ir(uint8_t RX_PIN, uint8_t TX_PIN);

    uint8_t state[kStateSizeMax];
    bool state_changed = false;

    error_t loop();
    void set_protocol(decode_type_t);
    bool send_state(uint8_t *, uint16_t);
    String results_as_string(void);
    String results_as_decoded_string(void);

private:
    const uint16_t m_rx_buffer_size = 1024; // ~511 bits
    // const uint16_t m_ir_freq = 36700;
    const uint8_t m_ir_timeout = 50;

    decode_results m_results;
    decode_type_t m_protocol;

    IRrecv *m_ir_rx;
    IRsend *m_ir_tx;
};