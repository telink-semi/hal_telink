/********************************************************************************************************
 * @file    cap.h
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

#define BLT_CAP_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_CAP_LOG, "[CAP]" fmt, ##__VA_ARGS__)


extern const u8 gAppAudioAclMaxNum;
extern const u8 gAppAudioAclCentralNum;
extern const u8 gAppAudioAclPeripheralNum;

typedef struct
{
    blc_adv_ltv_t ltv;
    u16           casUuid;
    u8            announcementType;
} blc_capAnnouncement_t;

extern const blc_capAnnouncement_t capTargetAnnouncement;

extern const blc_capAnnouncement_t capGeneralAnnouncement;

/**
 * @brief       This function initial initiator content control.
 * @return      none.
 */
void blc_cap_initiatorContentCtrl(void);

/**
 * @brief       This function initial initiator stream transitions.
 * @return      none.
 */
void blc_cap_initiatorStreamTrans(void);

/**
 * @brief       This function initial commander capture and rendering control.
 * @return      none.
 */
void blc_cap_commanderCaptureRenderingCtrl(void);

/**
 * @brief       This function initial commander stream transitions.
 * @return      none.
 */
void blc_cap_commanderStreamTrans(void);

/**
 * @brief       This function initial acceptor content control..
 * @return      none.
 */
void blc_cap_acceptorContentCtrl(void);

/**
 * @brief       This function initial acceptor capture and rendering control..
 * @return      none.
 */
void blc_cap_acceptorCaptureRenderingCtrl(void);

/**
 * @brief       This function initial acceptor unicast stream transitions..
 * @return      none.
 */
void blc_cap_acceptorUnicastStreamTrans(void);

/**
 * @brief       This function initial acceptor broadcast stream transitions..
 * @return      none.
 */
void blc_cap_acceptorBcstStreamTrans(void);

/**
 * @brief       This function initial acceptor stream transitions.
 * @return      none.
 */
void blc_cap_acceptorStreamTrans(void);

/**
 * @brief       This function initial unicast initiator.
 * @return      none.
 */
void blc_cap_initUnicastInitiator(void);

/**
 * @brief       This function initial broadcast commander.
 * @return      none.
 */
void blc_cap_initBcstCommander(void);

/**
 * @brief       This function initial unicast acceptor.
 * @return      none.
 */
void blc_cap_initUnicastAcceptor(void);

/**
 * @brief       This function initial broadcast acceptor.
 * @return      none.
 */
void blc_cap_initBcstAcceptor(void);

/**
 * @brief       This function initial audio acceptor.
 * @return      none.
 */
void blc_cap_initAudioAcceptor(void);
