/********************************************************************************************************
 * @file    hci_cmd_le_init.c
 *
 * @brief   This is the source file for TLSR/TL
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
#include "common/types.h"


#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../inc/ble_hci.h"
#include "../inc/ble_hci_cmd.h"
#include "../inc/ble_hci_log.h"

int ble_host_hci_le_create_connection(const struct ble_hci_le_create_conn_cp *p_create_conn)
{
    if (p_create_conn == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le create connection null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CREATE_CONN),
                                 p_create_conn,
                                 sizeof(struct ble_hci_le_create_conn_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE create connection return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_create_connection_cancel(void)
{
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CREATE_CONN_CANCEL),
                                 NULL,
                                 0,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE create connection cancel return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_read_white_list_size(struct ble_hci_le_rd_white_list_rp *p_white_list_size)
{
    if (p_white_list_size == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read white list size null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CLEAR_WHITE_LIST),
                                 NULL,
                                 0,
                                 p_white_list_size,
                                 sizeof(struct ble_hci_le_rd_white_list_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read white list size return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_clear_white_list(void)
{
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CLEAR_WHITE_LIST),
                                 NULL,
                                 0,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE clear white list return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_add_white_list(const struct ble_hci_le_add_whte_list_cp *p_white_list)
{
    if (p_white_list == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE add white list null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ADD_WHITE_LIST),
                                 p_white_list,
                                 sizeof(struct ble_hci_le_add_whte_list_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE add white list return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_remove_white_list(const struct ble_hci_le_rmv_white_list_cp *p_white_list)
{
    if (p_white_list == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE remove white list null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RMV_WHITE_LIST),
                                 p_white_list,
                                 sizeof(struct ble_hci_le_rmv_white_list_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE remove white list return error code 0x%x", rc);

    return rc;
}

// HCI command LE functions.
