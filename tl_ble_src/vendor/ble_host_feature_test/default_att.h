/********************************************************************************************************
 * @file    svc_spp.h
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
#if(1)
enum
{
    SPP_SVC_HDL = 0x8000,
    SPP_SER2CLI_CD_HDL,
    SPP_SER2CLI_DP_HDL,
    SPP_SER2CLI_CCC_HDL,
    SPP_CLI2SER_CD_HDL,
    SPP_CLI2SER_DP_HDL,
    SPP_MAX_HDL,
};

#define SPP_START_HDL                               0x8000
#define SPP_END_HDL                                 (SPP_MAX_HDL - 1)



void blc_svc_addSppGroup(void);

#endif
