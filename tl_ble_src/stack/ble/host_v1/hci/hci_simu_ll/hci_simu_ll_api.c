/********************************************************************************************************
 * @file    hci_simu_ll_api.c
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
#include "tl_common.h"
#include "hci_simu_ll_api.h"

#if (DUAL_CORE_MODE_ENABLED)

    #include "common/types.h"
    #include "stack/ble/host_v1/inc/ble_host.h"
    #include "stack/ble/host_v1/inc/ble_host_sal.h"
    #include "stack/ble/host_v1/hci/inc/ble_hci.h"
    #include "stack/ble/host_v1/hci/inc/ble_hci_cmd.h"
    #include "stack/ble/host_v1/hci/inc/ble_hci_log.h"

u8 blt_ll_getAclConnRole(u16 connHandle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(connHandle);

    if (conn != NULL) {
        return conn->role;
    }

    return 0xFF;
}

bool blc_ll_isAclConnEstablished(u16 connHandle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(connHandle);

    if (conn != NULL) {
        return conn->conn_created_ticks ? true : false;
    }

    return false;
}

u32 blc_ll_getConnectionStartTick(u16 connHandle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(connHandle);

    if (conn != NULL) {
        return conn->conn_created_ticks;
    }

    return 0;
}

void ble_host_set_encryption_busy(u16 connHandle, u8 enc_busy)
{
    (void)connHandle;
    (void)enc_busy;

    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(connHandle);

    if (conn != NULL) {
        conn->encryption_busy = enc_busy;
    }
}

int ble_host_is_encryption_busy(u16 connHandle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(connHandle);

    if (conn != NULL) {
        return conn->encryption_busy;
    }

    return 0;
}

#endif
