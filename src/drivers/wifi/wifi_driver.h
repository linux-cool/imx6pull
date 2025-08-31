/*
 * WiFi驱动头文件
 * 
 * 基于IMX6ULL Pro开发板的WiFi驱动接口定义
 * 
 * Copyright (C) 2024 Linux Cool Team
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __WIFI_DRIVER_H
#define __WIFI_DRIVER_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>

/* WiFi安全类型定义 */
enum wifi_security {
    WIFI_SECURITY_OPEN = 0,
    WIFI_SECURITY_WEP,
    WIFI_SECURITY_WPA_PSK,
    WIFI_SECURITY_WPA2_PSK,
    WIFI_SECURITY_WPA3_PSK,
    WIFI_SECURITY_WPA_ENTERPRISE,
    WIFI_SECURITY_WPA2_ENTERPRISE,
    WIFI_SECURITY_WPA3_ENTERPRISE,
    WIFI_SECURITY_MAX
};

/* WiFi加密类型定义 */
enum wifi_cipher {
    WIFI_CIPHER_NONE = 0,
    WIFI_CIPHER_WEP,
    WIFI_CIPHER_TKIP,
    WIFI_CIPHER_CCMP,
    WIFI_CIPHER_GCMP,
    WIFI_CIPHER_MAX
};

/* WiFi连接状态定义 */
enum wifi_connection_state {
    WIFI_STATE_INIT = 0,
    WIFI_STATE_READY,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTING,
    WIFI_STATE_ERROR,
    WIFI_STATE_MAX
};

/* WiFi模式定义 */
enum wifi_mode {
    WIFI_MODE_STATION = 0,
    WIFI_MODE_AP,
    WIFI_MODE_MONITOR,
    WIFI_MODE_AP_STA,
    WIFI_MODE_MAX
};

/* WiFi网络信息结构 */
struct wifi_network {
    char ssid[IEEE80211_MAX_SSID_LEN];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int signal_strength;
    int channel;
    int frequency;
    bool hidden;
    bool connected;
    time_t last_seen;
};

/* WiFi连接参数结构 */
struct wifi_connect_params {
    char ssid[IEEE80211_MAX_SSID_LEN];
    char password[64];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int channel;
    bool hidden;
    char identity[64];  // 企业级认证
    char ca_cert[256];  // CA证书路径
    char client_cert[256]; // 客户端证书路径
    char private_key[256]; // 私钥路径
};

/* WiFi状态结构 */
struct wifi_status {
    enum wifi_connection_state state;
    int signal_strength;
    int channel;
    char ssid[IEEE80211_MAX_SSID_LEN];
    enum wifi_security security;
    int tx_rate;
    int rx_rate;
    int tx_power;
    int noise_level;
    int link_quality;
};

/* WiFi连接信息结构 */
struct wifi_connection_info {
    char ssid[IEEE80211_MAX_SSID_LEN];
    char bssid[ETH_ALEN];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int channel;
    int frequency;
    int signal_strength;
    int tx_rate;
    int rx_rate;
    bool connected;
    time_t connect_time;
    time_t last_seen;
};

/* WiFi扫描结果结构 */
struct wifi_scan_result {
    char ssid[IEEE80211_MAX_SSID_LEN];
    char bssid[ETH_ALEN];
    enum wifi_security security;
    enum wifi_cipher cipher;
    int signal_strength;
    int channel;
    int frequency;
    int beacon_interval;
    int capability;
    bool hidden;
    time_t timestamp;
};

/* WiFi驱动操作结构 */
struct wifi_driver_ops {
    /* 基础操作 */
    int (*probe)(struct wifi_device *dev);
    int (*remove)(struct wifi_device *dev);
    int (*suspend)(struct wifi_device *dev);
    int (*resume)(struct wifi_device *dev);
    
    /* WiFi功能操作 */
    int (*init)(struct wifi_device *dev);
    int (*deinit)(struct wifi_device *dev);
    int (*reset)(struct wifi_device *dev);
    
    /* 网络管理 */
    int (*scan_start)(struct wifi_device *dev);
    int (*scan_stop)(struct wifi_device *dev);
    int (*connect)(struct wifi_device *dev, struct wifi_connect_params *params);
    int (*disconnect)(struct wifi_device *dev);
    
    /* 状态查询 */
    int (*get_status)(struct wifi_device *dev, struct wifi_status *status);
    int (*get_signal_strength)(struct wifi_device *dev, int *strength);
    int (*get_connection_info)(struct wifi_device *dev, struct wifi_connection_info *info);
    
    /* 配置管理 */
    int (*set_mode)(struct wifi_device *dev, enum wifi_mode mode);
    int (*set_power)(struct wifi_device *dev, int power);
    int (*set_channel)(struct wifi_device *dev, int channel);
    
    /* 统计信息 */
    int (*get_statistics)(struct wifi_device *dev, struct wireless_stats *stats);
    int (*reset_statistics)(struct wifi_device *dev);
};

/* WiFi设备结构 */
struct wifi_device {
    struct device *dev;
    struct wifi_driver_ops *ops;
    struct wifi_status status;
    struct wifi_connection_info conn_info;
    struct mutex lock;
    struct workqueue_struct *workqueue;
    struct delayed_work scan_work;
    struct delayed_work status_work;
    
    /* 硬件相关 */
    void *private_data;
    struct spi_device *spi;
    struct platform_device *pdev;
    
    /* 网络相关 */
    struct net_device *ndev;
    struct wireless_dev *wdev;
    
    /* 扫描相关 */
    struct list_head network_list;
    int network_count;
    struct completion scan_completion;
    
    /* 连接相关 */
    struct completion connect_completion;
    struct completion disconnect_completion;
    
    /* 统计信息 */
    struct wireless_stats stats;
    spinlock_t stats_lock;
    
    /* 调试信息 */
    bool debug_enabled;
    struct dentry *debug_dir;
};

/* WiFi驱动平台数据 */
struct wifi_platform_data {
    char *firmware_name;
    int gpio_reset;
    int gpio_power;
    int gpio_irq;
    int gpio_wake;
    unsigned long irq_flags;
    bool power_on_boot;
    int power_delay_ms;
    int reset_delay_ms;
    int init_delay_ms;
};

/* 函数声明 */
int wifi_driver_probe(struct platform_device *pdev);
int wifi_driver_remove(struct platform_device *pdev);
int wifi_driver_suspend(struct platform_device *pdev, pm_message_t state);
int wifi_driver_resume(struct platform_device *pdev);

/* 工具函数声明 */
int wifi_validate_connect_params(struct wifi_connect_params *params);
int wifi_encrypt_network_config(struct wifi_network_config *config);
int wifi_add_network_to_list(struct wifi_device *wdev, struct wifi_network *network);
int wifi_remove_network_from_list(struct wifi_device *wdev, const char *ssid);
struct wifi_network *wifi_find_network(struct wifi_device *wdev, const char *ssid);

/* 事件处理函数声明 */
void wifi_send_scan_complete_event(struct wifi_device *wdev);
void wifi_send_connection_event(struct wifi_device *wdev, enum wifi_connection_state state);
void wifi_send_disconnection_event(struct wifi_device *wdev);

/* 调试函数声明 */
void wifi_debug_init(struct wifi_device *wdev);
void wifi_debug_cleanup(struct wifi_device *wdev);
void wifi_debug_show_status(struct wifi_device *wdev, struct seq_file *seq);

#endif /* __WIFI_DRIVER_H */
