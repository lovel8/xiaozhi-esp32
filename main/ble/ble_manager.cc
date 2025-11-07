#include "ble_manager.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "BleManager";

// 单例实现
BleManager& BleManager::GetInstance() {
    static BleManager instance;
    return instance;
}

// 构造函数
BleManager::BleManager() {
}

// 初始化BLE设备
bool BleManager::Initialize(const std::string& deviceName) {
    if (m_initialized) {
        ESP_LOGW(TAG, "BLE already initialized");
        return true;
    }
    
    m_deviceName = deviceName;
    
    // 初始化BLE设备
    NimBLEDevice::init(m_deviceName);
    
    // 创建服务器并设置回调
    m_server = NimBLEDevice::createServer();
    m_server->setCallbacks(new ServerCallbacksImpl(this));
    
    // 获取广播对象
    m_advertising = NimBLEDevice::getAdvertising();
    
    // 启动数据发送任务
    StartDataSendTask();
    
    SetTxPower(ESP_PWR_LVL_P9, NimBLETxPowerType::All);
    SetSecurityAuth(false, false, false);
   // 设置MTU大小为最大支持值（512字节）
    SetMTU(512);

    m_initialized = true;
    ESP_LOGI(TAG, "BLE initialized successfully");
    return true;
}

// 关闭BLE设备
void BleManager::Deinitialize() {
    if (!m_initialized) {
        return;
    }
    
    // 停止数据发送任务
    StopDataSendTask();
    
    // 停止广播
    StopAdvertising();
    
    // 清除服务和特征
    m_services.clear();
    m_characteristics.clear();
    m_characteristicCallbacks.clear();
    
    // 清空数据队列
    ClearSendQueue();
    
    // 销毁服务器
    if (m_server) {
        m_server = nullptr;
    }
    
    // 重置设备
    NimBLEDevice::deinit(true);
    
    m_initialized = false;
    m_connectedDeviceCount = 0;
    ESP_LOGI(TAG, "BLE deinitialized");
}

// 发送数据（带队列管理和重试）
bool BleManager::SendData(const std::string& characteristicUuid, const std::string& data, int maxRetries) {
    if (!m_initialized || m_characteristics.find(characteristicUuid) == m_characteristics.end()) {
        ESP_LOGE(TAG, "BLE not initialized or characteristic not found");
        return false;
    }
    
    // 检查特征是否支持通知
    if (!(m_characteristics[characteristicUuid]->getProperties() & NIMBLE_PROPERTY::NOTIFY)) {
        ESP_LOGE(TAG, "Characteristic does not support notify");
        return false;
    }
    
    // 如果没有设备连接，将数据加入队列等待连接
    if (m_connectedDeviceCount == 0) {
        ESP_LOGW(TAG, "No device connected, queuing data");
    }
    
    // 将数据加入发送队列
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_dataQueue.push({characteristicUuid, data, maxRetries, ""});
    }
    
    // 通知数据发送任务
    m_queueCondition.notify_one();
    
    return true;
}

// 发送大数据（自动分片）
bool BleManager::SendLargeData(const std::string& characteristicUuid, const std::string& data, size_t chunkSize) {
    if (!m_initialized || m_characteristics.find(characteristicUuid) == m_characteristics.end()) {
        ESP_LOGE(TAG, "BLE not initialized or characteristic not found");
        return false;
    }
    
    // 确保chunkSize不超过MTU减去ATT头大小
    chunkSize = std::min(chunkSize, static_cast<size_t>(m_currentMTU - 3));
    
    // 计算分片数量
    size_t totalChunks = (data.size() + chunkSize - 1) / chunkSize;
    
    ESP_LOGI(TAG, "Sending large data: %zu bytes in %zu chunks of %zu bytes each", 
             data.size(), totalChunks, chunkSize);
    
    // 分片发送数据
    for (size_t i = 0; i < totalChunks; i++) {
        size_t startPos = i * chunkSize;
        size_t endPos = std::min(startPos + chunkSize, data.size());
        std::string chunk = data.substr(startPos, endPos - startPos);
        
        // 发送数据块，添加分片信息（可选）
        // 这里可以添加分片序号和总片数等信息
        
        if (!SendData(characteristicUuid, chunk)) {
            ESP_LOGE(TAG, "Failed to send chunk %zu of %zu", i + 1, totalChunks);
            return false;
        }
        
        // 为了避免发送过快导致的数据丢失，添加短暂延迟
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI(TAG, "Large data transmission completed");
    return true;
}

// 设置数据传输回调
void BleManager::SetDataTransferCallback(std::function<void(const std::string&, bool, const std::string&)> callback) {
    m_dataTransferCallback = callback;
}

// 设置MTU大小
bool BleManager::SetMTU(uint16_t mtuSize) {
    if (!m_initialized) {
        ESP_LOGE(TAG, "BLE not initialized");
        return false;
    }
    
    // MTU大小必须在23-517之间
    if (mtuSize < 23 || mtuSize > 517) {
        ESP_LOGE(TAG, "Invalid MTU size: %u (must be between 23 and 517)", mtuSize);
        return false;
    }
    
    // 设置期望的MTU大小
    NimBLEDevice::setMTU(mtuSize);
    
    ESP_LOGI(TAG, "MTU size set to: %u", mtuSize);
    return true;
}

// 清除发送队列
void BleManager::ClearSendQueue(const std::string& characteristicUuid) {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    
    if (characteristicUuid.empty()) {
        // 清空所有队列
        std::queue<DataQueueItem> empty;
        std::swap(m_dataQueue, empty);
        ESP_LOGI(TAG, "All data queues cleared");
    } else {
        // 只清空特定特征的队列
        std::queue<DataQueueItem> tempQueue;
        while (!m_dataQueue.empty()) {
            DataQueueItem item = m_dataQueue.front();
            m_dataQueue.pop();
            if (item.characteristicUuid != characteristicUuid) {
                tempQueue.push(item);
            }
        }
        std::swap(m_dataQueue, tempQueue);
        ESP_LOGI(TAG, "Data queue cleared for characteristic: %s", characteristicUuid.c_str());
    }
}

// 获取队列大小
size_t BleManager::GetQueueSize(const std::string& characteristicUuid) const {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    
    if (characteristicUuid.empty()) {
        return m_dataQueue.size();
    }
    
    // 计算特定特征的队列大小
    size_t count = 0;
    std::queue<DataQueueItem> tempQueue = m_dataQueue; // 创建副本进行遍历
    
    while (!tempQueue.empty()) {
        if (tempQueue.front().characteristicUuid == characteristicUuid) {
            count++;
        }
        tempQueue.pop();
    }
    
    return count;
}

// 启动数据发送任务
void BleManager::StartDataSendTask() {
    if (m_dataSendTaskRunning) {
        ESP_LOGW(TAG, "Data send task already running");
        return;
    }
    
    m_dataSendTaskRunning = true;
    
    // 创建数据发送任务
    xTaskCreate(DataSendTask, "ble_data_send_task", 4096, this, 4, &m_dataSendTaskHandle);
    
    ESP_LOGI(TAG, "Data send task started");
}

// 停止数据发送任务
void BleManager::StopDataSendTask() {
    if (!m_dataSendTaskRunning || m_dataSendTaskHandle == nullptr) {
        return;
    }
    
    m_dataSendTaskRunning = false;
    
    // 通知任务有新数据，以便它可以检查运行标志
    m_queueCondition.notify_one();
    
    // 等待任务结束
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 如果任务仍在运行，强制删除
    if (eTaskGetState(m_dataSendTaskHandle) != eDeleted) {
        vTaskDelete(m_dataSendTaskHandle);
    }
    
    m_dataSendTaskHandle = nullptr;
    ESP_LOGI(TAG, "Data send task stopped");
}

// 数据发送任务函数
void BleManager::DataSendTask(void* param) {
    BleManager* manager = static_cast<BleManager*>(param);
    
    while (manager->m_dataSendTaskRunning) {
        DataQueueItem item;
        bool hasItem = false;
        
        // 等待有数据或任务停止
        {
            std::unique_lock<std::mutex> lock(manager->m_queueMutex);
            
            manager->m_queueCondition.wait(lock, [manager]() {
                return !manager->m_dataSendTaskRunning || !manager->m_dataQueue.empty();
            });
            
            if (!manager->m_dataSendTaskRunning) {
                break;
            }
            
            if (!manager->m_dataQueue.empty()) {
                item = manager->m_dataQueue.front();
                manager->m_dataQueue.pop();
                hasItem = true;
            }
        }
        
        // 如果有数据项需要处理
        if (hasItem && manager->m_connectedDeviceCount > 0) {
            bool success = false;
            
            // 尝试发送数据
            auto it = manager->m_characteristics.find(item.characteristicUuid);
            if (it != manager->m_characteristics.end()) {
                it->second->setValue(item.data);
                success = it->second->notify();
                
                if (success) {
                    ESP_LOGI(TAG, "Data sent successfully: %s, length: %zu", 
                             item.characteristicUuid.c_str(), item.data.length());
                } else {
                    ESP_LOGE(TAG, "Failed to send data: %s", item.characteristicUuid.c_str());
                }
            }
            
            // 如果发送失败且还有重试次数，重新加入队列
            if (!success && item.retriesLeft > 0) {
                item.retriesLeft--;
                
                std::unique_lock<std::mutex> lock(manager->m_queueMutex);
                manager->m_dataQueue.push(item);
                
                ESP_LOGW(TAG, "Retrying data send (%d retries left): %s", 
                         item.retriesLeft, item.characteristicUuid.c_str());
            } else if (manager->m_dataTransferCallback) {
                // 通知数据传输结果
                manager->m_dataTransferCallback(item.characteristicUuid, success, item.data);
            }
        } else if (hasItem) {
            // 没有设备连接，将数据重新加入队列
            std::unique_lock<std::mutex> lock(manager->m_queueMutex);
            manager->m_dataQueue.push(item);
            
            // 等待一段时间后再尝试
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
    ESP_LOGI(TAG, "Data send task exiting");
    vTaskDelete(NULL);
}

// ServerCallbacksImpl实现
BleManager::ServerCallbacksImpl::ServerCallbacksImpl(BleManager* manager) : m_manager(manager) {
}

void BleManager::ServerCallbacksImpl::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    std::string address = connInfo.getAddress().toString();
    uint16_t connHandle = connInfo.getConnHandle();
    uint16_t connInterval = connInfo.getConnInterval();
    
    m_manager->m_connectedDeviceCount++;
    
    ESP_LOGI(TAG, "Device connected - address: %s, handle: %d, interval: %dms",
             address.c_str(), connHandle, connInterval);
    
    // 更新连接参数以提高稳定性
    pServer->updateConnParams(connHandle, 24, 48, 0, 60);
    
    // 通知用户回调
    if (m_manager->m_connectionCallbacks) {
        m_manager->m_connectionCallbacks->onConnect(address, connHandle, connInterval);
    }
    
    // 有新设备连接，通知数据发送任务检查队列
    m_manager->m_queueCondition.notify_one();
}

void BleManager::ServerCallbacksImpl::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    std::string address = connInfo.getAddress().toString();
    
    if (m_manager->m_connectedDeviceCount > 0) {
        m_manager->m_connectedDeviceCount--;
    }
    
    ESP_LOGI(TAG, "Device disconnected - address: %s, reason: %d",
             address.c_str(), reason);
    
    // 重新开始广播
    NimBLEDevice::getAdvertising()->start();
    
    // 通知用户回调
    if (m_manager->m_connectionCallbacks) {
        m_manager->m_connectionCallbacks->onDisconnect(address, reason);
    }
}

void BleManager::ServerCallbacksImpl::onMTUChange(uint16_t mtu, NimBLEConnInfo& connInfo) {
    m_manager->m_currentMTU = mtu;
    
    ESP_LOGI(TAG, "MTU changed to: %u, device address: %s", 
             mtu, connInfo.getAddress().toString().c_str());
}

// CharacteristicCallbacksImpl实现
BleManager::CharacteristicCallbacksImpl::CharacteristicCallbacksImpl(BleManager* manager, const std::string& uuid) 
    : m_manager(manager), m_uuid(uuid) {
}

void BleManager::CharacteristicCallbacksImpl::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string address = connInfo.getAddress().toString();
    
    ESP_LOGI(TAG, "Characteristic read - UUID: %s, address: %s",
             m_uuid.c_str(), address.c_str());
    
    // 通知用户回调
    auto it = m_manager->m_characteristicCallbacks.find(m_uuid);
    if (it != m_manager->m_characteristicCallbacks.end() && it->second) {
        it->second->onRead(m_uuid, address);
    }
}

void BleManager::CharacteristicCallbacksImpl::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string address = connInfo.getAddress().toString();
    std::string value = pCharacteristic->getValue();
    
    ESP_LOGI(TAG, "Characteristic written - UUID: %s, address: %s, length: %zu",
             m_uuid.c_str(), address.c_str(), value.length());
    
    // 通知用户回调
    auto it = m_manager->m_characteristicCallbacks.find(m_uuid);
    if (it != m_manager->m_characteristicCallbacks.end() && it->second) {
        it->second->onWrite(m_uuid, address, value);
    }
}

// 创建BLE服务
bool BleManager::CreateService(const std::string& serviceUuid) {
    if (!m_initialized || !m_server) {
        ESP_LOGE(TAG, "BLE not initialized");
        return false;
    }
    
    if (m_services.find(serviceUuid) != m_services.end()) {
        ESP_LOGW(TAG, "Service already exists: %s", serviceUuid.c_str());
        return true;
    }
    
    NimBLEService* service = m_server->createService(serviceUuid);
    if (!service) {
        ESP_LOGE(TAG, "Failed to create service: %s", serviceUuid.c_str());
        return false;
    }
    
    m_services[serviceUuid] = service;
    ESP_LOGI(TAG, "Created service: %s", serviceUuid.c_str());
    return true;
}

// 创建BLE特征
bool BleManager::CreateCharacteristic(const std::string& characteristicUuid, uint32_t properties) {
    if (!m_initialized || m_services.empty()) {
        ESP_LOGE(TAG, "BLE not initialized or no services created");
        return false;
    }
    
    if (m_characteristics.find(characteristicUuid) != m_characteristics.end()) {
        ESP_LOGW(TAG, "Characteristic already exists: %s", characteristicUuid.c_str());
        return true;
    }
    
    // 使用第一个服务作为特征的父服务
    NimBLEService* service = m_services.begin()->second;
    NimBLECharacteristic* characteristic = service->createCharacteristic(
        characteristicUuid,
        properties
    );
    
    if (!characteristic) {
        ESP_LOGE(TAG, "Failed to create characteristic: %s", characteristicUuid.c_str());
        return false;
    }
    
    // 设置内部回调
    characteristic->setCallbacks(new CharacteristicCallbacksImpl(this, characteristicUuid));
    m_characteristics[characteristicUuid] = characteristic;
    
    ESP_LOGI(TAG, "Created characteristic: %s", characteristicUuid.c_str());
    return true;
}

// 设置特征值
bool BleManager::SetCharacteristicValue(const std::string& characteristicUuid, const std::string& value) {
    auto it = m_characteristics.find(characteristicUuid);
    if (it == m_characteristics.end()) {
        ESP_LOGE(TAG, "Characteristic not found: %s", characteristicUuid.c_str());
        return false;
    }
    
    it->second->setValue(value);
    ESP_LOGI(TAG, "Set characteristic value: %s, length: %zu", characteristicUuid.c_str(), value.length());
    return true;
}

// 发送通知
bool BleManager::NotifyCharacteristic(const std::string& characteristicUuid, const std::string& value) {
    auto it = m_characteristics.find(characteristicUuid);
    if (it == m_characteristics.end()) {
        ESP_LOGE(TAG, "Characteristic not found: %s", characteristicUuid.c_str());
        return false;
    }
    
    // 检查特征是否支持通知
    if (!(it->second->getProperties() & NIMBLE_PROPERTY::NOTIFY)) {
        ESP_LOGE(TAG, "Characteristic does not support notify: %s", characteristicUuid.c_str());
        return false;
    }
    
    // 设置值并发送通知
    it->second->setValue(value);
    bool result = it->second->notify();
    
    if (result) {
        ESP_LOGI(TAG, "Sent notification: %s, length: %zu", characteristicUuid.c_str(), value.length());
    } else {
        ESP_LOGE(TAG, "Failed to send notification: %s", characteristicUuid.c_str());
    }
    
    return result;
}

// 启动服务
bool BleManager::StartService(const std::string& serviceUuid) {
    auto it = m_services.find(serviceUuid);
    if (it == m_services.end()) {
        ESP_LOGE(TAG, "Service not found: %s", serviceUuid.c_str());
        return false;
    }
    
    it->second->start();
    ESP_LOGI(TAG, "Started service: %s", serviceUuid.c_str());
    return true;
}

// 启动广播
bool BleManager::StartAdvertising() {
    if (!m_initialized || !m_advertising || m_services.empty()) {
        ESP_LOGE(TAG, "BLE not initialized or no services available");
        return false;
    }
    
    // 设置广播数据
    NimBLEAdvertisementData advData;
    advData.setName(m_deviceName);
    
    // 添加所有服务UUID
    for (auto& service : m_services) {
        advData.addServiceUUID(service.second->getUUID());
    }
    
    // 设置扫描响应数据
    NimBLEAdvertisementData scanData;
    scanData.setName(m_deviceName, true);
    
    // 配置广播参数
    m_advertising->setAdvertisementData(advData);
    m_advertising->setScanResponseData(scanData);
    m_advertising->setMinInterval(125);
    m_advertising->setMaxInterval(250);
    m_advertising->setConnectableMode(true);
    
    // 启动广播
    bool result = m_advertising->start();
    
    if (result) {
        ESP_LOGI(TAG, "Advertising started");
    } else {
        ESP_LOGE(TAG, "Failed to start advertising");
    }
    
    return result;
}

// 停止广播
void BleManager::StopAdvertising() {
    if (m_initialized && m_advertising) {
        m_advertising->stop();
        ESP_LOGI(TAG, "Advertising stopped");
    }
}

// 设置连接回调
void BleManager::SetConnectionCallbacks(std::shared_ptr<BleConnectionCallbacks> callbacks) {
    m_connectionCallbacks = callbacks;
}

// 设置特征回调
void BleManager::SetCharacteristicCallbacks(const std::string& characteristicUuid, 
                                         std::shared_ptr<BleCharacteristicCallbacks> callbacks) {
    m_characteristicCallbacks[characteristicUuid] = callbacks;
}

// 设置发射功率
bool BleManager::SetTxPower(int powerLevel, NimBLETxPowerType powerType) {
    if (!m_initialized) {
        ESP_LOGE(TAG, "BLE not initialized");
        return false;
    }
    
    NimBLEDevice::setPower(powerLevel, powerType);
    ESP_LOGI(TAG, "Set TX power: %d, type: %d", powerLevel, static_cast<int>(powerType));
    return true;
}

// 设置安全认证
void BleManager::SetSecurityAuth(bool enableBonding, bool enableMITM, bool enableSecureConn) {
    if (!m_initialized) {
        ESP_LOGE(TAG, "BLE not initialized");
        return;
    }
    
    NimBLEDevice::setSecurityAuth(enableBonding, enableMITM, enableSecureConn);
    ESP_LOGI(TAG, "Set security: bonding=%d, MITM=%d, secureConn=%d", 
             enableBonding, enableMITM, enableSecureConn);
}
