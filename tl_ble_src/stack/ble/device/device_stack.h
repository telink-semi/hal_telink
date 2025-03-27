/********************************************************************************************************
 * @file    device_stack.h
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
#ifndef STACK_BLE_DEVICE_DEVICE_STACK_H_
#define STACK_BLE_DEVICE_DEVICE_STACK_H_

#include "stack/ble/ble_config.h"

#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    #ifndef     LOCAL_DEVICE_NUM_MAX
    #define     LOCAL_DEVICE_NUM_MAX                            4
    #endif
#else
    #define     LOCAL_DEVICE_NUM_MAX                            1
#endif


typedef struct {
    u8 set;
    u8 type;
    u8 address[BLE_ADDR_LEN];
}dev_addr_t;

typedef struct{
    u8  mldev_en;
    u8  cur_dev_idx;  //current use index
    u8  rsvd1;
    u8  rsvd2;

    dev_addr_t  dev_mac[LOCAL_DEVICE_NUM_MAX];
}loc_dev_mng_t; //local device manage
extern loc_dev_mng_t    mlDevMng;


#endif /* STACK_BLE_DEVICE_DEVICE_STACK_H_ */
