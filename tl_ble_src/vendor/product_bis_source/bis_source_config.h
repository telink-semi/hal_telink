/********************************************************************************************************
 * @file    bis_source_config.h
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

#define PRODUCT_MCU_DEMO 0

//this product for google broadcast source.
#define PRODUCT_GOOGLE_BROADCAST_SOURCE 1

//this product for SIG Auracast transmitter
#define PRODUCT_SIG_AURACAST_TRANSMITTER 2

#define PRODUCT_BIS_SOURCE_SELECT        PRODUCT_SIG_AURACAST_TRANSMITTER

#define BLC_PM_EN                        0
#define BLC_PM_DEEP_RETENTION_MODE_EN    0
