# 蓝牙WiFi绑定流程设计

## 概述

本文档详细描述了嵌入式设备中蓝牙优先绑定的设计原理、实现流程和最佳实践，为开发者提供完整的绑定流程解决方案。

---

## 1. 绑定流程设计原理

### 1.1 设计原则

#### 1.1.1 蓝牙优先原则
```
设备启动 → 蓝牙广播 → 手机发现 → 蓝牙配对 → WiFi配置 → 功能使用
   ↓           ↓         ↓         ↓         ↓         ↓
 开机启动   快速发现   简单配对   建立连接   网络配置   完整功能
```

#### 1.1.2 为什么选择蓝牙优先？
- **功耗考虑**：BLE功耗极低，适合长期待机
- **连接速度**：蓝牙连接速度快，用户体验好
- **配置简单**：蓝牙配对过程简单，无需复杂设置
- **网络独立**：蓝牙配对不依赖WiFi网络
- **安全性**：蓝牙配对提供安全的初始信任关系

### 1.2 绑定流程状态机

```
┌─────────────┐   开机启动    ┌─────────────┐
│   设备关机   │ ──────────→ │   设备启动   │
└─────────────┘              └─────────────┘
                                      │
                                      ▼
                              ┌─────────────┐
                              │ 蓝牙广播模式 │
                              │             │
                              │ - 广播设备信息│
                              │ - 等待手机发现│
                              └─────────────┘
                                      │
                                      ▼
                              ┌─────────────┐
                              │ 蓝牙配对过程 │
                              │             │
                              │ - 建立安全连接│
                              │ - 交换设备信息│
                              └─────────────┘
                                      │
                                      ▼
                              ┌─────────────┐
                              │ WiFi配置过程 │
                              │             │
                              │ - 获取WiFi信息│
                              │ - 建立网络连接│
                              └─────────────┘
                                      │
                                      ▼
                              ┌─────────────┐
                              │ 功能使用模式 │
                              │             │
                              │ - 完整功能可用│
                              │ - 云端数据同步│
                              └─────────────┘
```

---

## 2. 蓝牙绑定实现

### 2.1 蓝牙广播配置

#### 2.1.1 广播数据格式
```c
// 广播数据包结构
typedef struct {
    uint8_t flags;                    // 标志位
    uint8_t complete_name[16];        // 完整设备名称
    uint8_t manufacturer_data[8];     // 厂商数据
    uint8_t service_uuid[16];         // 服务UUID
    uint8_t appearance;               // 外观值
} ble_adv_data_t;

// 广播参数配置
typedef struct {
    uint16_t adv_interval_min;        // 最小广播间隔 (20ms)
    uint16_t adv_interval_max;        // 最大广播间隔 (100ms)
    uint8_t adv_type;                 // 广播类型 (可连接的不定向)
    uint8_t own_addr_type;            // 自身地址类型 (随机地址)
    uint8_t peer_addr_type;           // 对等设备地址类型
    uint8_t peer_addr[6];             // 对等设备地址
    uint8_t adv_channel_map;          // 广播信道映射 (37,38,39)
    uint8_t filter_policy;            // 过滤策略
} ble_adv_params_t;
```

#### 2.1.2 广播数据设置
```c
// 设置广播数据
bool setup_advertising_data() {
    ble_adv_data_t adv_data = {0};
    
    // 设置标志位
    adv_data.flags = 0x06;  // LE General Discoverable + BR/EDR Not Supported
    
    // 设置设备名称
    strcpy((char*)adv_data.complete_name, "IMX6ULL_Device");
    
    // 设置厂商数据
    adv_data.manufacturer_data[0] = 0x4C;  // 厂商ID低字节
    adv_data.manufacturer_data[1] = 0x00;  // 厂商ID高字节
    adv_data.manufacturer_data[2] = 0x01;  // 设备类型
    adv_data.manufacturer_data[3] = 0x02;  // 固件版本
    
    // 设置服务UUID
    uint16_t service_uuid = 0x1800;  // Generic Access Service
    memcpy(adv_data.service_uuid, &service_uuid, 2);
    
    // 设置外观值
    adv_data.appearance = 0x03C0;  // 通用设备
    
    return ble_gap_adv_data_set(&adv_data);
}
```

### 2.2 蓝牙配对流程

#### 2.2.1 配对方法选择
```c
// 配对方法枚举
typedef enum {
    PAIRING_METHOD_JUST_WORKS = 0x00,     // 仅工作（默认）
    PAIRING_METHOD_PASSKEY_ENTRY = 0x01,  // 密码输入
    PAIRING_METHOD_OOB = 0x02,            // 带外认证
    PAIRING_METHOD_NUMERIC_COMPARISON = 0x03 // 数值比较
} pairing_method_t;

// 配对安全级别
typedef enum {
    PAIRING_SECURITY_LOW = 0,      // 低安全级别（仅工作）
    PAIRING_SECURITY_MEDIUM = 1,   // 中安全级别（密码输入）
    PAIRING_SECURITY_HIGH = 2      // 高安全级别（OOB认证）
} pairing_security_t;

// 配对配置
typedef struct {
    uint8_t io_capability;                // I/O能力
    uint8_t oob_data_flag;                // OOB数据标志
    uint8_t auth_req;                     // 认证要求
    uint8_t max_enc_key_size;             // 最大加密密钥大小
    uint8_t init_key_distribution;        // 发起方密钥分发
    uint8_t resp_key_distribution;        // 响应方密钥分发
} pairing_config_t;
```

#### 2.2.2 配对事件处理
```c
// 配对事件处理
void handle_pairing_event(ble_pairing_event_t event, void* data) {
    switch (event) {
        case BLE_PAIRING_REQUEST:
            // 收到配对请求
            handle_pairing_request((ble_pairing_request_t*)data);
            break;
            
        case BLE_PAIRING_CONFIRM:
            // 配对确认
            handle_pairing_confirm((ble_pairing_confirm_t*)data);
            break;
            
        case BLE_PAIRING_COMPLETE:
            // 配对完成
            handle_pairing_complete((ble_pairing_complete_t*)data);
            break;
            
        case BLE_PAIRING_FAILED:
            // 配对失败
            handle_pairing_failed((ble_pairing_failed_t*)data);
            break;
    }
}

// 处理配对请求
bool handle_pairing_request(ble_pairing_request_t* request) {
    // 检查配对方法
    if (request->method == PAIRING_METHOD_JUST_WORKS) {
        // 自动接受配对
        return accept_pairing();
    } else if (request->method == PAIRING_METHOD_PASSKEY_ENTRY) {
        // 显示配对码
        display_pairing_code(request->passkey);
        return true;
    }
    
    return false;
}
```

### 2.3 蓝牙服务实现

#### 2.3.1 设备配置服务
```c
// 设备配置服务UUID
#define DEVICE_CONFIG_SERVICE_UUID    0x1800
#define DEVICE_NAME_CHAR_UUID         0x2A00
#define DEVICE_MODEL_CHAR_UUID        0x2A24
#define DEVICE_SERIAL_CHAR_UUID       0x2A25
#define DEVICE_FIRMWARE_CHAR_UUID     0x2A26

// 设备配置服务
class DeviceConfigService {
private:
    uint16_t service_handle;
    uint16_t device_name_handle;
    uint16_t device_model_handle;
    uint16_t device_serial_handle;
    uint16_t device_firmware_handle;
    
public:
    bool initialize();
    bool add_service();
    bool set_device_name(const char* name);
    bool set_device_model(const char* model);
    bool set_device_serial(const char* serial);
    bool set_device_firmware(const char* firmware);
    
private:
    bool create_characteristics();
    void handle_read_request(uint16_t char_handle);
    void handle_write_request(uint16_t char_handle, uint8_t* data, uint16_t len);
};
```

#### 2.3.2 WiFi配置服务
```c
// WiFi配置服务UUID
#define WIFI_CONFIG_SERVICE_UUID      0x1801
#define WIFI_SSID_CHAR_UUID           0x2A19
#define WIFI_PASSWORD_CHAR_UUID       0x2A1A
#define WIFI_STATUS_CHAR_UUID         0x2A1B
#define WIFI_IP_CHAR_UUID             0x2A1C

// WiFi配置服务
class WiFiConfigService {
private:
    uint16_t service_handle;
    uint16_t ssid_handle;
    uint16_t password_handle;
    uint16_t status_handle;
    uint16_t ip_handle;
    
public:
    bool initialize();
    bool add_service();
    bool set_wifi_credentials(const char* ssid, const char* password);
    bool get_wifi_status(wifi_status_t* status);
    bool get_wifi_ip(uint32_t* ip);
    
private:
    bool create_characteristics();
    void handle_ssid_write(uint8_t* data, uint16_t len);
    void handle_password_write(uint8_t* data, uint16_t len);
    void update_wifi_status(wifi_status_t status);
};
```

---

## 3. WiFi配置实现

### 3.1 WiFi连接管理

#### 3.1.1 WiFi连接状态
```c
// WiFi连接状态
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,    // 未连接
    WIFI_STATE_CONNECTING,          // 连接中
    WIFI_STATE_CONNECTED,           // 已连接
    WIFI_STATE_FAILED,              // 连接失败
    WIFI_STATE_DISCONNECTING        // 断开连接中
} wifi_state_t;

// WiFi连接信息
typedef struct {
    char ssid[32];                  // 网络名称
    uint8_t security_type;          // 安全类型
    int8_t rssi;                    // 信号强度
    uint32_t ip_address;            // IP地址
    uint32_t gateway;               // 网关地址
    uint32_t netmask;               // 子网掩码
    uint32_t dns1;                  // 主DNS
    uint32_t dns2;                  // 备用DNS
} wifi_connection_info_t;
```

#### 3.1.2 WiFi连接流程
```c
// WiFi连接管理器
class WiFiConnectionManager {
private:
    wifi_state_t state;
    wifi_connection_info_t connection_info;
    uint8_t retry_count;
    uint8_t max_retries;
    
public:
    WiFiConnectionManager();
    bool initialize();
    bool connect(const char* ssid, const char* password);
    bool disconnect();
    wifi_state_t get_state();
    wifi_connection_info_t* get_connection_info();
    
private:
    void wifi_event_handler(wifi_event_t event);
    bool attempt_connection(const char* ssid, const char* password);
    void handle_connection_success();
    void handle_connection_failure();
    void reset_connection_info();
};

// WiFi连接实现
bool WiFiConnectionManager::connect(const char* ssid, const char* password) {
    if (state != WIFI_STATE_DISCONNECTED) {
        return false;
    }
    
    // 保存连接信息
    strncpy(connection_info.ssid, ssid, sizeof(connection_info.ssid) - 1);
    
    // 设置WiFi模式
    wifi_mode_t mode = WIFI_MODE_STA;
    esp_wifi_set_mode(mode);
    
    // 配置WiFi参数
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    
    // 设置WiFi配置
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        return false;
    }
    
    // 开始连接
    state = WIFI_STATE_CONNECTING;
    err = esp_wifi_connect();
    
    return (err == ESP_OK);
}
```

### 3.2 网络配置服务

#### 3.2.1 网络参数配置
```c
// 网络配置参数
typedef struct {
    uint8_t dhcp_enabled;            // DHCP启用标志
    uint32_t static_ip;              // 静态IP地址
    uint32_t static_gateway;         // 静态网关
    uint32_t static_netmask;         // 静态子网掩码
    uint32_t static_dns1;            // 静态主DNS
    uint32_t static_dns2;            // 静态备用DNS
} network_config_t;

// 网络配置服务
class NetworkConfigService {
private:
    network_config_t config;
    
public:
    bool initialize();
    bool set_dhcp_enabled(bool enabled);
    bool set_static_ip(uint32_t ip, uint32_t gateway, uint32_t netmask);
    bool set_static_dns(uint32_t dns1, uint32_t dns2);
    network_config_t* get_config();
    
private:
    bool apply_network_config();
    bool save_config_to_flash();
    bool load_config_from_flash();
};
```

---

## 4. 绑定流程实现

### 4.1 主绑定流程

#### 4.1.1 绑定状态管理
```c
// 绑定状态
typedef enum {
    BINDING_STATE_IDLE = 0,         // 空闲状态
    BINDING_STATE_BLUETOOTH_PAIRING, // 蓝牙配对中
    BINDING_STATE_WIFI_CONFIG,      // WiFi配置中
    BINDING_STATE_WIFI_CONNECTING,  // WiFi连接中
    BINDING_STATE_SECURITY_ACTIVATED, // 安全控制激活
    BINDING_STATE_COMPLETE,         // 绑定完成
    BINDING_STATE_FAILED            // 绑定失败
} binding_state_t;

// 绑定管理器
class BindingManager {
private:
    binding_state_t state;
    ble_pairing_info_t pairing_info;
    wifi_connection_info_t wifi_info;
    uint8_t retry_count;
    whitelist_manager_t* whitelist;
    security_controller_t* security;
    
public:
    BindingManager();
    bool start_binding_process();
    bool handle_bluetooth_pairing();
    bool handle_wifi_config();
    bool handle_wifi_connection();
    bool activate_security_controls();
    binding_state_t get_state();
    
private:
    void state_transition(binding_state_t new_state);
    void handle_binding_success();
    void handle_binding_failure();
    bool validate_binding_data();
    bool setup_device_whitelist();
};
```

#### 4.1.2 绑定流程实现
```c
// 启动绑定流程
bool BindingManager::start_binding_process() {
    if (state != BINDING_STATE_IDLE) {
        return false;
    }
    
    // 开始蓝牙配对
    state_transition(BINDING_STATE_BLUETOOTH_PAIRING);
    
    // 启动蓝牙广播
    if (!start_bluetooth_advertising()) {
        state_transition(BINDING_STATE_FAILED);
        return false;
    }
    
    return true;
}

// 处理蓝牙配对
bool BindingManager::handle_bluetooth_pairing() {
    if (state != BINDING_STATE_BLUETOOTH_PAIRING) {
        return false;
    }
    
    // 检查配对状态
    if (is_bluetooth_paired()) {
        // 获取配对信息
        if (get_pairing_info(&pairing_info)) {
            // 配对成功，进入WiFi配置阶段
            state_transition(BINDING_STATE_WIFI_CONFIG);
            return true;
        }
    }
    
    return false;
}

// 处理WiFi配置
bool BindingManager::handle_wifi_config() {
    if (state != BINDING_STATE_WIFI_CONFIG) {
        return false;
    }
    
    // 通过蓝牙接收WiFi配置信息
    if (receive_wifi_config(&wifi_info)) {
        // WiFi配置成功，进入连接阶段
        state_transition(BINDING_STATE_WIFI_CONNECTING);
        return true;
    }
    
    return false;
}

// 处理WiFi连接
bool BindingManager::handle_wifi_connection() {
    if (state != BINDING_STATE_WIFI_CONNECTING) {
        return false;
    }
    
    // 尝试连接WiFi
    if (connect_wifi(wifi_info.ssid, wifi_info.password)) {
        // WiFi连接成功，绑定完成
        state_transition(BINDING_STATE_COMPLETE);
        handle_binding_success();
        return true;
    } else {
        // WiFi连接失败
        retry_count++;
        if (retry_count >= MAX_RETRY_COUNT) {
            state_transition(BINDING_STATE_FAILED);
            handle_binding_failure();
            return false;
        }
    }
    
    return false;
}
```

### 4.2 错误处理和重试机制

#### 4.2.1 错误类型定义
```c
// 绑定错误类型
typedef enum {
    BINDING_ERROR_NONE = 0,         // 无错误
    BINDING_ERROR_BLUETOOTH_FAILED, // 蓝牙配对失败
    BINDING_ERROR_WIFI_CONFIG_FAILED, // WiFi配置失败
    BINDING_ERROR_WIFI_CONNECT_FAILED, // WiFi连接失败
    BINDING_ERROR_TIMEOUT,          // 超时错误
    BINDING_ERROR_USER_CANCELLED    // 用户取消
} binding_error_t;

// 错误处理
void BindingManager::handle_binding_failure() {
    binding_error_t error = get_last_error();
    
    switch (error) {
        case BINDING_ERROR_BLUETOOTH_FAILED:
            // 蓝牙配对失败，重新启动配对
            restart_bluetooth_pairing();
            break;
            
        case BINDING_ERROR_WIFI_CONFIG_FAILED:
            // WiFi配置失败，重新请求配置
            request_wifi_config_retry();
            break;
            
        case BINDING_ERROR_WIFI_CONNECT_FAILED:
            // WiFi连接失败，检查配置并重试
            if (retry_count < MAX_RETRY_COUNT) {
                retry_wifi_connection();
            } else {
                // 超过最大重试次数，返回蓝牙配对状态
                state_transition(BINDING_STATE_BLUETOOTH_PAIRING);
            }
            break;
            
        case BINDING_ERROR_TIMEOUT:
            // 超时错误，重置绑定流程
            reset_binding_process();
            break;
    }
}
```

---

## 5. 最佳实践

### 5.1 用户体验优化

#### 5.1.1 绑定流程简化
- **一键绑定**：用户只需在手机上点击"绑定设备"
- **自动发现**：设备自动广播，无需手动搜索
- **智能重试**：连接失败时自动重试，减少用户操作
- **进度提示**：显示绑定进度，让用户了解当前状态

#### 5.1.2 错误处理友好
- **清晰提示**：用简单语言说明错误原因
- **解决建议**：提供具体的解决步骤
- **自动恢复**：在可能的情况下自动恢复和重试

### 5.2 安全性考虑

#### 5.2.1 配对安全
- **加密通信**：使用LE Secure Connections
- **身份验证**：验证设备身份，防止假冒
- **密钥管理**：安全存储和管理配对密钥

#### 5.2.2 网络安全
- **WPA3支持**：使用最新的WiFi安全标准
- **证书验证**：验证网络证书的有效性
- **数据加密**：所有敏感数据都进行加密传输

### 5.3 性能优化

#### 5.3.1 功耗优化
- **智能休眠**：未使用时自动进入低功耗模式
- **连接优化**：优化蓝牙和WiFi的连接参数
- **资源管理**：合理分配系统资源

#### 5.3.2 连接速度
- **快速发现**：优化蓝牙广播参数
- **并行处理**：蓝牙和WiFi配置并行进行
- **缓存机制**：缓存常用配置，减少重复配置

---

## 6. 总结

### 6.1 设计优势

1. **用户体验好**：蓝牙配对简单快速，用户友好
2. **功耗低**：BLE功耗极低，适合长期待机
3. **可靠性高**：蓝牙+WiFi双重保障，提高连接成功率
4. **安全性强**：支持最新的安全标准和加密算法
5. **扩展性好**：可以轻松添加新的配置选项

### 6.2 应用场景

1. **智能家居设备**：智能灯泡、智能插座、智能门锁
2. **工业物联网设备**：传感器、控制器、网关设备
3. **可穿戴设备**：智能手环、智能手表、健康监测设备
4. **汽车电子设备**：车载WiFi、车载蓝牙、OBD设备

### 6.3 发展趋势

1. **AI集成**：智能化的设备发现和配置
2. **云端管理**：通过云端统一管理设备配置
3. **标准化**：行业标准的绑定流程和协议
4. **安全性增强**：更强的身份验证和加密机制

---

## 7. 安全控制实现

### 7.1 绑定后安全控制流程

#### 7.1.1 安全控制激活
```c
// 激活安全控制
bool BindingManager::activate_security_controls() {
    if (state != BINDING_STATE_WIFI_CONNECTING) {
        return false;
    }
    
    // 设置设备白名单
    if (!setup_device_whitelist()) {
        return false;
    }
    
    // 激活安全控制器
    if (!security->activate()) {
        return false;
    }
    
    // 更新广播状态为仅白名单可见
    if (!update_advertising_to_whitelist_only()) {
        return false;
    }
    
    // 进入安全控制激活状态
    state_transition(BINDING_STATE_SECURITY_ACTIVATED);
    
    return true;
}

// 设置设备白名单
bool BindingManager::setup_device_whitelist() {
    // 添加已配对设备到白名单
    if (!whitelist->add_device(pairing_info.mac_address, 
                              pairing_info.device_name, 
                              PERMISSION_OWNER)) {
        return false;
    }
    
    // 启用白名单过滤
    whitelist->enable_whitelist();
    
    // 保存白名单到Flash
    return whitelist->save_to_flash();
}

// 更新广播为仅白名单可见
bool update_advertising_to_whitelist_only() {
    // 停止当前广播
    esp_ble_gap_stop_advertising();
    
    // 配置为仅白名单可见模式
    ble_gap_adv_params_t adv_params = {0};
    adv_params.adv_interval_min = 100;  // 100ms
    adv_params.adv_interval_max = 500;  // 500ms
    adv_params.adv_type = BLE_GAP_ADV_TYPE_IND;
    
    // 设置广播过滤策略
    esp_ble_gap_set_adv_filter_policy(ESP_BLE_ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY);
    
    // 启动新的广播
    return (esp_ble_gap_start_advertising(&adv_params) == ESP_OK);
}
```

#### 7.1.2 白名单管理
```c
// 白名单管理器
class WhitelistManager {
private:
    bound_device_t* devices;
    uint8_t device_count;
    uint8_t max_devices;
    bool whitelist_enabled;
    
public:
    WhitelistManager(uint8_t max_dev);
    bool add_device(const uint8_t* mac_addr, const char* name, permission_level_t level);
    bool remove_device(const uint8_t* mac_addr);
    bool is_device_whitelisted(const uint8_t* mac_addr);
    bool enable_whitelist();
    bool disable_whitelist();
    bool save_to_flash();
    bool load_from_flash();
    
private:
    bool find_device_index(const uint8_t* mac_addr, uint8_t* index);
    void compact_whitelist();
};

// 添加设备到白名单
bool WhitelistManager::add_device(const uint8_t* mac_addr, const char* name, permission_level_t level) {
    if (device_count >= max_devices) {
        return false;
    }
    
    // 检查设备是否已存在
    uint8_t existing_index;
    if (find_device_index(mac_addr, &existing_index)) {
        // 更新现有设备
        bound_device_t* device = &devices[existing_index];
        strncpy(device->device_name, name, sizeof(device->device_name) - 1);
        device->permission_level = level;
        device->bind_time = time(NULL);
        return true;
    }
    
    // 添加新设备
    bound_device_t* new_device = &devices[device_count];
    memcpy(new_device->mac_address, mac_addr, 6);
    strncpy(new_device->device_name, name, sizeof(new_device->device_name) - 1);
    new_device->bind_time = time(NULL);
    new_device->permission_level = level;
    new_device->is_active = true;
    
    device_count++;
    return true;
}
```

#### 7.1.3 连接过滤实现
```c
// 连接过滤器
class ConnectionFilter {
private:
    WhitelistManager* whitelist;
    
public:
    ConnectionFilter(WhitelistManager* wl_manager);
    bool is_connection_allowed(const uint8_t* mac_addr);
    bool filter_connection_request(const uint8_t* mac_addr);
    
private:
    bool apply_whitelist_filter(const uint8_t* mac_addr);
    bool apply_rate_limiting(const uint8_t* mac_addr);
};

// 检查连接是否允许
bool ConnectionFilter::is_connection_allowed(const uint8_t* mac_addr) {
    // 1. 应用白名单过滤
    if (!apply_whitelist_filter(mac_addr)) {
        return false;
    }
    
    // 2. 应用速率限制
    if (!apply_rate_limiting(mac_addr)) {
        return false;
    }
    
    return true;
}

// 应用白名单过滤
bool ConnectionFilter::apply_whitelist_filter(const uint8_t* mac_addr) {
    if (!whitelist || !whitelist->is_whitelist_enabled()) {
        return true; // 无白名单时允许所有连接
    }
    
    return whitelist->is_device_whitelisted(mac_addr);
}
```

### 7.2 广播控制策略

#### 7.2.1 动态广播控制
```c
// 广播控制器
class AdvertisingController {
private:
    advertising_state_t current_state;
    WhitelistManager* whitelist;
    
public:
    AdvertisingController(WhitelistManager* wl_manager);
    bool set_advertising_state(advertising_state_t state);
    bool update_advertising_based_on_whitelist();
    bool start_whitelist_only_advertising();
    
private:
    void configure_advertising_parameters(advertising_state_t state);
    void update_advertising_data();
};

// 基于白名单更新广播
bool AdvertisingController::update_advertising_based_on_whitelist() {
    if (!whitelist || whitelist->get_device_count() == 0) {
        // 无绑定设备时，完全可见
        return set_advertising_state(ADV_STATE_VISIBLE);
    } else {
        // 有绑定设备时，仅白名单可见
        return set_advertising_state(ADV_STATE_WHITELIST_ONLY);
    }
}

// 设置仅白名单可见广播
bool AdvertisingController::set_advertising_state(advertising_state_t state) {
    current_state = state;
    
    switch (state) {
        case ADV_STATE_VISIBLE:
            // 完全可见：快速广播，所有设备可见
            configure_advertising_parameters(20, 100, BLE_GAP_ADV_TYPE_IND);
            break;
            
        case ADV_STATE_WHITELIST_ONLY:
            // 仅白名单可见：慢速广播，减少被发现概率
            configure_advertising_parameters(100, 500, BLE_GAP_ADV_TYPE_IND);
            break;
            
        case ADV_STATE_HIDDEN:
            // 完全隐藏：极慢广播或停止广播
            configure_advertising_parameters(1000, 2000, BLE_GAP_ADV_TYPE_IND);
            break;
    }
    
    return true;
}
```

### 7.3 安全状态监控

#### 7.3.1 安全状态管理
```c
// 安全状态
typedef enum {
    SECURITY_STATE_DISABLED = 0,    // 安全控制禁用
    SECURITY_STATE_ENABLED,         // 安全控制启用
    SECURITY_STATE_LOCKDOWN,        // 安全锁定模式
    SECURITY_STATE_MAINTENANCE      // 维护模式
} security_state_t;

// 安全控制器
class SecurityController {
private:
    security_state_t state;
    WhitelistManager* whitelist;
    ConnectionFilter* filter;
    AdvertisingController* advertising;
    
public:
    SecurityController(WhitelistManager* wl, ConnectionFilter* cf, AdvertisingController* ac);
    bool activate();
    bool deactivate();
    bool enable_lockdown_mode();
    bool disable_lockdown_mode();
    security_state_t get_state();
    
private:
    void update_security_status();
    void log_security_event(const char* event);
};
```

### 7.4 安全配置示例

#### 7.4.1 完整安全配置
```c
// 安全配置结构
typedef struct {
    bool whitelist_enabled;         // 白名单启用
    uint8_t max_bound_devices;      // 最大绑定设备数
    bool rate_limiting_enabled;     // 速率限制启用
    uint32_t max_connection_attempts; // 最大连接尝试次数
    uint32_t rate_limit_window;     // 速率限制时间窗口
    bool anomaly_detection_enabled; // 异常检测启用
} security_config_t;

// 默认安全配置
security_config_t default_security_config = {
    .whitelist_enabled = true,
    .max_bound_devices = 10,
    .rate_limiting_enabled = true,
    .max_connection_attempts = 5,
    .rate_limit_window = 300,      // 5分钟
    .anomaly_detection_enabled = true
};

// 应用安全配置
bool apply_security_config(const security_config_t* config) {
    // 1. 配置白名单
    whitelist->set_max_devices(config->max_bound_devices);
    whitelist->enable_whitelist();
    
    // 2. 配置连接过滤
    filter->enable_rate_limiting(config->max_connection_attempts, config->rate_limit_window);
    
    // 3. 配置异常检测
    if (config->anomaly_detection_enabled) {
        enable_anomaly_detection();
    }
    
    // 4. 更新广播状态
    advertising->update_advertising_based_on_whitelist();
    
    return true;
}
```

---

*文档版本：v1.0*
*最后更新时间：2024年12月*
*维护者：Linux Cool Team*