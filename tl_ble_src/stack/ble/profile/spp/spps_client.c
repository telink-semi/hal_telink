/********************************************************************************************************
 * @file    spps_client.c
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
#include "spps_internal.h"


static int  blt_sppsc_init(u8 initType, const void *param);
static int  blt_sppsc_connect(u16 connHandle, prf_acl_state_enum connState);
static int  blt_sppsc_discovery(u16 connHandle);
static int  blt_sppsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);
static void blt_sppsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t   discSpps;
static const blc_gapc_reconnList_t reconnSpps;

_attribute_ble_data_retention_ blc_spps_client_ctrl_t spps_client_ctrl = {
    .process = {
                .pNext       = NULL,
                .id          = SPPS_CLIENT,
                .usedAclRole = 0,
                .init        = blt_sppsc_init,
                .connect     = blt_sppsc_connect,
                .discov      = blt_sppsc_discovery,
                .loop        = NULL,
                .store       = blt_sppsc_nv_store,
                },
};

void blc_spps_registerSPPSControlClient(const blc_sppsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&spps_client_ctrl, param);
}

static int blt_sppsc_init(u8 initType, const void *param)
{
//#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
//    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_spps_client_t)), blc_spps_client_t);
//    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_spps_client_ctrl_t)), blc_spps_client_ctrl_t);
//#endif
    (void)param;

    if (initType == PRF_PROC_INIT) {
        BLT_SPPS_LOG("Client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_SPPS_LOG("Client deinit");
    //  }
    return 0;
}

static blc_spps_client_t *blt_sppsc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    return idx < ARRAY_SIZE(spps_client_ctrl.pSppsClient) ? &spps_client_ctrl.pSppsClient[idx] : NULL;
}

static int blt_sppsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_SPPS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_SPPS_LOG("Connect:0x%x", connHandle);
        memset(&spps_client_ctrl.pSppsClient[idx], 0, sizeof(blc_spps_client_t));
    }

    return 0;
}

static int blt_sppsc_discovery(u16 connHandle)
{
    if (blc_prf_checkDiscoveryBusy(connHandle)) {
        return 0;
    }
    if (blc_prf_checkReconnectFlag(connHandle)) {
        blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);
        if (client->ntfInput.startHdl) {
            if (blc_gapc_registerReconnectService(connHandle, &reconnSpps) == BLE_SUCCESS) {
                blc_prf_sendServiceDiscoveryFoundEvent(connHandle, SPPS_CLIENT, client->ntfInput.startHdl, client->ntfInput.endHdl);
                blc_prf_setDiscoveryStatusBusy(connHandle);
                BLT_SPPS_LOG("SDP start reconnect connect handle: 0x%x", connHandle);
            }
        } else {
            BLT_SPPS_LOG("ATT information not found, connect handle is 0x%x", connHandle);
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, SPPS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
    } else if (blc_gapc_registerDiscoveryService(connHandle, &discSpps) == BLE_SUCCESS) {
        blc_prf_setDiscoveryStatusBusy(connHandle);
        BLT_SPPS_LOG("sdp start discovery connect handle is 0x%x", connHandle);
    }

    return 0;
}

static int blt_sppsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    blc_spps_client_t* client = blt_sppsc_getClientInst(connHandle);

    if (nvState == PRF_NV_STATE_STORE) {
        if (client->ntfInput.startHdl) {\
            blt_spps_nv_info_t nvInfo;

            blt_prf_storeClientHdl(&nvInfo.att, client, &client->sppDataCccHdl);
            U8_TO_STREAM(param->dataPtr, sizeof(blt_spps_nv_info_t));
            U8_TO_STREAM(param->dataPtr, SPPS_CLIENT);
            STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_spps_nv_info_t));
            param->currentTotalLen += 2 + sizeof(blt_spps_nv_info_t);
        }
    } else if (nvState == PRF_NV_STATE_LOAD) {
        blt_spps_nv_info_t *nvInfo = (blt_spps_nv_info_t *) param->dataPtr;
        blt_prf_loadClientHdl(client, &nvInfo->att, &client->sppDataCccHdl);
        client->ntfInput.ntfOrIndFunc = blt_sppsc_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }
    return 0;
}

static void blt_sppsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);

    BLT_SPPS_LOG("receive data, connHandle:0x%x, attHdl:0x%x, value:%s", connHandle, attHdl, hex_to_str(val, valLen));

    if (!client) {
        return;
    }

    if (attHdl == client->sppDataHdl) {
        blc_sppsc_notifEvt_t *evt = (blc_sppsc_notifEvt_t *)val;

        blt_prf_sendEvent(connHandle, SPPSC_EVT_NOTIF, evt, valLen);
    }
}

/***************************BAS sdp discovery*******************************/

static void blt_sppsc_displayInfo(u16 connHandle, blc_spps_client_t *client)
{
    BLT_SPPS_LOG("SPPS sdp over connHandle[0x%x]", connHandle);
    BLT_SPPS_LOG("SPP data:[handle: 0x%x ccc: 0x%x]", client->sppDataHdl, client->sppDataCccHdl);
}

static void blt_sppsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);
    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, SPPS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_SPPS_LOG("ERR:not found SPPS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, SPPS_CLIENT);
        if (client) {
            blt_sppsc_displayInfo(connHandle, client);
            blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        }
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    if (client) {
        client->ntfInput.startHdl     = startHandle;
        client->ntfInput.endHdl       = endHandle;
        client->ntfInput.ntfOrIndFunc = blt_sppsc_dataInput;
    }

    BLT_SPPS_LOG("INFO: SPPS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, SPPS_CLIENT, startHandle, endHandle);
}

static const blc_gapc_discService_t sppsService = {
    .uuid = UUID128_INIT(TELINK_SPP_UUID_SERVICE),
    .sfun = blt_sppsc_foundService,
};

static void blt_sppc_foundSppDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_READ) && (properties & CHAR_PROP_WRITE_WITHOUT_RSP) && (properties & CHAR_PROP_NOTIFY)) {
        client->sppDataHdl = valueHandle;
    }

    BLT_SPPS_LOG("SPP Data connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_sppc_subscribedSppDataCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);

    (void)result;

    client->sppDataCccHdl = cccHandle;
}

static const blc_gapc_discChar_t sppsChar[] = {
    {
     .subscribeNtf = true,
     .uuid         = UUID128_INIT(TELINK_SPP_DATA_CLIENT2SERVER),
     .cfun         = blt_sppc_foundSppDataChar,
     .scfun        = blt_sppc_subscribedSppDataCcc,
     },
};

static const blc_gapc_discList_t discSpps = {
    .maxServiceCount = 1,
    .service         = &sppsService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(sppsChar),
                        .characteristic = sppsChar,
                        },
};

/***************************BAS sdp discovery end*******************************/
/**********reconnect function start*********/
static bool blt_sppsc_recService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);
        if (client) {
            blt_sppsc_displayInfo(connHandle, client);
            BLT_SPPS_LOG("   INFO: SPPS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        }
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, SPPS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if (count > 1) {
        return false;
    }

    return true;
}

static int blt_sppsc_sppDataGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->sppDataHdl;
    charInfo->cccHandle   = client->sppDataCccHdl;

    return 1;
}

static const blc_gapc_reconnChar_t reSppChar[] = {
    {
     .ifun = blt_sppsc_sppDataGetInfo,
     },
};

static const blc_gapc_reconnList_t reconnSpps = {
    .resfun = blt_sppsc_recService,
    .charTb = {
               .size           = ARRAY_SIZE(reSppChar),
               .characteristic = reSppChar,
               },
    .inclSize = 0,
};

static void blt_sppsc_writeCb(u16 connHandle, u8 err, void *data)
{
    (void)data;

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

ble_sts_t blc_sppsc_writeSppData(u16 connHandle, u16 length, u8 *data)
{
    blc_spps_client_t *client = blt_sppsc_getClientInst(connHandle);
    gapc_write_cfg_t        pGapWrCfg;

    if (blc_prf_getAclConnectIndex(connHandle) < 0) {
        BLT_SPPS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if (!client || !client->sppDataHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapWrCfg.func       = blt_sppsc_writeCb;
    pGapWrCfg.handle     = client->sppDataHdl;
    pGapWrCfg.data       = data;
    pGapWrCfg.length     = length;
    pGapWrCfg.withoutRsp = true;
    pGapWrCfg.cbData     = NULL;

    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, NULL);
}
