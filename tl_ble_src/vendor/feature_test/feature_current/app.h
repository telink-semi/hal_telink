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

#if (FEATURE_TEST_MODE == TEST_CURRENT)

typedef enum
{
    POWER_CFG_CCLK_24MHZ = 0x24,
    POWER_CFG_CCLK_32MHZ = 0x32,
    POWER_CFG_CCLK_48MHZ = 0x48,
    POWER_CFG_CCLK_64MHZ = 0x64,
    POWER_CFG_CCLK_96MHZ = 0x96,

    POWER_CFG_CCLK_0dBm = 0x00,
    POWER_CFG_CCLK_4dBm = 0x04,
    POWER_CFG_CCLK_8dBm = 0x08,
    POWER_CFG_CCLK_10dBm = 0x10,

    POWER_CFG_PDU_LEN_12BYTES = 0x12,
    POWER_CFG_PDU_LEN_31BYTES = 0x31
} app_power_cfg_e;

typedef struct
{
    unsigned char cclk;
    unsigned char tx_power;
    unsigned char pdu_len;
} app_power_cfg_t;

extern app_power_cfg_t app_power_cfg;

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


#endif //end of (FEATURE_TEST_MODE == ...)

#endif /* VENDOR_APP_H_ */
