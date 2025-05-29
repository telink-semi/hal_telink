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
#ifndef _APP_CS_H_
#define _APP_CS_H_

#if (INTER_TEST_MODE == TEST_RAS_SERVER)

#define CS_MAX_NUM 4

typedef struct __attribute__((packed))
{
    u16 connhandle;
    u8  config_id;
    u8  acl_role;
    u8  exch_start_state;
    u8  exch_cmplt_state;
    u32 exchange_tick;

} cs_control_t;

typedef struct __attribute__((packed))
{
    cs_control_t cs_ctrl[CS_MAX_NUM];
} cs_app_control_t;

extern cs_app_control_t         cs_app_ctrl;

/**
 * @brief      Clear CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x01 success 0x00 failed
 */
int user_clrCsCtrlByHadle(u16 connhandle);

/**
 * @brief      BLE CS  read remote support capabilities complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_read_remote_support_capabilities_complete_event_handle(u8 *p);

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
 * @brief      BLE channel sounding procedure control loop
 * @param      None
 * @return     None
 */
void cs_procedure_ctrl(void);

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void);

#if PRF_ENABLE
void app_le_cs_procedure_enable_complete_event_handle(u8 *p);
void app_le_cs_subevent_result_event_handle(u8 *p);
void app_le_cs_subevent_result_continue_event_handle(u8 *p);
void app_le_cs_iop_setConnectionHandle(u16 connHandle); //used to set a connection handle for static iop data
#endif

#endif //#if (INTER_TEST_MODE == TEST_RAS_SERVER)
#endif /* _APP_CS_H_ */
