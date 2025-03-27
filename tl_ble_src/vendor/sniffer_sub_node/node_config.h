/********************************************************************************************************
 * @file    node_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2024.01
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions
 *              in binary form must reproduce the above copyright notice, this list of
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *
 *              3. Neither the name of TELINK, nor the names of its contributors may be
 *              used to endorse or promote products derived from this software without
 *              specific prior written permission.
 *
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or
 *              relating to such deletion(s), modification(s) or alteration(s).
 *
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************************************/
#ifndef SUB_NODE_CONFIG_H_
#define SUB_NODE_CONFIG_H_

/** Sniffer sub node: monitor LE central devices
 *  Transport use CANFD or UART. */
#define MONITOR_CENTRAL                                 1
/** Sniffer sub node: monitor LE central and peripheral devices
 *  Transport use CANFD or UART. */
#define MONITOR_CENTRAL_PERIPHERAL                      2
/** Sniffer sub node: monitor LE advertising
 *  Transport use CANFD or UART. */
#define OBSERVER                                        3

/** Sniffer sub node select */
#define MONITOR_ROLE_SELECT                             MONITOR_CENTRAL_PERIPHERAL






#if (MONITOR_ROLE_SELECT == MONITOR_CENTRAL)
    #include "monitor_central/app_config.h"
#elif (MONITOR_ROLE_SELECT == MONITOR_CENTRAL_PERIPHERAL)
    #include "monitor_central_peripheral/app_config.h"
#elif (MONITOR_ROLE_SELECT == OBSERVER)
    #include "observer/app_config.h"
#else
    #error "monitor role select error!!!"
#endif








/** Sniffer Feature Configuration */
#define LL_RSSI_SNIFFER_SLAVE_ENABLE                1
#define LL_RSSI_SNIFFER_MASTER_ENABLE               1
#define DRV_RSSI_SNIFFER_MODE_ENABLE                1
#define SCAN_EN_MORE_STRATEGY                       1

/* Attention: */
#define APP_EXCEPTION_STUCK_EN                      0 //It can be used for debugging during the development stage and must be turned off for mass production
#if (APP_EXCEPTION_STUCK_EN != 0)
    #error "APP_EXCEPTION_STUCK_EN must be turned off for mass production"
#endif


#define REMOTE_DEVICE_MAX_NUM                       8
#define REMOTE_DEVICE_MAX_MASK                      0x07    // Note: 8(0~7) => 0x07
#if (REMOTE_DEVICE_MAX_NUM != 8)
    #error "REMOTE_DEVICE_MAX_NUM can not be changed"
#endif
#if (REMOTE_DEVICE_MAX_MASK != 0x07)
    #error "REMOTE_DEVICE_MAX_MASK can not be changed"
#endif

#endif /* SUB_NODE_CONFIG_H_ */
