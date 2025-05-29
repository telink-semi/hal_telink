/********************************************************************************************************
 * @file    hci_cmd_le_iso.c
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

#include "inc/hci_cmd_le_iso.h"

int ble_host_hci_le_setup_iso_data_path(const struct ble_hci_le_setup_iso_data_path_full_cp *p_cig_params,
                                        struct ble_hci_le_setup_iso_data_path_rp            *p_cig_params_rp)
{
    if ((p_cig_params == NULL) || (p_cig_params_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("set iso data path null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    if (p_cig_params->codec_config_len > BLE_HCI_LE_MAX_SUPPORTED_CODEC_CONFIG_LEN) {
        BLE_HOST_HCI_LE_CMD_ERROR("set iso data path codec config len error");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_INVALID_PARAM);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SETUP_ISO_DATA_PATH),
                                 p_cig_params,
                                 sizeof(struct ble_hci_le_setup_iso_data_path_full_cp) + p_cig_params->codec_config_len,
                                 p_cig_params_rp,
                                 sizeof(struct ble_hci_le_setup_iso_data_path_rp));

    BLE_HOST_HCI_LE_CMD_INFO("set iso data path return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_remove_iso_data_path(const struct ble_hci_le_remove_iso_data_path_cp *p_cig_params,
                                         struct ble_hci_le_remove_iso_data_path_rp       *p_cig_params_rp)
{
    if ((p_cig_params == NULL) || (p_cig_params_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("remove iso data path null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_REMOVE_ISO_DATA_PATH),
                                 p_cig_params,
                                 sizeof(struct ble_hci_le_remove_iso_data_path_cp),
                                 p_cig_params_rp,
                                 sizeof(struct ble_hci_le_remove_iso_data_path_rp));

    BLE_HOST_HCI_LE_CMD_INFO("remove iso data path return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_iso_receive_test(const struct ble_hci_le_iso_receive_test_cp *p_receive_test_cp,
                                     struct ble_hci_le_iso_receive_test_rp       *p_receive_test_rp)
{
    if ((p_receive_test_cp == NULL) || (p_receive_test_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("iso receive test null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ISO_RECEIVE_TEST),
                                 p_receive_test_cp,
                                 sizeof(struct ble_hci_le_iso_receive_test_cp),
                                 p_receive_test_rp,
                                 sizeof(struct ble_hci_le_iso_receive_test_rp));

    BLE_HOST_HCI_LE_CMD_INFO("iso receive test return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_iso_read_test_counters(const struct ble_hci_le_iso_read_test_counters_cp *p_read_test_counters_cp,
                                           struct ble_hci_le_iso_read_test_counters_rp       *p_read_test_counters_rp)
{
    if ((p_read_test_counters_cp == NULL) || (p_read_test_counters_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("iso read test counters null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ISO_READ_TEST_COUNTERS),
                                 p_read_test_counters_cp,
                                 sizeof(struct ble_hci_le_iso_read_test_counters_cp),
                                 p_read_test_counters_rp,
                                 sizeof(struct ble_hci_le_iso_read_test_counters_rp));

    BLE_HOST_HCI_LE_CMD_INFO("iso read test counters return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_iso_test_end(const struct ble_hci_le_iso_test_end_cp *p_test_end_cp,
                                 struct ble_hci_le_iso_test_end_rp       *p_test_end_rp)
{
    if ((p_test_end_cp == NULL) || (p_test_end_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("iso test end null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ISO_TEST_END),
                                 p_test_end_cp,
                                 sizeof(struct ble_hci_le_iso_test_end_cp),
                                 p_test_end_rp,
                                 sizeof(struct ble_hci_le_iso_test_end_rp));

    BLE_HOST_HCI_LE_CMD_INFO("iso test end return error code 0x%x", rc);
    return rc;
}
