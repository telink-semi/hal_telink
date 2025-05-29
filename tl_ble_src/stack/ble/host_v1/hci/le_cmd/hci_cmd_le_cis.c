/********************************************************************************************************
 * @file    hci_cmd_le_cis.c
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
#include "inc/hci_cmd_le_cis.h"

int ble_host_hci_le_set_cig_params(const struct ble_hci_le_set_cig_params_full_cp *p_cig_params,
                                   struct ble_hci_le_set_cig_params_full_rp       *p_cig_params_rp)
{
    if ((p_cig_params == NULL) || (p_cig_params_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("set cig params null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_CIG_PARAMS),
                                 p_cig_params,
                                 sizeof(struct ble_hci_le_set_cig_params_cp) + p_cig_params->cis_count * sizeof(struct ble_hci_le_cis_params),
                                 p_cig_params_rp,
                                 sizeof(struct ble_hci_le_set_cig_params_full_rp));

    BLE_HOST_HCI_LE_CMD_INFO("set cig params return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_cig_params_test(const struct ble_hci_le_set_cig_params_test_full_cp *p_cig_params_test,
                                        struct ble_hci_le_set_cig_params_test_full_rp       *p_cig_params_test_rp)
{
    if ((p_cig_params_test == NULL) || (p_cig_params_test_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("set cig params test null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_CIG_PARAMS_TEST),
                                 p_cig_params_test,
                                 sizeof(struct ble_hci_le_set_cig_params_cp) + p_cig_params_test->cis_count * sizeof(struct ble_hci_le_cis_params_test),
                                 p_cig_params_test_rp,
                                 sizeof(struct ble_hci_le_set_cig_params_test_full_rp));

    BLE_HOST_HCI_LE_CMD_INFO("set cig params test return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_create_cis(const struct ble_hci_le_create_cis_full_cp *p_create_cis)
{
    if (p_create_cis == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("create cis null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CREATE_CIS),
                                 p_create_cis,
                                 sizeof(struct ble_hci_le_create_cis_cp) + p_create_cis->cis_count * sizeof(struct ble_hci_le_create_cis_params),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("create cis return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_remove_cig(const struct ble_hci_le_remove_cig_cp *p_remove_cig, struct ble_hci_le_remove_cig_rp *p_remove_cig_rp)
{
    if (p_remove_cig == NULL || p_remove_cig_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("remove cig null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_REMOVE_CIG),
                                 p_remove_cig,
                                 sizeof(struct ble_hci_le_remove_cig_cp),
                                 p_remove_cig_rp,
                                 sizeof(struct ble_hci_le_remove_cig_rp));

    BLE_HOST_HCI_LE_CMD_INFO("remove cig return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_accept_cis_request(const struct ble_hci_le_accept_cis_request_cp *p_accept_cis_request)
{
    if (p_accept_cis_request == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("accept cis request null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ACCEPT_CIS_REQ),
                                 p_accept_cis_request,
                                 sizeof(struct ble_hci_le_accept_cis_request_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("accept cis request return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_reject_cis_request(const struct ble_hci_le_reject_cis_request_cp *p_reject_cis_request,
                                       struct ble_hci_le_reject_cis_request_rp       *p_reject_cis_request_rp)
{
    if ((p_reject_cis_request == NULL) || (p_reject_cis_request_rp == NULL)) {
        BLE_HOST_HCI_LE_CMD_ERROR("reject cis request null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_REJECT_CIS_REQ),
                                 p_reject_cis_request,
                                 sizeof(struct ble_hci_le_reject_cis_request_cp),
                                 p_reject_cis_request_rp,
                                 sizeof(struct ble_hci_le_reject_cis_request_rp));

    BLE_HOST_HCI_LE_CMD_INFO("reject cis request return error code 0x%x", rc);
    return rc;
}
