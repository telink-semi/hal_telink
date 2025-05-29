/********************************************************************************************************
 * @file    app_ap.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    12,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

/**
 * @brief      Callback function to handle controller events for the AP (Access Point) system.
 * @param[in]  h - The event handle associated with the controller event.
 * @param[in]  p - Pointer to the event data buffer.
 * @param[in]  n - The length of the event data buffer.
 * @return     none.
 */
void app_ap_controller_event_callback(u32 h, u8 *p, int n);

/**
 * @brief      Callback function to handle host events for the AP system.
 * @param[in]  h - The event handle associated with the host event.
 * @param[in]  p - Pointer to the event data buffer.
 * @param[in]  n - The length of the event data buffer.
 * @return     none.
 */
void app_ap_host_event_callback(u32 h, u8 *p, int n);

/**
 * @brief      Callback function to handle protocol events for the AP system.
 * @param[in]  aclHandle - The ACL (Asynchronous Connection-Less) handle associated with the event.
 * @param[in]  evtID - The event ID that describes the type of protocol event.
 * @param[in]  pData - Pointer to the event data buffer.
 * @param[in]  dataLen - The length of the event data buffer.
 * @return     int - The status of event handling (e.g., 0 for success, negative value for failure).
 */
int app_ap_prf_event_callback(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);

/**
 * @brief      Initialize the AP (Access Point) system.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_ap_init(void);

/**
 * @brief      Main loop for the AP system. Should be called periodically to process AP tasks.
 * @param[in]  none - No input parameters.
 * @return     none.
 */
void app_ap_loop(void);
