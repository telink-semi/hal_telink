/********************************************************************************************************
 * @file    bas_client.c
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

#include "bas_internal.h"
#include "bas_client_buf.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"

static int blt_basc_init(u8 initType, const void *param);
static int blt_basc_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_basc_discovery(u16 connHandle);
static int blt_basc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);

static void blt_basc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discBas;
#define BLC_BAS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discBas)

static const blc_gapc_reconnList_t reconnBas;
#define BLC_BAS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnBas)

#if ((!defined(HOST_V2_ENABLE)))

_attribute_ble_data_retention_ struct blc_bas_client_ctrl bas_client_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = BAS_CLIENT,
                .usedAclRole = 0,
                .init        = blt_basc_init,
                .connect     = blt_basc_connect,
                .discov      = blt_basc_discovery,
                .loop        = NULL,
                .store       = blt_basc_nv_store,
                },
};
#else
static const struct blc_prf_process_params s_bas_client_process_params = {
    .id          = BAS_CLIENT,
    .usedAclRole = PRF_GAP_ACL_UNSPECIF,
    .init        = blt_basc_init,
    .connect     = blt_basc_connect,
    .discovery   = blt_basc_discovery,
    .store       = blt_basc_nv_store,
};

_attribute_ble_data_retention_ struct blc_bas_client_ctrl bas_client_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_bas_client_process_params,
                },
};
#endif

void blc_basic_registerBASControlClient(const struct blc_basc_regParam *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&bas_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&bas_client_ctrl, param);
#endif
}

static int blt_basc_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_bas_client)), blc_bas_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_bas_client_ctrl)), blc_bas_client_ctrl);
#endif
    (void)param;

    if (initType == PRF_PROC_INIT) {
        BLT_BAS_LOG("Client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_BAS_LOG("Client deinit");
    //  }
    return 0;
}

static struct blc_bas_client *blt_basc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return bas_client_ctrl.pBasClient[idx];
}

static int blt_basc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_BAS_LOG("Disconnect:0x%x", connHandle);
        BAS_FREE(bas_client_ctrl.pBasClient[idx]);
        bas_client_ctrl.pBasClient[idx] = NULL;
    } else {
        BLT_BAS_LOG("Connect:0x%x", connHandle);
        bas_client_ctrl.pBasClient[idx] = BAS_MALLOC(sizeof(struct blc_bas_client));
        memset(bas_client_ctrl.pBasClient[idx], 0, sizeof(struct blc_bas_client));
    }

    return 0;
}

static int blt_basc_discovery(u16 connHandle)
{
    BLC_BASIC_SDP_DISCOVERY(connHandle, BAS, bas);
}

static int blt_basc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_BASIC_NV_STORE(connHandle, BAS, bas, batteryPowerStateHdl);
    return 0;
}

static void blt_basc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    BLT_BAS_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    struct blc_bas_client *client = blt_basc_getClientInst(connHandle);
    if (client == NULL) {
        return;
    }

    if (attHdl == client->batteryLevelHdl) {
        if (valLen == 1 && CHECK_BATTERY_LEVEL(*val)) {
            client->batteryLevel = *val;
        }

        struct blc_basc_batteryLevelChangeEvt evt = {
            .batteryLevel = client->batteryLevel};
        blt_prf_sendEvent(connHandle, BASC_EVT_BATTERY_LEVEL_CHANGE, &evt, sizeof(struct blc_basc_batteryLevelChangeEvt));
    } else if (attHdl == client->batteryPowerStateHdl) {
        if (valLen == 1) {
            client->batteryPowerState = *val;
        }

        struct blc_basc_batteryPowerStateChangeEvt evt = {
            .batteryPowerState = client->batteryPowerState};
        blt_prf_sendEvent(connHandle, BASC_EVT_BATTERY_POWER_STATE_CHANGE, &evt, sizeof(struct blc_basc_batteryPowerStateChangeEvt));
    }
}

/***************************BAS sdp discovery*******************************/

static void blt_basc_displayInfo(u16 connHandle, struct blc_bas_client *client)
{
    BLT_BAS_LOG("BAS sdp over connHandle[0x%x]", connHandle);
    BLT_BAS_LOG("Battery Level:[handle: 0x%x value %d]", client->batteryLevelHdl, client->batteryLevel);
    BLT_BAS_LOG("Battery Power State:[handle: 0x%x value 0x%x]", client->batteryPowerStateHdl, client->batteryPowerState);
}

BLT_BASIC_SDP_DISCOVERY_SERVICE(bas, BAS)
BLT_DEFINE_BAS_DISCOVERY_FOUND_CHAR(batteryLevel)
BLT_DEFINE_BAS_DISCOVERY_START_READ_FIX_LEN(batteryLevel)
BLT_DEFINE_BAS_DISCOVERY_FOUND_CHAR(batteryPowerState)
BLT_DEFINE_BAS_DISCOVERY_START_READ_FIX_LEN(batteryPowerState)


static const blc_gapc_discService_t basService = {
    .uuid = UUID16_INIT(SERVICE_UUID_BATTERY),
    .sfun = blt_basc_foundService,
};

static const blc_gapc_discChar_t basChar[] = {
    BLT_BAS_DISCOVERY_READ_NOTIFY_CHAR(CHARACTERISTIC_UUID_BATTERY_LEVEL, batteryLevel),
    BLT_BAS_DISCOVERY_READ_NOTIFY_CHAR(CHARACTERISTIC_UUID_BATTERY_POWER_STATE, batteryPowerState),
};

static const blc_gapc_discList_t discBas = {
    .maxServiceCount = 1,
    .service         = &basService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(basChar),
                        .characteristic = basChar,
                        },
};

/***************************BAS sdp discovery end*******************************/

/**********reconnect function start*********/
BLT_BASIC_RECONNECT_SERVICE(bas, BAS)
BLT_BAS_RECONNECT_GET_INFO_READ(batteryLevel)
BLT_BAS_RECONNECT_GET_INFO_READ(batteryPowerState)

static const blc_gapc_reconnChar_t reBasChar[] = {
    BLT_BAS_RECONNECT_CHAR(batteryLevel),
    BLT_BAS_RECONNECT_CHAR(batteryPowerState),
};

static const blc_gapc_reconnList_t reconnBas = {
    .resfun = blt_basc_recService,
    .charTb = {
               .size           = ARRAY_SIZE(reBasChar),
               .characteristic = reBasChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/


/**********Read Characteristic Attribute Value*********/
int blc_basc_readBatteryLevel(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_BAS_READ_ATTR_VALUE_FIX_LEN(batteryLevel);
}

int blc_basc_readBatteryPowerState(u16 connHandle, prf_read_cb_t readCb)
{
    BLT_BAS_READ_ATTR_VALUE_FIX_LEN(batteryPowerState);
}

/**********Read Characteristic Attribute Value End*********/

/**********Get Characteristic Attribute Value*********/

int blc_basc_getBatteryLevel(u16 connHandle, u8 *batteryLevel)
{
    BLT_BAS_GET_ATTR_VALUE_FIX_LEN(batteryLevel);
}

int blc_basc_getBatteryPowerState(u16 connHandle, u8 *batteryPowerState)
{
    BLT_BAS_GET_ATTR_VALUE_FIX_LEN(batteryPowerState);
}

/**********Get Characteristic Attribute Value End*********/
