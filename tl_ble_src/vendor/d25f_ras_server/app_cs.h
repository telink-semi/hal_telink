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

/**
 * @brief      BLE CS  read remote support capabilities complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_read_remote_support_capabilities_complete_event_handle(u16 aclHandle, u8 *p, u16 n);

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u16 aclHandle, u8 *p, u16 n);

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u16 aclHandle, u8 *p, u16 n);

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u16 aclHandle, u8 *p, u16 n);

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u16 aclHandle, u8 *p, u16 n);

/**
 * @brief      BLE channel sounding procedure control loop
 * @param      None
 * @return     None
 */
void cs_procedure_ctrl(void);

#endif /* _APP_CS_H_ */
