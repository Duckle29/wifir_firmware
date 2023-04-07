#include "ir_wrapper.h"

Ir::Ir(uint_fast8_t RX_PIN, uint_fast8_t TX_PIN, int_fast8_t LED_PIN, bool LED_INVERT)
    : m_led_pin(LED_PIN),
      m_led_invert(LED_INVERT)
{

    m_ir_rx = new IRrecv(RX_PIN, m_rx_buffer_size, m_ir_timeout, false);
    m_ir_ac = new IRac(TX_PIN);
}

api_error_t Ir::begin()
{
    decode_type_t protocol;
    api_error_t response = m_read_protocol(&protocol);
    if (response == I_SUCCESS && protocol != UNUSED && protocol != UNKNOWN)
    {
        m_ir_ac->next.protocol = protocol;
        m_ir_ac->markAsSent();
    }

    m_ir_rx->enableIRIn();

    return response;
}

api_error_t  Ir::loop()
{

    // Other functions might deal with the LED differently. Ensure it's an output
    if (m_led_pin >= 0)
    {
        pinMode(m_led_pin, OUTPUT);
    }

    // Turn off the LED
    if (millis() >= m_led_off_time && digitalRead(m_led_pin) != m_led_invert)
    {
        digitalWrite(m_led_pin, m_led_invert);
    }

    if (m_ir_rx->decode(&m_results, nullptr, 0, 10))
    {
        decode_type_t m_protocol = m_results.decode_type;

        if (hasACState(m_protocol))
        {

            if (m_protocol != UNKNOWN && (m_ir_ac->next.protocol == UNKNOWN || m_ir_ac->next.protocol == UNUSED))
            {
                m_ir_ac->next.protocol = m_protocol;
                m_ir_ac->markAsSent();
                m_save_protocol(m_protocol);

                Log.trace("Protocol set to %s\n", typeToString(m_protocol).c_str());
            }

            Log.trace("New IR state(local): %s\n%s\n\n",
                      results_as_string().c_str(),
                      results_as_decoded_string().c_str());

            rx_results = m_results;
            stdAc::state_t state;
            stdAc::state_t prev = m_ir_ac->getStatePrev();

            IRAcUtils::decodeToState(&m_results, &state, &prev);

            m_state_to_str(state_str, state, sizeof(state_str));

            state_changed = true;

            // Blink the LED
            digitalWrite(m_led_pin, !m_led_invert);
            m_led_off_time = millis() + 250; // Set the led to turn off in 500ms
        }
        m_ir_rx->resume();
    }
    return I_SUCCESS;
}

bool Ir::send_state(bool force)
{
    if (m_ir_ac->hasStateChanged() || force)
    {
        if (m_ir_ac->isProtocolSupported(m_ir_ac->next.protocol))
        {
            Log.trace("Sending AC state\n");
            m_ir_rx->disableIRIn(); // Disable IR in, or sending might keep triggering interrupts
            m_ir_ac->sendAc();
            m_ir_rx->enableIRIn();
            return true;
        }
        else
        {
            m_ir_ac->markAsSent();
            Log.warning(
                "Protocol \"%s\" is not supported. Use original remote once to set the protocol\n",
                typeToString(m_ir_ac->next.protocol).c_str());
        }
    }
    return false;
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

        if (state.startsWith("T=")) // Temperature
        {
            m_ir_ac->next.celsius = !sub.endsWith("F"); // This way it defaults to C if no suffix
            if (sub.endsWith("C") || sub.endsWith("F"))
            {
                sub.remove(sub.length() - 1);
            }
            m_ir_ac->next.degrees = atof(sub.c_str());
        }
        else if (state.startsWith("P=")) // Power
        {
            m_ir_ac->next.power = IRac::strToBool(sub.c_str());
        }
        else if (state.startsWith("M=")) // Mode
        {
            m_ir_ac->next.mode = IRac::strToOpmode(sub.c_str());
        }
        else if (state.startsWith("H=")) // Horizontal swing
        {
            m_ir_ac->next.swingh = IRac::strToSwingH(sub.c_str());
        }
        else if (state.startsWith("V=")) // Vertical swing
        {
            m_ir_ac->next.swingv = IRac::strToSwingV(sub.c_str());
        }
        else if (state.startsWith("S=")) // Fan speed
        {
            m_ir_ac->next.fanspeed = IRac::strToFanspeed(sub.c_str());
        }
        else if (state.startsWith("B="))
        {
            m_ir_ac->next.beep = IRac::strToBool(sub.c_str());
        }
        state.remove(0, idx + 1);
    }
}

/**
 * @brief Write an AC state to an easily parseable string
 *
 * @param dest Buffer to write the string to. Should be ~100 bytes
 * @param state The AC state to print
 * @param max_len The size of the buffer
 * @return int The snprintf return.
 */
int Ir::m_state_to_str(char * dest, stdAc::state_t state, uint_fast16_t max_len)
{
    return snprintf(dest, max_len,
    "p=%d:m=%d:P=%d:M=%d:T=%.2f°%s:S=%d:V=%d:H=%d:Q=%d:t=%d:E=%d:L=%d:F=%d:C=%d:B=%d:s=%d:C=%d:",
    state.protocol,
    state.mode,
    state.power,
    state.mode,
    state.degrees,
    state.celsius ? "C" : "F",
    state.fanspeed,
    state.swingv,
    state.swingh,
    state.quiet,
    state.turbo,
    state.econo,
    state.light,
    state.filter,
    state.clean,
    state.beep,
    state.sleep,
    state.clock
    );
}

String Ir::results_as_string()
{
    return resultToHumanReadableBasic(&rx_results);
}

String Ir::results_as_decoded_string()
{
    return IRAcUtils::resultAcToString(&rx_results);
}

api_error_t  Ir::reset_protocol()
{
    File prot_f = LittleFS.open(F("IR_protocol"), "w");
    if (!prot_f)
    {
        return E_FILE_ACCESS;
    }

    prot_f.println(-1);
    return I_SUCCESS;
}

api_error_t  Ir::m_save_protocol(decode_type_t protocol)
{
    File prot_f = LittleFS.open(F("IR_protocol"), "w");
    if (!prot_f)
    {
        Log.trace("Couldn't open IR prot file");
        return E_FILE_ACCESS;
    }

    Log.trace("Save Prot: %d\n", (long)protocol);

    prot_f.println((long)protocol);
    return I_SUCCESS;
}

api_error_t  Ir::m_read_protocol(decode_type_t * protocol)
{
    if (!LittleFS.exists(F("IR_protocol")))
    {
        Log.trace("No saved IR protocol");
        return W_FILE_NOT_FOUND;
    }

    File prot_f = LittleFS.open(F("IR_protocol"), "r");
    if (!prot_f)
    {
        Log.trace("Couldn't open IR prot file");
        return E_FILE_ACCESS;
    }

    int prot = prot_f.parseInt();
    Log.trace("Read Prot: %d\n", prot);

    *protocol = (decode_type_t)prot;
    return I_SUCCESS;
}