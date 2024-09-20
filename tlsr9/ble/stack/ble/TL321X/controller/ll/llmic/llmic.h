/********************************************************************************************************
 * @file    llmic.h
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

#ifndef STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_H_
#define STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_H_


/*
 * brief:The current task priority level of the BLE module
 * */
typedef enum{
    BLE_SIGL_IDLE                 =0 ,//BLE task,
    BLE_SIGL_NORMAL                  ,
    BLE_SIGL_TSYNC                   ,//ACL Peripheral connect or update, need highest priority

}llmic_ble_sigl_e;


/*
 * brief:LLMIC module controls BLE module
 * */
typedef enum{
    LLMIC_SIGL_NONE               = 0,
    LLMIC_SIGL_COMPROMISE            ,//LLMIC compromise
    LLMIC_SIGL_REJECT                ,//LLMIC reject. maybe BLE inform too late, llmic can not process timely
}llmic_mic_sigl_e;


/*
 * brief: In the LLMIC function module, define the BLE controller scheduling parameters
 * */
_attribute_aligned_(4)
typedef struct {
    u8 ble_new_notify; // 1 with notification, 0 with no notification
    u8 ble_signal;     // For the parameter type, see llmic_ble_sigl_e
    u8 llmic_signal;   //  For the parameter type, see llmic_mic_sigl_e
    u8 u8_rsvd;        // unused

    u32    stick_task_begin;
    u32    stick_task_end;

}signal_fifo_t;


/*
 * brief: Used in blc_ll_registerTelinkControllerFinishCallback
 * */
typedef void (*blc_ll_llmic_callback_t)(void);


/**
 * @brief      This function is used to notify BLE stack LLMIC enable or disable
 *             attention: if enable, notify to BLE first, then start LLMIC task, leave at least 2mS between them.
 * @param[in]  enable - enable or disable
 * @return     none
 */
void blc_ll_set_llmic_enable(int enable);


/**
 * @brief      This function is used to obtain the protocol stack parameters
 * @param[in]  signal_fifo_t
 * @return     none
 */
signal_fifo_t * blc_ll_get_llmic_param(void);


/**
 * @brief      This function is used to set the function to be called when the task ends.
 * @param[in]  blc_ll_llmic_callback_t  (The registered function **must** be ram code)
 * @return     none
 */
void blc_ll_registerTelinkControllerFinishCallback ( blc_ll_llmic_callback_t  p);


void blc_llmic_switch_2M(void);
#endif /* STACK_BLE_CONTROLLER_LL_LLMIC_LLMIC_H_ */
