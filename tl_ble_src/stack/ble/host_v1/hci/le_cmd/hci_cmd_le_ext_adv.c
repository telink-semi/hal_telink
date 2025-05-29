/********************************************************************************************************
 * @file    hci_cmd_le_ext_adv.c
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

#include "inc/hci_cmd_le_ext_adv.h"

// HCI command LE functions.

int ble_host_hci_le_set_ext_adv_params(const struct ble_hci_le_set_ext_adv_params_cp *p_ext_adv_params,
                                       struct ble_hci_le_set_ext_adv_params_rp       *p_ext_adv_params_rp)
{
    if ((p_ext_adv_params == NULL) || (p_ext_adv_params_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("set ext adv params null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_EXT_ADV_PARAM),
                                 p_ext_adv_params,
                                 sizeof(struct ble_hci_le_set_ext_adv_params_cp),
                                 p_ext_adv_params_rp,
                                 sizeof(struct ble_hci_le_set_ext_adv_params_rp));

    BLE_HOST_HCI_LE_CMD_INFO("set ext adv params return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_ext_adv_data(const struct ble_hci_le_set_ext_adv_data_full_cp *p_ext_adv_data)
{
    if (p_ext_adv_data == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set ext adv data null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_EXT_ADV_DATA),
                                 p_ext_adv_data,
                                 sizeof(struct ble_hci_le_set_ext_adv_data_cp) + p_ext_adv_data->adv_data_len,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set ext adv data return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_ext_scan_rsp_data(const struct ble_hci_le_set_ext_scan_rsp_data_full_cp *p_ext_scan_rsp_data)
{
    if (p_ext_scan_rsp_data == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set ext scan rsp data null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_EXT_SCAN_RSP_DATA),
                                 p_ext_scan_rsp_data,
                                 sizeof(struct ble_hci_le_set_ext_scan_rsp_data_cp) + p_ext_scan_rsp_data->scan_rsp_len,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set ext scan rsp data return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_ext_adv_enable(const struct ble_hci_le_set_ext_adv_enable_full_cp *p_ext_adv_enable)
{
    if (p_ext_adv_enable == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("set ext adv enable null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_EXT_ADV_ENABLE),
                                 p_ext_adv_enable,
                                 sizeof(struct ble_hci_le_set_ext_adv_enable_cp) + p_ext_adv_enable->num_sets * sizeof(struct adv_set),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("set ext adv enable return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_read_max_adv_data_len(struct ble_hci_le_rd_max_adv_data_len_rp *p_max_adv_data_len_rp)
{
    if (p_max_adv_data_len_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("read max adv data len null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_MAX_ADV_DATA_LEN),
                                 NULL,
                                 0,
                                 p_max_adv_data_len_rp,
                                 sizeof(struct ble_hci_le_rd_max_adv_data_len_rp));

    BLE_HOST_HCI_LE_CMD_INFO("read max adv data len return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_read_number_of_adv_sets(struct ble_hci_le_rd_num_of_adv_sets_rp *p_num_of_adv_sets_rp)
{
    if (p_num_of_adv_sets_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("read num of adv sets null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_NUM_OF_ADV_SETS),
                                 NULL,
                                 0,
                                 p_num_of_adv_sets_rp,
                                 sizeof(struct ble_hci_le_rd_num_of_adv_sets_rp));

    BLE_HOST_HCI_LE_CMD_INFO("read num of adv sets return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_remove_adv_set(const struct ble_hci_le_remove_adv_set_cp *p_remove_adv_set_cp)
{
    if (p_remove_adv_set_cp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("remove adv set null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_NUM_OF_ADV_SETS),
                                 p_remove_adv_set_cp,
                                 sizeof(struct ble_hci_le_remove_adv_set_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("remove adv_set return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_clear_adv_sets(void)
{
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CLEAR_ADV_SETS),
                                 NULL,
                                 0,
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("clear adv_sets return error code 0x%x", rc);
    return rc;
}
