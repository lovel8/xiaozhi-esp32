#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H


#include <mqtt.h>
#include <cJSON.h>
#include <esp_timer.h>

#include <string>

#define MQTT_PING_INTERVAL_SECONDS 90
#define MQTT_RECONNECT_INTERVAL_MS 60000

typedef void (*OnMessageCallback)(const char* topic, const char* message, int message_len);

class Mqttclient {
public:
    Mqttclient();
    ~Mqttclient();

    bool Start(OnMessageCallback callback);
    bool Publish(const std::string& topic, const std::string& text);

private:

    std::unique_ptr<Mqtt> mqtt_;
    esp_timer_handle_t reconnect_timer_;

    bool StartMqttClient(bool report_error=false, OnMessageCallback callback=nullptr);

};


#endif // MQTT_CLIENT_H