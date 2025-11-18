#include "mqtt_client.h"
#include "board.h"
#include "application.h"
#include "settings.h"

#include <esp_log.h>
#include <cstring>
#include <arpa/inet.h>
#include "assets/lang_config.h"

#define TAG "MQTT-Client"

// 初始化静态成员
Mqttclient* Mqttclient::m_instance = nullptr;

Mqttclient::Mqttclient() {
    ESP_LOGI(TAG, "Mqttclient instance created");

    // Initialize reconnect timer
    esp_timer_create_args_t reconnect_timer_args = {
        .callback = [](void* arg) {
            Mqttclient* client = (Mqttclient*)arg;
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateIdle) {
                ESP_LOGI(TAG, "Reconnecting to MQTT server");
                app.Schedule([client]() {
                    client->StartMqttClient(false);
                });
            }
        },
        .arg = this,
    };
    esp_timer_create(&reconnect_timer_args, &reconnect_timer_);
}

Mqttclient::~Mqttclient() {
    ESP_LOGI(TAG, "Mqttclient deinit");
    if (reconnect_timer_ != nullptr) {
        esp_timer_stop(reconnect_timer_);
        esp_timer_delete(reconnect_timer_);
    }

    mqtt_.reset();
}

Mqttclient* Mqttclient::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new Mqttclient();
    }
    return m_instance;
}

bool Mqttclient::initialize(OnMessageCallback callback) {
    return getInstance()->StartMqttClient(false, callback);
}

void Mqttclient::shutdown() {
    if (m_instance != nullptr) {
        delete m_instance;
        m_instance = nullptr;
    }
}

bool Mqttclient::StartMqttClient(bool report_error, OnMessageCallback callback) {
    if (mqtt_ != nullptr) {
        ESP_LOGW(TAG, "Mqttclient already started");
        mqtt_.reset();
    }

    Settings settings("mqtt_smarthome", false);
    auto endpoint = settings.GetString("endpoint");
    auto client_id = settings.GetString("client_id");
    auto username = settings.GetString("username");
    auto password = settings.GetString("password");
    int keepalive_interval = settings.GetInt("keepalive", 240);

    if (endpoint.empty()) {
        ESP_LOGW(TAG, "MQTT endpoint is not specified");
        if (report_error) {
            SetError(Lang::Strings::SERVER_NOT_FOUND);
        }
        return false;
    }

    auto network = Board::GetInstance().GetNetwork();
    mqtt_ = network->CreateMqtt(0);
    mqtt_->SetKeepAlive(keepalive_interval);

    mqtt_->OnDisconnected([this]() {
        ESP_LOGI(TAG, "MQTT disconnected, schedule reconnect in %d seconds", MQTT_RECONNECT_INTERVAL_MS / 1000);
        esp_timer_start_once(reconnect_timer_, MQTT_RECONNECT_INTERVAL_MS * 1000);
    });

    mqtt_->OnConnected([this]() {
        esp_timer_stop(reconnect_timer_);
    });

    mqtt_->OnMessage(callback);

    ESP_LOGI(TAG, "Connecting to endpoint %s", endpoint.c_str());
    std::string broker_address;
    int broker_port = 8883;
    size_t pos = endpoint.find(':');
    if (pos != std::string::npos) {
        broker_address = endpoint.substr(0, pos);
        broker_port = std::stoi(endpoint.substr(pos + 1));
    } else {
        broker_address = endpoint;
    }
    if (!mqtt_->Connect(broker_address, broker_port, client_id, username, password)) {
        ESP_LOGE(TAG, "Failed to connect to endpoint");
        SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return false;
    }

    ESP_LOGI(TAG, "Connected to endpoint");
    return true;
}

bool Mqttclient::Publish(const std::string& topic, const std::string& text) {
    if (topic.empty()) {
        ESP_LOGE(TAG, "Topic is empty");
        return false;
    }
    if (!mqtt_->Publish(topic, text)) {
        ESP_LOGE(TAG, "Failed to publish message: %s", text.c_str());
        SetError(Lang::Strings::SERVER_ERROR);
        return false;
    }
    return true;
}