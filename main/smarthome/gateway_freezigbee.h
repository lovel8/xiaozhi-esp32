#ifndef GATEWAY_FREEZIGBEE_H
#define GATEWAY_FREEZIGBEE_H

#include "gateway.h"
#include <string>
#include <vector>

// FreeZigbeeGateway类，继承自Gateway接口
class FreeZigbeeGateway : public Gateway {
private:
    std::string m_transactionId;

    std::string generateNextTransactionId();

public:
    // 构造函数
    explicit FreeZigbeeGateway(const std::string& gatewayId);
    
    // 析构函数
    virtual ~FreeZigbeeGateway();
    
    // 处理MQTT消息
    virtual void handleMqttMessage(const std::string& topic, const std::string& message, int message_len) override;

    // 子设备发现
    virtual bool discoverDevices(Mqttclient* mqttClient) override;
    virtual bool onDiscoverDevicesResponse(const std::string& message) override;
    
    // 网关特定功能
    virtual bool sendCommand(Mqttclient* mqttClient, const std::string& command, const std::string& params) override;
    virtual bool updateFirmware(Mqttclient* mqttClient, const std::string& firmwareUrl) override;
    virtual bool reset(Mqttclient* mqttClient) override;
};

#endif // GATEWAY_FREEZIGBEE_H
