/********************************************************************************************************
 * @file    hdt_phy_test.h
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
#ifndef STACK_BLE_CONTROLLER_PHY_HDT_PHY_TEST_H_
#define STACK_BLE_CONTROLLER_PHY_HDT_PHY_TEST_H_
#include "tl_common.h"

/**
 *  @brief  Define RF mode.
 */
typedef enum
{
    RF_MODE_BLE_HDT_SHORT_FORMAT            = BIT(0),
    RF_MODE_BLE_HDT_2M                      = BIT(1),
    RF_MODE_BLE_HDT_3M                      = BIT(2),
    RF_MODE_BLE_HDT_4M                      = BIT(3),
    RF_MODE_BLE_HDT_6M                      = BIT(4),
    RF_MODE_BLE_HDT_7P5M                    = BIT(5),

} hdt_rf_mode_e;

void blt_InitHdtPhyTestDriver(hdt_rf_mode_e mode);

/**
 * @brief      for user to set HDT PHY test enable or disable
 * @param[in]  en - 1: enable; 0:disable
 * @return     status: 0x00 command OK, no other rvalue
 */
ble_sts_t blc_phy_setHdtPhyTestEnable(u8 en);

#endif /* STACK_BLE_CONTROLLER_PHY_HDT_PHY_TEST_H_ */
