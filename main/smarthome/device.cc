#include "device.h"
#include <iostream>
#include <sstream>

Device::Device(const std::string& mid, const std::string& manufacturer, const std::string& mac, const std::string& name)
    : m_mid(mid), m_name(name), m_manufacturer(manufacturer), m_mac(mac)
{}

Device::~Device() {}
std::string Device::getMid() const {
    return m_mid;
}

std::string Device::getMac() const {
    return m_mac;
}

std::string Device::getName() const {
    return m_name;
}

std::string Device::getManufacturer() const {
    return m_manufacturer;
}


bool Device::setProperty(const std::string& key, const std::string& value) {
    // 使用静态成员存储属性，这样每个实例都有自己的属性映射
    // 这里使用静态变量确保每个实例有自己的属性映射
    // 注意：在实际实现中，应该在头文件中将properties声明为成员变量
    static thread_local std::map<std::string, std::map<std::string, std::string>> allDeviceProperties;
    
    // 使用MAC地址作为键来识别不同设备的属性集
    allDeviceProperties[m_mac][key] = value;
    return true;
}

std::string Device::getProperty(const std::string& key) const {
    // 获取设备属性
    static thread_local std::map<std::string, std::map<std::string, std::string>> allDeviceProperties;
    
    auto deviceIt = allDeviceProperties.find(m_mac);
    if (deviceIt != allDeviceProperties.end()) {
        auto propIt = deviceIt->second.find(key);
        if (propIt != deviceIt->second.end()) {
            return propIt->second;
        }
    }
    
    return ""; // 属性不存在时返回空字符串
}

std::map<std::string, std::string> Device::getAllProperties() const {
    // 获取设备的所有属性
    static thread_local std::map<std::string, std::map<std::string, std::string>> allDeviceProperties;
    
    auto deviceIt = allDeviceProperties.find(m_mac);
    if (deviceIt != allDeviceProperties.end()) {
        return deviceIt->second;
    }
    
    return {}; // 设备属性不存在时返回空映射
}

bool Device::sendCommand(const std::string& command, const std::string& params) {
    // 发送命令到设备
    // 这里是基础实现，具体设备类型可能需要重写此方法
    std::cout << "Sending command to device " << m_mac << ": " << command;
    if (!params.empty()) {
        std::cout << " with params: " << params;
    }
    std::cout << std::endl;
    
    // 记录命令历史
    std::string commandHistory = getProperty("commandHistory");
    if (!commandHistory.empty()) {
        commandHistory += ";";
    }
    
    std::stringstream ss;
    ss << command << "(" << params << ")";
    commandHistory += ss.str();
    
    setProperty("commandHistory", commandHistory);
    setProperty("lastCommand", ss.str());
    
    // 这里应该有实际的命令发送逻辑
    // 例如通过MQTT、Zigbee等协议发送命令
    return true; // 假设命令发送成功
}
