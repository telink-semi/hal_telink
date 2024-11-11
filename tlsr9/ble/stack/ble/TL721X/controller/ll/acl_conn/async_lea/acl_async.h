/********************************************************************************************************
 * @file    acl_async.h
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

#ifndef STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_H_
#define STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_H_
#include "vendor/common/user_config.h"
#if (LL_ASYNC_LEA_EN)
#ifndef ASYNC_TX_FIFO_SIZE
#define ASYNC_TX_FIFO_SIZE                      24
#endif


#ifndef ASYNC_TX_FIFO_NUM
#define ASYNC_TX_FIFO_NUM                       4
#endif

#ifndef ASYNC_RX_FIFO_SIZE
#define ASYNC_RX_FIFO_SIZE                      24
#endif


#ifndef ASYNC_RX_FIFO_NUM
#define ASYNC_RX_FIFO_NUM                       4
#endif

typedef enum{
    TYPE_ASYNC,//asynchronous
    TYPE_SYNC, //synchronous
}blc_async_type_e;

typedef enum{
    OPCODE_LED,
    OPCODE_TONE,
    OPCODE_KEY,
    OPCODE_SWITCH,
}blc_async_opcode_e;


//async error code
typedef enum
{
    BLC_ASYNC_SUCCESS,
    BLC_ASYNC_PARAMETER_ERROR,
    BLC_ASYNC_RESOURCE_INSUFFICIENT,
    BLC_ASYNC_ERROR_LINK,
    BLC_ASYNC_ERROR_DATA,
    BLC_ASYNC_ERROR_STATE,
}blc_async_sts_e;

typedef struct{
    u8  type;//blc_async_type_e
    u8  opcode;//blc_async_opcode_e
    u8  data[10];
}blc_async_message_t;

typedef struct{
    u16 eventCounter;
    u16 ackIndex;
    u32 syncTick;//us
    blc_async_message_t message;
}blc_async_sdu_t;

/**
 *  @brief  Definition for Async_lea protocol PDUs
 */
typedef enum{
    ASYNC_LEA_OP_UI_MSG     = 0x55,
    ASYNC_LEA_OP_TIMING     = 0x56,
    ASYNC_LEA_OP_GET_TIMING = 0x57,
}async_pdu_type;

typedef struct __attribute__((packed)) {
    // if async connection is valid
    uint08  leaUsed;
    uint08  connState;
    uint16  connHandle;

    uint32  cmdTick;

    uint08  flow;
    uint08  cmdInstant;
    uint08  updateIndex;
    uint08  rsvd;

    uint32  connInstant;
    //when async established,offset decide the author point of async connection.
    uint32  windowStartTick;
    uint16  windowSizeOffsetBslot;
    uint16  windowOffsetBslot;
    //async acl interval
    uint32  aclIntervalTick;
    //iso interval with cis master,aysnc share
    uint32  isoIntervalUs;
    //async share author point,used to calculate the async offset
    uint32  asyncApTick;

    uint32  gapPosTick;

    //own  information with cis master
    uint32  ownAclApTick;
    uint32  ownCisAPTick;
    uint32  ownCisSyncDelayTick;
    //peer information with cis master
    uint32  peerAclApTick;
    uint32  peerCisAPTick;
    uint32  peerCisSyncDelayTick;
} async_ctrl_t;

typedef int (*async_data_cb_t) (u32 syncTick,blc_async_message_t *pMessage);

extern  async_ctrl_t asyncCtrl;
extern  async_data_cb_t  async_tx_cb;
extern  async_data_cb_t  async_rx_cb;



/**
 *  @brief  MACRO: check if the connection is a async_lea link
 */
#define IS_ASYNC_LEA_LINK(connHandle)       (asyncCtrl.leaUsed && ((connHandle) & BIT(4)))


/**
 * @brief       This function serves to register tx and rx function of async module.
 * @param[in]   handler  -
 * @return      none.
 */
void blc_async_registerDataHandler(async_data_cb_t tx_cb,async_data_cb_t rx_cb);

/**
 * @brief       This function serves to send synchronous message to connected device.
 * @param[in]   pMessage - sync data,search 'blc_async_message_t' for detail
 * @return      search 'blc_async_sts_e' for detail.
 */
blc_async_sts_e blc_async_push_message(u32 syncTime,blc_async_message_t *pMessage);


/**
 * @brief       This function serves to process the loop function of aysnc module.
 * @param[in]   handler  -
 * @return      none.
 */
void blc_async_loopProcess(void);




#endif

#endif /* STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_H_ */
