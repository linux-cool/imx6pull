/*
 * 蓝牙驱动头文件
 * 
 * 基于IMX6ULL Pro开发板的蓝牙驱动接口定义
 * 
 * Copyright (C) 2024 Linux Cool Team
 */

#ifndef __BLUETOOTH_DRIVER_H
#define __BLUETOOTH_DRIVER_H

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/bluetooth/bluetooth.h>
#include <linux/bluetooth/hci_core.h>

/* 蓝牙设备类型 */
enum bluetooth_device_type {
    BT_DEVICE_CLASSIC = 0,
    BT_DEVICE_LE,
    BT_DEVICE_DUAL,
    BT_DEVICE_MAX
};

/* 蓝牙连接状态 */
enum bluetooth_connection_state {
    BT_STATE_INIT = 0,
    BT_STATE_READY,
    BT_STATE_SCANNING,
    BT_STATE_CONNECTING,
    BT_STATE_CONNECTED,
    BT_STATE_DISCONNECTING,
    BT_STATE_ERROR,
    BT_STATE_MAX
};

/* 蓝牙设备信息 */
struct bluetooth_device_info {
    bdaddr_t addr;
    char name[32];
    uint8_t device_class[3];
    uint8_t rssi;
    uint8_t flags;
    uint16_t appearance;
    uint8_t data_len;
    uint8_t data[31];
};

/* 蓝牙连接信息 */
struct bluetooth_connection_info {
    bdaddr_t addr;
    char name[32];
    enum bluetooth_device_type type;
    bool connected;
    bool paired;
    bool trusted;
    time_t connect_time;
    time_t last_seen;
};

/* 蓝牙驱动操作 */
struct bluetooth_driver_ops {
    int (*init)(struct bluetooth_device *bdev);
    int (*deinit)(struct bluetooth_device *bdev);
    int (*scan_start)(struct bluetooth_device *bdev);
    int (*scan_stop)(struct bluetooth_device *bdev);
    int (*connect)(struct bluetooth_device *bdev, bdaddr_t *addr);
    int (*disconnect)(struct bluetooth_device *bdev);
    int (*gatt_service_add)(struct bluetooth_device *bdev, struct gatt_service *service);
};

/* 蓝牙设备结构 */
struct bluetooth_device {
    struct device *dev;
    struct bluetooth_driver_ops *ops;
    struct bluetooth_status status;
    struct bluetooth_connection_info conn_info;
    struct mutex lock;
    struct workqueue_struct *workqueue;
    struct delayed_work scan_work;
    struct delayed_work status_work;
    
    void *private_data;
    struct hci_dev *hdev;
    struct list_head gatt_services;
    struct mutex gatt_lock;
};

/* 函数声明 */
int bluetooth_driver_probe(struct platform_device *pdev);
int bluetooth_driver_remove(struct platform_device *pdev);

#endif /* __BLUETOOTH_DRIVER_H */
