#include "mqtt_wrapper.h"

MqttWrapper::MqttWrapper(Client *client, const char *hostname,
                         const char *server, const uint16_t port,
                         const char *username, const char *key)
{
    m_hostname = hostname;
    m_username = username;

    m_mqtt = new Adafruit_MQTT_Client(client, server, port, m_username, key);
}

void MqttWrapper::add_feed(String name, feed_type type, uint8_t qos, void (*cb)(char *data, uint16_t len))
{
    add_feed(name.c_str(), type, qos, *cb);
}

void MqttWrapper::add_feed(const char *name, feed_type type, uint8_t qos, void (*cb)(char *data, uint16_t len))
{
    Feed *new_feed = new Feed;
    new_feed->name = name;

    String feed_url = String(m_username) + "/feeds/" + m_hostname + "." + name;
    if (type == PUBLISHING)
    {
        new_feed->pub_obj = new Adafruit_MQTT_Publish(m_mqtt, feed_url.c_str(), qos);
    }
    else if (type == SUBSCRIPTION)
    {
        new_feed->sub_obj = new Adafruit_MQTT_Subscribe(m_mqtt, feed_url.c_str(), qos);

        if (cb != nullptr)
        {
            new_feed->sub_obj->setCallback(cb);
        }

        m_mqtt->subscribe(new_feed->sub_obj);
    }
    m_feeds.add_feed(new_feed);
}

Feed *MqttWrapper::get_feed_by_name(String name)
{
    Log.trace("get_feed_by_name String overload\n");
    return get_feed_by_name(name.c_str());
}

Feed *MqttWrapper::get_feed_by_name(const char *name)
{
    Log.trace("get_feed_by_name const char* helper\n");
    return m_feeds.get_feed_by_name(name);
}

Feed *Feed_list::get_feed_by_name(const char *name)
{
    Log.trace("get_feed_by_name linked list\n");
    Feed *feed = m_head;
    Log.trace("Head address %X\n", feed);

    while (feed != nullptr)
    {
        Log.trace("Feed address %X\n", feed);
        if (strcmp(feed->name, name) == 0)
        {
            break;
        }
        feed = feed->next;
    }

    return feed;
}

Feed_list::Feed_list()
{
    m_head = nullptr;
    m_tail = nullptr;
}

void Feed_list::add_feed(Feed *feed)
{
    feed->next = nullptr;

    if (m_head == nullptr)
    {
        m_head = feed;
        m_tail = feed;
    }
    else
    {
        m_tail->next = feed;
        m_tail = m_tail->next;
    }
}