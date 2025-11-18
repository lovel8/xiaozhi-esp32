#ifndef DEVICE_SOCKET_H
#define DEVICE_SOCKET_H

#include "device.h"
#include <cstdint>
#include <string>
#include <map>

/**
 * @brief 智能插座类
 * 
 * 继承自Device基类，实现智能插座特有的功能
 * 包括开关控制、功率监控、定时功能等
 */
class DeviceSocket : public Device {
public:
    // 构造函数
    DeviceSocket(const std::string& mid, const std::string& manufacturer, const std::string& mac, const std::string& name);
    virtual ~DeviceSocket() override;
    
    // 继电器控制方法
    bool turnOn();
    bool turnOff();
    bool toggleState();
    bool getRelayState() const; // 获取当前继电器状态
    
    // 断电记忆设置
    bool setPowerOnMemory(uint8_t mode); // mode: 0-上电关, 1-上电开, 255-记忆断电前状态
    uint8_t getPowerOnMemory() const;
    
    // 童锁功能
    bool enableChildLock(bool enable);
    bool isChildLockEnabled() const;
    
    // 倒计时功能
    bool setCountdown(uint32_t seconds); // 设置倒计时秒数，0表示关闭
    void cancelCountdown(); // 取消倒计时
    uint32_t getCountdownRemaining() const;
    
    // 设备数据获取
    float getCurrentPower() const; // 获取当前功率(W)
    float getVoltage() const; // 获取当前电压(V)
    float getCurrent() const; // 获取当前电流(A)
    float getEnergyConsumed() const; // 获取累计用电量(kWh)
    int getLinkQuality() const; // 获取信号质量
    
    // 重写父类的sendCommand方法
    virtual bool sendCommand(const std::string& command, const std::any& params) override;
    
    // 处理设备上报的数据
    bool handleReportData(const std::map<std::string, std::any>& reportData);
    
    // 设备状态监控
    bool checkOverload();
    void setPowerThreshold(float threshold);
    bool enableOverloadProtection(bool enable);
    
private:
    
    // 发送控制命令的辅助方法
    bool sendZbSendCommand(const std::string& id, const std::string& device, uint8_t endpoint, const std::map<std::string, std::any>& cmd);
    
    // 生成事务ID
    std::string generateTransactionId();
};

#endif // DEVICE_SOCKET_H