#pragma once

#include <Arduino.h>
#include <LittleFS.h>

#include <ArduinoLog.h>

#include "config.h"
#include "error_types.h"

#include <IRac.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

class Ir
{
  public:
    Ir(uint_fast8_t RX_PIN, uint_fast8_t TX_PIN, int_fast8_t LED_PIN = -1, bool LED_INVERT = false);

    decode_results rx_results;
    char state_str[100];

    bool state_changed = false;

    api_error_t  loop();

    /**
     * @param[in] state A String with the desired changes to the state.
     * @brief Fill the internal state by parsing the given string
     * @details
     *  Fills the internal stdAc::state_t variable by parsing the supplied string.
     *  The format of the string should be "X=V:" where X is the particular setting,
     *  and V is the value it should be set to.
     *  Supported settings are:
     *      - P: power:
     *          - On
     *          - Off
     *      - T: temperature: Value should be a float, optionally followed by a C or F for scale
     *      - M: mode: Changing to/from off won't power cycle. Use P for that.
     *          - Off
     *          - Auto
     *          - Cool
     *          - Heat
     *          - Dry
     *          - Fan
     *      - H: Swing Horizontal:
     *          - Off
     *          - Auto
     *          - Highest
     *          - High
     *          - Middle
     *          - Low
     *          - Lowest
     *      - S: Fan speed
     *          - Auto
     *          - Min
     *          - Low
     *          - Medium
     *          - High
     *          - Max
     */
    void set_state(String state);
    void set_state(char *state, uint_fast16_t len);

    bool send_state();
    api_error_t  reset_protocol();

    String results_as_string(void);
    String results_as_decoded_string(void);

  private:
    const uint16_t m_rx_buffer_size = 1024; // ~511 bits
    // const uint16_t m_ir_freq = 36700;
    const uint8_t m_ir_timeout = 50;

    const int_fast8_t m_led_pin;
    const bool m_led_invert;
    uint_fast32_t m_led_off_time;

    decode_results m_results;

    IRrecv *m_ir_rx;
    IRac *m_ir_ac;

    api_error_t  m_save_protocol(decode_type_t protocol);
    api_error_t  m_read_protocol(decode_type_t *protocol);

    int m_state_to_str(char *dest, stdAc::state_t state, uint_fast16_t max_len);
};