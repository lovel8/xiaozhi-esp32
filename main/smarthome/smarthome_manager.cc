#include <esp_log.h>
#include <algorithm>

#include "smarthome_manager.h"
#include "freezigbee_gateway.h"

#define TAG "SmarthomeManager"

// 初始化静态成员
SmarthomeManager* SmarthomeManager::m_instance = nullptr;

SmarthomeManager::SmarthomeManager() {
    ESP_LOGI(TAG, "SmarthomeManager instance created");
}

SmarthomeManager* SmarthomeManager::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new SmarthomeManager();
    }
    return m_instance;
}

bool SmarthomeManager::initialize() {
    ESP_LOGI(TAG, "Initializing smarthome manager");

    m_mqttClient = new Mqttclient();
    m_mqttClient->Start(HandleMqttMessage);
    
    return true;
}

Mqttclient* SmarthomeManager::getMqttClient() const {
    return m_mqttClient;
}

bool SmarthomeManager::shutdown() {
    ESP_LOGI(TAG, "Shutting down smarthome manager");

    delete m_mqttClient; 
    m_mqttClient = nullptr;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_gateways.clear();

    return true;
}

void SmarthomeManager::HandleMqttMessage(const char* topic, const char* message, int message_len) {
    ESP_LOGI(TAG, "Received MQTT message on topic %s: %.*s", topic, message_len, message);

    // 转换为字符串以便处理
    std::string topicStr(topic);
    std::string messageStr(message, message_len);
    std::string gatewayId = SmarthomeManager::getInstance()->parseGatewayIdFromTopic(topic);

    // 1. 检查是否是网关上下线消息
    // 格式: tele/网关ID/LWT
    if (topicStr.find("tele/") == 0 && topicStr.rfind("/LWT") == topicStr.length() - 4) {
        if (messageStr == "\"online\"") {
            ESP_LOGI("SmarthomeManager", "网关上线: %s", gatewayId.c_str());
            SmarthomeManager::getInstance()->addGateway(gatewayId, GatewayType::FREEZIGBEE);
        } else if (messageStr == "\"offline\"") {
            ESP_LOGI("SmarthomeManager", "网关离线: %s", gatewayId.c_str());
            SmarthomeManager::getInstance()->removeGateway(gatewayId);
        }
        
        return; // 处理完LWT消息后返回
    }

    // 将消息转发给对应的网关处理
    if (auto gateway = SmarthomeManager::getInstance()->getGateway(gatewayId)) {
        gateway->handleMqttMessage(topic, message, message_len);
    } else {
        ESP_LOGE(TAG, "No gateway found for ID: %s", gatewayId.c_str());
    }
}

// 添加网关
bool SmarthomeManager::addGateway(const std::string& gatewayId, const GatewayType gatewayType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 检查网关是否已存在
    for (const auto& gateway : m_gateways) {
        if (gateway->getGatewayId() == gatewayId) {
            ESP_LOGE(TAG, "Gateway already exists: %s", gatewayId.c_str());
            return false;
        }
    }
    
    // 创建网关实例
    auto gateway = createGatewayInstance(gatewayId, gatewayType);
    if (!gateway) {
        ESP_LOGE(TAG, "Failed to create gateway instance for type: %s", gatewayType);
        return false;
    }
    
    // 添加到网关列表
    m_gateways.push_back(gateway);
    ESP_LOGI(TAG, "Gateway added successfully: %s (Type: %s)", 
             gatewayId.c_str(), gatewayType);
    
    // 开始发现设备
    gateway->discoverDevices(getMqttClient());
    
    return true;
}

// 移除网关
bool SmarthomeManager::removeGateway(const std::string& gatewayId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 查找并移除网关
    auto it = std::find_if(m_gateways.begin(), m_gateways.end(),
                         [&gatewayId](const std::shared_ptr<Gateway>& gateway) {
                             return gateway->getGatewayId() == gatewayId;
                         });
    
    if (it != m_gateways.end()) {
        // 从列表中移除
        m_gateways.erase(it);
        ESP_LOGI(TAG, "Gateway removed successfully: %s", gatewayId.c_str());

        // 网关离线时可以触发设备移除
        (*it)->removeAllDevices();

        return true;
    } else {
        ESP_LOGE(TAG, "Gateway not found: %s", gatewayId.c_str());
        return false;
    }
}

// 根据类型创建网关实例
std::shared_ptr<Gateway> SmarthomeManager::createGatewayInstance(const std::string& gatewayId, const GatewayType gatewayType) {
    if (gatewayType == GatewayType::FREEZIGBEE) {
        return std::make_shared<FreeZigbeeGateway>(gatewayId);
    }
    
    ESP_LOGE(TAG, "Unsupported gateway type: %s", gatewayType);
    return nullptr;
}

std::string SmarthomeManager::parseGatewayIdFromTopic(const char* topic) {
    std::string topicStr(topic);
    size_t firstSlashPos = topicStr.find('/');
    
    // 检查是否有第一个斜杠
    if (firstSlashPos == std::string::npos || firstSlashPos == topicStr.length() - 1) {
        ESP_LOGE(TAG, "Invalid topic format: %s", topic);
        return "";
    }
    
    // 找到第二个斜杠
    size_t secondSlashPos = topicStr.find('/', firstSlashPos + 1);
    
    // 检查是否有第二个斜杠
    if (secondSlashPos == std::string::npos) {
        // 如果没有第二个斜杠，则认为格式是xx/gatewayId（gatewayId在末尾）
        return topicStr.substr(firstSlashPos + 1);
    }
    
    // 提取gatewayId（两个斜杠之间的部分）
    return topicStr.substr(firstSlashPos + 1, secondSlashPos - firstSlashPos - 1);
}