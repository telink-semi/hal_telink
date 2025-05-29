/********************************************************************************************************
 * @file    mac.h
 *
 * @brief   This is the header file for 2.4G SDK
 *
 * @author  2.4G Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef _MAC_H_
#define _MAC_H_

typedef void (*MAC_Cb)(unsigned char *Data);

extern void MAC_Init(const unsigned short Channel,
                     const MAC_Cb         RxCb,
                     const MAC_Cb         TxCb,
                     const MAC_Cb         RxTimeoutCb,
                     const MAC_Cb         RxFirstTimeoutCb,
                     unsigned int         acc);

extern void MAC_SendData(const unsigned char *Payload,
                         const int            PayloadLen);

void MAC_BroadcastData(const unsigned char *Payload,
                       const unsigned char  PayloadLen);

extern void MAC_RecvData(unsigned int TimeUs);
extern void MAC_RxIrqHandler(void);
extern void MAC_RxIrqHandler_Batch(void);
extern void MAC_RxTimeOutHandler(void);
extern void MAC_RxFirstTimeOutHandler(void);
void        MAC_TxIrqHandler(void);

#endif /* _MAC_H_ */
