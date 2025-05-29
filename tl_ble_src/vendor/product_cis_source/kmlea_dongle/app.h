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
#ifndef VENDOR_APP_H_
#define VENDOR_APP_H_

#include "app_config.h"


#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)


    #define PEER_DEVICE_USE_CIS_PERIPHERAL_TEST 0


extern int central_smp_pending;
extern u8  aclCen_for_cis_number;
extern u16 acl_cen_connnected[];

    #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
extern u8  app_cis_established;
extern u16 app_cisConnHandle;
    #endif


typedef struct
{
    u8  length;
    u8  type;
    u8  data[0];
    u16 resved;
} app_advdata_LTV;

typedef struct
{
    u8  announcement_type;
    u32 available_audio_context;
    u8  metadata_length;
    u8  metadata[0];
} app_basp_adv;

/*
 * @brief   unicast server announcement
 */
typedef struct __attribute__((packed))
{
    u8  announcement_type;
    u32 available_audio_context;
    u8  metadata_length;
    u8  metadata[0];
} app_adv_announcement_t;

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
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop(void);


/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n);


/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback(u32 h, u8 *para, int n);


/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt);

typedef enum
{
    ACLCEN_IDX_MOUSE    = 0,
    ACLCEN_IDX_KEYBOARD = 1,
    ACLCEN_IDX_CIS      = 2,
    ACLCEN_IDX_MAX      = 2,
} aclc_idx_t;

typedef struct
{
    u8 acl_central_idx;
} dev_cus_info_t;


#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)

#endif
