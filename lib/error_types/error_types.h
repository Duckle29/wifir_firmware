#pragma once
#include <Arduino.h>

enum _error_codes
{
    W_FILE_EXISTS = 4,
    W_RATE_LIMIT = 3,
    W_OLD_BASELINE = 2,
    W_FILE_NOT_FOUND = 1,
    I_SUCCESS = 0,
    E_SENSOR = -1,
    E_FILE_ACCESS = -2,
    E_CONNECTION_FAILURE = -3
};

/* type to provide in your API */
typedef enum _error_codes error_t;

/* use this to provide a perror style method to help consumers out */
struct _errordesc
{
    error_t code;
    const char *message;
};

const char *get_error_desc(error_t);