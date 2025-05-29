/********************************************************************************************************
 * @file    hci_bb.c
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

// HCI command controller baseband functions.

int ble_host_hci_send_reset(void)
{
    int rc;

    rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_CTLR_BASEBAND, BLE_HCI_OCF_CB_RESET),
                             NULL,
                             0,
                             NULL,
                             0);
    BLE_HOST_HCI_COMMON_CMD_INFO("hci reset return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_set_event_mask(const struct ble_hci_cb_set_event_mask_cp *p_event_mask)
{
    if (p_event_mask == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("controller baseband set event mask null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_CTLR_BASEBAND, BLE_HCI_OCF_CB_SET_EVENT_MASK),
                                 p_event_mask,
                                 sizeof(struct ble_hci_ip_rd_loc_supp_feat_rp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_COMMON_CMD_INFO("controller baseband set event mask return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_set_event_mask2(const struct ble_hci_cb_set_event_mask2_cp *p_event_mask2)
{
    if (p_event_mask2 == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("controller baseband set event mask2 null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_CTLR_BASEBAND, BLE_HCI_OCF_CB_SET_EVENT_MASK2),
                                 p_event_mask2,
                                 sizeof(struct ble_hci_ip_rd_loc_supp_feat_rp),
                                 NULL,
                                 0);


    BLE_HOST_HCI_COMMON_CMD_DEBUG("CTLR baseband set event mask2 return error code 0x%x", rc);

    return rc;
}
