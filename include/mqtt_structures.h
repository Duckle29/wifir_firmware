#pragma once
#include <Adafruit_MQTT.h>
#include <Arduino.h>

enum feed_type
{
    PUBLISH,
    SUBSCRIBE
};

enum data_type
{
    NONE,
    FLOAT,
    RXRES,
    UINT16,
    CB
};

typedef void (*cb_p)(char *, uint16_t);

struct Feed
{
    String name;
    feed_type f_type;
    uint8_t qos;

    data_type d_type;
    void *data;
    cb_p cb;

    union {
        Adafruit_MQTT_Publish *pub_obj;
        Adafruit_MQTT_Subscribe *sub_obj;
    };
};