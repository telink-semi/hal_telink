/********************************************************************************************************
 * @file    prf_hid.h
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

#define BLC_HID_PRF_LOG                     BLC_PROFILE_DEBUG

/*
 * all profile: Human Interface Device(HID), Ultra Low Latency HID(ULL HID)
 */

enum{
    HUMAN_INTERFACE_DEVICE_CLIENT_START = PRF_HUMAN_INTERFACE_DEVICE_CLIENT_START - 1,
    HID_CLIENT,
    ULLHID_CLIENT,

    HUMAN_INTERFACE_DEVICE_SERVER_START = HUMAN_INTERFACE_DEVICE_CLIENT_START + PRF_SERVER_OFFSET,
    HID_SERVER,
    ULLHID_SERVER,

};


enum{
    HUMAN_INTERFACE_DEVICE_EVT_TYPE_CLIENT_START = PRF_EVTID_HUMAN_INTERFACE_DEVICE_START,
    HID_EVT_TYPE_HID_CLIENT = HUMAN_INTERFACE_DEVICE_EVT_TYPE_CLIENT_START,
    HID_EVT_TYPE_ULLHID_CLIENT = HID_EVT_TYPE_HID_CLIENT + PRF_EVENT_ID_SIZE,

    HUMAN_INTERFACE_DEVICE_EVT_TYPE_SERVER_START = PRF_EVTID_HUMAN_INTERFACE_DEVICE_START + PRF_EVENT_ID_SIZE*PRF_SERVER_OFFSET,
    HID_EVT_TYPE_HID_SERVER = HUMAN_INTERFACE_DEVICE_EVT_TYPE_CLIENT_START,
    HID_EVT_TYPE_ULLHID_SERVER = HID_EVT_TYPE_HID_SERVER + PRF_EVENT_ID_SIZE,

};

#include "hid/hid.h"
#include "ullhid/ullhid.h"

