#ifndef BLE_CALLBACKS_H
#define BLE_CALLBACKS_H

#include <string>
#include <functional>
#include "NimBLEDevice.h"

// BLE连接回调接口
class BleConnectionCallbacks {
public:
    virtual ~BleConnectionCallbacks() = default;
    virtual void onConnect(const std::string& address, uint16_t connHandle, uint16_t connInterval) = 0;
    virtual void onDisconnect(const std::string& address, int reason) = 0;
};

// BLE特征回调接口
class BleCharacteristicCallbacks {
public:
    virtual ~BleCharacteristicCallbacks() = default;
    virtual void onRead(const std::string& uuid, const std::string& address) = 0;
    virtual void onWrite(const std::string& uuid, const std::string& address, const std::string& value) = 0;
};

#endif // BLE_CALLBACKS_H