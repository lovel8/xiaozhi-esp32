#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <map>

class Device {
private:
    std::string m_mid;       // 设备品类ID
    std::string m_mac;           // 设备MAC地址,设备唯一标识符
    std::string m_name;           // 设备名称
    std::string m_manufacturer;   // 制造商
    
public:
    Device(const std::string& mid, const std::string& manufacturer, const std::string& mac, const std::string& name);
    virtual ~Device();
    
    // 设备基本信息
    std::string getMid() const;
    std::string getMac() const;
    std::string getName() const;
    std::string getManufacturer() const;
    
    
    // 属性管理
    bool setProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key) const;
    std::map<std::string, std::string> getAllProperties() const;
    
    // 设备控制
    virtual bool sendCommand(const std::string& command, const std::string& params);
};

#endif // DEVICE_H