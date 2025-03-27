/********************************************************************************************************
 * @file    csis_server.c
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


static void blt_csiss_serviceInit(const blc_csiss_regParam_t *server);
static int blt_csiss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8** outValue, u16* outValueLen);
static int blt_csiss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);

_attribute_ble_data_retention_
blc_csis_server_ctrl_t  csis_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_CSIS_SERVER,
        .usedAclRole = 0,
        .init = blt_csiss_init,
        .connect = blt_csiss_connect,
        .discov = NULL,
        .loop = blt_csiss_loop,
    },
};

#define CSISS_GET_SIZE                          *blc_gatts_getAttributeValueByHandle(0xFFFF, csis_server_ctrl.server.CSSizeHandle)
#define CSISS_GET_LOCK                          *blc_gatts_getAttributeValueByHandle(0xFFFF, csis_server_ctrl.server.memberLockHandle)
#define CSISS_GET_RANK                          *blc_gatts_getAttributeValueByHandle(0xFFFF, csis_server_ctrl.server.memberRankHandle)
#define CSISS_GET_SIRK                          blc_gatts_getAttributeValueByHandle(0xFFFF, csis_server_ctrl.server.SIRKHandle)

#define CSISS_SET_SIZE(size)                    CSISS_GET_SIZE = size
#define CSISS_SET_LOCK(lock)                    CSISS_GET_LOCK = lock
#define CSISS_SET_RANK(rank)                    CSISS_GET_RANK = rank

void blc_audio_registerCSISControlServer(const blc_csiss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t*)&csis_server_ctrl, (const void*)param);
}

#if (0)
static blc_csis_server_t* blt_csiss_getCtrl(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_CSIS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* CSIP Set Member GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_CSIS_SERVER, ret);
        }

        return NULL;
    }

    return &csis_server_ctrl.server;
}
#else
static blc_csis_server_t* blt_csiss_getCtrl(void)
{
    return &csis_server_ctrl.server;
}
#endif

int blt_csiss_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_csis_server_t)), blc_csis_server_t);
#endif

    if(initType == PRF_PROC_INIT) {
        blc_svc_addCasGroup();
        blc_svc_addCsisGroup();
        blc_svc_csisCbackRegister(blt_csiss_readCback, blt_csiss_writeCback);

        blt_csiss_serviceInit(param);
//      BLT_CSIS_LOG("Server init");
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removeCasGroup();
//      blc_svc_removeCsisGroup();
//      BLT_CSIS_LOG("Server Deinit");
//  }
    return 0;
}

static void blt_csiss_startLockTimer(u16 connHandle)
{
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    CSISS_SET_LOCK(BLC_CSIS_LOCKED);
    csiss->memberLockedConnHandle = connHandle;
    csiss->memberLockedTimer = clock_time() | 1;
}

static void blt_csiss_stopLockTimer(void)
{
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    CSISS_SET_LOCK(BLC_CSIS_UNLOCKED);
    csiss->memberLockedTimer = 0;
}

int blt_csiss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    if(connState == PRF_ACL_STATE_DISCONN) {
        if(csiss->memberLockedConnHandle == connHandle)
            blt_csiss_stopLockTimer();
        BLT_CSIS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_CSIS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

int blt_csiss_loop(u16 connHandle)
{
    (void)connHandle;
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    if(csiss->memberLockedTimer && clock_time_exceed(csiss->memberLockedTimer, csiss->memberLockedTimeout*1000*1000))
        blt_csiss_stopLockTimer();

    return 0;
}

static int blt_csiss_readSIRKCback(u16 connHandle)
{
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    u8* sirk = CSISS_GET_SIRK;
    if(csiss->type == BLT_CSIS_PLAIN_TEXT_SIRK)     //1:plain text
    {
        U8_TO_STREAM(sirk, BLT_CSIS_PLAIN_TEXT_SIRK);
        memcpy(sirk, csiss->plainSIRK, 16);
    }
    else if(csiss->type == BLT_CSIS_ENCRYPTED_SIRK) //0:Encrypted
    {
        U8_TO_STREAM(sirk, BLT_CSIS_ENCRYPTED_SIRK);
        blt_csis_cryptoSIRKEncDec(connHandle, csiss->plainSIRK, sirk);
    }
    else                        //2:only OOB
    {
        return BLT_CSIS_ERROR_CODE_OOB_SIRK_ONLY;
    }
    return ATT_SUCCESS;
}

static int blt_csiss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8** outValue, u16* outValueLen)
{
    (void)opcode;
    (void)outValue;
    (void)outValueLen;

    BLT_CSIS_LOG("connHandle is 0x%x, attribute handle is 0x%x", connHandle, attrHandle);
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    if(attrHandle == csiss->SIRKHandle)
        return blt_csiss_readSIRKCback(connHandle);
    return ATT_SUCCESS;
}

static int blt_csiss_memberLockWrite(u16 connHandle, u8 setMemberLock)
{
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    if(setMemberLock == BLC_CSIS_LOCKED)
    {//lock request
        /*
         * If a client requests to write Locked to the Set Member Lock characteristic (i.e., the client requests the
         * lock), and the value of the Set Member Lock characteristic is equal to Unlocked, then the server shall
         * write the requested value, shall reply with the Write Response (i.e., the server grants the lock to the
         * client), and shall set a timer to TCSIS(lock_timeout)
         */
        if(CSISS_GET_LOCK == BLC_CSIS_UNLOCKED)
        {
            blt_csiss_startLockTimer(connHandle);
            return ATT_SUCCESS;
        }

        if(connHandle != csiss->memberLockedConnHandle)
            return BLT_CSIS_ERROR_CODE_LOCK_DENIED;

        return BLT_CSIS_ERROR_CODE_LOCK_ALREADY_GRANTED;

    }
    else if(setMemberLock == BLC_CSIS_UNLOCKED)
    {//lock release
        /*
         * If a client requests to write Unlocked to the Set Member Lock characteristic, and the value of the Set
         * Member Lock characteristic is already set to Unlocked, then the server shall reply with the Write Response
         */
        if(CSISS_GET_LOCK == BLC_CSIS_UNLOCKED)
        {
            return ATT_SUCCESS;
        }
        /*
         * If a client requests to write Unlocked to the Set Member Lock characteristic, the value of the Set Member
         * Lock characteristic is equal to Locked, and the Set Member Lock is not granted to the client, then the
         * server shall not write the value, and the server shall reply with an Error Response with the error code
         * Lock Release Not Allowed value
         */
        if(connHandle != csiss->memberLockedConnHandle)
            return BLT_CSIS_ERROR_CODE_LOCK_RELEASE_NOT_ALLOWED;

        /*
         * If a client requests to write Unlocked to the Set Member Lock characteristic (i.e., the client requests to
         * release the lock), the value of the Set Member Lock characteristic is equal to Locked, and the lock is
         * granted to the client, then the server shall write the requested value and shall reply with the Write
         * Response. The server shall also stop the timer to T_CSIS(lock_timeout)
         */
        blt_csiss_stopLockTimer();
        return ATT_SUCCESS;
    }
    else
    {
        return BLT_CSIS_ERROR_CODE_INVALID_LOCK_VALUE;
    }
}

static int blt_csiss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;

    if(valueLen != 1)
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    if(csiss->memberLockHandle == attrHandle)
        return blt_csiss_memberLockWrite(connHandle, writeValue[0]);
    return ATT_ERR_INVALID_HANDLE;
}

int blc_csiss_initSIRK(u8 type, const u8 SIRK[16])
{
    if(SIRK == NULL)
        return AUDIO_ERR_NULL_POINTER;
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    csiss->type = type;
    memcpy(csiss->plainSIRK, SIRK, 16);
    return AUDIO_ESUCC;
}

int blc_csiss_initSizeRank(u8 coordinatedSetSize, u8 memberRank)
{
    if(coordinatedSetSize == 0 || memberRank == 0 || memberRank>coordinatedSetSize)
        return AUDIO_ERR_PARAM_INVALID;
    CSISS_SET_SIZE(coordinatedSetSize);
    CSISS_SET_RANK(memberRank);
    return AUDIO_ESUCC;
}

int blc_csiss_initLockTimer(u8 timer)
{
    if(timer == 0)
        return AUDIO_ERR_PARAM_INVALID;
    blc_csis_server_t *csiss = blt_csiss_getCtrl();
    csiss->memberLockedTimeout = timer;
    return AUDIO_ESUCC;
}

/******************csis server init all characteristic handle*************************/

static void blt_csiss_initSIRK(atts_foundCharParam_t* p, void *input)
{
    blc_csis_server_t *csiss = (blc_csis_server_t*)input;
    if(p->num)
    {
        BLT_CSIS_LOG("ERR: SIRK char too many, max num is %d", p->num);
    }
    else
    {
        csiss->SIRKHandle = p->charHandle;
    }
}

static void blt_csiss_initCSSize(atts_foundCharParam_t* p, void *input)
{
    blc_csis_server_t *csiss = (blc_csis_server_t*)input;
    if(p->num)
    {
        BLT_CSIS_LOG("ERR: CS Size char too many, max num is %d", p->num);
    }
    else
    {
        csiss->CSSizeHandle = p->charHandle;
    }
}

static void blt_csiss_initMemberLock(atts_foundCharParam_t* p, void *input)
{
    blc_csis_server_t *csiss = (blc_csis_server_t*)input;
    if(p->num)
    {
        BLT_CSIS_LOG("ERR: Member Lock char too many, max num is %d", p->num);
    }
    else
    {
        csiss->memberLockHandle = p->charHandle;
    }
}

static void blt_csiss_initMemberRank(atts_foundCharParam_t* p, void *input)
{
    blc_csis_server_t *csiss = (blc_csis_server_t*)input;
    if(p->num)
    {
        BLT_CSIS_LOG("ERR: Member Rank char too many, max num is %d", p->num);
    }
    else
    {
        csiss->memberRankHandle = p->charHandle;
    }
}

static const atts_findCharList_t csissChar[] = {
    {
        .charUuid = characteristicSetIdentityResolvingKeyUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_csiss_initSIRK,
    },
    {
        .charUuid = characteristicCoordinatedSetSizeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_csiss_initCSSize,
    },
    {
        .charUuid = characteristicSetMemberLockUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_csiss_initMemberLock,
    },
    {
        .charUuid = characteristicSetMemberRankUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_csiss_initMemberRank,
    },
};

static void blt_csiss_serviceInit(const blc_csiss_regParam_t *param)
{
    blc_csis_server_t* server = blt_csiss_getCtrl();
    memset((u8*)server, 0, sizeof(blc_csis_server_t));

    blc_atts_findCharacteristicByServiceUuid(serviceCoordinatedSetIdentificationUuid, ATT_16_UUID_LEN, csissChar, ARRAY_SIZE(csissChar), server);
    BLT_CSIS_LOG("Handle information, SIRK:0x%x, Size:0x%x, Lock:0x%x, Rank:0x%x",
            server->SIRKHandle, server->CSSizeHandle, server->memberLockHandle, server->memberRankHandle);

    const blc_csiss_regParam_t *csisParam = param;

    if(csisParam == NULL) { //use default parameters
        csisParam = &defaultCsipSetMemberParam;
    }

    blc_csiss_initSizeRank(csisParam->setSize, csisParam->setRank);
    blc_csiss_initLockTimer(csisParam->lockedTimeout);
    blc_csiss_initSIRK(csisParam->SIRK_type, csisParam->SIRK);
    blt_csiss_stopLockTimer();

    blc_csis_cryptoGenerateRSI(csisParam->SIRK, server->rsi);
    BLT_CSIS_LOG("RSI: %X:%X:%X:%X:%X:%X", server->rsi[5],server->rsi[4],server->rsi[3],server->rsi[2],server->rsi[1],server->rsi[0]);
}

void blc_csiss_getResolvableSetIdentifier(u8 outRSI[6])
{
    blc_csis_server_t* server = blt_csiss_getCtrl();
    memcpy(outRSI, server->rsi, 6);
}






