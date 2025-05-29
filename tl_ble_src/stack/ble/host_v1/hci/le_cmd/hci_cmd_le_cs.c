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

int ble_host_hci_le_cs_read_remote_supported_capabilities(const struct ble_hci_le_cs_rd_rem_supp_cap_cp *p_read_remote)
{
    if (p_read_remote == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs_read_remote_support_capa_null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_RD_REM_SUPP_CAP),
                                 p_read_remote,
                                 sizeof(struct ble_hci_le_cs_rd_rem_supp_cap_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs read remote support capabilities error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_security_enable(const struct ble_hci_le_cs_sec_enable_cp *p_sec_enable)
{
    if (p_sec_enable == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs security enable null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_SEC_ENABLE),
                                 p_sec_enable,
                                 sizeof(struct ble_hci_le_cs_sec_enable_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs security enable error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_set_default_settings(const struct ble_hci_le_cs_set_def_settings_cp *p_set_default_setting)
{
    if (p_set_default_setting == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs set default setting null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_SET_DEF_SETTINGS),
                                 p_set_default_setting,
                                 sizeof(struct ble_hci_le_cs_set_def_settings_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs set default setting error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_read_remote_FAE_table(const struct ble_hci_le_cs_rd_rem_fae_cp *p_rd_rem_fae)
{
    if (p_rd_rem_fae == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs read remote fae table null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_RD_REM_FAE),
                                 p_rd_rem_fae,
                                 sizeof(struct ble_hci_le_cs_rd_rem_fae_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs read remote fae table error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_create_config(const struct ble_hci_le_cs_create_config_cp *p_create_config)
{
    if (p_create_config == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs create config null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_CREATE_CONFIG),
                                 p_create_config,
                                 sizeof(struct ble_hci_le_cs_create_config_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs create config error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_remove_config(const struct ble_hci_le_cs_remove_config_cp *p_remove_config)
{
    if (p_remove_config == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs remove config null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_REMOVE_CONFIG),
                                 p_remove_config,
                                 sizeof(struct ble_hci_le_cs_remove_config_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs remove config error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_set_channel_classification(const struct ble_hci_le_cs_set_chan_class_cp *p_set_chan_class)
{
    if (p_set_chan_class == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs set channel classfication null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_SET_CHAN_CLASS),
                                 p_set_chan_class,
                                 sizeof(struct ble_hci_le_cs_set_chan_class_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs set channel classfication error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_set_procedure_parameters(const struct ble_hci_le_cs_set_proc_params_cp *p_set_proc_params)
{
    if (p_set_proc_params == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs set procedure parameters null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_SET_PROC_PARAMS),
                                 p_set_proc_params,
                                 sizeof(struct ble_hci_le_cs_set_proc_params_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs set procedure parameters error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_cs_procedure_enable(const struct ble_hci_le_cs_proc_enable_cp *p_set_proc_enable)
{
    if (p_set_proc_enable == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le cs set procedure enable null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CS_PROC_ENABLE),
                                 p_set_proc_enable,
                                 sizeof(struct ble_hci_le_cs_proc_enable_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE cs set procedure enable error code 0x%x", rc);
    return rc;
}

// HCI command LE functions.
