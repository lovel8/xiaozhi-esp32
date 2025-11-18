#include "device_socket.h"
#include <sstream>
#include <chrono>
#include <iomanip>
#include <random>
#include <iostream>
#include <cstdlib>
#include <ctime>

// 构造函数
DeviceSocket::DeviceSocket(const std::string& mid, const std::string& manufacturer, const std::string& mac, const std::string& name) 
    : Device(mid, manufacturer, mac, name){}

// 析构函数
DeviceSocket::~DeviceSocket() {}

// 继电器控制方法
bool DeviceSocket::turnOn() {
    // 根据协议规范，Power=1表示打开继电器
    std::map<std::string, std::any> cmd;
    cmd["Power"] = 1;
    
    std::string transactionId = generateTransactionId();
    bool result = sendZbSendCommand(transactionId, getMac(), 1, cmd);
    
    if (result) {
        m_relayState = true;
        std::cout << "设备 " << getMac() << " 继电器已打开" << std::endl;
    }
    
    return result;
}

bool DeviceSocket::turnOff() {
    // 根据协议规范，Power=0表示关闭继电器
    std::map<std::string, std::any> cmd;
    cmd["Power"] = 0;
    
    std::string transactionId = generateTransactionId();
    bool result = sendZbSendCommand(transactionId, getMac(), 1, cmd);
    
    if (result) {
        m_relayState = false;
        std::cout << "设备 " << getMac() << " 继电器已关闭" << std::endl;
    }
    
    return result;
}

bool DeviceSocket::toggleState() {
    if (m_relayState) {
        return turnOff();
    } else {
        return turnOn();
    }
}

bool DeviceSocket::getRelayState() const {
    return m_relayState;
}

// 断电记忆设置
bool DeviceSocket::setPowerOnMemory(uint8_t mode) {
    // 验证模式值，根据协议规范只支持0、1和255
    if (mode != 0 && mode != 1 && mode != 255) {
        std::cerr << "错误：断电记忆模式无效，只支持0、1或255" << std::endl;
        return false;
    }
    
    std::map<std::string, std::any> cmd;
    cmd["StartUpOnOff"] = mode;
    
    std::string transactionId = generateTransactionId();
    bool result = sendZbSendCommand(transactionId, getMac(), 1, cmd);
    
    if (result) {
        m_powerOnMemoryMode = mode;
        std::cout << "设备 " << getMac() << " 断电记忆模式已设置为：" << static_cast<int>(mode) << std::endl;
    }
    
    return result;
}

uint8_t DeviceSocket::getPowerOnMemory() const {
    return m_powerOnMemoryMode;
}

// 童锁功能
bool DeviceSocket::enableChildLock(bool enable) {
    std::map<std::string, std::any> cmd;
    // 根据协议规范，CLock=1表示打开童锁，CLock=0表示关闭童锁
    cmd["CLock"] = enable ? 1 : 0;
    
    std::string transactionId = generateTransactionId();
    bool result = sendZbSendCommand(transactionId, getMac(), 1, cmd);
    
    if (result) {
        m_childLockEnabled = enable;
        std::cout << "设备 " << getMac() << " 童锁已" << (enable ? "打开" : "关闭") << std::endl;
    }
    
    return result;
}

bool DeviceSocket::isChildLockEnabled() const {
    return m_childLockEnabled;
}

// 倒计时功能
bool DeviceSocket::setCountdown(uint32_t seconds) {
    std::map<std::string, std::any> cmd;
    // 根据协议规范，OnOffTime=0表示关闭倒计时，其他值表示倒计时秒数
    cmd["OnOffTime"] = seconds;
    
    std::string transactionId = generateTransactionId();
    bool result = sendZbSendCommand(transactionId, getMac(), 1, cmd);
    
    if (result) {
        m_countdownSeconds = seconds;
        m_countdownRemaining = seconds;
        if (seconds == 0) {
            std::cout << "设备 " << getMac() << " 倒计时已关闭" << std::endl;
        } else {
            std::cout << "设备 " << getMac() << " 倒计时已设置为：" << seconds << "秒" << std::endl;
        }
    }
    
    return result;
}

void DeviceSocket::cancelCountdown() {
    setCountdown(0);
}

uint32_t DeviceSocket::getCountdownRemaining() const {
    return m_countdownRemaining;
}

// 设备数据获取
float DeviceSocket::getCurrentPower() const {
    return m_currentPower;
}

float DeviceSocket::getVoltage() const {
    return m_voltage;
}

float DeviceSocket::getCurrent() const {
    return m_current;
}

float DeviceSocket::getEnergyConsumed() const {
    return m_energyConsumed;
}

int DeviceSocket::getLinkQuality() const {
    return m_linkQuality;
}

// 重写父类的sendCommand方法
bool DeviceSocket::sendCommand(const std::string& command, const std::any& params) {
    if (command == "turnOn") {
        return turnOn();
    } else if (command == "turnOff") {
        return turnOff();
    } else if (command == "toggle") {
        return toggleState();
    } else if (command == "setPowerOnMemory") {
        try {
            return setPowerOnMemory(std::any_cast<uint8_t>(params));
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：setPowerOnMemory参数类型错误" << std::endl;
            return false;
        }
    } else if (command == "enableChildLock") {
        try {
            return enableChildLock(std::any_cast<bool>(params));
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：enableChildLock参数类型错误" << std::endl;
            return false;
        }
    } else if (command == "setCountdown") {
        try {
            return setCountdown(std::any_cast<uint32_t>(params));
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：setCountdown参数类型错误" << std::endl;
            return false;
        }
    } else if (command == "cancelCountdown") {
        cancelCountdown();
        return true;
    }
    
    std::cerr << "错误：未知命令 " << command << std::endl;
    return false;
}

// 处理设备上报的数据
bool DeviceSocket::handleReportData(const std::map<std::string, std::any>& reportData) {
    bool updated = false;
    
    // 更新继电器状态 (Power)
    auto powerIt = reportData.find("Power");
    if (powerIt != reportData.end()) {
        try {
            int powerValue = std::any_cast<int>(powerIt->second);
            bool newState = (powerValue == 1);
            if (m_relayState != newState) {
                m_relayState = newState;
                updated = true;
                std::cout << "设备 " << getMac() << " 继电器状态更新为：" << (newState ? "开" : "关") << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析Power字段失败" << std::endl;
        }
    }
    
    // 更新电量数据 (CurrentSummationDelivered)
    auto energyIt = reportData.find("CurrentSummationDelivered");
    if (energyIt != reportData.end()) {
        try {
            int energyValue = std::any_cast<int>(energyIt->second);
            float newEnergy = static_cast<float>(energyValue) / 1000.0f; // 转换为kWh
            if (m_energyConsumed != newEnergy) {
                m_energyConsumed = newEnergy;
                updated = true;
                std::cout << "设备 " << getMac() << " 累计用电量更新为：" << newEnergy << "kWh" << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析CurrentSummationDelivered字段失败" << std::endl;
        }
    }
    
    // 更新电压数据 (RMSVoltage)
    auto voltageIt = reportData.find("RMSVoltage");
    if (voltageIt != reportData.end()) {
        try {
            float newVoltage = static_cast<float>(std::any_cast<int>(voltageIt->second));
            if (m_voltage != newVoltage) {
                m_voltage = newVoltage;
                updated = true;
                std::cout << "设备 " << getMac() << " 电压更新为：" << newVoltage << "V" << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析RMSVoltage字段失败" << std::endl;
        }
    }
    
    // 更新电流数据 (RMSCurrent)
    auto currentIt = reportData.find("RMSCurrent");
    if (currentIt != reportData.end()) {
        try {
            float newCurrent = static_cast<float>(std::any_cast<int>(currentIt->second)) / 1000.0f; // 转换为A
            if (m_current != newCurrent) {
                m_current = newCurrent;
                updated = true;
                std::cout << "设备 " << getMac() << " 电流更新为：" << newCurrent * 1000 << "mA" << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析RMSCurrent字段失败" << std::endl;
        }
    }
    
    // 更新功率数据 (ActivePower)
    auto activePowerIt = reportData.find("ActivePower");
    if (activePowerIt != reportData.end()) {
        try {
            float newPower = static_cast<float>(std::any_cast<int>(activePowerIt->second));
            if (m_currentPower != newPower) {
                m_currentPower = newPower;
                updated = true;
                std::cout << "设备 " << getMac() << " 功率更新为：" << newPower << "W" << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析ActivePower字段失败" << std::endl;
        }
    }
    
    // 更新信号质量 (LinkQuality)
    auto linkQualityIt = reportData.find("LinkQuality");
    if (linkQualityIt != reportData.end()) {
        try {
            int newLinkQuality = std::any_cast<int>(linkQualityIt->second);
            if (m_linkQuality != newLinkQuality) {
                m_linkQuality = newLinkQuality;
                updated = true;
                std::cout << "设备 " << getMac() << " 信号质量更新为：" << newLinkQuality << std::endl;
            }
        } catch (const std::bad_any_cast& e) {
            std::cerr << "错误：解析LinkQuality字段失败" << std::endl;
        }
    }
    
    // 如果启用了过载保护，检查是否过载
    if (m_overloadProtectionEnabled && updated && m_currentPower > m_powerThreshold) {
        if (checkOverload()) {
            std::cout << "警告：设备 " << getMac() << " 发生过载，已自动关闭继电器" << std::endl;
        }
    }
    
    return updated;
}

// 设备状态监控
bool DeviceSocket::checkOverload() {
    if (m_currentPower > m_powerThreshold) {
        // 触发过载保护，关闭继电器
        turnOff();
        return true; // 表示发生了过载
    }
    return false; // 没有过载
}

void DeviceSocket::setPowerThreshold(float threshold) {
    m_powerThreshold = threshold;
    std::cout << "设备 " << getMac() << " 功率阈值已设置为：" << threshold << "W" << std::endl;
}

bool DeviceSocket::enableOverloadProtection(bool enable) {
    m_overloadProtectionEnabled = enable;
    std::cout << "设备 " << getMac() << " 过载保护已" << (enable ? "启用" : "禁用") << std::endl;
    return true;
}

// 发送控制命令的辅助方法
bool DeviceSocket::sendZbSendCommand(const std::string& id, const std::string& device, uint8_t endpoint, const std::map<std::string, std::any>& cmd) {
    // 根据协议规范，需要发送到 cmnd/[gatewayId]/ZbSend topic
    // 构建符合协议的JSON消息
    std::cout << "发送ZbSend命令：" << std::endl;
    std::cout << "  ID: " << id << std::endl;
    std::cout << "  Device: " << device << std::endl;
    std::cout << "  Endpoint: " << static_cast<int>(endpoint) << std::endl;
    std::cout << "  Cmd: {";
    
    bool first = true;
    for (const auto& [key, value] : cmd) {
        if (!first) {
            std::cout << ", ";
        }
        first = false;
        
        std::cout << key << ": ";
        
        // 尝试将any类型转换为不同类型
        try {
            if (key == "Power" || key == "StartUpOnOff" || key == "CLock" || key == "OnOffTime") {
                // 这些字段都是整数类型
                std::cout << std::any_cast<int>(value);
            } else {
                // 其他类型暂时输出为字符串
                std::cout << "[未知类型]";
            }
        } catch (const std::bad_any_cast& e) {
            std::cout << "[类型错误]";
        }
    }
    std::cout << "}" << std::endl;
    
    // 注意：在实际应用中，这里应该调用MQTT客户端的publish方法
    // 发布到格式为 "cmnd/[gatewayId]/ZbSend" 的topic
    // 由于没有具体的MQTT客户端实现和gatewayId信息，这里仅作为示例
    
    return true; // 假设命令发送成功
}

// 生成事务ID
std::string DeviceSocket::generateTransactionId() {
    // 根据协议规范，生成10位的16进制事务ID
    // 使用当前时间戳和随机数组合生成
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    // 生成随机数部分
    int randomValue = std::rand() & 0xFFFF;
    
    // 组合时间戳和随机数
    uint64_t transactionValue = (static_cast<uint64_t>(timestamp) << 16) | (randomValue & 0xFFFF);
    
    // 格式化为10位16进制字符串
    std::stringstream ss;
    ss << std::hex << std::setw(10) << std::setfill('0') << std::uppercase << transactionValue;
    
    std::string result = ss.str();
    return result.substr(0, 10); // 确保是10位
}