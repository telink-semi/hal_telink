/********************************************************************************************************
 * @file    audio.c
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
#include "stack/ble/host/gatt/tlk_list_stack.h"
#include "stack/ble/profile/audio/stream/bap_internal.h"


_attribute_ble_data_retention_ blt_audio_cap_ctrl_t blt_audio_cap_ctrl;

_attribute_ble_data_retention_ audio_le_evt_func_t bap_unicast_clt_cb = NULL;

_attribute_ble_data_retention_ audio_le_evt_func_t bap_unicast_svr_cb = NULL;

_attribute_ble_data_retention_ audio_le_evt_func_t bap_bcst_sink_cb = NULL;

_attribute_ble_data_retention_ audio_le_evt_func_t bap_bcst_assistant_cb = NULL;


static int  blt_audio_leEvtWrapHandler(u32 h, u8 *p, int n);

_attribute_ble_data_retention_
static struct gap_hciEventNode leAudioHciEventCallBack = {
    .cb = blt_audio_leEvtWrapHandler,
};

//TODO: if any MACRO need to cheek
//BLT_STRUCT_4B_ALIGN_CHECK_EN use 0

void blc_audio_initialModule(prf_evt_cb_t evtCb)
{
    /* Special protection code for use */
    if(pm_check_info){
        blc_prf_initialModule(evtCb);
        blt_gap_regHciEventCb(&leAudioHciEventCallBack);
    }
}

#if (0)
/* 0: cis central role; 1: cis peripheral role; others: err */
int blc_audio_getCisRole(u16 cisHandle)
{
    if (!blmsParam.cis_en) {
        return -AUDIO_ENOSUPP;
    }

    ll_cis_conn_t * pCisConn = blt_isCisEstablished_by_handle(cisHandle);

    if (!pCisConn) {
        return -AUDIO_EPARAM;
    }

    /* 1: CIS_ROLE_MASTER; 0:CIS_ROLE_SLAVE */
    return pCisConn->cisRole;
}
#endif

int blt_audio_getAclHdlByCisHdl(u16 cisHandle)
{
    if (!blmsParam.cis_en) {
        return -AUDIO_ENOSUPP;
    }

    ll_cis_conn_t * pCisConn = blt_isCisEstablished_by_handle(cisHandle);

    if (!pCisConn) {
        return -AUDIO_EPARAM;
    }

    return pCisConn->link_acl_handle;
}

#if (0)
/* 1: bis central (sync) role; 0: bis peripheral (bcst) role; others: err */
int blc_audio_getBisRole(u16 bisHandle)
{
    if (!blmsParam.bis_en) {
        return -AUDIO_ENOSUPP;
    }

    ll_bis_t *pBis = blt_ll_findBisByHandle(bisHandle);

    if (!pBis) {
        return -AUDIO_EPARAM;
    }

    /* 1: BIS_ROLE_SYNC; 0:BIS_ROLE_BCST */
    return pBis->bis_role;
}
#endif

u8 blt_audio_setMetadata(blc_audio_metadata_parsed_t *pParam, u8 *pMeta)
{
    u8* pMetaTemp = pMeta;
    if(pParam->fieldExistFlg & BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS_MASK)
    {
        U8_TO_STREAM(pMeta, 0x03);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS);
        U16_TO_STREAM(pMeta, pParam->prefCtx);
    }

    if(pParam->fieldExistFlg & BLC_AUDIO_METATYPE_STREAMING_CONTEXTS_MASK)
    {
        U8_TO_STREAM(pMeta, 0x03);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_STREAMING_CONTEXTS);
        U16_TO_STREAM(pMeta, pParam->streamingCtx);
    }

    if(pParam->fieldExistFlg & BLC_AUDIO_METATYPE_LANGUAGE_MASK)
    {
        U8_TO_STREAM(pMeta, 0x04);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_LANGUAGE);
        U24_TO_STREAM(pMeta, pParam->language);
    }

    if(pParam->fieldExistFlg & BLC_AUDIO_METATYPE_PARENTAL_RATING_MASK)
    {
        U8_TO_STREAM(pMeta, 0x02);
        U8_TO_STREAM(pMeta, BLC_AUDIO_METATYPE_PARENTAL_RATING);
        U8_TO_STREAM(pMeta, pParam->parentalRating);
    }

    return pMeta - pMetaTemp;
}

int blt_audio_getMetadataParams(u8 metaLen, u8 *pMeta, blc_audio_metadata_parsed_t *pParam)
{
    if(pParam == NULL || pMeta == NULL || (metaLen && metaLen < 3))
    {
        return AUDIO_EPARAM;
    }
    u8 length, type;
    pParam->fieldExistFlg = 0;
    pParam->metadataLen = 0;
    pParam->rspCode = 0;
    pParam->rsnMark = 0;

    while(metaLen > 0)
    {
        STREAM_TO_U8(length, pMeta);
        STREAM_TO_U8(type, pMeta);
        if(metaLen < 1+length)
        {
            goto lengthErr;
        }
        metaLen -= length+1;
        if(type == BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS)
        { // Preferred_Audio_Contexts
            if(length != 3)
            {
                goto lengthErr;
            }
            STREAM_TO_U16(pParam->prefCtx, pMeta);
            if(BLC_AUDIO_CONTEXT_TYPE_CHECK_RFU(pParam->prefCtx))
            {
                pParam->rspCode = 0x0C;
                pParam->rsnMark = type;
                return AUDIO_EPARAM;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS_MASK;
            BLT_AUD_LOG("preferredContext: 0x%x", pParam->prefCtx);
        }
        else if(type == BLC_AUDIO_METATYPE_STREAMING_CONTEXTS)
        { // Streaming_Audio_Contexts
            if(length != 3)
            {
                goto lengthErr;
            }
            STREAM_TO_U16(pParam->streamingCtx, pMeta);
            if(BLC_AUDIO_CONTEXT_TYPE_CHECK_RFU(pParam->streamingCtx))
            {
                pParam->rspCode = 0x0C;
                pParam->rsnMark = type;
                return AUDIO_EPARAM;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_STREAMING_CONTEXTS_MASK;
            BLT_AUD_LOG("streamingContext: 0x%x", pParam->streamingCtx);
        }
        else if(type == BLC_AUDIO_METATYPE_PROGRAM_INFO)
        { //Program Info
            if(length < 2)
            {
                goto lengthErr;
            }
            pParam->programInfoLen = length-1;
            pParam->pProgramInfo = pMeta;
            pMeta += length-1;
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_PROGRAM_INFO_MASK;
            BLT_AUD_LOG("Profram Info is %s", hex_to_str(pParam->pProgramInfo, pParam->programInfoLen));
        }
        else if(type == BLC_AUDIO_METATYPE_LANGUAGE)
        { //Language
            if(length != 4)
            {
                goto lengthErr;
            }
            STREAM_TO_U24(pParam->language, pMeta);
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_LANGUAGE_MASK;
            BLT_AUD_LOG("Language: 0x%x", pParam->language);
        }
        else if(type == BLC_AUDIO_METATYPE_CCID_LIST)
        { //CCID_LIST
            if(length < 2)
            {
                goto lengthErr;
            }
            pParam->ccidListLen = length-1;
            pParam->pCcidList = pMeta;
            pMeta += length-1;
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_CCID_LIST_MASK;
            BLT_AUD_LOG("CCID List is %s", hex_to_str(pParam->pCcidList, pParam->ccidListLen));
        }
        else if(type == BLC_AUDIO_METATYPE_PARENTAL_RATING)
        { //Parental rating
            if(length != 2)
            {
                goto lengthErr;
            }
            STREAM_TO_U8(pParam->parentalRating, pMeta);
            if(pParam->parentalRating & BITS(4,5,6,7))
            {
                pParam->rspCode = 0x0C;
                pParam->rsnMark = type;
                return AUDIO_EPARAM;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_PARENTAL_RATING_MASK;
            BLT_AUD_LOG("Parental Rating: %d", pParam->parentalRating);
        }
        else if(type == BLC_AUDIO_METATYPE_PROGRAM_INFO_URI)
        { //Program Info URI
            if(length < 2)
            {
                goto lengthErr;
            }
            pParam->programInfoURILen = length-1;
            pParam->pProgramInfoURI = pMeta;
            pMeta += length-1;
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_PROGRAM_INFO_URI_MASK;
            BLT_AUD_LOG("Profram Info URI is %s", hex_to_str(pParam->pProgramInfoURI, pParam->programInfoURILen));
        }
        else if(type == BLC_AUDIO_METATYPE_EXTENDED_METADATA)
        { // Extended Metadata
            if(length <= 3)
            {
                goto lengthErr;
            }
            pParam->extMetadataLen = length-1;
            pParam->pExtMetadata = pMeta;
            pMeta += length-1;
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_EXTENDED_METADATA_MASK;
            BLT_AUD_LOG("Extended Metadata is %s", hex_to_str(pParam->pExtMetadata, pParam->extMetadataLen));
        }
        else if(type == BLC_AUDIO_METATYPE_VENDOR_SPECIFIC)
        { // Vendor_Specific
            if(length <= 3)
            {
                goto lengthErr;
            }
            pParam->vsMetadataLen = length-1;
            pParam->pVendorSpecMetadata = pMeta;
            pMeta += length-1;
            pParam->fieldExistFlg |= BLC_AUDIO_METATYPE_VENDOR_SPECIFIC_MASK;
            BLT_AUD_LOG("Vendor_Specific is %s", hex_to_str(pParam->pVendorSpecMetadata, pParam->vsMetadataLen));
        }
        else
        {
            if(!pParam->ignoreUnsuppMetadataFlag)
            {
                pParam->fieldExistFlg = 0;
                pParam->rspCode = 0x0A;
                pParam->rsnMark = type;
                BLT_AUD_LOG("Unsupport type: %d", type);
                return AUDIO_ERR_PARAM_INVALID;
            }
        }
    }
    pParam->metadataLen = metaLen;
    return AUDIO_ESUCC;

lengthErr:
    pParam->rspCode = 2;
    pParam->rsnMark = 0;
    return AUDIO_ELENGTH;
}

int blt_audio_getCodecSpecCapParam(u8 *pSpecCap, blt_audio_codecSpecCapParam_t *pParam)
{
    if(pParam == NULL || pSpecCap == NULL)
    {
        return AUDIO_ERR_NULL_POINTER;
    }

    if(pSpecCap[0] < 3)
    {
        return AUDIO_ERR_PARAM_INVALID;
    }

    u8 tempspecLen = pSpecCap[0];
    u8 length, type;
    pSpecCap++;
    pParam->fieldExistFlg = 0;

    while(tempspecLen != 0)
    {
        STREAM_TO_U8(length, pSpecCap);
        STREAM_TO_U8(type, pSpecCap);
        if(tempspecLen < (1+length))
        {
            return AUDIO_ERR_LTV_STRUCT_INVALID;
        }
        tempspecLen -= length+1;

        if(type == BLC_AUDIO_CAPTYPE_SUP_SAMPLE_FREQUENCY && length == 3)
        {// Supported_Sampling_Frequencies
            STREAM_TO_U16(pParam->frequency, pSpecCap);
            if(BLC_AUDIO_SUPP_FREQ_FLAG_CHECK_RFU(pParam->frequency))
            {
                return AUDIO_ERR_RFU_SUPP_FREQ;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_SAMPLE_FREQUENCY_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_SUP_FRAME_DURATION && length == 2)
        { // Supported_Frame_Durations
            STREAM_TO_U8(pParam->duration, pSpecCap);
            //TODO: PTS test,  RFU bits not need check
            pParam->duration = BLC_AUDIO_SUPP_DURATION_FLAG_VALID_BITS(pParam->duration);
            if(0 && BLC_AUDIO_SUPP_DURATION_FLAG_RFU(pParam->duration))
            {
                return AUDIO_ERR_RFU_SUPP_DURATIONS;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_FRAME_DURATION_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_SUP_AUDIO_CHN_COUNTS && length == 2)
        { // Audio_Channel_Counts
            STREAM_TO_U8(pParam->counts, pSpecCap);
            if(BLC_AUDIO_CHANNEL_COUNTS_RFU(pParam->counts))
            {
                return AUDIO_ERR_RFU_SUPP_CHANNEL_COUNTS;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_AUDIO_CHANNEL_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_SUP_OCTETS_PER_CODEC_FRAME && length == 5)
        { // Supported_Octets_Per_Codec_Frame
            STREAM_TO_U16(pParam->minOctets, pSpecCap);
            STREAM_TO_U16(pParam->maxOctets, pSpecCap);
            if(pParam->minOctets == 0 || pParam->maxOctets == 0)
            {
                return AUDIO_ERR_RFU_SUPP_PER_CODEC_FRAME;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_OCTETS_PER_CODEC_FRAME_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_SUP_MAX_CODEC_FRAMES_PER_SDU && length == 2)
        { // Supported_Max_Codec_Frames_Per_SDU
            STREAM_TO_U8(pParam->maxCodecFramesPerSDU, pSpecCap);
            if(pParam->maxCodecFramesPerSDU == 0)
            {
                return AUDIO_ERR_RFU_SUPP_MAX_CODEC_FRAME_PER_SDU;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_CODEC_FRAMES_PER_SDU_MASK;
        }
        else
        {
            pParam->fieldExistFlg = 0;
            return AUDIO_ERR_LTV_STRUCT_INVALID; /* Specific err */
        }
    }
    return AUDIO_ESUCC;
}

int blt_audio_getCodecSpecCfgParam(u8 *pSpecCfg, blc_audio_codecSpecCfgParsed_t *pParam)
{
    if(pParam == NULL || pSpecCfg == NULL)
    {
        return AUDIO_ERR_NULL_POINTER;
    }
    if(pSpecCfg[0] < 3)
    {
        return AUDIO_ERR_PARAM_INVALID;
    }

    u8 tempspecLen = pSpecCfg[0];
//  pParam->fieldExistFlg = 0;
    u8 length, type;
    pSpecCfg++;

    while(tempspecLen > 0)
    {
        STREAM_TO_U8(length, pSpecCfg);
        STREAM_TO_U8(type, pSpecCfg);
        if(tempspecLen < (1+length))
        {
            return AUDIO_ERR_LTV_STRUCT_INVALID;
        }

        tempspecLen -= length+1;

        if(type == BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY && length == 2)
        { // Sampling_Frequency
            STREAM_TO_U8(pParam->frequency, pSpecCfg);
            if(BLC_AUDIO_FREQ_CFG_RFU(pParam->frequency))
            {
                return AUDIO_ERR_RFU_SUPP_FREQ;
            }

            pParam->fieldExistFlg |= BLC_AUDIO_SAMPLE_FREQUENCY_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION && length == 2)
        { // Frame_Duration
            STREAM_TO_U8(pParam->duration, pSpecCfg);
            if(BLC_AUDIO_DURATION_RFU(pParam->duration))
            {
                return AUDIO_ERR_RFU_SUPP_DURATIONS; /* 0, 1 are valid */
            }
            pParam->fieldExistFlg |= BLC_AUDIO_FRAME_DURATION_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION && length == 5)
        { // Audio_Channel_Allocation
            STREAM_TO_U32(pParam->allocation, pSpecCfg);
            if(BLC_AUDIO_CHANNEL_ALLOCATION_RFU(pParam->allocation))
            {
                return AUDIO_ERR_RFU_CHANNEL_ALLOCATION;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_AUDIO_CHANNEL_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME && length == 3)
        { // Octets_Per_Codec_Frame
            STREAM_TO_U16(pParam->frameOcts, pSpecCfg);
            if(pParam->frameOcts == 0)
            {
                return AUDIO_ERR_RFU_SUPP_PER_CODEC_FRAME;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_OCTETS_PER_CODEC_FRAME_MASK;
        }
        else if(type == BLC_AUDIO_CAPTYPE_CFG_CODEC_FRAME_BLCKS_PER_SDU && length == 2)
        { // Codec_Frame_Blocks_Per_SDU
            STREAM_TO_U16(pParam->codecFrameBlksPerSDU, pSpecCfg);
            if(pParam->codecFrameBlksPerSDU == 0)
            {
                return AUDIO_ERR_RFU_SUPP_MAX_CODEC_FRAME_PER_SDU;
            }
            pParam->fieldExistFlg |= BLC_AUDIO_CODEC_FRAMES_PER_SDU_MASK;
        }
        else
        {
            pParam->fieldExistFlg = 0;
            return AUDIO_ERR_LTV_STRUCT_INVALID; /* Specific err */
        }
    }

    return AUDIO_ESUCC;
}

int blt_audio_checkCodecParamValid(blt_audio_codecSpecCapParam_t* codecCap, blc_audio_codecSpecCfgParsed_t* codecCfg)
{
    u16 flag = codecCap->fieldExistFlg & codecCfg->fieldExistFlg;

    if((flag & BLC_AUDIO_SAMPLE_FREQUENCY_MASK) && ((codecCap->frequency & BIT(codecCfg->frequency-1)) == 0))
    {
        return AUDIO_EPARAM;
    }

    if((flag & BLC_AUDIO_FRAME_DURATION_MASK) && ((codecCap->duration & BIT(codecCfg->duration)) == 0))
    {
        return AUDIO_EPARAM;
    }

    if(flag & BLC_AUDIO_AUDIO_CHANNEL_MASK)
    {
        u8 counts = blt_calBit1Number(codecCfg->allocation);
        if(codecCap->counts<counts)
        {
            return AUDIO_EPARAM;
        }
    }

    if((flag & BLC_AUDIO_OCTETS_PER_CODEC_FRAME_MASK) && \
            (codecCfg->frameOcts < codecCap->minOctets || codecCfg->frameOcts > codecCap->maxOctets))
    {
        return AUDIO_EPARAM;
    }

    if((flag & BLC_AUDIO_CODEC_FRAMES_PER_SDU_MASK) && (codecCfg->codecFrameBlksPerSDU > codecCap->maxCodecFramesPerSDU))
    {
        return AUDIO_EPARAM;
    }

    return AUDIO_ESUCC;
}

u16 blt_audio_getFrameOctsBySampleAndDuration(u8 audioSampleIdx, u8 durationType)
{
    if(durationType>2 || !audioSampleIdx || audioSampleIdx>14)
    {
        return 0;
    }

    const u32 audioFreqCfg[14] = {0, 8000, 11025, 16000, 22050, 24000, 32000, 44100, \
                               48000, 88200, 96000, 176400, 192000, 384000};
    const u16 audioDuration[2] = {7500, 10000};
    BLT_AUD_LOG("[audioFreq]: %d", audioFreqCfg[audioSampleIdx]);
    BLT_AUD_LOG("[duration]: %d", audioDuration[durationType]);

    u32 CodecParams = (2*audioFreqCfg[audioSampleIdx]*audioDuration[durationType])/8000000;
    BLT_AUD_LOG(">>CodecParams: %d", CodecParams);
    return (u16)CodecParams; //LC3 8 : 1
}






void blc_audio_setAclCentralIndexForCIS(u8 aclIdx1,u8 aclIdx2,u8 aclIdx3,u8 aclIdx4)
{
    blt_audio_cap_ctrl.kmaMark = true;
    blt_audio_cap_ctrl.aclIdx1 = aclIdx1;
    blt_audio_cap_ctrl.aclIdx2 = aclIdx2;
    blt_audio_cap_ctrl.aclIdx3 = aclIdx3;
    blt_audio_cap_ctrl.aclIdx4 = aclIdx4;
//  tlkapi_send_string_data(1, "blt_audio_cap_ctrl.aclIdx1", (u8*)&blt_audio_cap_ctrl.aclIdx1, 1);
//  tlkapi_send_string_data(1, "blt_audio_cap_ctrl.aclIdx2", (u8*)&blt_audio_cap_ctrl.aclIdx2, 1);
//  tlkapi_send_string_data(1, "blt_audio_cap_ctrl.aclIdx3", (u8*)&blt_audio_cap_ctrl.aclIdx3, 1);
//  tlkapi_send_string_data(1, "blt_audio_cap_ctrl.aclIdx4", (u8*)&blt_audio_cap_ctrl.aclIdx4, 1);
}

static int blt_audio_leEvtWrapHandler(u32 h, u8 *p, int n)
{
    if (h & HCI_FLAG_EVENT_BT_STD)      //Controller HCI event
    {
        /* LE controller event for Unicast Client */
        if (bap_unicast_clt_cb) {
            bap_unicast_clt_cb(h, p, n);
        }

        /* LE controller event for Unicast Server */
        if (bap_unicast_svr_cb) {
            bap_unicast_svr_cb(h, p, n);
        }
        /* LE controller event for Broadcast Sink(+Scan Delegator) */
        if (bap_bcst_sink_cb) {
            bap_bcst_sink_cb(h, p, n);
        }
        /* LE controller event for Broadcast Assistant */
        if (bap_bcst_assistant_cb) {
            bap_bcst_assistant_cb(h, p, n);
        }
    }

    return 0;
}


