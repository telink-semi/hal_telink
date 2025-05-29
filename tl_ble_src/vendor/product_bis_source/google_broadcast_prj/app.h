/********************************************************************************************************
 * @file    app.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_GOOGLE_BROADCAST_SOURCE)

    #include "stack/ble/ble.h"


    #define APP_PRESENTATION_DELAY       40 * 1000 //presentation delay
    #define APP_BIS_PARAM_SDU_INTERVAL   5000      //5000us
    #define APP_BIS_PARAM_ISO_INTERVAL   10000     //5000us
    #define APP_BIS_PARAM_NSE            8
    #define APP_BIS_PARAM_BN             2
    #define APP_BIS_PARAM_IRC            2
    #define APP_BIS_PARAM_PTO            2

    #define APP_BIS_PARAM_SDU_SIZE       215

    #define APP_BIS_SEND_DATA_FLOW_CTRL  0
    #define APP_BIS_SEND_DATA_SOFT_TIMER 1
    #define APP_BIS_SEND_DATA_MODE       APP_BIS_SEND_DATA_FLOW_CTRL

/*
 * Broadcast source extend Advertise parameter
 * must have: local Name/ Flags
 * BAP: broadcast ID
 */
typedef struct
{
    //adv local name
    u8 localNameLen;
    u8 localNameType;
    u8 localName[sizeof(DEFAULT_LOCAL_NAME) - 1];
    //adv flags
    u8 flagsLen;
    u8 flagsType;
    u8 flags;
    //adv BAAS UUID (BAP)
    u8  broadcastIDLen;
    u8  broadcastIDType;
    u16 BAAS_UUID;
    u8  broadcastID[3];
    //adv PBAS UUID (PBP)
    u8  PBAFeatureLen;
    u8  PBAFeatureType;
    u16 PBAS_UUID;
    u8  PBAFeature;
    u8  metadataLen;
    u8  metadata[DEFAULT_METADATA_LENGTH];
    //adv Broadcast Name (PBP)
    u8 broadcastNameLen;
    u8 broadcastNameType;
    u8 broadcastName[sizeof(DEFAULT_BROADCAST_NAME) - 1];

} bisSourceAdvData_t;

/*
 * Broadcast source PDA Advertise parameter
 * must have: local Name/ Flags
 * BAP: broadcast ID
 * PBP: Broadcast Name/Public Broadcast Announcement
 */
typedef struct
{
} bisPdaCodecSpecificConfig_t;

typedef struct
{
} bisPdaMetadata_t;

typedef struct
{
    u8 bisIndex;
    u8 size;
} bisPdaEachAddiInfo_t;

typedef struct
{
    u8 numBis;
    // codec id
    blc_audio_codec_id_t codecId;
    //codec specific configuration
    u8                          CodecSpecificConfigLen;
    bisPdaCodecSpecificConfig_t CodecSpecificConfig;
    u8                          metadataLen;
    bisPdaMetadata_t            metadata;
    bisPdaEachAddiInfo_t        addiInfo;
} bisGroupInfo_t;

typedef struct
{
    u8             length;
    u8             type;
    u16            BASS_UUID;
    u8             presentationDelay[3];
    u8             numSubgroups;
    bisGroupInfo_t groupInfo;
} bisSourcPdaeAdvData_t;

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void user_init_normal(void);


/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop(void);


/**
 * @brief     BIS ISO send timer0 loop
 * @param[in]  none.
 * @return     none.
 */
void app_timer_test_irq_proc(void);


#endif
