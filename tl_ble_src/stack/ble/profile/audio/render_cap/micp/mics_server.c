/********************************************************************************************************
 * @file    mics_server.c
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

static blc_mics_server_t* blt_micss_getServerInst(u16 connHandle);
static int blt_micss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);
static void blt_micss_serviceInit(const blc_micss_regParam_t *param);

#define MICS_MUTE_HANDLE(connHandle)                (blt_micss_getServerInst(connHandle)->muteHdl)

/* There shall be no more than one instance of the microphone control service(MICS) on a device. */
_attribute_ble_data_retention_
blc_mics_server_ctrl_t mics_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_MICS_SERVER,
        .usedAclRole = 0,
        .init = blt_micss_init,
        .connect = blt_micss_connect,
        .discov = NULL,
        .loop = NULL,
    },
};

void blc_audio_registerMICSControlServer(const blc_micss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t*)&mics_server_ctrl, param);
}

int blt_micss_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_mics_server_t)), blc_mics_server_t);
#endif

    if(initType == PRF_PROC_INIT) {
        blc_svc_addMicpGroup();
        blt_micss_serviceInit(param);
        blc_svc_micpCbackRegister(NULL, blt_micss_writeCback);
    }
//  else if (initType == PRF_PROC_DEINIT) {
//  }
    return 0;
}

int blt_micss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_MICS_LOG("blt_mics_server_disconnect Handle:0x%x", connHandle);
    } else {
        BLT_MICS_LOG("blt_mics_server_connect Handle:0x%x", connHandle);
    }
    return 0;
}

blc_mics_server_t* blt_micss_getServerInst(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_CENTRAL)
    {
        BLT_MICS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* MICP Microphone Device GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_MICS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &mics_server_ctrl.micsServer;
}


static void blt_micss_initMuteChar(atts_foundCharParam_t * p, void *input)
{
    blc_mics_server_t* server = (blc_mics_server_t*)input;
    if(p->num > 0)
    {
        BLT_MICS_LOG("ERR: Mute char too many");
        return ;
    }
    server->muteHdl = p->charHandle;
}

static const atts_findCharList_t micsChar[] = {
    {
        .charUuid = characteristicMuteUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_micss_initMuteChar,
    },
};

static const atts_findServiceList_t micsService = {
    .serviceUuidLen = ATT_16_UUID_LEN,
    .serviceUuid = serviceMicrophoneControlUuid,
    .charSize = ARRAY_SIZE(micsChar),
    .charList = micsChar,
    .inclSize = 0,
};

static void blt_micss_serviceInit(const blc_micss_regParam_t *param)
{
    blc_mics_server_t* micss = blt_micss_getServerInst(0xFFFF);

    blc_atts_findCharacteristic(&micsService, micss);

    const blc_micss_regParam_t  *micpParam = param;

    if(micpParam == NULL)
    {
        micpParam = &defaultMicpParam;
    }

    blc_micss_initMute(micpParam->mute);

    BLT_MICS_LOG("Handle information, mute:0x%x, muteState:0x%x", micss->muteHdl, micpParam->mute);
}

u8* blc_micss_getMute(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, MICS_MUTE_HANDLE(connHandle));
}

static int blt_micss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;
    BLT_MICS_LOG("write handle 0x%x", attrHandle);
    blc_mics_server_t* mics = blt_micss_getServerInst(connHandle);
    int err = ATT_SUCCESS;
    if(attrHandle == mics->muteHdl){
        if(valueLen != 1) {
            return ATT_ERR_INVALID_PDU;
        }

        if(*writeValue >= MICS_MUTE_VALUE_DISABLED) {
            return ATT_ERR_VALUE_NOT_ALLOWED;
        }

        u8* mute = blc_micss_getMute(connHandle);

        if(*mute == MICS_MUTE_VALUE_DISABLED) {
            return MICS_ERRCODE_MUTE_DISABLED;
        }
        *mute = *writeValue;
        blc_gatts_notifyAttr(connHandle, MICS_MUTE_HANDLE(connHandle));

        blc_micss_muteChangeEvt_t evt = {
            .mute = *writeValue
        };

        blt_prf_sendEvent(connHandle, AUDIO_EVT_MICSS_CHANGE_MUTE, (u8*)&evt, sizeof(blc_micss_muteChangeEvt_t));
    }
    else
    {
        err = ATT_ERR_INVALID_HANDLE;
    }

    return err;
}

int blc_micss_initMute(blc_mics_mute_value_enum mute)
{
    if(mute >= MICS_MUTE_VALUE_RFU) {
        return -1;
    }

    u8* pMute = blc_micss_getMute(0xFFFF);

    *pMute = mute;

    return 0;
}


int blc_micss_updateMute(u16 connHandle, blc_mics_mute_value_enum mute)
{
    if(blc_micss_initMute(mute))
        return AUDIO_ERR_INVALID_PARAMETER;
    return blc_gatts_notifyAttr(connHandle, MICS_MUTE_HANDLE(connHandle));
}




