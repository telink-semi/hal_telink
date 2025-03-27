/********************************************************************************************************
 * @file    svc_hid.h
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


//sdk fix service uuid handle

#define SERVICE_HUMAN_INTERFACE_DEVICE_HDL              SERVICE_HID_START_HDL
#define HID_MAX_HDL_NUM                                 0x40

#define SERVICE_ULTRA_LOW_LATENCY_HID_HDL               SERVICE_HUMAN_INTERFACE_DEVICE_HDL + HID_MAX_HDL_NUM
#define ULL_HID_MAX_HDL_NUM                             0x08

#include "svc_keyboard/svc_keyboard.h"
#include "svc_mouse/svc_mouse.h"
#include "svc_km/svc_km.h"
#include "hid/svc_hid.h"
#include "ull_hid/svc_ull_hid.h"

