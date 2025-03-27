/********************************************************************************************************
 * @file    scps_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "drivers.h"
#include "stack/ble/ble.h"

#include "scps_internal.h"
#include "scps_client_buf.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"


static int blt_scpsc_init(u8 initType, const void* param);
static int blt_scpsc_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_scpsc_discovery(u16 connHandle);
static int blt_scpsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

static void blt_scpsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discScps;
#define BLC_SCPS_START_SDP(connHandle)          blc_gapc_registerDiscoveryService(connHandle, &discScps)

static const blc_gapc_reconnList_t reconnScps;
#define BLC_SCPS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnScps)

_attribute_ble_data_retention_
struct blc_scps_client_ctrl scps_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = SCPS_CLIENT,
        .usedAclRole = 0,
        .init = blt_scpsc_init,
        .connect = blt_scpsc_connect,
        .discov = blt_scpsc_discovery,
        .loop = NULL,
        .store = blt_scpsc_nv_store,
    },
};

void blc_basic_registerSCPSControlClient(const struct blc_scpsc_regParam *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&scps_client_ctrl, param);
}

static int blt_scpsc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_scps_client)), blc_scps_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_scps_client_ctrl)), blc_scps_client_ctrl);
#endif

    (void)param;

    if(initType == PRF_PROC_INIT) {
        BLT_SCPS_LOG("Client init");
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      BLT_SCPS_LOG("Client deinit");
//  }
    return 0;
}

static struct blc_scps_client *blt_scpsc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return scps_client_ctrl.pScpsClient[idx];
}

static int blt_scpsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_SCPS_LOG("Disconnect:0x%x", connHandle);
        SCPS_FREE(scps_client_ctrl.pScpsClient[idx]);
        scps_client_ctrl.pScpsClient[idx] = NULL;
    } else {
        BLT_SCPS_LOG("Connect:0x%x", connHandle);
        scps_client_ctrl.pScpsClient[idx] = SCPS_MALLOC(sizeof(struct blc_scps_client));
        memset(scps_client_ctrl.pScpsClient[idx], 0, sizeof(struct blc_scps_client));
    }

    return 0;
}

static int blt_scpsc_discovery(u16 connHandle)
{
    BLC_BASIC_SDP_DISCOVERY(connHandle, SCPS, scps);
}

static int blt_scpsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    BLC_BASIC_NV_STORE(connHandle, SCPS, scps, scanRefreshHdl);
    return 0;
}

static void blt_scpsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    BLT_SCPS_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    struct blc_scps_client* client = blt_scpsc_getClientInst(connHandle);
    if(client == NULL ||
        attHdl != client->scanRefreshHdl ||
        valLen != 1 || *val != SERVER_REQUIRES_REFRESH)
    {
        return ;
    }

    blt_prf_sendEvent(connHandle, SCPSC_EVT_RECV_SERVER_REQUIRES_REFRESH, NULL, 0);
}

/***************************ScPS sdp discovery*******************************/

static void blt_scpsc_displayInfo(u16 connHandle, struct blc_scps_client* client)
{
    BLT_SCPS_LOG("ScPS sdp over connHandle[0x%x]", connHandle);

    BLT_SCPS_LOG("Scan Interval Window:[handle:0x%x]", client->scanIntervalWindowHdl);

    if(client->scanRefreshHdl){
        BLT_SCPS_LOG("Scan Refresh:[handle:0x%x]", client->scanRefreshHdl);
    }
}

BLT_BASIC_SDP_DISCOVERY_SERVICE(scps, SCPS)
BLT_DEFINE_SCPS_DISCOVERY_FOUND_CHAR(scanIntervalWindow)
BLT_DEFINE_SCPS_DISCOVERY_FOUND_CHAR(scanRefresh)

static const blc_gapc_discService_t scpsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_SCAN_PARAMETERS),
    .sfun = blt_scpsc_foundService,
};

static const blc_gapc_discChar_t scpsChar[] = {
    BLT_PRF_DISCOVERY_WRITE_CHAR(scps, CHARACTERISTIC_UUID_SCAN_INTERVAL_WINDOW, scanIntervalWindow),
    BLT_PRF_DISCOVERY_NOTIFY_CHAR(scps, CHARACTERISTIC_UUID_SCAN_REFRESH, scanRefresh),
};

static const blc_gapc_discList_t discScps = {
    .maxServiceCount = 1,
    .service = &scpsService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(scpsChar),
        .characteristic = scpsChar,
    },
};

/***************************ScPS sdp discovery end*******************************/

/**********reconnect function********/
BLT_BASIC_RECONNECT_SERVICE(scps, SCPS)
BLT_DEFINE_PRF_RECONNECT_GET_INFO(scps, CHAR_PROP_NOTIFY, scanRefresh)  //Not used.

static const blc_gapc_reconnChar_t reScpsChar[] = {
    BLT_PRF_RECONNECT_NOTIFY_CHAR(scps, scanRefresh),
};

static const blc_gapc_reconnList_t reconnScps = {
    .resfun = blt_scpsc_recService,
    .charTb = {
        .size = ARRAY_SIZE(reScpsChar),
        .characteristic = reScpsChar,
    },
    .inclSize = 0,
};
/**********reconnect function ending********/

/**********Write Characteristic Attribute Value*********/

int blc_scpsc_writeScanIntervalWindow(u16 connHandle, struct scan_interval_window *scanIntervalWindow)
{
    if(!(CHECK_LE_SCAN_INTERVAL(scanIntervalWindow->leScanInterval) && CHECK_LE_SCAN_WINDOW(scanIntervalWindow->leScanWindow))) {
        BLT_SCPS_LOG("ERR: write scan interval is %.3fms, scan window is %.3fms", scanIntervalWindow->leScanInterval*0.625, scanIntervalWindow->leScanWindow*0.625);
        return PRF_COMMON_ERR_INPUT_PARAM_INVALID;
    }

    u16 scanIntervalWindowLen = sizeof(struct scan_interval_window);
    BLT_SCPS_WRITE_ATTR_VALUE_WITHOUT_RSP(scanIntervalWindow);
}


/**********Write Characteristic Attribute Value End*********/
