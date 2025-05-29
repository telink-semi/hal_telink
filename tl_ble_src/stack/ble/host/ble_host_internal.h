/********************************************************************************************************
 * @file    ble_host_internal.h
 *
 * @brief   This is the header file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifdef MCU_CORE_D25F_ENABLE
#include <sys/queue.h>

enum
{
    BLE_HOST_MALLOC_TYPE_START = 0x0100,
    BLE_ACL_MANAGER_TYPE,
    BLE_HOST_GATT_MANAGER_TYPE,
    BLE_GATTS_TYPE,

    BLE_PRF_ASCS_SERVER_TYPE,
    BLE_PRF_PACS_SERVER_TYPE,
    BLE_PRF_GMCS_CLIENT_TYPE,
    BLE_PRF_GTBS_CLIENT_TYPE,
    BLE_PRF_HIDS_SERVER_TYPE,
};

//void blt_host_init(void);

//void  ble_host_malloc_init(void);
//void *ble_host_malloc_v0(uint32_t size, uint16_t type);
//void  ble_host_free_v0(void *ptr);

#define BLE_ROLE_PERIPHERAL 1
#define BLE_ROLE_CENTRAL    0

struct ble_host_acl_connected_info
{
    uint16_t conn_handle;
    uint8_t  peer_addr_type;
    uint8_t  peer_addr[6];
    // maybe more info here
};

//bool ble_host_check_acl_connect_handle_central(uint16_t conn_handle);
//bool ble_host_check_acl_connect_handle_peripheral(uint16_t conn_handle);
//bool ble_host_check_acl_connected(uint16_t conn_handle);
//bool ble_host_acl_connected_insert_info(struct ble_host_acl_connected_info *info);
//bool ble_host_acl_connected_remove_info(uint16_t conn_handle);

enum
{
    GATT_SERVER_NOTIFY,
    GATT_MAX_CONNECTED_INFO,
};

typedef void (*gatt_disconnect_cb_t)(void *user_data);

struct ble_host_gatt_connected_info_node
{
    TAILQ_ENTRY(ble_host_gatt_connected_info_node)
    next;
    uint16_t             conn_handle;
    uint16_t             channel_id;
    gatt_disconnect_cb_t disconnect_cb;
    // user data
    uint8_t user_data[0];
};

TAILQ_HEAD(ble_host_gatt_connected_info_list, ble_host_gatt_connected_info_node);

//void *blt_host_gatt_get_user_info(int index, uint16_t conn_handle, uint16_t channel_id);
//
//void *blt_host_gatt_malloc_info(int index, uint16_t conn_handle, uint16_t channel_id, gatt_disconnect_cb_t disconnect_cb, uint16_t size);
//
//void *blt_host_gatt_malloc_info_att(int index, uint16_t conn_handle, gatt_disconnect_cb_t disconnect_cb, uint16_t size);
//
//void blt_host_gatt_free_info(int index, uint16_t conn_handle, uint16_t channel_id);
//
//void blt_host_gatt_free_info_att(int index, uint16_t conn_handle);
//
//typedef void (*gatt_traverse_cb_t)(uint16_t conn_handle, uint16_t channel_id, void *user_data);
//
//void blt_host_gatt_traverse_all_info(int index, gatt_traverse_cb_t traverse_cb);
//
//struct ble_host_gatt_connected_info_list *blt_host_gatt_get_info_by_conn_handle(int index);
#endif
