# WiFi/蓝牙基础架构功能拆解

## 概述

本文档详细拆解"第一阶段：基础架构 (4周)"的功能模块，包括WiFi/蓝牙驱动开发、用户APP开发、网络配置等具体实现细节。

---

## 1. WiFi驱动开发详细拆解

### 1.1 WiFi驱动架构设计

#### 1.1.1 驱动整体架构
```c
// WiFi驱动整体架构
struct wifi_driver_ops {
    // 基础操作
    int (*probe)(struct wifi_device *dev);
    int (*remove)(struct wifi_device *dev);
    int (*suspend)(struct wifi_device *dev);
    int (*resume)(struct wifi_device *dev);
    
    // WiFi功能操作
    int (*init)(struct wifi_device *dev);
    int (*deinit)(struct wifi_device *dev);
    int (*reset)(struct wifi_device *dev);
    
    // 网络管理
    int (*scan_start)(struct wifi_device *dev);
    int (*scan_stop)(struct wifi_device *dev);
    int (*connect)(struct wifi_device *dev, struct wifi_connect_params *params);
    int (*disconnect)(struct wifi_device *dev);
    
    // 状态查询
    int (*get_status)(struct wifi_device *dev, struct wifi_status *status);
    int (*get_signal_strength)(struct wifi_device *dev, int *strength);
    int (*get_connection_info)(struct wifi_device *dev, struct wifi_connection_info *info);
};

// WiFi设备结构
struct wifi_device {
    struct device *dev;
    struct wifi_driver_ops *ops;
    struct wifi_status status;
    struct wifi_connection_info conn_info;
    struct mutex lock;
    struct workqueue_struct *workqueue;
    struct delayed_work scan_work;
    struct delayed_work status_work;
    
    // 硬件相关
    void *private_data;
    struct spi_device *spi;
    struct platform_device *pdev;
    
    // 网络相关
    struct net_device *ndev;
    struct wireless_dev *wdev;
};
```

#### 1.1.2 驱动初始化流程
```c
// WiFi驱动初始化
static int wifi_driver_probe(struct platform_device *pdev)
{
    struct wifi_device *wdev;
    struct wifi_platform_data *pdata;
    int ret;
    
    // 1. 分配设备结构
    wdev = devm_kzalloc(&pdev->dev, sizeof(*wdev), GFP_KERNEL);
    if (!wdev)
        return -ENOMEM;
    
    // 2. 获取平台数据
    pdata = dev_get_platdata(&pdev->dev);
    if (!pdata) {
        dev_err(&pdev->dev, "No platform data\n");
        return -EINVAL;
    }
    
    // 3. 初始化设备
    wdev->dev = &pdev->dev;
    wdev->pdev = pdev;
    wdev->ops = &wifi_driver_ops;
    
    // 4. 初始化互斥锁
    mutex_init(&wdev->lock);
    
    // 5. 创建工作队列
    wdev->workqueue = create_singlethread_workqueue("wifi_workqueue");
    if (!wdev->workqueue) {
        ret = -ENOMEM;
        goto err_workqueue;
    }
    
    // 6. 初始化工作项
    INIT_DELAYED_WORK(&wdev->scan_work, wifi_scan_work);
    INIT_DELAYED_WORK(&wdev->status_work, wifi_status_work);
    
    // 7. 注册网络设备
    ret = wifi_register_netdev(wdev);
    if (ret)
        goto err_netdev;
    
    // 8. 注册无线设备
    ret = wifi_register_wireless_dev(wdev);
    if (ret)
        goto err_wireless;
    
    // 9. 设置平台数据
    platform_set_drvdata(pdev, wdev);
    
    // 10. 初始化硬件
    ret = wdev->ops->init(wdev);
    if (ret)
        goto err_init;
    
    dev_info(&pdev->dev, "WiFi driver probed successfully\n");
    return 0;
    
err_init:
    wifi_unregister_wireless_dev(wdev);
err_wireless:
    wifi_unregister_netdev(wdev);
err_netdev:
    destroy_workqueue(wdev->workqueue);
err_workqueue:
    mutex_destroy(&wdev->lock);
    return ret;
}
```

### 1.2 WiFi扫描功能实现

#### 1.2.1 扫描工作函数
```c
// WiFi扫描工作函数
static void wifi_scan_work(struct work_struct *work)
{
    struct wifi_device *wdev = container_of(to_delayed_work(work),
                                           struct wifi_device, scan_work);
    struct wifi_scan_result *results;
    int num_results;
    
    mutex_lock(&wdev->lock);
    
    // 1. 启动硬件扫描
    if (wdev->ops->scan_start(wdev) < 0) {
        dev_err(wdev->dev, "Failed to start scan\n");
        mutex_unlock(&wdev->lock);
        return;
    }
    
    // 2. 等待扫描完成
    msleep(5000); // 等待5秒
    
    // 3. 获取扫描结果
    num_results = wifi_get_scan_results(wdev, &results);
    if (num_results > 0) {
        // 4. 处理扫描结果
        wifi_process_scan_results(wdev, results, num_results);
        kfree(results);
    }
    
    // 5. 停止扫描
    wdev->ops->scan_stop(wdev);
    
    mutex_unlock(&wdev->lock);
    
    // 6. 发送扫描完成事件
    wifi_send_scan_complete_event(wdev);
}
```

#### 1.2.2 扫描结果处理
```c
// 扫描结果处理
static void wifi_process_scan_results(struct wifi_device *wdev,
                                     struct wifi_scan_result *results,
                                     int num_results)
{
    int i;
    struct wifi_network *network;
    
    for (i = 0; i < num_results; i++) {
        network = &results[i];
        
        // 过滤信号强度
        if (network->signal_strength < -80)
            continue;
            
        // 过滤加密类型
        if (network->security == WIFI_SECURITY_OPEN)
            continue;
            
        // 添加到网络列表
        wifi_add_network_to_list(wdev, network);
    }
}
```

### 1.3 WiFi连接管理

#### 1.3.1 连接参数结构
```c
// WiFi连接参数
struct wifi_connect_params {
    char ssid[IEEE80211_MAX_SSID_LEN];
    char password[64];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int channel;
    bool hidden;
};
```

#### 1.3.2 连接实现
```c
// WiFi连接实现
static int wifi_connect_network(struct wifi_device *wdev,
                               struct wifi_connect_params *params)
{
    int ret;
    struct wifi_connection_info *conn_info;
    
    mutex_lock(&wdev->lock);
    
    // 1. 检查设备状态
    if (wdev->status.state != WIFI_STATE_READY) {
        ret = -EBUSY;
        goto out;
    }
    
    // 2. 验证连接参数
    ret = wifi_validate_connect_params(params);
    if (ret < 0)
        goto out;
    
    // 3. 设置连接状态
    wdev->status.state = WIFI_STATE_CONNECTING;
    
    // 4. 调用硬件连接
    ret = wdev->ops->connect(wdev, params);
    if (ret < 0) {
        wdev->status.state = WIFI_STATE_READY;
        goto out;
    }
    
    // 5. 等待连接完成
    ret = wait_for_completion_timeout(&wdev->connect_completion, 30000);
    if (ret == 0) {
        // 连接超时
        wdev->ops->disconnect(wdev);
        wdev->status.state = WIFI_STATE_READY;
        ret = -ETIMEDOUT;
        goto out;
    }
    
    // 6. 更新连接信息
    conn_info = &wdev->conn_info;
    strcpy(conn_info->ssid, params->ssid);
    conn_info->security = params->security;
    conn_info->channel = params->channel;
    conn_info->connected = true;
    
    // 7. 更新设备状态
    wdev->status.state = WIFI_STATE_CONNECTED;
    
    // 8. 发送连接成功事件
    wifi_send_connection_event(wdev, WIFI_EVENT_CONNECTED);
    
    ret = 0;
    
out:
    mutex_unlock(&wdev->lock);
    return ret;
}
```

---

## 2. 蓝牙驱动开发详细拆解

### 2.1 蓝牙驱动架构设计

#### 2.1.1 驱动操作结构
```c
// 蓝牙驱动操作结构
struct bluetooth_driver_ops {
    // 基础操作
    int (*probe)(struct bluetooth_device *bdev);
    int (*remove)(struct bluetooth_device *bdev);
    int (*suspend)(struct bluetooth_device *bdev);
    int (*resume)(struct bluetooth_device *bdev);
    
    // 蓝牙功能操作
    int (*init)(struct bluetooth_device *bdev);
    int (*deinit)(struct bluetooth_device *bdev);
    int (*reset)(struct bluetooth_device *bdev);
    
    // 经典蓝牙操作
    int (*classic_scan_start)(struct bluetooth_device *bdev);
    int (*classic_scan_stop)(struct bluetooth_device *bdev);
    int (*classic_connect)(struct bluetooth_device *bdev, bdaddr_t *addr);
    int (*classic_disconnect)(struct bluetooth_device *bdev);
    
    // BLE操作
    int (*ble_scan_start)(struct bluetooth_device *bdev);
    int (*ble_scan_stop)(struct bluetooth_device *bdev);
    int (*ble_connect)(struct bluetooth_device *bdev, bdaddr_t *addr);
    int (*ble_disconnect)(struct bluetooth_device *bdev);
    
    // GATT服务操作
    int (*gatt_service_add)(struct bluetooth_device *bdev, struct gatt_service *service);
    int (*gatt_service_remove)(struct bluetooth_device *bdev, struct gatt_service *service);
};
```

#### 2.1.2 蓝牙设备结构
```c
// 蓝牙设备结构
struct bluetooth_device {
    struct device *dev;
    struct bluetooth_driver_ops *ops;
    struct bluetooth_status status;
    struct bluetooth_connection_info conn_info;
    struct mutex lock;
    struct workqueue_struct *workqueue;
    struct delayed_work scan_work;
    struct delayed_work status_work;
    
    // 硬件相关
    void *private_data;
    struct spi_device *spi;
    struct platform_device *pdev;
    
    // 蓝牙相关
    struct hci_dev *hdev;
    struct l2cap_conn *l2cap_conn;
    
    // GATT服务
    struct list_head gatt_services;
    struct mutex gatt_lock;
};
```

### 2.2 BLE扫描功能实现

#### 2.2.1 BLE扫描工作函数
```c
// BLE扫描工作函数
static void ble_scan_work(struct work_struct *work)
{
    struct bluetooth_device *bdev = container_of(to_delayed_work(work),
                                                struct bluetooth_device, scan_work);
    struct ble_scan_result *results;
    int num_results;
    
    mutex_lock(&bdev->lock);
    
    // 1. 启动BLE扫描
    if (bdev->ops->ble_scan_start(bdev) < 0) {
        dev_err(bdev->dev, "Failed to start BLE scan\n");
        mutex_unlock(&bdev->lock);
        return;
    }
    
    // 2. 等待扫描完成
    msleep(3000); // 等待3秒
    
    // 3. 获取扫描结果
    num_results = ble_get_scan_results(bdev, &results);
    if (num_results > 0) {
        // 4. 处理扫描结果
        ble_process_scan_results(bdev, results, num_results);
        kfree(results);
    }
    
    // 5. 停止扫描
    bdev->ops->ble_scan_stop(bdev);
    
    mutex_unlock(&bdev->lock);
    
    // 6. 发送扫描完成事件
    ble_send_scan_complete_event(bdev);
}
```

#### 2.2.2 BLE扫描结果处理
```c
// BLE扫描结果处理
static void ble_process_scan_results(struct bluetooth_device *bdev,
                                    struct ble_scan_result *results,
                                    int num_results)
{
    int i;
    struct ble_device *device;
    
    for (i = 0; i < num_results; i++) {
        device = &results[i];
        
        // 过滤信号强度
        if (device->rssi < -80)
            continue;
            
        // 过滤设备类型
        if (device->appearance != BLE_APPEARANCE_GENERIC_SENSOR)
            continue;
            
        // 添加到设备列表
        ble_add_device_to_list(bdev, device);
    }
}
```

### 2.3 GATT服务实现

#### 2.3.1 GATT服务结构
```c
// GATT服务结构
struct gatt_service {
    struct list_head list;
    uuid_t uuid;
    bool primary;
    struct list_head characteristics;
    struct gatt_service_ops *ops;
    void *private_data;
};

// GATT特征结构
struct gatt_characteristic {
    struct list_head list;
    uuid_t uuid;
    uint8_t properties;
    uint8_t permissions;
    uint16_t value_handle;
    uint16_t ccc_handle;
    struct gatt_char_ops *ops;
    void *private_data;
};
```

#### 2.3.2 GATT服务添加
```c
// GATT服务添加
static int gatt_service_add(struct bluetooth_device *bdev,
                           struct gatt_service *service)
{
    int ret;
    
    mutex_lock(&bdev->gatt_lock);
    
    // 1. 验证服务参数
    ret = gatt_validate_service(service);
    if (ret < 0)
        goto out;
    
    // 2. 添加到服务列表
    list_add_tail(&service->list, &bdev->gatt_services);
    
    // 3. 注册到蓝牙协议栈
    ret = gatt_register_service(bdev, service);
    if (ret < 0) {
        list_del(&service->list);
        goto out;
    }
    
    // 4. 初始化服务
    if (service->ops && service->ops->init) {
        ret = service->ops->init(service);
        if (ret < 0) {
            gatt_unregister_service(bdev, service);
            list_del(&service->list);
            goto out;
        }
    }
    
    ret = 0;
    
out:
    mutex_unlock(&bdev->gatt_lock);
    return ret;
}
```

---

## 3. 用户空间APP开发详细拆解

### 3.1 WiFi管理APP架构

#### 3.1.1 APP主结构
```c
// WiFi管理APP主结构
struct wifi_manager_app {
    struct wifi_device *wifi_dev;
    struct wifi_network_list *network_list;
    struct wifi_connection *current_connection;
    struct wifi_config *config;
    
    // 用户界面
    struct wifi_ui *ui;
    struct wifi_settings *settings;
    
    // 事件处理
    struct wifi_event_handler *event_handler;
    struct wifi_callback *callback;
    
    // 数据存储
    struct wifi_database *database;
    struct wifi_logger *logger;
};
```

#### 3.1.2 WiFi网络列表管理
```c
// WiFi网络列表管理
struct wifi_network_list {
    struct list_head networks;
    int count;
    struct mutex lock;
    
    // 扫描状态
    bool scanning;
    struct completion scan_completion;
    
    // 网络过滤
    struct wifi_filter *filter;
};

// WiFi网络信息
struct wifi_network {
    struct list_head list;
    char ssid[IEEE80211_MAX_SSID_LEN];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int signal_strength;
    int channel;
    int frequency;
    bool hidden;
    bool connected;
    
    // 连接历史
    time_t last_connected;
    int connection_count;
    int success_rate;
};
```

### 3.2 WiFi网络扫描APP实现

#### 3.2.1 WiFi扫描APP实现
```c
// WiFi扫描APP实现
static int wifi_scan_app_start(struct wifi_manager_app *app)
{
    int ret;
    struct wifi_network_list *network_list = app->network_list;
    
    // 1. 检查扫描状态
    if (network_list->scanning) {
        return -EBUSY;
    }
    
    // 2. 设置扫描状态
    network_list->scanning = true;
    init_completion(&network_list->scan_completion);
    
    // 3. 清空网络列表
    wifi_clear_network_list(network_list);
    
    // 4. 启动硬件扫描
    ret = wifi_start_scan(app->wifi_dev);
    if (ret < 0) {
        network_list->scanning = false;
        return ret;
    }
    
    // 5. 启动扫描超时定时器
    mod_timer(&app->scan_timer, jiffies + msecs_to_jiffies(30000));
    
    // 6. 发送扫描开始事件
    wifi_send_ui_event(app, WIFI_UI_EVENT_SCAN_STARTED);
    
    return 0;
}
```

#### 3.2.2 WiFi扫描结果处理APP
```c
// WiFi扫描结果处理APP
static int wifi_scan_app_process_results(struct wifi_manager_app *app,
                                        struct wifi_scan_result *results,
                                        int num_results)
{
    int i;
    struct wifi_network *network;
    struct wifi_network_list *network_list = app->network_list;
    
    mutex_lock(&network_list->lock);
    
    for (i = 0; i < num_results; i++) {
        // 1. 创建网络对象
        network = wifi_create_network(&results[i]);
        if (!network)
            continue;
        
        // 2. 应用过滤器
        if (wifi_apply_filter(network, network_list->filter) < 0) {
            wifi_destroy_network(network);
            continue;
        }
        
        // 3. 添加到网络列表
        list_add_tail(&network->list, &network_list->networks);
        network_list->count++;
        
        // 4. 更新UI
        wifi_update_ui_network(app, network);
    }
    
    mutex_unlock(&network_list->lock);
    
    // 5. 发送扫描完成事件
    wifi_send_ui_event(app, WIFI_UI_EVENT_SCAN_COMPLETED);
    
    return 0;
}
```

### 3.3 蓝牙设备管理APP架构

#### 3.3.1 蓝牙管理APP主结构
```c
// 蓝牙管理APP主结构
struct bluetooth_manager_app {
    struct bluetooth_device *bt_dev;
    struct bluetooth_device_list *device_list;
    struct bluetooth_connection *current_connection;
    struct bluetooth_config *config;
    
    // 用户界面
    struct bluetooth_ui *ui;
    struct bluetooth_settings *settings;
    
    // 事件处理
    struct bluetooth_event_handler *event_handler;
    struct bluetooth_callback *callback;
    
    // 数据存储
    struct bluetooth_database *database;
    struct bluetooth_logger *logger;
    
    // GATT服务管理
    struct gatt_service_manager *gatt_manager;
};
```

#### 3.3.2 蓝牙设备列表管理
```c
// 蓝牙设备列表管理
struct bluetooth_device_list {
    struct list_head devices;
    int count;
    struct mutex lock;
    
    // 扫描状态
    bool scanning;
    struct completion scan_completion;
    
    // 设备过滤
    struct bluetooth_filter *filter;
    
    // 设备分类
    struct list_head classic_devices;
    struct list_head ble_devices;
    struct list_head paired_devices;
};

// 蓝牙设备信息
struct bluetooth_device_info {
    struct list_head list;
    bdaddr_t addr;
    char name[32];
    uint8_t device_class[3];
    uint8_t rssi;
    uint8_t flags;
    uint16_t appearance;
    
    // 连接状态
    bool connected;
    bool paired;
    bool trusted;
    
    // 服务信息
    struct list_head services;
    int service_count;
    
    // 连接历史
    time_t last_connected;
    int connection_count;
    int success_rate;
};
```

### 3.4 蓝牙设备扫描APP实现

#### 3.4.1 蓝牙扫描APP实现
```c
// 蓝牙扫描APP实现
static int bluetooth_scan_app_start(struct bluetooth_manager_app *app)
{
    int ret;
    struct bluetooth_device_list *device_list = app->device_list;
    
    // 1. 检查扫描状态
    if (device_list->scanning) {
        return -EBUSY;
    }
    
    // 2. 设置扫描状态
    device_list->scanning = true;
    init_completion(&device_list->scan_completion);
    
    // 3. 清空设备列表
    bluetooth_clear_device_list(device_list);
    
    // 4. 启动经典蓝牙扫描
    ret = bluetooth_start_classic_scan(app->bt_dev);
    if (ret < 0) {
        device_list->scanning = false;
        return ret;
    }
    
    // 5. 启动BLE扫描
    ret = bluetooth_start_ble_scan(app->bt_dev);
    if (ret < 0) {
        bluetooth_stop_classic_scan(app->bt_dev);
        device_list->scanning = false;
        return ret;
    }
    
    // 6. 启动扫描超时定时器
    mod_timer(&app->scan_timer, jiffies + msecs_to_jiffies(20000));
    
    // 7. 发送扫描开始事件
    bluetooth_send_ui_event(app, BT_UI_EVENT_SCAN_STARTED);
    
    return 0;
}
```

#### 3.4.2 蓝牙设备发现处理APP
```c
// 蓝牙设备发现处理APP
static int bluetooth_device_discovered(struct bluetooth_manager_app *app,
                                      struct bluetooth_device_info *device)
{
    struct bluetooth_device_list *device_list = app->device_list;
    
    mutex_lock(&device_list->lock);
    
    // 1. 检查设备是否已存在
    if (bluetooth_find_device(device_list, &device->addr)) {
        mutex_unlock(&device_list->lock);
        return 0;
    }
    
    // 2. 应用过滤器
    if (bluetooth_apply_filter(device, device_list->filter) < 0) {
        mutex_unlock(&device_list->lock);
        return 0;
    }
    
    // 3. 添加到设备列表
    if (device->flags & BT_DEVICE_FLAG_LE) {
        list_add_tail(&device->list, &device_list->ble_devices);
    } else {
        list_add_tail(&device->list, &device_list->classic_devices);
    }
    device_list->count++;
    
    // 4. 更新UI
    bluetooth_update_ui_device(app, device);
    
    mutex_unlock(&device_list->lock);
    
    // 5. 发送设备发现事件
    bluetooth_send_ui_event(app, BT_UI_EVENT_DEVICE_DISCOVERED);
    
    return 0;
}
```

---

## 4. 网络配置管理详细拆解

### 4.1 WiFi网络配置管理

#### 4.1.1 网络配置结构
```c
// WiFi网络配置结构
struct wifi_network_config {
    char ssid[IEEE80211_MAX_SSID_LEN];
    enum wifi_security security;
    enum wifi_cipher cipher;
    char password[64];
    char identity[64];  // 企业级认证
    char ca_cert[256];  // CA证书路径
    char client_cert[256]; // 客户端证书路径
    char private_key[256]; // 私钥路径
    
    // 高级配置
    int channel;
    bool hidden;
    int priority;
    bool auto_connect;
    
    // 企业级配置
    enum wifi_eap_method eap_method;
    char phase2_auth[32];
    char anonymous_identity[64];
};
```

#### 4.1.2 配置管理APP
```c
// WiFi配置管理APP
static int wifi_config_app_save_network(struct wifi_manager_app *app,
                                       struct wifi_network_config *config)
{
    int ret;
    struct wifi_database *database = app->database;
    
    // 1. 验证配置参数
    ret = wifi_validate_network_config(config);
    if (ret < 0)
        return ret;
    
    // 2. 加密敏感信息
    ret = wifi_encrypt_network_config(config);
    if (ret < 0)
        return ret;
    
    // 3. 保存到数据库
    ret = wifi_database_save_network(database, config);
    if (ret < 0)
        return ret;
    
    // 4. 更新wpa_supplicant配置
    ret = wifi_update_wpa_config(app, config);
    if (ret < 0) {
        // 回滚数据库操作
        wifi_database_remove_network(database, config->ssid);
        return ret;
    }
    
    // 5. 重新加载wpa_supplicant
    ret = wifi_reload_wpa_supplicant(app);
    if (ret < 0)
        return ret;
    
    // 6. 发送配置更新事件
    wifi_send_ui_event(app, WIFI_UI_EVENT_NETWORK_SAVED);
    
    return 0;
}
```

### 4.2 蓝牙配置管理

#### 4.2.1 蓝牙配置结构
```c
// 蓝牙配置结构
struct bluetooth_config {
    // 基础配置
    char device_name[32];
    bool discoverable;
    bool pairable;
    int discoverable_timeout;
    int pairable_timeout;
    
    // 安全配置
    bool secure_simple_pairing;
    bool low_energy_security;
    int io_capability;
    
    // 服务配置
    struct list_head enabled_services;
    struct list_head disabled_services;
    
    // 连接配置
    int max_connections;
    int connection_timeout;
    int inquiry_timeout;
};
```

#### 4.2.2 蓝牙配置管理APP
```c
// 蓝牙配置管理APP
static int bluetooth_config_app_update(struct bluetooth_manager_app *app,
                                      struct bluetooth_config *config)
{
    int ret;
    struct bluetooth_database *database = app->database;
    
    // 1. 验证配置参数
    ret = bluetooth_validate_config(config);
    if (ret < 0)
        return ret;
    
    // 2. 保存配置到数据库
    ret = bluetooth_database_save_config(database, config);
    if (ret < 0)
        return ret;
    
    // 3. 更新蓝牙协议栈配置
    ret = bluetooth_update_stack_config(app, config);
    if (ret < 0) {
        // 回滚数据库操作
        bluetooth_database_restore_config(database);
        return ret;
    }
    
    // 4. 重启蓝牙服务
    ret = bluetooth_restart_service(app);
    if (ret < 0)
        return ret;
    
    // 5. 发送配置更新事件
    bluetooth_send_ui_event(app, BT_UI_EVENT_CONFIG_UPDATED);
    
    return 0;
}
```

---

## 5. 基础功能测试详细拆解

### 5.1 WiFi功能测试

#### 5.1.1 WiFi测试套件
```c
// WiFi功能测试套件
struct wifi_test_suite {
    struct wifi_manager_app *app;
    struct wifi_test_results *results;
    struct wifi_test_config *config;
    
    // 测试用例
    struct list_head test_cases;
    int total_tests;
    int passed_tests;
    int failed_tests;
};
```

#### 5.1.2 WiFi连接测试
```c
// WiFi连接测试
static int wifi_test_connection(struct wifi_test_suite *test_suite)
{
    int ret;
    struct wifi_network_config *test_config;
    struct wifi_test_case *test_case;
    
    // 1. 创建测试用例
    test_case = wifi_create_test_case("WiFi Connection Test");
    if (!test_case)
        return -ENOMEM;
    
    // 2. 设置测试配置
    test_config = wifi_create_test_network_config();
    if (!test_config) {
        wifi_destroy_test_case(test_case);
        return -ENOMEM;
    }
    
    // 3. 执行连接测试
    ret = wifi_connect_to_network(test_suite->app, test_config);
    if (ret < 0) {
        wifi_test_case_set_result(test_case, WIFI_TEST_FAILED, "Connection failed");
        goto out;
    }
    
    // 4. 验证连接状态
    ret = wifi_verify_connection(test_suite->app);
    if (ret < 0) {
        wifi_test_case_set_result(test_case, WIFI_TEST_FAILED, "Connection verification failed");
        goto out;
    }
    
    // 5. 测试数据传输
    ret = wifi_test_data_transfer(test_suite->app);
    if (ret < 0) {
        wifi_test_case_set_result(test_case, WIFI_TEST_FAILED, "Data transfer failed");
        goto out;
    }
    
    // 6. 设置测试通过
    wifi_test_case_set_result(test_case, WIFI_TEST_PASSED, "All tests passed");
    
out:
    // 7. 清理测试环境
    wifi_disconnect_from_network(test_suite->app);
    wifi_destroy_test_network_config(test_config);
    
    // 8. 添加到测试结果
    wifi_test_suite_add_case(test_suite, test_case);
    
    return ret;
}
```

### 5.2 蓝牙功能测试

#### 5.2.1 蓝牙测试套件
```c
// 蓝牙功能测试套件
struct bluetooth_test_suite {
    struct bluetooth_manager_app *app;
    struct bluetooth_test_results *results;
    struct bluetooth_test_config *config;
    
    // 测试用例
    struct list_head test_cases;
    int total_tests;
    int passed_tests;
    int failed_tests;
};
```

#### 5.2.2 蓝牙扫描测试
```c
// 蓝牙扫描测试
static int bluetooth_test_scanning(struct bluetooth_test_suite *test_suite)
{
    int ret;
    struct bluetooth_test_case *test_case;
    
    // 1. 创建测试用例
    test_case = bluetooth_create_test_case("Bluetooth Scanning Test");
    if (!test_case)
        return -ENOMEM;
    
    // 2. 启动扫描
    ret = bluetooth_start_scan(test_suite->app);
    if (ret < 0) {
        bluetooth_test_case_set_result(test_case, BT_TEST_FAILED, "Scan start failed");
        goto out;
    }
    
    // 3. 等待扫描完成
    ret = wait_for_completion_timeout(&test_suite->app->device_list->scan_completion, 30000);
    if (ret == 0) {
        bluetooth_test_case_set_result(test_case, BT_TEST_FAILED, "Scan timeout");
        goto out;
    }
    
    // 4. 验证扫描结果
    ret = bluetooth_verify_scan_results(test_suite->app);
    if (ret < 0) {
        bluetooth_test_case_set_result(test_case, BT_TEST_FAILED, "Scan results verification failed");
        goto out;
    }
    
    // 5. 设置测试通过
    bluetooth_test_case_set_result(test_case, BT_TEST_PASSED, "Scan test passed");
    
out:
    // 6. 停止扫描
    bluetooth_stop_scan(test_suite->app);
    
    // 7. 添加到测试结果
    bluetooth_test_suite_add_case(test_suite, test_case);
    
    return ret;
}
```

---

## 6. 网络配置详细拆解

### 6.1 网络接口配置

#### 6.1.1 网络接口配置结构
```c
// 网络接口配置结构
struct network_interface_config {
    char interface_name[16];
    enum network_type type;
    bool enabled;
    
    // IP配置
    enum ip_config_method ip_method;
    char ip_address[16];
    char netmask[16];
    char gateway[16];
    char dns_servers[64];
    
    // DHCP配置
    bool dhcp_enabled;
    char dhcp_hostname[64];
    int dhcp_timeout;
    
    // 路由配置
    struct list_head static_routes;
    
    // 防火墙配置
    struct firewall_config *firewall;
};
```

#### 6.1.2 网络配置管理APP
```c
// 网络配置管理APP
static int network_config_app_apply(struct wifi_manager_app *app,
                                   struct network_interface_config *config)
{
    int ret;
    
    // 1. 验证配置参数
    ret = network_validate_interface_config(config);
    if (ret < 0)
        return ret;
    
    // 2. 停止网络接口
    ret = network_stop_interface(config->interface_name);
    if (ret < 0)
        return ret;
    
    // 3. 配置IP地址
    if (config->ip_method == IP_CONFIG_STATIC) {
        ret = network_configure_static_ip(config);
    } else {
        ret = network_configure_dhcp(config);
    }
    if (ret < 0)
        goto err_ip_config;
    
    // 4. 配置路由
    ret = network_configure_routes(config);
    if (ret < 0)
        goto err_route_config;
    
    // 5. 配置防火墙
    ret = network_configure_firewall(config);
    if (ret < 0)
        goto err_firewall_config;
    
    // 6. 启动网络接口
    ret = network_start_interface(config->interface_name);
    if (ret < 0)
        goto err_start_interface;
    
    // 7. 验证网络连接
    ret = network_verify_connectivity(config);
    if (ret < 0)
        goto err_connectivity;
    
    // 8. 发送配置应用事件
    network_send_ui_event(app, NETWORK_UI_EVENT_CONFIG_APPLIED);
    
    return 0;
    
err_connectivity:
    network_stop_interface(config->interface_name);
err_start_interface:
    network_clear_firewall(config);
err_firewall_config:
    network_clear_routes(config);
err_route_config:
    network_clear_ip_config(config);
err_ip_config:
    network_start_interface(config->interface_name);
    return ret;
}
```

---

## 📋 基础架构开发检查清单

### Week 1-2: 环境搭建和驱动开发
- [ ] 交叉编译环境配置
- [ ] 内核源码下载和配置
- [ ] WiFi驱动框架搭建
- [ ] 蓝牙驱动框架搭建
- [ ] 基础驱动功能测试

### Week 3-4: 基础功能实现和网络配置
- [ ] WiFi扫描功能实现
- [ ] WiFi连接功能实现
- [ ] BLE扫描功能实现
- [ ] GATT服务实现
- [ ] 网络接口配置
- [ ] 基础功能测试完成

### 技术要点检查
- [ ] 驱动架构设计合理
- [ ] 错误处理机制完善
- [ ] 内存管理安全
- [ ] 并发访问控制
- [ ] 电源管理支持
- [ ] 调试接口完善

### 代码质量检查
- [ ] 代码风格规范
- [ ] 注释完整清晰
- [ ] 错误码定义合理
- [ ] 日志记录完善
- [ ] 单元测试覆盖
- [ ] 性能测试通过

---

## 🚀 快速开始指南

### 1. 环境准备
```bash
# 安装交叉编译工具链
sudo apt-get install gcc-arm-linux-gnueabihf

# 下载内核源码
git clone https://github.com/torvalds/linux.git
cd linux
git checkout v5.10

# 配置内核
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- imx6ull_defconfig
```

### 2. 驱动开发
```bash
# 创建驱动目录
mkdir -p drivers/net/wireless/wifi_driver
mkdir -p drivers/bluetooth/bt_driver

# 编写驱动代码
# 参考上述代码示例
```

### 3. 编译测试
```bash
# 编译内核
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc)

# 编译驱动模块
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

# 安装驱动
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules_install
```

---

## 📚 参考资料

### 技术文档
- [Linux内核文档](https://www.kernel.org/doc/)
- [WiFi驱动开发指南](https://wireless.wiki.kernel.org/)
- [蓝牙协议栈文档](https://www.bluetooth.com/specifications/)

### 开源项目
- [wpa_supplicant](https://w1.fi/wpa_supplicant/)
- [BlueZ](http://www.bluez.org/)
- [Linux内核](https://www.kernel.org/)

### 开发工具
- [GDB调试器](https://www.gnu.org/software/gdb/)
- [Wireshark网络分析](https://www.wireshark.org/)
- [hcitool蓝牙工具](http://www.bluez.org/)

---

*文档版本：v1.0*
*最后更新时间：2024年12月*
