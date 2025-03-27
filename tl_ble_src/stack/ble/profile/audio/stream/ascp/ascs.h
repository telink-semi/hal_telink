/********************************************************************************************************
 * @file    ascs.h
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

#include "ascs_client_buf.h"
#include "ascs_server_buf.h"




/******************************* ASCS Common Start **********************************************************************/

/******************************* ASCS Common End **********************************************************************/



/******************************* ASCS Client Start **********************************************************************/

extern const u8 gAppAscsCltSinkAseNum;
extern const u8 gAppAscsCltSrcAseNum;

typedef struct{
    u8  cigID;
    u8  cisID;
    blc_audio_codec_id_t  codecId;
    u8  codecFrameBlksPerSDU;
    u8  frequency;
    u8  duration;
    u16 frameOcts;
    u32 location;
} blc_ascsc_aseConfig_t;

//ASCS Client Event ID
typedef enum{
    AUDIO_EVT_ASCSC_START = AUDIO_EVT_TYPE_ASCSC,
    //NONE: all take over by BAP Unicast_Client Event, refer to 'audio_bapuc_evt_enum'
} audio_ascsc_evt_enum;


/**
 * @brief       This function serves to register ASCS Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerASCSControlClient(const blc_ascsc_regParam_t *param);


/**
 * @brief       This function serves to set the cig packing type
 * @param[in]   type - SEQUENTIAL or INTERLEAVED
 * @return      none.
 */
void blc_ascss_setCigPackingType(packing_type_t type);
/******************************* ASCS Client End **********************************************************************/


/******************************* ASCS Server Start **********************************************************************/

extern const u8 gAscssSinkAseCnt;
extern const u8 gAscssSrcAseCnt;

//ASCS Server Event ID
typedef enum{
    AUDIO_EVT_ASCSS_START = AUDIO_EVT_TYPE_ASCSS,
    //NONE: all take over by BAP Unicast_Server Event, refer to 'audio_bapus_evt_enum'
} audio_ascss_evt_enum;


/**
 * @brief       This function serves to register ASCS Server function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void                    blc_audio_registerASCSControlServer(const blc_ascss_regParam_t *param);

/**
 * @brief       This function serves to get ASE state information.
 * @param[in]   index       -
 * @return      NULL        - not find
 *              NON NULL    - the structure pointer of the ASE State.
 */
blt_ascss_ase_state_t*  blc_ascss_getAseStateInfo(u8 index);

/**
 * @brief       This function serves to get ASE state information.
 * @param[in]   index       -
 * @return      NULL        - not find
 *              NON NULL    - the structure pointer of the ASE State.
 */
blc_ascs_server_t*      blc_ascss_getAscssInfo(u8 index);

/**
 * @brief       This function serves to initialize ASE state information.
 * @param[in]   aseState    - the structure pointer of the ASE State.
 * @return      none.
 */
void                    blc_ascss_initAseParam(blt_ascss_ase_state_t* aseState);


/******************************* ASCS Server End **********************************************************************/
