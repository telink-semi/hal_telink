/********************************************************************************************************
 * @file    cs.h
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
#ifndef CS_SNIFFER_H_
#define CS_SNIFFER_H_

#include "tl_common.h"
#include "stack/ble/controller/ble_controller.h"


#define CS_COUNTER_CONVERT_SUB_NODE_INDEX_INVALID   0xFF // main node index

/**
 * @brief   update cs sniffer parameter result
 */
enum
{
    //0x00
    CS_SNIFFER_PARAMETER_UPDATE = 0,                 //
    CS_SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD, //
    CS_SNIFFER_PARAMETER_CHECKSUM_ERR,               //
    CS_SNIFFER_PARAMETER_INVALID,                    //

    //0x04
    CS_SNIFFER_UNKNOWN_SNIFHANDLE,      //
    CS_SNIFFER_UNSUPPORTED_FEATURE,     //
    CS_SNIFFER_PARAMETER_STATUS_FAILED, //
};

/**
 * @brief      for user to initialize CS sniffer for main node.
 * @param[in]  totalNodeNum: main node number and all sub node number
 * @return     none
 */
void blc_ll_initCsSnifferMainNode_module(u8 totalNodeNum);

/**
 * @brief      for user to initialize CS sniffer for sub node.
 * @param[in]  currentNodeIdx: SubNode curNodeIdx must be greater than 0 and less than 7
 * @return     none
 */
void blc_ll_initCsSnifferSubNode_module(u8 currentNodeIdx);

/**
 * @brief      CS get subNode index by csCounter.
 * @param[in]  csCounter   - CS rangingCounter or procedureCounter
 * @return     subNode index, 0xFF when is mainNode
 */
u8 blc_sniffer_getSubNodeIndexByCsCounter(u16 csCounter);

#endif /* CS_SNIFFER_H_ */
