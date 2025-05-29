/********************************************************************************************************
 * @file    hci_cmd_le_adv.c
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
#include "inc/hci_cmd_le_adv.h"

// HCI command LE functions.

int ble_host_hci_le_set_adv_param(const struct ble_hci_le_set_adv_params_cp *p_adv_param)
{
    if (p_adv_param == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set adv param null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_ADV_PARAMS),
                                 p_adv_param,
                                 sizeof(struct ble_hci_le_set_adv_params_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set adv param return error code 0x%x", rc);

    return rc;
}
int ble_host_hci_le_read_adv_chnl_tx_power(struct ble_hci_le_rd_adv_chan_txpwr_rp *p_tx_power)
{
    if (p_tx_power == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("read adv chnl tx power null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_ADV_CHAN_TXPWR),
                                 NULL,
                                 0,
                                 p_tx_power,
                                 sizeof(struct ble_hci_le_rd_adv_chan_txpwr_rp));

    BLE_HOST_HCI_LE_CMD_INFO("read adv chnl tx power return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_adv_data(const struct ble_hci_le_set_adv_data_full_cp *p_adv_data)
{
    if (p_adv_data == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set adv data null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    if (p_adv_data->adv_data_len > BLE_HCI_LE_MAX_SUPPORTED_ADV_DATA_LEN) {
        BLE_HOST_HCI_LE_CMD_ERROR("set adv data len error");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_INVALID_PARAM);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_ADV_DATA),
                                 p_adv_data,
                                 sizeof(struct ble_hci_le_set_adv_data_cp) + p_adv_data->adv_data_len,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set adv data return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_scan_rsp_data(const struct ble_hci_le_set_scan_rsp_data_full_cp *p_scan_rsp_data)
{
    if (p_scan_rsp_data == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set scan rsp data null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    if (p_scan_rsp_data->scan_rsp_len > BLE_HCI_LE_MAX_SUPPORTED_SCAN_RSP_DATA_LEN) {
        BLE_HOST_HCI_LE_CMD_ERROR("set scan rsp data len error");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_INVALID_PARAM);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_SCAN_RSP_DATA),
                                 p_scan_rsp_data,
                                 sizeof(struct ble_hci_le_set_scan_rsp_data_cp) + p_scan_rsp_data->scan_rsp_len,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set scan rsp data return error code 0x%x", rc);
    return rc;
}



int ble_host_hci_le_set_adv_enable(const struct ble_hci_le_set_adv_enable_cp *p_adv_enable)
{
    if (p_adv_enable == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set adv enable null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_ADV_ENABLE),
                                 p_adv_enable,
                                 sizeof(struct ble_hci_le_set_adv_enable_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set adv enable return error code 0x%x", rc);
    return rc;
}
