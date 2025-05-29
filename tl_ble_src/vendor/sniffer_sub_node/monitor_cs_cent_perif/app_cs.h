/********************************************************************************************************
 * @file    app_cs.h
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
#ifndef APP_CS_H_
#define APP_CS_H_

#include "app_config.h"
#include "vendor/common/sniffer_common/sniffer_common.h"

#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

typedef struct __attribute__((packed))
{
    u16 Connection_Handle;
    u8  Config_ID;
    u8  Main_Mode;
    u8  Sub_Mode;
    u8  Role;
    u8  RTT_Type;
    u8  valid;
} app_cs_config_t;

typedef struct __attribute__((packed))
{
    float state;
    float err_cov;
    float proc_noise_cov;
    float msr_noise_cov;
    float kal_gain;
    u32   update_tick;
} kalmanFilter_t;

extern chn_sound_capabilities_t appCsLocalSupportCap;

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u8 *p);

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u8 *p);

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u8 *p);

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u8 *p);

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void);

/**
 * @brief      Get cs config buffer by acl connect handle and config ID
 * @param[in]  connhandle ACL connect handle
 * @param[in]  Config_ID  config ID
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_getCSConfig(u16 connHandle, u8 Config_ID);

#endif
#endif /* APP_CS_H_ */
