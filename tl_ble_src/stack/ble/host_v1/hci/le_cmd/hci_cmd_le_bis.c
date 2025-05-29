/********************************************************************************************************
 * @file    hci_cmd_le_bis.c
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
#include "inc/hci_cmd_le_bis.h"

// HCI command LE functions.
int ble_host_hci_le_create_big(const struct ble_hci_le_create_big_cp *p_create_big)
{
    if ((p_create_big == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("create big null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CREATE_BIG),
                                 p_create_big,
                                 sizeof(struct ble_hci_le_create_big_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("create big return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_create_big_test(const struct ble_hci_le_create_big_test_cp *p_create_big_test)
{
    if ((p_create_big_test == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("create big test null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CREATE_BIG_TEST),
                                 p_create_big_test,
                                 sizeof(struct ble_hci_le_create_big_test_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("create big test return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_terminate_big(const struct ble_hci_le_terminate_big_cp *p_terminate_big)
{
    if ((p_terminate_big == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("terminate big null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_TERMINATE_BIG),
                                 p_terminate_big,
                                 sizeof(struct ble_hci_le_terminate_big_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("terminate big return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_big_create_sync(const struct ble_hci_le_big_create_sync_full_cp *p_create_sync)
{
    if ((p_create_sync == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("big create sync null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_BIG_CREATE_SYNC),
                                 p_create_sync,
                                 sizeof(struct ble_hci_le_big_create_sync_cp) + p_create_sync->num_bis * sizeof(uint8_t),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("big create sync return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_big_terminate_sync(const struct ble_hci_le_big_terminate_sync_cp *p_terminate_sync, struct ble_hci_le_big_terminate_sync_rp *p_terminate_sync_rp)
{
    if (p_terminate_sync == NULL || p_terminate_sync_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("big terminate sync null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_BIG_TERMINATE_SYNC),
                                 p_terminate_sync,
                                 sizeof(struct ble_hci_le_big_terminate_sync_cp),
                                 p_terminate_sync_rp,
                                 sizeof(struct ble_hci_le_big_terminate_sync_rp));

    BLE_HOST_HCI_LE_CMD_INFO("big terminate sync return error code 0x%x", rc);
    return rc;
}
