/********************************************************************************************************
 * @file    ullhid_server_buf.h
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

struct blc_ullhid_server
{
    u16 propertiesHdl; //ULL HID properties attribute handle
    u16 operationHdl;  //LE HID operation mode attribute handle
    u8  mode;          // default mode or hybrid mode, if disconnect, profile will set default mode.
};

struct blc_ullhid_server_ctrl
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_proc_t           process;
#else
    struct blc_prf_process   process;
#endif
    struct blc_ullhid_server ullhidServer;
};
