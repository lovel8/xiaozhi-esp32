#ifndef SMARTHOME_MANAGER_H
#define SMARTHOME_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>

#include "mqtt_client.h"
#include "gateway.h"
#include "device.h"

class SmarthomeManager {
private:
    static SmarthomeManager* m_instance;            // 单例实例
    std::mutex m_mutex;                            // 互斥锁，保证线程安全
    std::vector<std::shared_ptr<Gateway>> m_gateways; // 网关列表

    
    // 私有构造函数（单例模式）
    SmarthomeManager();
    
    // 内部方法
    static void HandleMqttMessage(const char* topic, const char* message, int message_len);

    bool addGateway(const std::string& gatewayId, const GatewayType gatewayTyp);     // 添加网关
    bool removeGateway(const std::string& gatewayId); // 移除网关
    std::shared_ptr<Gateway> createGatewayInstance(const std::string& gatewayId, const GatewayType gatewayType);
    std::string parseGatewayIdFromTopic(const char* topic); // 从MQTT主题中解析网关ID
    
public:
    // 获取单例实例
    static SmarthomeManager* getInstance();

    Mqttclient* getMqttClient() const;
    
    // 初始化和清理
    bool initialize();
    bool shutdown();
    
    // 网关发现功能
    bool scanGateways();                          // 扫描在线网关
    std::vector<std::shared_ptr<Gateway>> getAllGateways() const; // 获取所有网关
    std::shared_ptr<Gateway> getGateway(const std::string& gatewayId) const; // 获取特定网关
};

#endif // SMARTHOME_MANAGER_H