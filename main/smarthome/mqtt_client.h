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
private:
    // 单例实例
    static Mqttclient* m_instance;
    
    // 私有构造函数
    Mqttclient();
    
    // 私有析构函数
    ~Mqttclient();

public:
    // 获取单例实例
    static Mqttclient* getInstance();
    
    // 初始化和清理
    static bool initialize(OnMessageCallback callback);
    static void shutdown();

    // MQTT功能方法
    bool Publish(const std::string& topic, const std::string& text);

private:
    std::unique_ptr<Mqtt> mqtt_;
    esp_timer_handle_t reconnect_timer_;

    bool StartMqttClient(bool report_error=false, OnMessageCallback callback=nullptr);
    
    // 防止拷贝和赋值
    Mqttclient(const Mqttclient&) = delete;
    Mqttclient& operator=(const Mqttclient&) = delete;
};

#endif // MQTT_CLIENT_H