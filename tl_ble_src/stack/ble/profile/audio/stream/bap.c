/********************************************************************************************************
 * @file    bap.c
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

#include "bap.h"
#include "bap_internal.h"


u8* blt_bap_getCodecCfgTarget(u8 *pSpecCfg, u8 tgtType)
{
    if(pSpecCfg == NULL)
    {
        return NULL;
    }

    if(pSpecCfg[0] < 3)
    {
        return NULL;
    }

    u8 tempCfgLen = pSpecCfg[0];
    u8 length, type;
    pSpecCfg++;

    while(tempCfgLen > 0)
    {
        STREAM_TO_U8(length, pSpecCfg);
        STREAM_TO_U8(type, pSpecCfg);
        if(tempCfgLen < (1+length))
        {
            return NULL;
        }
        if(tgtType == type)
        {
            return pSpecCfg;
        }

        pSpecCfg += length-1;
        tempCfgLen -= length+1;

    }

    return NULL;
}

void blt_audio_sendCisConnEvt(hci_le_cisEstablishedEvt_t *pData)
{
    int aclHandle = blt_audio_getAclHdlByCisHdl(pData->cisHandle);
    if(aclHandle < 0) return;

    blc_audio_cisConnEvt_t pEvt;
    memcpy((u8*)&pEvt,(u8*)&pData->cisHandle, sizeof(hci_le_cisEstablishedEvt_t)-2);
    blt_prf_sendEvent(aclHandle, AUDIO_EVT_CIS_CONNECT, (u8*)&pEvt, sizeof(blc_audio_cisConnEvt_t));
}

void blt_audio_sendCisDisconnEvt(hci_disconnectionCompleteEvt_t *pData)
{
    int aclHandle = blt_audio_getAclHdlByCisHdl(pData->connHandle);
    if(aclHandle < 0) return;

    blc_audio_cisDisconnEvt_t pEvt;
    pEvt.cisHandle = pData->connHandle;
    pEvt.reason = pData->reason;
    blt_prf_sendEvent(aclHandle, AUDIO_EVT_CIS_DISCONNECT, (u8*)&pEvt, sizeof(blc_audio_cisDisconnEvt_t));
}

void blt_audio_sendCisReqEvt(hci_le_cisReqEvt_t *pData)
{
    blc_audio_cisReqEvt_t pEvt;
    pEvt.aclHandle = pData->aclHandle;
    pEvt.cisHandle = pData->cisHandle;
    pEvt.cigId     = pData->cigId;
    pEvt.cisId     = pData->cisId;
    blt_prf_sendEvent(pData->aclHandle, AUDIO_EVT_CIS_REQUEST, (u8*)&pEvt, sizeof(blc_audio_cisReqEvt_t));
}

int blt_audio_unicastDataPathSetup(blt_ascss_ase_state_t *pAse, u8 type, u8 role)
{
    /* type 0: Codec inside controller, others: Codec inside host */

    if(pAse == NULL || !pAse->cisEstablish)
    {
        return AUDIO_EHANDLE;
    }

    if(!pAse->dataPathSetup)
    { /* Audio data path setup */
        hci_le_setupIsoDataPath_cmdParam_t isoDataPath;
        hci_le_setupIsoDataPath_retParam_t isoDataPathRet;
        isoDataPath.conn_handle = pAse->cisHandle;
        if(role == 0)//role 0:server
        {
            isoDataPath.data_path_dir = pAse->dir == AUDIO_DIR_SINK ? 0x01 : 0x00;
        }
        else if(role == 1)//role 1:client
        {
            isoDataPath.data_path_dir = pAse->dir == AUDIO_DIR_SINK ? 0x00 : 0x01;
        }
        isoDataPath.data_path_id = 0x00; /* When set to 0x00, the data path shall be over the HCI transport. When set to 0xFF the path shall be disabled. */
        memcpy(&isoDataPath.codec_id_assignNum, (u8*)&pAse->codecState.codecId, 5);

/*      When Data_Path_Direction is set to 0x00 (input), the Controller_Delay parameter
        specifies the delay at the data source from the reference time of an SDU to
        the CIG reference point (see [Vol 6] Part B, Section 4.5.14.1) or BIG anchor
        point (see [Vol 6] Part B, Section 4.4.6.4). When Data_Path_Direction is set to
        0x01 (output), Controller_Delay specifies the delay from the
        SDU_Synchronization_Reference to the point in time at which the Controller
        begins to transfer the corresponding data to the data path interface. The Host
        should use the HCI_Read_Local_Supported_Controller_Delay command to
        obtain a suitable value for Controller_Delay.*/
        //TODO,how to specify the controller delay?  temporarily set to 1ms.
        isoDataPath.control_delay[0] = 0xe8;//
        isoDataPath.control_delay[1] = 0x03;
        isoDataPath.control_delay[2] = 0x00;

        if(type)
        { /* if the codec in use resides in the Bluetooth Controller  */
            isoDataPath.codec_config_len = pAse->codecState.codecSpecCfgLen;
            memcpy(isoDataPath.codec_config,pAse->codecState.codecSpecCfg,pAse->codecState.codecSpecCfgLen);
        }
        else
        { /* if the codec in use resides in the Bluetooth Host  */
            isoDataPath.codec_config_len = 0;
            isoDataPath.codec_id_assignNum = BLC_AUDIO_CODING_FORMAT_TRANSPARENT;
        }

        if(blc_hci_le_setupIsoDataPath(&isoDataPath,&isoDataPathRet) == BLE_SUCCESS)
        {
            pAse->dataPathSetup = 1;
            BLT_BAP_LOG("Audio data path setup-dir: %d", pAse->dir);
            return AUDIO_ESUCC;
        }
        else
        {
            BLT_BAP_LOG("Audio data path setup failed: %d", isoDataPathRet.status);
        }
    }
    else
    {
        BLT_BAP_LOG("Audio data path already setup - dir: %d", pAse->dir);
        return AUDIO_ESUCC;
    }
    return -AUDIO_EHANDLE;
}









///////////////////Generic Audio /////////////////////////

u16 blc_bap_calculateMetadataLength(const blc_audio_metadataParam_t* metadata)
{
    u16 totalLen = 0;

    if(metadata->preferredContexts)     totalLen+=4;
    if(metadata->StreamingContexts)     totalLen+=4;
    if(metadata->programInfo)           totalLen+=2+strlen(metadata->programInfo);
    if(metadata->language)              totalLen+=5;
    if(metadata->ccidListLen && metadata->ccidList)         totalLen+=2+metadata->ccidListLen;
    if(metadata->parentalRating)        totalLen+=3;
    if(metadata->programInfoURI)        totalLen+=2+strlen(metadata->programInfoURI);
    if(metadata->audioState)            totalLen+=3;
    if(metadata->immediateRendering)    totalLen+=2;
    if(metadata->otherMetadataLen && metadata->otherMetadata)       totalLen+=2+metadata->otherMetadataLen;

    return totalLen>0xFF? 0x00: totalLen;
}

u8* blc_bap_setMetadataToAddress(const blc_audio_metadataParam_t* metadata, u8* dst)
{
    u8 len;
    if(metadata->preferredContexts){
        U8_TO_STREAM(dst, 0x03);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS);
        U16_TO_STREAM(dst, BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(metadata->preferredContexts));
    }

    if(metadata->StreamingContexts){
        U8_TO_STREAM(dst, 0x03);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_STREAMING_CONTEXTS);
        U16_TO_STREAM(dst, BLC_AUDIO_CONTEXT_TYPE_VALID_BITS(metadata->StreamingContexts));
    }

    if(metadata->programInfo){
        len = strlen(metadata->programInfo);
        U8_TO_STREAM(dst, len+1);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_PROGRAM_INFO);
        STR_TO_STREAM(dst, metadata->programInfo, len);
    }

    if(metadata->language){
        U8_TO_STREAM(dst, 0x04);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_LANGUAGE);
        STR_TO_STREAM(dst, metadata->language, 3);
    }

    if(metadata->ccidListLen && metadata->ccidList){
        U8_TO_STREAM(dst, metadata->ccidListLen + 1);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_CCID_LIST);
        STR_TO_STREAM(dst, metadata->ccidList, metadata->ccidListLen);
    }

    if(metadata->parentalRating){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_PARENTAL_RATING);
        U8_TO_STREAM(dst, metadata->parentalRating);
    }

    if(metadata->programInfoURI){
        len = strlen(metadata->programInfoURI);
        U8_TO_STREAM(dst, len+1);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_PROGRAM_INFO_URI);
        STR_TO_STREAM(dst, metadata->programInfoURI, len);
    }

    if(metadata->audioState){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_AUDIO_ACTIVE_STATE);
        U8_TO_STREAM(dst, metadata->audioState==0xFFFF? 0x00: metadata->audioState);
    }

    if(metadata->immediateRendering){
        U8_TO_STREAM(dst, 0x01);
        U8_TO_STREAM(dst, BLC_AUDIO_METATYPE_IMMEDIATE_RENDERING);
    }

    if(metadata->otherMetadataLen && metadata->otherMetadata){
        STR_TO_STREAM(dst, metadata->otherMetadata, metadata->otherMetadataLen);
    }

    return dst;
}

u8 blc_bap_calculateCodecSpecificCapabilitiesLength(const blc_audio_codecSpecCapParam_t* codecSpec)
{
    u8 totalLen = 0;

    if(codecSpec->samplingFreq)         totalLen+=4;
    if(codecSpec->frameDurations)       totalLen+=3;
    if(codecSpec->channelCounts)        totalLen+=3;
    if(codecSpec->minPerCodecFrame || codecSpec->maxPerCodecFrame)      totalLen+=6;
    if(codecSpec->maxPerSdu)            totalLen+=3;

    return totalLen;
}

u8* blc_bap_setCodecSpecificCapabilitiesToAddress(const blc_audio_codecSpecCapParam_t* codecSpec, u8* dst)
{
    if(codecSpec->samplingFreq){
        U8_TO_STREAM(dst, 0x03);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_SAMPLE_FREQUENCY);
        U16_TO_STREAM(dst, BLC_AUDIO_SUPP_FREQ_FLAG_VALID_BITS(codecSpec->samplingFreq));
    }

    if(codecSpec->frameDurations){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_FRAME_DURATION);
        U8_TO_STREAM(dst, BLC_AUDIO_SUPP_DURATION_FLAG_VALID_BITS(codecSpec->frameDurations));
    }

    if(codecSpec->channelCounts){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_AUDIO_CHN_COUNTS);
        U8_TO_STREAM(dst, codecSpec->channelCounts);
    }

    if(codecSpec->minPerCodecFrame || codecSpec->maxPerCodecFrame){
        U8_TO_STREAM(dst, 0x05);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_OCTETS_PER_CODEC_FRAME);
        U16_TO_STREAM(dst, codecSpec->minPerCodecFrame);
        U16_TO_STREAM(dst, codecSpec->maxPerCodecFrame);
    }

    if(codecSpec->maxPerSdu){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_MAX_CODEC_FRAMES_PER_SDU);
        U8_TO_STREAM(dst, codecSpec->maxPerSdu);
    }

    return dst;
}

u8 blc_bap_calculateCodecSpecificConfigurationLength(blc_audio_codecSpecCfgParam_t* codecCfg)
{
    u8 totalLen = 0;

    if(codecCfg->samplingFreq && codecCfg->perCodecFrame && (!BLC_AUDIO_DURATION_RFU(codecCfg->frameDuration))) totalLen+= (3+3+4);
    if(codecCfg->channelAllocation)     totalLen+=6;
    if(codecCfg->perSduFrameBlocks)     totalLen+=3;

    return totalLen;
}

u8* blc_bap_setCodecSpecificConfigurationToAddress(blc_audio_codecSpecCfgParam_t* codecCfg, u8* dst)
{
    if(codecCfg->samplingFreq && codecCfg->perCodecFrame && (!BLC_AUDIO_DURATION_RFU(codecCfg->frameDuration))){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY);
        U8_TO_STREAM(dst, codecCfg->samplingFreq);
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION);
        U8_TO_STREAM(dst, codecCfg->frameDuration);
        U8_TO_STREAM(dst, 0x03);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME);
        U16_TO_STREAM(dst, codecCfg->perCodecFrame);
    }
    if(codecCfg->channelAllocation){
        U8_TO_STREAM(dst, 0x05);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_AUDIO_CHN_COUNTS);
        U32_TO_STREAM(dst, codecCfg->channelAllocation);
    }
    if(codecCfg->perSduFrameBlocks){
        U8_TO_STREAM(dst, 0x02);
        U8_TO_STREAM(dst, BLC_AUDIO_CAPTYPE_SUP_MAX_CODEC_FRAMES_PER_SDU);
        U8_TO_STREAM(dst, codecCfg->perSduFrameBlocks);
    }

    return dst;
}















