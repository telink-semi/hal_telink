/********************************************************************************************************
 * @file    aud_cfg_tbl.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "bap.h"


//BAP_v1.0.1 page 25, Table 3.5: Unicast Server audio capability support requirements
//BAP_v1.0.1 page 33, Table 3.11: Unicast Client audio capability support requirements
//BAP_v1.0.1 page 34, Table 3.12: Broadcast Source audio capability configuration support requirements
//BAP_v1.0.1 page 42, Table 3.17: Broadcast Sink audio capability support requirements
const std_codec_settings_t codecSettings[16] = {
    {BLC_AUDIO_SUPP_FREQ_FLAG_8000, BLC_AUDIO_FREQ_CFG_8000,  BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 26}, //8_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_8000, BLC_AUDIO_FREQ_CFG_8000,  BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  30}, //8_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_16000,BLC_AUDIO_FREQ_CFG_16000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 30}, //16_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_16000,BLC_AUDIO_FREQ_CFG_16000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  40}, //16_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_24000,BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 45}, //24_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_24000,BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  60}, //24_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_32000,BLC_AUDIO_FREQ_CFG_32000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 60}, //32_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_32000,BLC_AUDIO_FREQ_CFG_32000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  80}, //32_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_44100,BLC_AUDIO_FREQ_CFG_44100, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 97}, //441_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_44100,BLC_AUDIO_FREQ_CFG_44100, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  130},//441_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 75}, //48_1
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  100},//48_2
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 90}, //48_3
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  120},//48_4
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5,BLC_AUDIO_DURATION_CFG_7_5, 117},//48_5
    {BLC_AUDIO_SUPP_FREQ_FLAG_48000,BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  155},//48_6
};

//BAP_v1.0.1 page 87, Table 5.2: QoS configuration support setting requirements for the Unicast Client and Unicast Server
const std_qos_settings_t unicastQosSettings[2][16] = {
    //low latency
    {{7500,  CIS_UNFRAMED, 26, 2, 8},     //8_1_1
     {10000, CIS_UNFRAMED, 30, 2, 10},    //8_2_1
     {7500,  CIS_UNFRAMED, 30, 2, 8},     //16_1_1
     {10000, CIS_UNFRAMED, 40, 2, 10},    //16_2_1
     {7500,  CIS_UNFRAMED, 45, 2, 8},     //24_1_1
     {10000, CIS_UNFRAMED, 60, 2, 10},    //24_2_1
     {7500,  CIS_UNFRAMED, 60, 2, 8},     //32_1_1
     {10000, CIS_UNFRAMED, 80, 2, 10},    //32_2_1
     {8163,  CIS_FRAMED,   97, 5, 24},    //441_1_1
     {10884, CIS_FRAMED,   130,5, 31},    //441_2_1
     {7500,  CIS_UNFRAMED, 75, 5, 15},    //48_1_1
     {10000, CIS_UNFRAMED, 100,5, 20},    //48_2_1
     {7500,  CIS_UNFRAMED, 90, 5, 15},    //48_3_1
     {10000, CIS_UNFRAMED, 120,5, 20},    //48_4_1
     {7500,  CIS_UNFRAMED, 117,5, 15},    //48_5_1
     {10000, CIS_UNFRAMED, 155,5, 20}},   //48_6_1
    //high reliability
    {{7500,  CIS_UNFRAMED, 26, 13,75},    //8_1_1
     {10000, CIS_UNFRAMED, 30, 13,95},    //8_2_1
     {7500,  CIS_UNFRAMED, 30, 13,75},    //16_1_2
     {10000, CIS_UNFRAMED, 40, 13,95},    //16_2_2
     {7500,  CIS_UNFRAMED, 45, 13,75},    //24_1_2
     {10000, CIS_UNFRAMED, 60, 13,95},    //24_2_2
     {7500,  CIS_UNFRAMED, 60, 13,75},    //32_1_2
     {10000, CIS_UNFRAMED, 80, 13,95},    //32_2_2
     {8163,  CIS_FRAMED,   97 ,13,80},    //441_1_2
     {10884, CIS_FRAMED,   130,13,85},    //441_2_2
     {7500,  CIS_UNFRAMED, 75 ,13,75},    //48_1_2
     {10000, CIS_UNFRAMED, 100,13,95},    //48_2_2
     {7500,  CIS_UNFRAMED, 90, 13,75},    //48_3_2
     {10000, CIS_UNFRAMED, 120,13,100},   //48_4_2
     {7500,  CIS_UNFRAMED, 117,13,75},    //48_5_2
     {10000, CIS_UNFRAMED, 155,13,100}}   //48_6_2
};

//BAP_v1.0.1 page 112, Table 6.4: Broadcast Audio Stream configuration support requirements for the Broadcast Source and Broadcast Sink
const std_qos_settings_t broadcastQosSettings[2][16] = {
   //low latency
   {{7500,  BIS_UNFRAMED,   26,     2,  8},     //8_1_1
    {10000, BIS_UNFRAMED,   30,     2,  10},    //8_2_1
    {7500,  BIS_UNFRAMED,   30,     2,  8},     //16_1_1
    {10000, BIS_UNFRAMED,   40,     2,  10},    //16_2_1
    {7500,  BIS_UNFRAMED,   45,     2,  8},     //24_1_1
    {10000, BIS_UNFRAMED,   60,     2,  10},    //24_2_1
    {7500,  BIS_UNFRAMED,   60,     2,  8},     //32_1_1
    {10000, BIS_UNFRAMED,   80,     2,  10},    //32_2_1
    {8163,  BIS_FRAMED,     97,     4,  24},    //441_1_1
    {10884, BIS_FRAMED,     130,    4,  31},    //441_2_1
    {7500,  BIS_UNFRAMED,   75,     4,  15},    //48_1_1
    {10000, BIS_UNFRAMED,   100,    4,  20},    //48_2_1
    {7500,  BIS_UNFRAMED,   90,     4,  15},    //48_3_1
    {10000, BIS_UNFRAMED,   120,    4,  20},    //48_4_1
    {7500,  BIS_UNFRAMED,   117,    4,  15},    //48_5_1
    {10000, BIS_UNFRAMED,   155,    4,  20}},   //48_6_1
    //high reliability
   {{7500,  BIS_UNFRAMED,   26,     4,  45},    //8_1_2
    {10000, BIS_UNFRAMED,   30,     4,  60},    //8_2_2
    {7500,  BIS_UNFRAMED,   30,     4,  45},    //16_1_2
    {10000, BIS_UNFRAMED,   40,     4,  60},    //16_2_2
    {7500,  BIS_UNFRAMED,   45,     4,  45},    //24_1_2
    {10000, BIS_UNFRAMED,   60,     4,  60},    //24_2_2
    {7500,  BIS_UNFRAMED,   60,     4,  45},    //32_1_2
    {10000, BIS_UNFRAMED,   80,     4,  60},    //32_2_2
    {8163,  BIS_FRAMED,     97,     4,  54},    //441_1_2
    {10884, BIS_FRAMED,     130,    4,  60},    //441_2_2
    {7500,  BIS_UNFRAMED,   75,     4,  50},    //48_1_2
    {10000, BIS_UNFRAMED,   100,    4,  65},    //48_2_2
    {7500,  BIS_UNFRAMED,   90,     4,  50},    //48_3_2
    {10000, BIS_UNFRAMED,   120,    4,  65},    //48_4_2
    {7500,  BIS_UNFRAMED,   117,    4,  50},    //48_5_2
    {10000, BIS_UNFRAMED,   155,    4,  65}},   //48_6_2
};

//BAP_v1.0.1 page 52, Table 4.1: Unicast LC3 Audio Configurations
const std_unicast_aud_cfg_t     unicastAudioConfigurations[16] =
{
    {1,1,0,1,0,0,0,1,1}, //1
    {1,0,1,0,0,1,0,1,1}, //2
    {1,1,1,1,0,1,0,1,2}, //3
    {1,1,0,2,2,0,0,1,1}, //4
    {1,1,1,2,2,1,0,1,2}, //5
    {1,2,0,1,2,0,0,2,2}, //6(i)
    {2,2,0,1,1,0,0,2,2}, //6(ii)
    {1,1,1,1,0,1,0,2,2}, //7(i)
    {2,1,1,1,0,1,0,2,2}, //7(ii)
    {1,2,1,1,2,1,0,2,3}, //8(i)
    {2,2,1,1,1,1,1,2,3}, //8(ii)
    {1,0,2,0,0,1,2,2,2}, //9(i)
    {2,0,2,0,0,1,1,2,2}, //9(ii)
    {1,0,1,0,0,2,2,1,1}, //10
    {1,2,2,1,2,1,2,2,4}, //11(i)
    {2,2,2,1,1,1,1,2,4}, //11(ii)
};

//BAP_v1.0.1 page 77, Table 4.24: Broadcast LC3 Audio Configurations and support requirements
const std_bcst_aud_cfg_t        bcstAudioConfigurations[3] =
{
    {1,1,1}, //12
    {1,2,2}, //13
    {2,1,1}, //14
};

