/********************************************************************************************************
 * @file    svc_dis.h
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

//DIS: Device Information Service.

typedef struct
{
    u8 manufacturer[5]; //manufacturer-defined identifier
    u8 oui[3];          //Organizationally Unique Identifier(OUI)
} dis_system_id_t;

typedef struct __attribute__((packed))
{
    u8  vidSrc;
    u16 vid;
    u16 pid;
    u16 ver;
} dis_pnp_t;

/**
 * @brief      for user add default DIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addDisGroup(void);

/**
 * @brief      for user remove default DIS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeDisGroup(void);
