#include "application_ble_callbacks.h"
#include "application.h" // 包含Application类的定义
#include <esp_log.h>

static const char* TAG = "ApplicationBleCallbacks";

/**
 * MyBleConnectionCallbacks实现
 */
MyBleConnectionCallbacks::MyBleConnectionCallbacks(Application* app) : m_app(app) {
    // 构造函数实现
}

void MyBleConnectionCallbacks::onConnect(const std::string& address, uint16_t connHandle, uint16_t connInterval) {
    ESP_LOGI(TAG, "BLE device connected: %s, handle: %d, interval: %dms", 
             address.c_str(), connHandle, connInterval);
    
    // 在这里添加连接处理逻辑，可以调用Application的方法
    // 例如：m_app->HandleBleConnect(address, connHandle, connInterval);
}

void MyBleConnectionCallbacks::onDisconnect(const std::string& address, int reason) {
    ESP_LOGI(TAG, "BLE device disconnected: %s, reason: %d", 
             address.c_str(), reason);
    
    // 在这里添加断开连接处理逻辑，可以调用Application的方法
    // 例如：m_app->HandleBleDisconnect(address, reason);
}

/**
 * MyBleCharacteristicCallbacks实现
 */
MyBleCharacteristicCallbacks::MyBleCharacteristicCallbacks(Application* app) : m_app(app) {
    // 构造函数实现
}

void MyBleCharacteristicCallbacks::onRead(const std::string& uuid, const std::string& address) {
    ESP_LOGI(TAG, "Characteristic read: UUID=%s, address=%s", 
             uuid.c_str(), address.c_str());
    
    // 在这里添加特征读取处理逻辑，可以调用Application的方法
    // 例如：m_app->HandleBleCharacteristicRead(uuid, address);
}

void MyBleCharacteristicCallbacks::onWrite(const std::string& uuid, const std::string& address, const std::string& value) {
    ESP_LOGI(TAG, "Characteristic written: UUID=%s, address=%s, length=%zu", 
             uuid.c_str(), address.c_str(), value.length());
    
    // 在这里添加特征写入处理逻辑，根据之前的错误信息，应该调用HandleBleDataReceived方法
    HandleBleDataReceived(uuid, address, value);
}

// HandleBleDataReceived方法的直接实现
void MyBleCharacteristicCallbacks::HandleBleDataReceived(const std::string& uuid, const std::string& address, const std::string& value) {
    ESP_LOGI(TAG, "Processing BLE data - UUID: %s, Address: %s, Value length: %zu", 
             uuid.c_str(), address.c_str(), value.length());
    
    // 这里添加对接收数据的处理逻辑
    // 例如解析JSON格式的数据、执行相应的命令等
    if (m_app) {
        // 示例：记录接收到的数据
        ESP_LOGI(TAG, "Received BLE data: %s", value.c_str());
        
        // 假设我们要处理特定UUID的数据
        if (uuid == "19B10001-E8F2-537E-4F6C-D104768A1214") {
            // 处理特定特征值的数据
            ESP_LOGI(TAG, "Processing data for characteristic 19B10001-E8F2-537E-4F6C-D104768A1214");
            
            // 您可以根据实际需求添加具体的数据处理逻辑
        }
    }
}