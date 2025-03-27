/********************************************************************************************************
 * @file    pacs_server.c
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
#include <stdarg.h>


#include "../bap_internal.h"

static int blt_pacss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);
static void blt_pacss_serviceInit(const blc_pacss_regParam_t *param);


_attribute_ble_data_retention_
blc_pacs_server_ctrl_t pacs_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_PACS_SERVER,
        .usedAclRole = 0,
        .init = blt_pacss_init,
        .connect = blt_pacss_connect,
        .discov = NULL,
        .loop = NULL,
    },
};

void blc_audio_registerPACSControlServer(const blc_pacss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_PERIPHERAL, (blc_prf_proc_t*)&pacs_server_ctrl, param);
}

static blc_pacs_server_t* blt_pacss_getCtrl(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_CENTRAL)
    {
        BLT_PACS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* BAP Unicast Server GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_PACS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &pacs_server_ctrl.pacsServer;
}

#define PACSS_SINK_PAC_HANDLE(connHandle)               (blt_pacss_getCtrl(connHandle)->sinkPacHandle)
#define PACSS_SINK_AUDIO_LOCA_HANDLE(connHandle)        (blt_pacss_getCtrl(connHandle)->sinkAudioLocationsHandle)
#define PACSS_SRC_PAC_HANDLE(connHandle)                (blt_pacss_getCtrl(connHandle)->sourcePacHandle)
#define PACSS_SRC_AUDIO_LOCA_HANDLE(connHandle)         (blt_pacss_getCtrl(connHandle)->SourceAudioLocationsHandle)
#define PACSS_AVA_AUDIO_CONTEXT_HANDLE(connHandle)      (blt_pacss_getCtrl(connHandle)->availableAudioContextsHandle)
#define PACSS_SUPP_AUDIO_CONTEXT_HANDLE(connHandle)     (blt_pacss_getCtrl(connHandle)->suppAudioContextsHandle)


int blt_pacss_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_pacs_server_t)), blc_pacs_server_t);
#endif

    if(initType == PRF_PROC_INIT) {
        BLT_PACS_LOG("server init");
        blc_svc_addPacsGroup();
        blc_svc_pacsCbackRegister(NULL, blt_pacss_writeCback);
        blt_pacss_serviceInit(param);
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      blc_svc_removePacsGroup();
//      BLT_PACS_LOG("server deinit");
//  }
    return 0;
}

int blt_pacss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_PACS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLT_PACS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

static u8* blt_pacss_getSinkPAC(u16 connHandle)
{
    return (u8*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_SINK_PAC_HANDLE(connHandle));
}

static u32* blt_pacss_getSinkAudioLocations(u16 connHandle)
{
    return (u32*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_SINK_AUDIO_LOCA_HANDLE(connHandle));
}

static u8* blt_pacss_getSrcPAC(u16 connHandle)
{
    return (u8*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_SRC_PAC_HANDLE(connHandle));
}

static u32* blt_pacss_getSourceAudioLocations(u16 connHandle)
{
    return (u32*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_SRC_AUDIO_LOCA_HANDLE(connHandle));
}

static u16* blt_pacss_getAvaAudioContext(u16 connHandle)
{
    return (u16*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_AVA_AUDIO_CONTEXT_HANDLE(connHandle));
}

static u16* blt_pacss_getSuppAudioContext(u16 connHandle)
{
    return (u16*)blc_gatts_getAttributeValueByHandle(connHandle, PACSS_SUPP_AUDIO_CONTEXT_HANDLE(connHandle));
}

static u16 blt_pacss_setPacParam(u8 inPacNum, const blc_audio_pacParam_t* inPac, u8* dstPac)
{
    extern const u16 gPacMaxSize;
    if(gPacMaxSize <= 8)    return 0;

    U8_TO_STREAM(dstPac, inPacNum);
    u16 pacLen = gPacMaxSize - 1;
    for(int i=0; i<inPacNum; i++)
    {
        if(pacLen <= 5)     return 0;

        STR_TO_STREAM(dstPac, &inPac[i].codecId, sizeof(blc_audio_codec_id_t));
        pacLen -= sizeof(blc_audio_codec_id_t);
        u8 codecSpecLen = blc_bap_calculateCodecSpecificCapabilitiesLength(&inPac[i].codecSpec);
        if(pacLen <= codecSpecLen+1)    return 0;
        U8_TO_STREAM(dstPac, codecSpecLen);
        dstPac = blc_bap_setCodecSpecificCapabilitiesToAddress(&inPac[i].codecSpec, dstPac);
        pacLen -= 1+codecSpecLen;

        u16 metadataLen = blc_bap_calculateMetadataLength(&inPac[i].metadata);
        if(pacLen <= metadataLen+1) return 0;

        U8_TO_STREAM(dstPac, metadataLen);
        pacLen -= 1+metadataLen;
        if(!metadataLen)    continue;
        dstPac = blc_bap_setMetadataToAddress(&inPac[i].metadata, dstPac);

    }

    return gPacMaxSize - pacLen;
}

void blt_pacss_setSinkPac(u8 pacNum, const blc_audio_pacParam_t* pac)
{
    u8* pSinkPac = NULL;
    u16* pSinkPacLen = NULL;
    blc_gatts_getAttributeInformationByHandle(0xFFFF, PACSS_SINK_PAC_HANDLE(0xFFFF), &pSinkPac, &pSinkPacLen);

    if(!pSinkPac)   return ;
    *pSinkPacLen = blt_pacss_setPacParam(pacNum, pac, pSinkPac);
}

void blt_pacss_setSourcePac(u8 pacNum, const blc_audio_pacParam_t* pac)
{
    u8* pSourcePac = NULL;
    u16* pSourcePacLen = NULL;
    blc_gatts_getAttributeInformationByHandle(0xFFFF, PACSS_SRC_PAC_HANDLE(0xFFFF), &pSourcePac, &pSourcePacLen);

    if(!pSourcePac) return ;
    *pSourcePacLen = blt_pacss_setPacParam(pacNum, pac, pSourcePac);
}


void blt_pacss_setSinkAudioLocations(u32 locations)
{
    u32 *pLocations = blt_pacss_getSinkAudioLocations(0xFFFF);
    if(!pLocations)     return ;

    *pLocations = locations & (~BLC_AUDIO_LOCATION_FLAG_RFU);
}

void blt_pacss_setSourceAudioLocations(u32 locations)
{
    u32 *pLocations = blt_pacss_getSourceAudioLocations(0xFFFF);
    if(!pLocations)     return ;

    *pLocations = locations & (~BLC_AUDIO_LOCATION_FLAG_RFU);
}

void blt_pacss_setAvaAudioContext(u16 sinkContexts, u16 sourceContexts)
{
    u16* avaAudioContext = blt_pacss_getAvaAudioContext(0xFFFF);
    if(!avaAudioContext)    return ;

    *avaAudioContext = BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(sinkContexts);
    *(avaAudioContext+1) = BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(sourceContexts);
}

void blt_pacss_setSuppAudioContext(u16 sinkContexts, u16 sourceContexts)
{
    u16* suppAudioContext = blt_pacss_getSuppAudioContext(0xFFFF);
    if(!suppAudioContext)   return ;

    *suppAudioContext = BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(sinkContexts);
    *(suppAudioContext+1) = BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(sourceContexts);
}

static int blt_pacss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;
    BLT_PACS_LOG("Write[0x%x] handle is 0x%x value is %s", connHandle, attrHandle, hex_to_str(writeValue, valueLen));
    blc_pacs_server_t * server = blt_pacss_getCtrl(connHandle);
    u32 audioLocation = 0;
    STREAM_TO_U32(audioLocation, writeValue);
    /* is not 4 octets in length, or if the parameter value written includes any RFU bits set
     * to a value of 0b1, the server shall respond with an ATT Error Response and shall set
     * the Error Code parameter to Write Request Rejected as defined in BCSS v9. */
    if(valueLen != 4 || (audioLocation & BLC_AUDIO_LOCATION_FLAG_RFU) || audioLocation == 0)
    {
        return ATT_ERR_WRITE_REQUEST_REJECT;
    }
    if(attrHandle == server->sinkAudioLocationsHandle)
    {
        /* Update value */
        blc_pacss_updateSinkAudioLocations(connHandle, audioLocation);
        return ATT_SUCCESS;     //write data and send notify
    }
    else if(attrHandle == server->SourceAudioLocationsHandle)
    {
        /* Update value */
        blc_pacss_updateSourceAudioLocations(connHandle, audioLocation);
        return ATT_SUCCESS;     //write data and send notify
    }
    return ATT_ERR_INVALID_HANDLE;
}


/******************pacs server init all characteristic handle*************************/
static void blt_pacss_initSinkPacChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: sink PAC char too many");
        return ;
    }
    pacss->sinkPacHandle = p->charHandle;
}

static void blt_pacss_initSinkAudioLocationsChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: sink audio locations char too many");
        return ;
    }
    pacss->sinkAudioLocationsHandle = p->charHandle;
}

static void blt_pacss_initSourcePacChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: source PAC char too many");
        return ;
    }
    pacss->sourcePacHandle = p->charHandle;
}

static void blt_pacss_initSourceAudioLocationsChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: source audio locations char too many");
        return ;
    }
    pacss->SourceAudioLocationsHandle = p->charHandle;
}

static void blt_pacss_initAvailableAudioContextsChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: available audio contexts char too many");
        return ;
    }
    pacss->availableAudioContextsHandle = p->charHandle;
}

static void blt_pacss_initSuppAudioContextsChar(atts_foundCharParam_t * p, void *input)
{
    blc_pacs_server_t *pacss = (blc_pacs_server_t*)input;
    if(p->num > 0)
    {
        BLT_PACS_LOG("ERR: supported audio contexts char too many");
        return ;
    }
    pacss->suppAudioContextsHandle = p->charHandle;
}

static const atts_findCharList_t pacssChar[] = {
    {
        .charUuid = characteristicSinkPacUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initSinkPacChar,
    },
    {
        .charUuid = characteristicSinkAudioLocationsUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initSinkAudioLocationsChar,
    },
    {
        .charUuid = characteristicSourcePacUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initSourcePacChar,
    },
    {
        .charUuid = characteristicSourceAudioLocationsUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initSourceAudioLocationsChar,
    },
    {
        .charUuid = characteristicAvailableAudioContextsUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initAvailableAudioContextsChar,
    },
    {
        .charUuid = characteristicSupportedAudioContextsUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_pacss_initSuppAudioContextsChar,
    },
};

static void blt_pacss_serviceInit(const blc_pacss_regParam_t *param)
{
    blc_pacs_server_t *server = blt_pacss_getCtrl(0xFFFF);
    blc_atts_findCharacteristicByServiceUuid(servicePublishedAudioCapabilitiesUuid, ATT_16_UUID_LEN, pacssChar, ARRAY_SIZE(pacssChar), server);
    BLT_PACS_LOG("Handle information, sink[PAC:0x%x, Locations:0x%x], source[PAC:0x%x, Location:0x%x], AvailableContexts:0x%x, SupportedContexts:0x%x",
            server->sinkPacHandle, server->sinkAudioLocationsHandle, server->sourcePacHandle, server->SourceAudioLocationsHandle
            ,server->availableAudioContextsHandle, server->suppAudioContextsHandle);
    const blc_pacss_regParam_t* pacsParam = param;

    if(pacsParam == NULL) { //use default parameters
        pacsParam = &defaultPacsParam;
    }

    blt_pacss_setSinkPac(pacsParam->sinkPacNum, pacsParam->sinkPac);
    blt_pacss_setSourcePac(pacsParam->sourcePacNum, pacsParam->sourcePac);
    blt_pacss_setSinkAudioLocations(pacsParam->sinkAudioLocations);
    blt_pacss_setSourceAudioLocations(pacsParam->sourceAudioLocations);
    blt_pacss_setAvaAudioContext(pacsParam->availableSinkContexts, pacsParam->availableSourceContexts);
    blt_pacss_setSuppAudioContext(pacsParam->supportedSinkContexts, pacsParam->supportedSourceContexts);
}
/****************pacs server init all characteristic handle end***********************/

int blc_pacss_updateSinkAudioLocations(u16 connHandle, u32 locations)
{
    blt_pacss_setSinkAudioLocations(locations);
    return blc_gatts_notifyAttr(connHandle, PACSS_SINK_AUDIO_LOCA_HANDLE(connHandle));
}

int blc_pacss_updateSourceAudioLocations(u16 connHandle, u32 locations)
{
    blt_pacss_setSourceAudioLocations(locations);
    return blc_gatts_notifyAttr(connHandle, PACSS_SRC_AUDIO_LOCA_HANDLE(connHandle));
}


static u8* blt_pacs_getPacRecord(u16 connHandle, u8 type, u8 *pCodecId)
{
    u8 *pTemp = type==BLT_ASE_DIRECTION_SINK? blt_pacss_getSinkPAC(connHandle):blt_pacss_getSrcPAC(connHandle);

    if(pTemp == NULL || pTemp[0] == 0)
    {
        return NULL;
    }

    u8 count = pTemp[0];
    pTemp += 1;
    for(u8 i=0; i<count; i++)
    {
        if(memcmp(pTemp, pCodecId, 5) == 0)
        {
            return pTemp;
        }
        pTemp += 5+2+pTemp[5]+pTemp[6+pTemp[5]];
    }
    return NULL;
}


u8 blt_pacss_getRecordParam(u16 connHandle, u8 type, u8 *pCodecId, blt_audio_pacParam_t *pParam)
{
    u8 *pRac;
    u8 ret = AUDIO_ESUCC;

    if(pCodecId == NULL || pParam == NULL)
    {
        return AUDIO_ERR_NULL_POINTER;
    }
    pRac = blt_pacs_getPacRecord(connHandle, type, pCodecId);

    if((ret = blt_audio_getCodecSpecCapParam(pRac+5, &pParam->codecSpecCapParam)) != AUDIO_ESUCC)
    {
        return ret;
    }


//  length = pRac[6+pRac[5]]; /* Metadata_Length[i]: 1 */
//  pTemp = pRac+7+pRac[5]; /* Metadata[i] Varies */
//
//  ret = blt_audio_getMetadataParam(length, pTemp, &pParam->metadataParam);
//  if(ret != BLC_AUDIO_SUCCESS)
//  {
//      return ret;
//  }
    return ret;
}

u16 blt_pacss_getAvailableContext(u16 connHandle, u8 type)
{
    u16* context = blt_pacss_getAvaAudioContext(connHandle);

    return type == BLT_ASE_DIRECTION_SINK ? *context:*(context+1);
}


int blt_pacss_checkCodecCfgParam(u16 connHandle,blt_pac_e type,blc_audio_codecSpecCfgParsed_t *codecCfg)
{
    if(codecCfg == NULL)
    {
        return AUDIO_EPARAM;
    }
    u8* localPac = type==BLT_PAC_SINK?blt_pacss_getSinkPAC(connHandle):blt_pacss_getSrcPAC(connHandle);
    u8 pacNum = localPac[0];
    u8 localOffset = 1;
    for(u8 i=0;i<pacNum;i++)
    {
        blt_audio_codecSpecCapParam_t localCap;
        int ret = blt_audio_getCodecSpecCapParam(localPac+localOffset+5, &localCap);
        if(ret != AUDIO_ESUCC)
        {
            return AUDIO_ELENGTH;
        }
        if(blt_audio_checkCodecParamValid(&localCap,codecCfg) == AUDIO_ESUCC)
        {
            return AUDIO_ESUCC;
        }
        u8 codecSpecLen = localPac[localOffset+5];
        u8 metadataLen  = localPac[localOffset + 6 + codecSpecLen];
        localOffset = localOffset + 5 + 1 + codecSpecLen + 1 + metadataLen;
    }
    return AUDIO_EFAIL;
}




