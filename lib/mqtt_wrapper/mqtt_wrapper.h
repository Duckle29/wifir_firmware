#pragma once

#include <Arduino.h>
#include <ArduinoLog.h>

#include <config.h>
#include <error_types.h>

// SSL
#include <ESP8266WiFi.h>
#include <CertStoreBearSSL.h>

// MQTT
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

enum feed_type
{
    SUBSCRIPTION,
    PUBLISHING
};

struct Feed
{
    const char *name;
    union
    {
        Adafruit_MQTT_Publish *pub_obj;
        Adafruit_MQTT_Subscribe *sub_obj;
    };
    Feed *next;
};

class Feed_list
{
public:
    Feed_list();
    void add_feed(Feed *);
    Feed *get_feed_by_name(const char *);

private:
    Feed *m_head,
        *m_tail;
};

class MqttWrapper
{
public:
    MqttWrapper(Client *client, const char *hostname,
                const char *server, const uint16_t port,
                const char *username, const char *key);

    void add_feed(String name, feed_type type, uint8_t qos, void (*cb)(char *data, uint16_t len) = nullptr);
    void add_feed(const char *name, feed_type type, uint8_t qos, void (*cb)(char *data, uint16_t len) = nullptr);
    Feed *get_feed_by_name(const char *);
    Feed *get_feed_by_name(String);

private:
    const char *m_hostname, *m_username;
    Feed_list m_feeds;

    Adafruit_MQTT_Client *m_mqtt;
};