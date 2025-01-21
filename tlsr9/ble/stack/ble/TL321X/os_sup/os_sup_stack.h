/********************************************************************************************************
 * @file    os_sup_stack.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2022
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
#ifndef OS_SUP_STACK_H_
#define OS_SUP_STACK_H_


extern os_give_sem_t blt_os_giveSem_cb;
extern os_give_sem_t blt_os_giveSemFromIrq_cb;

extern os_give_sem_t blt_os_semCountIncrement_cb;
extern os_give_sem_t blt_os_semCountIncrementIrq_cb;


/**
 * @brief srack use
 */
extern bool is_os_sup_en;
#define blt_isOsSupEnable() (is_os_sup_en)

#endif /* OS_SUP_STACK_H_ */
