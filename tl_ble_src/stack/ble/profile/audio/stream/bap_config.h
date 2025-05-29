/********************************************************************************************************
 * @file    bap_config.h
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


/* Service Role Support for Unicast Server */

#ifndef AUDIO_UNICAST_SERVER_SUPPORT_FRAMING
    #define AUDIO_UNICAST_SERVER_SUPPORT_FRAMING 0x00 //0x00 unframed ISOAL PDUs support,0x01 unframed ISOAL PDUs not support
#endif

#ifndef AUDIO_UNICAST_SERVER_PREFERRED_PHY
    #define AUDIO_UNICAST_SERVER_PREFERRED_PHY BIT(1) //BIT(0) 1M,BIT(1) 2M,BIT(2) Codec
#endif

#ifndef AUDIO_UNICAST_SERVER_PREFERRED_RTN
    #define AUDIO_UNICAST_SERVER_PREFERRED_RTN 5 //0x00-0xFF
#endif

#ifndef AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY
    #define AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY 0x32 //0x0005-0x0FA0
#endif

#ifndef AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MIN
    #define AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MIN 0x1388 //3byte,us count,0x1388=5000us
#endif

#ifndef AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MAX
    #define AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MAX 0x3a98 //3byte,us count,0x3a98=15000us
#endif

#ifndef AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MIN
    #define AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MIN 0x1388 //3byte,us count,0x1388=5000us
#endif

#ifndef AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MAX
    #define AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MAX 0x3a98 //3byte,us count,0x3a98=15000us
#endif

#ifndef AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT
    #define AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT (BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED | BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA)
#endif


/* Service Role Support for Unicast Client */

/* The maximum number of CISs per unicast group to support. */
#ifndef AUDIO_UNICAST_CLIENT_STREAMS
    #define AUDIO_UNICAST_CLIENT_STREAMS min(2, CIS_IN_CIGM_NUM_MAX) //The number of CIS supported by each CIG Master
#endif

#ifndef AUDIO_UNICAST_CLIENT_PREFERRED_PHY
    #define AUDIO_UNICAST_CLIENT_PREFERRED_PHY BIT(1) //BIT(0) 1M,BIT(1) 2M,BIT(2) Codec
#endif

#ifndef AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY
    #define AUDIO_UNICAST_CLIENT_MAX_TRANSPORT_LATENCY 0x32 //0x0005-0x0FA0
#endif


/* Number of Sink ASE characteristic */
#ifndef APP_AUDIO_ASCSS_SINK_ASE_CNT
    #define APP_AUDIO_ASCSS_SINK_ASE_CNT 2 /* default 2, can be changed by user */
#endif
#ifndef APP_AUDIO_ASCSS_SINK_ASE_ID
    #define APP_AUDIO_ASCSS_SINK_ASE_ID 1
#endif
/* Number of Source ASE characteristics */
#ifndef APP_AUDIO_ASCSS_SRC_ASE_CNT
    #define APP_AUDIO_ASCSS_SRC_ASE_CNT 2 /* default 2, can be changed by user */
#endif
#ifndef APP_AUDIO_ASCSS_SRC_ASE_ID
    #define APP_AUDIO_ASCSS_SRC_ASE_ID 0x10
#endif

#define APP_AUDIO_ASCSS_ASE_CNT (APP_AUDIO_ASCSS_SINK_ASE_CNT + APP_AUDIO_ASCSS_SRC_ASE_CNT)


/* Number of Broadcast Receive State characteristic */
#define APP_AUDIO_BASS_CLIENT_RECV_STATE_CNT 1 /* default 1, can be changed by user */
#define APP_AUDIO_BASS_SERVER_RECV_STATE_CNT 1


#define STACK_AUDIO_ASCSS_MAX_SINK_ASE_CNT   4
#define STACK_AUDIO_ASCSS_MAX_SRC_ASE_CNT    4
#define STACK_AUDIO_ASCSS_MAX_ASE_CNT        (STACK_AUDIO_ASCSS_MAX_SINK_ASE_CNT + STACK_AUDIO_ASCSS_MAX_SRC_ASE_CNT)

#define STACK_AUDIO_BASS_RECV_STATE_CNT      4


#if APP_AUDIO_ASCSS_SINK_ASE_CNT > STACK_AUDIO_ASCSS_MAX_SINK_ASE_CNT
    #error "APP_AUDIO_ASCSS_SINK_ASE_CNT too large"
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_CNT > STACK_AUDIO_ASCSS_MAX_SRC_ASE_CNT
    #error "APP_AUDIO_ASCSS_SRC_ASE_CNT too large"
#endif

#if APP_AUDIO_BASS_CLIENT_RECV_STATE_CNT > STACK_AUDIO_BASS_RECV_STATE_CNT || APP_AUDIO_BASS_SERVER_RECV_STATE_CNT > STACK_AUDIO_BASS_RECV_STATE_CNT
    #error "APP_AUDIO_BASS_RECV_STATE_NUM too large"
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_CNT + APP_AUDIO_ASCSS_SRC_ASE_CNT <= 0
    #error "ascs:At least one of the sink ASE or Source ASE characteristics shall be supported"
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_ID <= 0 || APP_AUDIO_ASCSS_SRC_ASE_ID <= 0
    #error "ascs sink/source ASE_ID error"
#endif

#if APP_AUDIO_ASCSS_SINK_ASE_ID + APP_AUDIO_ASCSS_SINK_ASE_CNT > APP_AUDIO_ASCSS_SRC_ASE_ID
    #error "ascs sink id init error"
#endif

#if APP_AUDIO_ASCSS_SRC_ASE_ID + APP_AUDIO_ASCSS_SRC_ASE_CNT > 0x100
    #error "ascs source id init error"
#endif
