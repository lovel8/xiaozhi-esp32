#ifndef GATEWAY_H
#define GATEWAY_H

#include <string>
#include <vector>
#include <memory>
#include "device.h"
#include "mqtt_client.h"

enum class GatewayType {
    FREEZIGBEE,
    UNKNOWN
};

class Gateway {
private:
    std::string m_gatewayName;
    std::string m_gatewayId;
    std::vector<std::shared_ptr<Device>> m_devices;
    
public:
    Gateway(const std::string& gatewayId, const std::string& gatewayName = "Gateway");
    virtual ~Gateway();
    
    // 获取网关ID
    std::string getGatewayId() const;

    // 子设备管理
    bool addDevice(const std::shared_ptr<Device>& device);
    bool removeDevice(const std::string& deviceId);
    std::shared_ptr<Device> getDevice(const std::string& deviceId);
    std::vector<std::shared_ptr<Device>> getAllDevices() const;
    

    // 处理MQTT消息
    virtual void handleMqttMessage(const std::string& topic, const std::string& message, int message_len) = 0;

    // 子设备发现
    virtual bool discoverDevices(Mqttclient* mqttClient) = 0;
    virtual bool onDiscoverDevicesResponse(const std::string& message) = 0;

    // 网关特定功能
    virtual bool sendCommand(Mqttclient* mqttClient, const std::string& command, const std::string& params) = 0;
    virtual bool updateFirmware(Mqttclient* mqttClient, const std::string& firmwareUrl) = 0;
    virtual bool reset(Mqttclient* mqttClient) = 0;
};

#endif // GATEWAY_H