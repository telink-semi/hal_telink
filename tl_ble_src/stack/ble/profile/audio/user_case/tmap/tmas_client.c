/********************************************************************************************************
 * @file    tmas_client.c
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


static int blt_tmasc_disconnect(u16 connHandle);

static const blc_gapc_discList_t discTmas;
#define BLC_TMAS_START_SDP(connHandle)          blc_gapc_registerDiscoveryService(connHandle, &discTmas)

static const blc_gapc_reconnList_t reconnTmas;
#define BLC_TMAS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnTmas)

_attribute_ble_data_retention_
blc_tmas_client_ctrl_t tmas_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_TMAS_CLIENT,
        .usedAclRole = 0,
        .init = blt_tmasc_init,
        .connect = blt_tmasc_connect,
        .discov = blt_tmasc_discovery,
        .loop = NULL,
        .store = blt_tmasc_nv_store,
    },
};

void blc_audio_registerTMASControlClient(const blc_tmasc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&tmas_client_ctrl, param);
}

blc_tmas_client_t *blt_tmasc_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);

    if(ret < 0 || (0 && ret == ACL_ROLE_PERIPHERAL)) {
        BLT_TMAS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* TMAP Client GAP Central and Peripheral ? */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_TMAS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return tmas_client_ctrl.pTmasClient[idx];
}

int blt_tmasc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_tmas_client_t)), blc_tmas_client_t);
#endif
    (void)param;

    if(initType == PRF_PROC_INIT) {

        for (int i = 0; i < gAppAudioAclMaxNum; i++) {
            blc_tmas_client_t *tmasClient = blt_tmasc_getClientBuf(i);
            tmas_client_ctrl.pTmasClient[i] = tmasClient;
            /* Clear PACS Client parameters  */
            memset(tmasClient, 0, sizeof(blc_tmas_client_t));
        }
        BLT_TMAS_LOG("client init");
    }
//  else if (initType == PRF_PROC_DEINIT) {
//  }
    return 0;
}

int blt_tmasc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_TMAS_LOG("Disconnect:0x%x", connHandle);
        blt_tmasc_disconnect(connHandle);
    } else {
        BLT_TMAS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

int blt_tmasc_discovery(u16 connHandle)
{
    if(blc_prf_checkDiscoveryBusy(connHandle))
        return 0;

    if(blc_prf_checkReconnectFlag(connHandle))
    {
        blc_tmas_client_t *client = blt_tmasc_getClientInst(connHandle);
        if(client->tmasRoleHdl)
        {
            if(BLC_TMAS_START_RECONN(connHandle) == BLE_SUCCESS)
            {
            blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_TMAS_CLIENT, client->tmasRoleHdl, client->tmasRoleHdl);
            blc_prf_setDiscoveryStatusBusy(connHandle);
            BLT_AUD_LOG("SDP start reconnect connect handle: 0x%x", connHandle);
            }
        }
        else
        {
            BLT_AUD_LOG("ATT information not found, connect handle is 0x%x", connHandle);
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_TMAS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
        return 0;
    }

    if(BLC_TMAS_START_SDP(connHandle) == BLE_SUCCESS)
    {
        blc_prf_setDiscoveryStatusBusy(connHandle);
        BLT_AUD_LOG("sdp start discovery connect handle is 0x%x", connHandle);
    }
    return 0;
}

int blt_tmasc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    blc_tmas_client_t* client = blt_tmasc_getClientInst(connHandle);
    if(nvState == PRF_NV_STATE_STORE)   {
        if(client->tmasRoleHdl) {
            U8_TO_STREAM(param->dataPtr, sizeof(blt_tmas_nv_info_t));
            U8_TO_STREAM(param->dataPtr, AUDIO_TMAS_CLIENT);
            U16_TO_STREAM(param->dataPtr, client->tmasRoleHdl);
            param->currentTotalLen += 4;
        }
    }
    else if(nvState == PRF_NV_STATE_LOAD) {
        STREAM_TO_U16(client->tmasRoleHdl, param->dataPtr);
    }
    return 0;
}

static int blt_tmasc_disconnect(u16 connHandle)
{
    blc_tmas_client_t *pTmasClt = blt_tmasc_getClientInst(connHandle);
    //TODO: clear pending variable
    /* Clear PACS Client parameters  */
    memset(pTmasClt, 0, sizeof(blc_tmas_client_t));

    return BLE_SUCCESS;
}

/***************************sdp discovery start*******************************/

static void blt_tmasc_displayInfo(u16 connHandle, blc_tmas_client_t* client)
{
    BLT_TMAS_LOG("TMAS sdp over connHandle[0x%x]", connHandle);

    BLT_TMAS_LOG("TMAP Role Handle is 0x%x", client->tmasRoleHdl);

    BLT_TMAS_LOG("CALL Gateway: %s", client->tmasRole & BLC_TMAP_ROLE_CALL_GATEWAY? "Supported": "Not Supported");
    BLT_TMAS_LOG("CALL Terminal: %s", client->tmasRole & BLC_TMAP_ROLE_CALL_TERMINAL? "Supported": "Not Supported");
    BLT_TMAS_LOG("Unicast Media Sender: %s", client->tmasRole & BLC_TMAP_ROLE_UNICAST_MEDIA_SENDER? "Supported": "Not Supported");
    BLT_TMAS_LOG("Unicast Media Receiver: %s", client->tmasRole & BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER? "Supported": "Not Supported");
    BLT_TMAS_LOG("Broadcast Media Sender: %s", client->tmasRole & BLC_TMAP_ROLE_BROADCAST_MEDIA_SENDER? "Supported": "Not Supported");
    BLT_TMAS_LOG("Broadcast Media Receiver: %s", client->tmasRole & BLC_TMAP_ROLE_BROADCAST_MEDIA_RECEIVER? "Supported": "Not Supported");

}

static void blt_tmasc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_tmas_client_t* client = blt_tmasc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_TMAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_TMAS_LOG("ERR:not found TMAS");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_TMAS_CLIENT);
        blt_tmasc_displayInfo(connHandle, client);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    BLT_TMAS_LOG("  INFO: TMAS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_TMAS_CLIENT, startHandle, endHandle);
}

static void blt_tmasc_foundTmapRoleChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_tmas_client_t* client = blt_tmasc_getClientInst(connHandle);

    client->tmasRoleHdl = valueHandle;
    BLT_TMAS_LOG("TMAS tmap Role ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_tmasc_tmapRoleStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_tmas_client_t* client = blt_tmasc_getClientInst(connHandle);
    *read = (u8*)&client->tmasRole;
    *readLen = NULL;
    *readMaxSize = sizeof(client->tmasRole);
    *rdCbFunc = NULL;
}

static const blc_gapc_discService_t tmasService = {
    .uuid = UUID16_INIT(SERVICE_UUID_TELEPHONY_AND_MEDIA_AUDIO),
    .sfun = blt_tmasc_foundService,
};

static const blc_gapc_discChar_t tmasChar[] = {
    {
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_TMAP_ROLE),
        .cfun = blt_tmasc_foundTmapRoleChar,
        .rfun = blt_tmasc_tmapRoleStartRead,
    },
};

static const blc_gapc_discList_t discTmas = {
    .maxServiceCount = 1,
    .service = &tmasService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(tmasChar),
        .characteristic = tmasChar,
    },
};

/***************************sdp discovery end*******************************/

/**********reconnect function start*********/
static bool blt_tmasc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_tmas_client_t *client = blt_tmasc_getClientInst(connHandle);
        blt_tmasc_displayInfo(connHandle, client);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_TMAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}


static int blt_tmasc_tmapRoleGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_tmas_client_t* client = blt_tmasc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = client->tmasRoleHdl;

    return 1;
}
static const blc_gapc_reconnChar_t reTmasChar[] = {

    {
        .ifun = blt_tmasc_tmapRoleGetInfo,
        .rfun = blt_tmasc_tmapRoleStartRead,
    },
};

static const blc_gapc_reconnList_t reconnTmas = {
    .resfun = blt_tmasc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reTmasChar),
        .characteristic = reTmasChar,
    },
    .inclSize = 0,
};

/**********reconnect function ending********/
