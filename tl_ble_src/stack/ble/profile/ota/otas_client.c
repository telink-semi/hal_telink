/********************************************************************************************************
 * @file    otas_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    03,2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "otas_internal.h"


static int blt_otasc_init(u8 initType, const void* param);
static int blt_otasc_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_otasc_discovery(u16 connHandle);
static int blt_otasc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);
static void blt_otasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discOtas;
static const blc_gapc_reconnList_t reconnOtas;

#if (MCU_CORE_TYPE != CHIP_TYPE_TL322X)
_attribute_ble_data_retention_
struct blc_otas_client_ctrl otas_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = OTAS_CLIENT,
        .usedAclRole = 0,
        .init = blt_otasc_init,
        .connect = blt_otasc_connect,
        .discov = blt_otasc_discovery,
        .loop = NULL,
        .store = blt_otasc_nv_store,
    },
};
#else
static const struct blc_prf_process_params s_otas_client_process_params = {
    .id          = OTAS_CLIENT,
    .usedAclRole = PRF_GAP_ACL_UNSPECIF,
    .init        = blt_otasc_init,
    .connect     = blt_otasc_connect,
    .discovery   = blt_otasc_discovery,
    .store       = blt_otasc_nv_store,
};

_attribute_ble_data_retention_ struct blc_otas_client_ctrl otas_client_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_otas_client_process_params,
                },
};
#endif

void blc_otas_registerOTASControlClient(const struct blc_otasc_regParam *param)
{
#if (MCU_CORE_TYPE != CHIP_TYPE_TL322X)
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&otas_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&otas_client_ctrl, param);
#endif
}

static int blt_otasc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_otas_client)), blc_otas_client);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(struct blc_otas_client_ctrl)), blc_otas_client_ctrl);
#endif
    (void)param;

    if(initType == PRF_PROC_INIT) {
        BLT_OTAS_LOG("Client init");
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      BLT_OTAS_LOG("Client deinit");
//  }
    return 0;
}

static struct blc_otas_client* blt_otasc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    return idx < ARRAY_SIZE(otas_client_ctrl.pOtasClient) ? &otas_client_ctrl.pOtasClient[idx] : NULL;
}

static int blt_otasc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_OTAS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_OTAS_LOG("Connect:0x%x", connHandle);
        memset(&otas_client_ctrl.pOtasClient[idx], 0, sizeof(struct blc_otas_client));
    }

    return 0;
}

static int blt_otasc_discovery(u16 connHandle)
{
    if(blc_prf_checkDiscoveryBusy(connHandle)) {
        return 0;
    }
    if(blc_prf_checkReconnectFlag(connHandle)) {
        struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);
        if(client->ntfInput.startHdl) {
            if(blc_gapc_registerReconnectService(connHandle, &reconnOtas) == BLE_SUCCESS) {
                blc_prf_sendServiceDiscoveryFoundEvent(connHandle, OTAS_CLIENT, client->ntfInput.startHdl, client->ntfInput.endHdl);
                blc_prf_setDiscoveryStatusBusy(connHandle);
                BLT_COMMON_LOG("SDP start reconnect connect handle: 0x%x", connHandle);
            }
        }
        else {
            BLT_COMMON_LOG("ATT information not found, connect handle is 0x%x", connHandle);
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, OTAS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
    } else if (blc_gapc_registerDiscoveryService(connHandle, &discOtas) == BLE_SUCCESS) {
        blc_prf_setDiscoveryStatusBusy(connHandle);
        BLT_COMMON_LOG("sdp start discovery connect handle is 0x%x", connHandle);
    }

    return 0;
}

static int blt_otasc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    BLC_PRF_NV_STORE1(connHandle, OTAS_CLIENT, otas, otaDataCccHdl);
    return 0;
}


static void blt_otasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    BLT_OTAS_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    if (!client) {
        return;
    }

    if (attHdl == client->otaDataHdl) {
        struct blc_otasc_notifEvt *evt = (struct blc_otasc_notifEvt *) val;

        blt_prf_sendEvent(connHandle, OTASC_EVT_NOTIF, evt, valLen);
    }
}

/***************************BAS sdp discovery*******************************/

static void blt_otasc_displayInfo(u16 connHandle, struct blc_otas_client* client)
{
    BLT_OTAS_LOG("OTAS sdp over connHandle[0x%x]", connHandle);
    BLT_OTAS_LOG("OTA data:[handle: 0x%x ccc: 0x%x]", client->otaDataHdl, client->otaDataCccHdl);
}

static void blt_otasc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);
    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, OTAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_OTAS_LOG("ERR:not found OTAS");
        return;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, OTAS_CLIENT);
        if(client) {
            blt_otasc_displayInfo(connHandle, client);
            blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        }
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    if(client) {
        client->ntfInput.startHdl = startHandle;
        client->ntfInput.endHdl = endHandle;
        client->ntfInput.ntfOrIndFunc = blt_otasc_dataInput;
    }

    BLT_OTAS_LOG("INFO: OTAS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, OTAS_CLIENT, startHandle, endHandle);
}

static const blc_gapc_discService_t otasService = {
    .uuid = UUID128_INIT(TELINK_OTA_UUID_SERVICE),
    .sfun = blt_otasc_foundService,
};

static void blt_otsc_foundOtaDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    (void) serviceCount;

    if ((properties & CHAR_PROP_READ) && (properties & CHAR_PROP_WRITE_WITHOUT_RSP) && (properties & CHAR_PROP_NOTIFY)) {
        client->otaDataHdl = valueHandle;
    }

    BLT_OTAS_LOG("OTA Data connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_otsc_subscribedOtaDataCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    (void) result;

    client->otaDataCccHdl = cccHandle;
}

static const blc_gapc_discChar_t otasChar[] = {
    {
        .subscribeNtf = true,
        .uuid = UUID128_INIT(TELINK_SPP_DATA_OTA),
        .cfun = blt_otsc_foundOtaDataChar,
        .scfun = blt_otsc_subscribedOtaDataCcc,
    },
};

static const blc_gapc_discList_t discOtas = {
    .maxServiceCount = 1,
    .service = &otasService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(otasChar),
        .characteristic = otasChar,
    },
};

/***************************BAS sdp discovery end*******************************/
/**********reconnect function start*********/
static bool blt_otasc_recService(u16 connHandle, int count)
{
    if(count == 0)
    {
        struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);
        if(client) {
            blt_otasc_displayInfo(connHandle, client);
            BLT_OTAS_LOG("   INFO: OTAS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);\
        }
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, OTAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;

    return true;
}

static int blt_otasc_otaDataGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->otaDataHdl;
    charInfo->cccHandle = client->otaDataCccHdl;

    return 1;
}

static const blc_gapc_reconnChar_t reOtsChar[] = {
    {
        .ifun = blt_otasc_otaDataGetInfo,
    },
};

static const blc_gapc_reconnList_t reconnOtas = {
    .resfun = blt_otasc_recService,
    .charTb = {
        .size = ARRAY_SIZE(reOtsChar),
        .characteristic = reOtsChar,
    },
    .inclSize = 0,
};
static void blt_otasc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    if (pRdCfg->single.handle == client->otaDataHdl) {
        BLT_OTAS_LOG("OTA Data read callback, status: 0x%02X", err);
    }

    blc_prf_readAttributeValueCallback(connHandle, err);
}

ble_sts_t blc_otasc_readOtaData(u16 connHandle, prf_read_cb_t readCb)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);
    gapc_read_cfg_t pGapReCfg;

    if (blc_prf_getAclConnectIndex(connHandle) < 0) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (!client || !client->otaDataHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapReCfg.handle = client->otaDataHdl;
    pGapReCfg.wBuff = (u8 *) client->otaDataReadResult;
    pGapReCfg.wBuffLen = &client->otaDataReadResultLength;
    pGapReCfg.maxLen = sizeof(client->otaDataReadResult);
    pGapReCfg.func = blt_otasc_readAttrValCb;

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

static void blt_otasc_writeCb(u16 connHandle, u8 err, void* data)
{
    (void) data;

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

ble_sts_t blc_otasc_writeOtaData(u16 connHandle, u16 length, u8 *data)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);
    gapc_write_cfg_t pGapWrCfg;

    if (blc_prf_getAclConnectIndex(connHandle) < 0) {
        BLT_OTAS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (!client || !client->otaDataHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapWrCfg.func = blt_otasc_writeCb;
    pGapWrCfg.handle = client->otaDataHdl;
    pGapWrCfg.data = data;
    pGapWrCfg.length = length;
    pGapWrCfg.withoutRsp = true;
    pGapWrCfg.cbData = NULL;

    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, NULL);
}

ble_sts_t blc_otasc_getOtaData(u16 connHandle, u16 *length, u8* buffer)
{
    struct blc_otas_client *client = blt_otasc_getClientInst(connHandle);

    if (!client || !client->otaDataHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (*length < client->otaDataReadResultLength) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    *length = client->otaDataReadResultLength;
    memcpy(buffer, client->otaDataReadResult, client->otaDataReadResultLength);

    return BLE_SUCCESS;
}
