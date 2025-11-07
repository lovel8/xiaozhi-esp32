#ifndef APPLICATION_BLE_CALLBACKS_H
#define APPLICATION_BLE_CALLBACKS_H

#include <string>
#include "ble_callbacks.h"

// 前向声明Application类，避免循环依赖
class Application;

/**
 * BLE连接回调实现类
 */
class MyBleConnectionCallbacks : public BleConnectionCallbacks {
public:
    explicit MyBleConnectionCallbacks(Application* app);
    void onConnect(const std::string& address, uint16_t connHandle, uint16_t connInterval) override;
    void onDisconnect(const std::string& address, int reason) override;

private:
    Application* m_app; // 指向Application实例的指针
};

/**
 * BLE特征回调实现类
 */
class MyBleCharacteristicCallbacks : public BleCharacteristicCallbacks {
public:
    explicit MyBleCharacteristicCallbacks(Application* app);
    void onRead(const std::string& uuid, const std::string& address) override;
    void onWrite(const std::string& uuid, const std::string& address, const std::string& value) override;
    void HandleBleDataReceived(const std::string& uuid, const std::string& address, const std::string& value);

    private:
    Application* m_app; // 指向Application实例的指针
};

#endif // APPLICATION_BLE_CALLBACKS_H