/********************************************************************************************************
 * @file    csis_client.c
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




static int blt_csis_disconnect(u16 connHandle);
static void blt_csisc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discCsis;
#define BLC_CSIS_START_SDP(connHandle)          blc_gapc_registerDiscoveryService(connHandle, &discCsis)

static const blc_gapc_reconnList_t reconnCsis;
#define BLC_CSIS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnCsis)

_attribute_ble_data_retention_
blc_csis_client_ctrl_t csis_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_CSIS_CLIENT,
        .usedAclRole = 0,
        .init = blt_csisc_init,
        .connect = blt_csisc_connect,
        .discov = blt_csisc_discovery,
        .loop = NULL,
        .store = blt_csisc_nv_store,
    },
};

void blc_audio_registerCSISControlClient(const blc_csisc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&csis_client_ctrl, param);
}

int blt_csisc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_csis_client_t)), blc_csis_client_t);
#endif
    (void)param;

    if(initType == PRF_PROC_INIT) {

        for(int i=0; i<gAppAudioAclCentralNum; i++) {
            csis_client_ctrl.pCsisClient[i] = blt_csisc_getClientBuf(i);
            /* Clear CSIS Client parameters  */
            memset(csis_client_ctrl.pCsisClient[i], 0, sizeof(blc_csis_client_t));
            /* Initialize Pointer buffer */
        }
    } else if (initType == PRF_PROC_DEINIT) {
    }
    return 0;
}

blc_csis_client_t *blt_csisc_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_PERIPHERAL) {
        BLT_CSIS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* CSIP Set Coordinator GAP Central */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_CSIS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return csis_client_ctrl.pCsisClient[idx];
}


int blt_csisc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        blt_csis_disconnect(connHandle);
        BLT_CSIS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_CSIS_LOG("Connect:0x%x", connHandle);
    }
    return 0;
}


int blt_csisc_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, CSIS, csis);
}

int blt_csisc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    BLC_COMMON_NV_STORE(connHandle, CSIS, csis, setMemberRankHdl);
    return 0;
}

static int blt_csis_disconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_csis_client_t* csis = blt_csisc_getClientInst(connHandle);

    memset(csis, 0, sizeof(blc_csis_client_t));

    return BLE_SUCCESS;
}



void blt_csisc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    (void)connHandle;
    (void)attHdl;
    (void)val;
    (void)valLen;
}

static void blt_csisc_displayInfo(u16 connHandle, blc_csis_client_t* client)
{
    BLT_CSIS_LOG("sdp over connHandle[0x%x]", connHandle);
    BLT_CSIS_LOG("  INFO:Set Identity Resolving Key: type is %d, value is %s", client->sirk.type, hex_to_str(client->sirk.value, 16));
    BLT_CSIS_LOG("  INFO:Coordinated Set Size is %d", client->coordinatedSetSize);
    BLT_CSIS_LOG("  INFO:Lock value is %d, Rank value is %d", client->lock, client->rank);
}


static void blt_csisc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_CSIS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_CSIS_LOG("ERR:not found CSIS");
        return ;
    }
    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_CSIS_CLIENT);
        blt_csisc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }
    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_csisc_dataInput;
    BLT_CSIS_LOG("  INFO: CSIS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_CSIS_CLIENT, startHandle, endHandle);
}

static void blt_csisc_foundSirkChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    (void)serviceCount;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    client->setIdentityResolvingKeyHdl = valueHandle;
    BLT_CSIS_LOG("set Identity Resolving Key ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_csisc_sirkStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    (void)attrHandle;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    *read = (u8*)&client->sirk;
    *readLen = NULL;
    *readMaxSize = sizeof(svc_csis_SIRK_t);
    *rdCbFunc = NULL;
}

static void blt_csisc_foundSizeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    (void)serviceCount;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    client->coordinatedSetSizeHdl = valueHandle;
    BLT_CSIS_LOG("coordinated Set Size ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_csisc_sizeStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    (void)attrHandle;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    *read = (u8*)&client->coordinatedSetSize;
    *readLen = NULL;
    *readMaxSize = 1;
    *rdCbFunc = NULL;
}

static void blt_csisc_foundLockChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    (void)serviceCount;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    client->setMemberLockHdl = valueHandle;
    BLT_CSIS_LOG("set Member Lock ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_csisc_lockStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    (void)attrHandle;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    *read = (u8*)&client->lock;
    *readLen = NULL;
    *readMaxSize = 1;
    *rdCbFunc = NULL;
}

static void blt_csisc_foundRankChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    (void)serviceCount;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    client->setMemberRankHdl = valueHandle;
    BLT_CSIS_LOG("set Member Rank ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_csisc_rankStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    (void)attrHandle;
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);
    *read = (u8*)&client->rank;
    *readLen = NULL;
    *readMaxSize = 1;
    *rdCbFunc = NULL;
}


static const blc_gapc_discService_t csisService = {
    .uuid = UUID16_INIT(SERVICE_UUID_COORDINATED_SET_IDENTIFICATION),
    .sfun = blt_csisc_foundService,
};

static const blc_gapc_discChar_t csisChar[] = {
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_SET_IDENTITY_RESOLVING_KEY),
        .cfun = blt_csisc_foundSirkChar,
        .rfun = blt_csisc_sirkStartRead,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_COORDINATED_SET_SIZE),
        .cfun = blt_csisc_foundSizeChar,
        .rfun = blt_csisc_sizeStartRead,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_SET_MEMBER_LOCK),
        .cfun = blt_csisc_foundLockChar,
        .rfun = blt_csisc_lockStartRead,
    },
    {
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_SET_MEMBER_RANK),
        .cfun = blt_csisc_foundRankChar,
        .rfun = blt_csisc_rankStartRead,
    },
};

static const blc_gapc_discList_t discCsis = {
    .maxServiceCount = 1,
    .service = &csisService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(csisChar),
        .characteristic = csisChar,
    },
};


/**********reconnect function start*********/
static bool blt_csisc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_csis_client_t *client = blt_csisc_getClientInst(connHandle);
        blt_csisc_displayInfo(connHandle, client);
        BLT_CSIS_LOG("  INFO: CSIS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_CSIS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static int blt_csisc_sirkGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->setIdentityResolvingKeyHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_csisc_sizeGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->coordinatedSetSizeHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_csisc_lockGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->setMemberLockHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_csisc_rankGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = client->setMemberRankHdl;

    return 1;
}
static const blc_gapc_reconnChar_t reCsisChar[] = {

    {
        .ifun = blt_csisc_sirkGetInfo,
        .rfun = blt_csisc_sirkStartRead,
    },

    {
        .ifun = blt_csisc_sizeGetInfo,
        .rfun = blt_csisc_sizeStartRead,
    },

    {
        .ifun = blt_csisc_lockGetInfo,
        .rfun = blt_csisc_lockStartRead,
    },

    {
        .ifun = blt_csisc_rankGetInfo,
        .rfun = blt_csisc_rankStartRead,
    },
};

static const blc_gapc_reconnList_t reconnCsis = {
    .resfun = blt_csisc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reCsisChar),
        .characteristic = reCsisChar,
    },
    .inclSize = 0,
};

/**********reconnect function ending********/


int blc_csisc_getSetIdentityResolvingKey(u16 connHandle, u8 outSIRK[16])
{
    if(outSIRK == NULL || blt_ll_isAclHandleOutOfRange(connHandle)) {
        BLT_CSIS_LOG("ERR:outSIRK=0x%x, connHandle=0x%x", outSIRK, connHandle);
        return AUDIO_EPARAM;
    }

    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    if(client == NULL || !client->setIdentityResolvingKeyHdl){
        BLT_CSIS_LOG("ERR:client=0x%x, setIdentityResolvingKeyHdl=0x%x", client, client->setIdentityResolvingKeyHdl);
        return AUDIO_EPARAM;
    }

    if(client->sirk.type == BLT_CSIS_ENCRYPTED_SIRK){ //0:Encrypted
        blt_csis_cryptoSIRKEncDec(connHandle, client->sirk.value, outSIRK);
        BLT_CSIS_LOG("Encrypted SIRK => Plaintext SIRK", hex_to_str(client->sirk.value, 16), hex_to_str(outSIRK, 16));
    }else{
        memcpy(outSIRK, client->sirk.value, 16);
        BLT_CSIS_LOG("Plaintext SIRK", hex_to_str(client->sirk.value, 16));
    }

    return AUDIO_ESUCC;
}

int blc_csisc_getCoordinatedSetSize(u16 connHandle, u8 outSetSize[1])
{
    if(outSetSize == NULL || blt_ll_isAclHandleOutOfRange(connHandle)) {
        BLT_CSIS_LOG("ERR:outSetSize=0x%x, connHandle=0x%x", outSetSize, connHandle);
        return AUDIO_EPARAM;
    }

    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    if(client == NULL || !client->coordinatedSetSizeHdl){
        BLT_CSIS_LOG("ERR:client=0x%x, coordinatedSetSizeHdl=0x%x", client, client->coordinatedSetSizeHdl);
        return AUDIO_EPARAM;
    }

    *outSetSize = client->coordinatedSetSize;

    return AUDIO_ESUCC;
}

int blc_csisc_getSetMemberRank(u16 connHandle, u8 outRank[1])
{
    if(outRank == NULL || blt_ll_isAclHandleOutOfRange(connHandle)) {
        BLT_CSIS_LOG("ERR:outRank=0x%x, connHandle=0x%x", outRank, connHandle);
        return AUDIO_EPARAM;
    }

    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    if(client == NULL || !client->setMemberRankHdl){
        BLT_CSIS_LOG("ERR:client=0x%x, setMemberRankHdl=0x%x", client, client->setMemberRankHdl);
        return AUDIO_EPARAM;
    }

    *outRank = client->rank;

    return AUDIO_ESUCC;
}

int blc_csisc_getSetMemberLock(u16 connHandle, u8 outLock[1])
{
    if(outLock == NULL || blt_ll_isAclHandleOutOfRange(connHandle)) {
        BLT_CSIS_LOG("ERR:outLock=0x%x, connHandle=0x%x", outLock, connHandle);
        return AUDIO_EPARAM;
    }

    blc_csis_client_t* client = blt_csisc_getClientInst(connHandle);

    if(client == NULL || !client->setMemberLockHdl){
        BLT_CSIS_LOG("ERR:client=0x%x, setMemberLockHdl=0x%x", client, client->setMemberLockHdl);
        return AUDIO_EPARAM;
    }

    *outLock = client->lock;

    return AUDIO_ESUCC;
}




