/********************************************************************************************************
 * @file    scps_server.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "scps_internal.h"
#include "scps_server_buf.h"

static int  blt_scpss_init(u8 initType, const void *param);
static void blt_scpss_serviceInit(const struct blc_scpss_regParam *param);
static int  blt_scpss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);
#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_ struct blc_scps_server_ctrl scps_server_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = SCPS_SERVER,
                .usedAclRole = 0,
                .init        = blt_scpss_init,
                .connect     = NULL,
                .discov      = NULL,
                .loop        = NULL,
                },
};
#else
static const struct blc_prf_process_params s_scpss_server_process_params = {
    .id          = SCPS_SERVER,
    .usedAclRole = PRF_GAP_ACL_PERIPHERAL,
    .init        = blt_scpss_init,
    .connect     = NULL,
    .discovery   = NULL,
    .store       = NULL,
};

_attribute_ble_data_retention_ struct blc_scps_server_ctrl scps_server_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_scpss_server_process_params,
                },
};
#endif
/**
 * @brief       register scan parameters service server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_basic_registerSCPSControlServer(const struct blc_scpss_regParam *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t *)&scps_server_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&scps_server_ctrl, param);
#endif
}

static struct blc_scps_server *blt_scpss_getCtrl(u16 connHandle)
{
    (void)connHandle;
    return &scps_server_ctrl.scpsServer;
}

static int blt_scpss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_scps_server_ctrl)), blc_scps_server_ctrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_scps_server)), blc_scps_server);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_SCPS_LOG("Server init");
        blc_svc_addScpsGroup();
        blt_scpss_serviceInit(param);
        blc_svc_scpsCbackRegister(blt_scpss_writeCback);
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      blc_svc_removeBasGroup();
    //      BLT_SCPS_LOG("Server deinit");
    //  }
    return 0;
}

/******************ScPS server init all characteristic handle*************************/
BLT_SCPS_SERVER_INIT_HANDLE(scanIntervalWindow)
BLT_SCPS_SERVER_INIT_HANDLE(scanRefresh)

static const atts_findCharList_t scpssChar[] = {
    BLT_SCPS_SERVER_FIND_CHAR(scanIntervalWindow, characteristicScanIntervalWindowUuid),
    BLT_SCPS_SERVER_FIND_CHAR(scanRefresh, characteristicScanRefreshUuid),
};

static void blt_scpss_serviceInit(const struct blc_scpss_regParam *param)
{
    (void)param;
    struct blc_scps_server *server = blt_scpss_getCtrl(PRF_RFU_CONN_HANDLE);
    blc_atts_findCharacteristicByServiceUuid(serviceScanParametersUuid, ATT_16_UUID_LEN, scpssChar, ARRAY_SIZE(scpssChar), server);
    BLT_SCPS_LOG("Handle information, Scan Interval Window:0x%x, Scan Refresh:0x%x", server->scanIntervalWindowHdl, server->scanRefreshHdl);
}

/****************ScPS server init all characteristic handle end***********************/

static int blt_scpss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;
    BLT_SCPS_LOG("Write ConnHandle:0x%x, attrHandle:0x%x, value is %s", connHandle, attrHandle, hex_to_str(writeValue, valueLen));
    struct blc_scps_server *scpss = blt_scpss_getCtrl(connHandle);

    if (scpss->scanIntervalWindowHdl != attrHandle) {
        BLT_SCPS_LOG("ERR: write attrHandle is 0x%x, correct handle is 0x%x", attrHandle, scpss->scanIntervalWindowHdl);
        return ATT_ERR_INVALID_HANDLE;
    }

    if (valueLen != sizeof(struct scan_interval_window)) {
        return ATT_ERR_INVALID_PDU;
    }

    struct scan_interval_window *param = (struct scan_interval_window *)writeValue;

    if (CHECK_LE_SCAN_INTERVAL(param->leScanInterval) && CHECK_LE_SCAN_WINDOW(param->leScanWindow)) {
        struct blc_scpsc_scanIntervalWindowChangeEvt evt = {
            .param = {
                      .leScanInterval = param->leScanInterval,
                      .leScanWindow   = param->leScanWindow,
                      }
        };
        blt_prf_sendEvent(connHandle, SCPSS_EVT_SCAN_INTERVAL_WINDOW_CHANGE, &evt, sizeof(struct blc_scpsc_scanIntervalWindowChangeEvt));
    }

    return ATT_SUCCESS;
}

_attribute_ble_data_retention_ static u8 scanRefreshValue = SERVER_REQUIRES_REFRESH;

int blc_scpss_updateServerRequiresRefresh(u16 connHandle)
{
    struct blc_scps_server *scpss = blt_scpss_getCtrl(connHandle);

    return blc_gatts_notifyValue(connHandle, scpss->scanRefreshHdl, &scanRefreshValue, sizeof(scanRefreshValue));
}
