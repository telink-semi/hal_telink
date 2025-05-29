/********************************************************************************************************
 * @file    svc_tmas.h
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

#include "vendor/common/user_config.h"

//TMAS: Telephony and Media Audio service

enum
{
    BLC_TMAP_ROLE_CALL_GATEWAY             = BIT(0),
    BLC_TMAP_ROLE_CALL_TERMINAL            = BIT(1),
    BLC_TMAP_ROLE_UNICAST_MEDIA_SENDER     = BIT(2),
    BLC_TMAP_ROLE_UNICAST_MEDIA_RECEIVER   = BIT(3),
    BLC_TMAP_ROLE_BROADCAST_MEDIA_SENDER   = BIT(4),
    BLC_TMAP_ROLE_BROADCAST_MEDIA_RECEIVER = BIT(5),
    BLC_TMAP_ROLE_RFU                      = 0xFFC0, //Bit6-15
};

#ifndef DEFAULT_TMAP_ROLE
    #define DEFAULT_TMAP_ROLE BLC_TMAP_ROLE_CALL_GATEWAY
#endif

/**
 * @brief      for user add default TMAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addTmasGroup(void);

/**
 * @brief      for user remove default TMAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeTmasGroup(void);
