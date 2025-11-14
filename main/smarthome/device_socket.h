#ifndef DEVICE_SOCKET_H
#define DEVICE_SOCKET_H

#include "device.h"
#include <string>
#include <map>

/**
 * @brief 智能插座类
 * 
 * 继承自Device基类，实现智能插座特有的功能
 * 包括开关控制、功率监控、定时功能等
 */
class DeviceSocket : public Device {
private:
    bool m_isOn;                  // 插座状态：true表示开启，false表示关闭
    float m_currentPower;         // 当前功率（瓦特）
    float m_totalEnergy;          // 总用电量（千瓦时）
    std::string m_timerSetting;   // 定时设置信息
    bool m_overloadProtection;    // 是否开启过载保护
    float m_powerThreshold;       // 功率阈值（过载保护触发值）
    std::string m_lastStatusTime; // 上次状态变化时间

public:
    /**
     * @brief 构造函数
     * 
     * @param mid 设备品类ID
     * @param manufacturer 制造商
     * @param mac 设备MAC地址
     * @param name 设备名称
     */
    DeviceSocket(const std::string& mid, const std::string& manufacturer, const std::string& mac, const std::string& name);
    
    /**
     * @brief 析构函数
     */
    virtual ~DeviceSocket();
    
    /**
     * @brief 获取插座开关状态
     * 
     * @return bool true表示开启，false表示关闭
     */
    bool isOn() const;
    
    /**
     * @brief 打开插座
     * 
     * @return bool 操作是否成功
     */
    bool turnOn();
    
    /**
     * @brief 关闭插座
     * 
     * @return bool 操作是否成功
     */
    bool turnOff();
    
    /**
     * @brief 切换插座状态
     * 
     * @return bool 操作是否成功
     */
    bool toggleState();
    
    /**
     * @brief 获取当前功率
     * 
     * @return float 当前功率值（瓦特）
     */
    float getCurrentPower() const;
    
    /**
     * @brief 获取总用电量
     * 
     * @return float 总用电量（千瓦时）
     */
    float getTotalEnergy() const;
    
    /**
     * @brief 设置功率阈值
     * 
     * @param threshold 功率阈值（瓦特）
     * @return bool 操作是否成功
     */
    bool setPowerThreshold(float threshold);
    
    /**
     * @brief 获取功率阈值
     * 
     * @return float 功率阈值（瓦特）
     */
    float getPowerThreshold() const;
    
    /**
     * @brief 启用过载保护
     * 
     * @return bool 操作是否成功
     */
    bool enableOverloadProtection();
    
    /**
     * @brief 禁用过载保护
     * 
     * @return bool 操作是否成功
     */
    bool disableOverloadProtection();
    
    /**
     * @brief 检查是否开启过载保护
     * 
     * @return bool true表示开启，false表示关闭
     */
    bool isOverloadProtectionEnabled() const;
    
    /**
     * @brief 设置定时功能
     * 
     * @param timerSetting 定时设置字符串，格式例如："on:12:00", "off:18:00", "cycle:00:30:01:30"
     * @return bool 操作是否成功
     */
    bool setTimer(const std::string& timerSetting);
    
    /**
     * @brief 获取定时设置
     * 
     * @return std::string 定时设置信息
     */
    std::string getTimer() const;
    
    /**
     * @brief 取消定时设置
     * 
     * @return bool 操作是否成功
     */
    bool cancelTimer();
    
    /**
     * @brief 重写发送命令方法
     * 
     * @param command 命令名称
     * @param params 命令参数
     * @return bool 操作是否成功
     */
    virtual bool sendCommand(const std::string& command, const std::string& params) override;
    
    /**
     * @brief 更新设备状态数据
     * 
     * @param power 当前功率
     * @param energy 累计电量
     * @return bool 操作是否成功
     */
    bool updateStatusData(float power, float energy);
    
    /**
     * @brief 获取设备类型描述
     * 
     * @return std::string 设备类型描述文本
     */
    std::string getDeviceTypeDescription() const;

private:
    /**
     * @brief 记录状态变化时间
     */
    void recordStatusChangeTime();
    
    /**
     * @brief 检查是否过载
     * 
     * @return bool true表示过载，false表示正常
     */
    bool checkOverload();
    
    /**
     * @brief 处理过载情况
     */
    void handleOverload();
};

#endif // DEVICE_SOCKET_H