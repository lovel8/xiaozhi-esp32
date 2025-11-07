#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <map>
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "NimBLEDevice.h"
#include "ble_callbacks.h"

// BLE操作管理器
class BleManager {
public:
    // 获取单例实例
    static BleManager& GetInstance();
    
    // 初始化BLE设备
    bool Initialize(const std::string& deviceName = "xiaozhi");
    
    // 关闭BLE设备
    void Deinitialize();
    
    // 创建BLE服务
    bool CreateService(const std::string& serviceUuid);
    
    // 创建BLE特征
    bool CreateCharacteristic(const std::string& characteristicUuid, 
                              uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
    
    // 设置特征值
    bool SetCharacteristicValue(const std::string& characteristicUuid, const std::string& value);
    
    // 发送通知
    bool NotifyCharacteristic(const std::string& characteristicUuid, const std::string& value);
    
    // 启动服务
    bool StartService(const std::string& serviceUuid);
    
    // 启动广播
    bool StartAdvertising();
    
    // 停止广播
    void StopAdvertising();
    
    // 设置连接回调
    void SetConnectionCallbacks(std::shared_ptr<BleConnectionCallbacks> callbacks);
    
    // 设置特征回调
    void SetCharacteristicCallbacks(const std::string& characteristicUuid, 
                                  std::shared_ptr<BleCharacteristicCallbacks> callbacks);
    
    // 设置发射功率
    bool SetTxPower(int powerLevel, NimBLETxPowerType powerType = NimBLETxPowerType::All);
    
    // 设置安全认证
    void SetSecurityAuth(bool enableBonding, bool enableMITM, bool enableSecureConn);
    
    // 数据传输管理功能
    
    // 检查设备是否已连接
    bool IsDeviceConnected() const { return m_connectedDeviceCount > 0; }
    
    // 获取已连接设备数量
    int GetConnectedDeviceCount() const { return m_connectedDeviceCount; }
    
    // 发送数据（带队列管理和重试）
    bool SendData(const std::string& characteristicUuid, const std::string& data, int maxRetries = 3);
    
    // 发送大数据（自动分片）
    bool SendLargeData(const std::string& characteristicUuid, const std::string& data, size_t chunkSize = 20);
    
    // 设置数据传输回调
    void SetDataTransferCallback(std::function<void(const std::string&, bool, const std::string&)> callback);
    
    // 设置MTU大小
    bool SetMTU(uint16_t mtuSize);
    
    // 获取当前MTU大小
    uint16_t GetMTU() const { return m_currentMTU; }
    
    // 清除发送队列
    void ClearSendQueue(const std::string& characteristicUuid = "");
    
    // 获取队列大小
    size_t GetQueueSize(const std::string& characteristicUuid = "") const;

private:
    // 私有构造函数（单例模式）
    BleManager();
    
    // 禁止拷贝构造和赋值操作
    BleManager(const BleManager&) = delete;
    BleManager& operator=(const BleManager&) = delete;
    
    // 内部回调实现类
    class ServerCallbacksImpl : public NimBLEServerCallbacks {
    public:
        ServerCallbacksImpl(BleManager* manager);
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
        void onMTUChange(uint16_t mtu, NimBLEConnInfo& connInfo) override; // 添加MTU变化回调
    private:
        BleManager* m_manager;
    };
    
    class CharacteristicCallbacksImpl : public NimBLECharacteristicCallbacks {
    public:
        CharacteristicCallbacksImpl(BleManager* manager, const std::string& uuid);
        void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    private:
        BleManager* m_manager;
        std::string m_uuid;
    };
    
    // 数据传输队列项
    struct DataQueueItem {
        std::string characteristicUuid;
        std::string data;
        int retriesLeft;
        std::string deviceAddress; // 目标设备地址，空表示广播给所有设备
    };
    
    // 启动数据发送任务
    void StartDataSendTask();
    
    // 停止数据发送任务
    void StopDataSendTask();
    
    // 数据发送任务函数
    static void DataSendTask(void* param);
    
    // 成员变量
    NimBLEServer* m_server{nullptr};
    NimBLEAdvertising* m_advertising{nullptr};
    std::map<std::string, NimBLEService*> m_services;
    std::map<std::string, NimBLECharacteristic*> m_characteristics;
    std::shared_ptr<BleConnectionCallbacks> m_connectionCallbacks;
    std::map<std::string, std::shared_ptr<BleCharacteristicCallbacks>> m_characteristicCallbacks;
    bool m_initialized{false};
    std::string m_deviceName{"xiaozhi"};
    
    // 数据传输管理相关成员
    int m_connectedDeviceCount{0};
    uint16_t m_currentMTU{23}; // 默认MTU大小（23字节，其中ATT头占3字节，实际数据20字节）
    
    // 数据队列和同步原语
    std::queue<DataQueueItem> m_dataQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    
    // 数据传输回调
    std::function<void(const std::string&, bool, const std::string&)> m_dataTransferCallback;
    
    // 任务句柄
    TaskHandle_t m_dataSendTaskHandle{nullptr};
    bool m_dataSendTaskRunning{false};
};

#endif // BLE_MANAGER_H