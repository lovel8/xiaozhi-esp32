#include "gateway.h"
#include <algorithm>
#include <iostream>

// 构造函数实现
Gateway::Gateway(const std::string& gatewayId, const std::string& gatewayName) : m_gatewayId(gatewayId), m_gatewayName(gatewayName) {
}

// 析构函数实现
Gateway::~Gateway() {
    m_devices.clear();
}

// 获取网关ID
std::string Gateway::getGatewayId() const {
    return m_gatewayId;
}

// 添加设备
bool Gateway::addDevice(const std::shared_ptr<Device>& device) {
    if (!device) {
        return false;
    }
    
    // 检查设备是否已存在
    auto it = std::find_if(m_devices.begin(), m_devices.end(), 
        [&device](const std::shared_ptr<Device>& existingDevice) {
            return existingDevice && existingDevice->getDeviceId() == device->getDeviceId();
        });
    
    if (it != m_devices.end()) {
        // 设备已存在
        return false;
    }
    
    // 添加新设备
    m_devices.push_back(device);
    return true;
}

// 移除设备
bool Gateway::removeDevice(const std::string& deviceId) {
    auto it = std::remove_if(m_devices.begin(), m_devices.end(),
        [&deviceId](const std::shared_ptr<Device>& device) {
            return device && device->getDeviceId() == deviceId;
        });
    
    bool removed = (it != m_devices.end());
    m_devices.erase(it, m_devices.end());
    return removed;
}

// 获取设备
std::shared_ptr<Device> Gateway::getDevice(const std::string& deviceId) {
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
        [&deviceId](const std::shared_ptr<Device>& device) {
            return device && device->getDeviceId() == deviceId;
        });
    
    if (it != m_devices.end()) {
        return *it;
    }
    
    return nullptr;
}

// 获取所有设备
std::vector<std::shared_ptr<Device>> Gateway::getAllDevices() const {
    return m_devices;
}
