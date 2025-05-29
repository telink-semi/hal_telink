/********************************************************************************************************
 * @file    hci_cmd_info_param.c
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

int ble_host_hci_read_local_version_info(struct ble_hci_ip_rd_local_ver_rp *p_ver_info)
{
    if (p_ver_info == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("read local version info null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_INFO_PARAMS, BLE_HCI_OCF_IP_RD_LOCAL_VER),
                                 NULL,
                                 0,
                                 p_ver_info,
                                 sizeof(struct ble_hci_ip_rd_local_ver_rp));

    BLE_HOST_HCI_COMMON_CMD_INFO("read local version info return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_read_bd_address(struct ble_hci_ip_rd_bd_addr_rp *p_bd_addr)
{
    if (p_bd_addr == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("read bd address null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_INFO_PARAMS, BLE_HCI_OCF_IP_RD_BD_ADDR),
                                 NULL,
                                 0,
                                 p_bd_addr,
                                 sizeof(struct ble_hci_ip_rd_bd_addr_rp));

    BLE_HOST_HCI_COMMON_CMD_INFO("read bd address return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_read_local_supported_features(struct ble_hci_ip_rd_loc_supp_feat_rp *p_local_supp_feat)
{
    if (p_local_supp_feat == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("read local supported features null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_INFO_PARAMS, BLE_HCI_OCF_IP_RD_LOC_SUPP_FEAT),
                                 NULL,
                                 0,
                                 p_local_supp_feat,
                                 sizeof(struct ble_hci_ip_rd_loc_supp_feat_rp));

    BLE_HOST_HCI_COMMON_CMD_INFO("read local supported features return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_read_local_supported_commands(struct ble_hci_ip_rd_loc_supp_cmd_rp *p_local_supp_cmd)
{
    if (p_local_supp_cmd == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("read local supported commands null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_INFO_PARAMS, BLE_HCI_OCF_IP_RD_LOC_SUPP_CMD),
                                 NULL,
                                 0,
                                 p_local_supp_cmd,
                                 sizeof(struct ble_hci_ip_rd_loc_supp_cmd_rp));

    BLE_HOST_HCI_COMMON_CMD_INFO("read local supported commands return error code 0x%x", rc);

    return rc;
}
