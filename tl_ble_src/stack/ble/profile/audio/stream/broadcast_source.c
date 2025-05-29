/********************************************************************************************************
 * @file    broadcast_source.c
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

//Broadcast Source
typedef struct
{
    u8                            BIS_index;
    blc_audio_codecSpecCfgParam_t codecCfg;
} blt_BASE_BIS_param_t;

typedef struct
{
    u8                            BIS_num;
    blc_audio_codec_id_t          codecId; //Codec ID, 06 0000 0000 mean LC3 codec
    blc_audio_codecSpecCfgParam_t codecCfg;
    blc_audio_metadataParam_t     metadata;
    blt_BASE_BIS_param_t          BIS_param[0];
} blt_BASE_BIG_param_t;

typedef struct
{
    u32 presentation_delay; //Range:0x000000-0xFFFFFF  Units: us

    u8                   subGroupNum;
    blt_BASE_BIG_param_t BIG_param[0];
} blt_bcstAudioAnnouncements_param_t;

int blc_bap_calculateBASELength(void *base)
{
    blt_bcstAudioAnnouncements_param_t *ptr    = (blt_bcstAudioAnnouncements_param_t *)base;
    int                                 length = 8; //length:1Byte, Type:1Byte, BAAS:2Byte, Presentation Delay:3Byte, Num Subgroups:1Byte

    blt_BASE_BIG_param_t *bigPtr = NULL;
    base                         = ptr->BIG_param;

    for (int i = 0; i < ptr->subGroupNum; i++) {
        bigPtr = (blt_BASE_BIG_param_t *)base;
        length++;                               //Num_BIS
        length += sizeof(blc_audio_codec_id_t); //Codec_ID[i]

        length++;                               //Codec_Specific_Configuration_Length[i]
        length += blc_bap_calculateCodecSpecificConfigurationLength(&bigPtr->codecCfg);

        length++;                               //Metadata Length[i]
        length += blc_bap_calculateMetadataLength(&bigPtr->metadata);

        for (int j = 0; j < bigPtr->BIS_num; j++) {
            blt_BASE_BIS_param_t *bisPtr = &bigPtr->BIS_param[j];
            length++; //Bis index[i[j]];
            length++; //Codec_Specific_Configuration_Length[i[j]];
            length += blc_bap_calculateCodecSpecificConfigurationLength(&bisPtr->codecCfg);
        }
        base = (u8 *)base + sizeof(blt_BASE_BIG_param_t) + bigPtr->BIS_num * sizeof(blt_BASE_BIS_param_t);
    }
    return length;
}

u8 *blc_bap_setBASEToAddress(void *base, u8 *dst)
{
    blt_bcstAudioAnnouncements_param_t *ptr = (blt_bcstAudioAnnouncements_param_t *)base;

    U8_TO_STREAM(dst, blc_bap_calculateBASELength(base) - 1);

    U8_TO_STREAM(dst, DT_SERVICE_DATA);
    U16_TO_STREAM(dst, SERVICE_UUID_BASIC_AUDIO_ANNOUNCEMENT);
    U24_TO_STREAM(dst, ptr->presentation_delay);

    blt_BASE_BIG_param_t *bigPtr = NULL;
    base                         = ptr->BIG_param;
    U8_TO_STREAM(dst, ptr->subGroupNum);

    for (int i = 0; i < ptr->subGroupNum; i++) {
        bigPtr = (blt_BASE_BIG_param_t *)base;
        //Num_BIS
        U8_TO_STREAM(dst, bigPtr->BIS_num);

        //Codec_ID[i]
        STR_TO_STREAM(dst, &bigPtr->codecId, sizeof(blc_audio_codec_id_t));

        //Codec_Specific_Configuration_Length[i]
        U8_TO_STREAM(dst, blc_bap_calculateCodecSpecificConfigurationLength(&bigPtr->codecCfg));
        dst = blc_bap_setCodecSpecificConfigurationToAddress(&bigPtr->codecCfg, dst);

        //Metadata Length[i]
        U8_TO_STREAM(dst, blc_bap_calculateMetadataLength(&bigPtr->metadata));
        dst = blc_bap_setMetadataToAddress(&bigPtr->metadata, dst);

        for (int j = 0; j < bigPtr->BIS_num; j++) {
            blt_BASE_BIS_param_t *bisPtr = &bigPtr->BIS_param[j];
            //Bis index[i[j]];
            U8_TO_STREAM(dst, bisPtr->BIS_index);
            //Codec_Specific_Configuration_Length[i[j]];
            U8_TO_STREAM(dst, blc_bap_calculateCodecSpecificConfigurationLength(&bisPtr->codecCfg));
            dst = blc_bap_setCodecSpecificConfigurationToAddress(&bisPtr->codecCfg, dst);
        }
        base = (u8 *)base + sizeof(blt_BASE_BIG_param_t) + bigPtr->BIS_num * sizeof(blt_BASE_BIS_param_t);
    }

    return dst;
}
