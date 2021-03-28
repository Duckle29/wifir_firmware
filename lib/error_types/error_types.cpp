#include "error_types.h"
#include "ArduinoLog.h"

struct _errordesc errordesc[] = {
    {W_RATE_LIMIT, "Call has been rate limited"},
    {W_OLD_BASELINE, "Baseline is old"},
    {W_FILE_NOT_FOUND, "No such file"},
    {I_SUCCESS, "No error"},
    {E_SENSOR, "Error getting reading from sensor"},
    {E_FILE_ACCESS, "Error accessing file"},
    {E_CONNECTION_FAILURE, "Error establishing a connection"}};

const char *get_error_desc(error_t err)
{
    for (uint16_t i = 0; i < sizeof(errordesc) / sizeof(errordesc[0]); i++)
    {
        if (errordesc[i].code == err)
        {
            return errordesc[i].message;
        }
    }
    return nullptr;
}