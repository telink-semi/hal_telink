/********************************************************************************************************
 * @file    app_att.h
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
#ifndef BLM_ATT_H_
#define BLM_ATT_H_

#include "tl_common.h"

extern attribute_t* g_MyAttributes;

///////////////////////////////////// peripheral-role ATT service HANDLER define ///////////////////////////////////////
typedef enum
{
    ATT_H_START = 0,


    //// Gap ////
    /**********************************************************************************************/
    GenericAccess_PS_H,            //UUID: 2800,   VALUE: uuid 1800
    GenericAccess_DeviceName_CD_H, //UUID: 2803,   VALUE:              Prop: Read | Notify
    GenericAccess_DeviceName_DP_H, //UUID: 2A00,   VALUE: device name
    GenericAccess_Appearance_CD_H, //UUID: 2803,   VALUE:              Prop: Read
    GenericAccess_Appearance_DP_H, //UUID: 2A01,   VALUE: appearance
    CONN_PARAM_CD_H,               //UUID: 2803,   VALUE:              Prop: Read
    CONN_PARAM_DP_H,               //UUID: 2A04,   VALUE: connParameter


    //// gatt ////
    /**********************************************************************************************/
    GenericAttribute_PS_H,                 //UUID: 2800,   VALUE: uuid 1801
    GenericAttribute_ServiceChanged_CD_H,  //UUID: 2803,   VALUE:              Prop: Indicate
    GenericAttribute_ServiceChanged_DP_H,  //UUID: 2A05,   VALUE: service change
    GenericAttribute_ServiceChanged_CCB_H, //UUID: 2902,   VALUE: serviceChangeCCC



    ATT_END_H,

} ATT_HANDLE;



typedef struct __attribute__((packed))
{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u8  value;
} app_ble_rf_packet_att_write_t;

typedef struct __attribute__((packed))
{
    u8  type;
    u8  rf_len;
    u16 l2capLen;
    u16 chanId;
    u8  opcode;
    u16 handle;
    u16 offset;
} app_rf_packet_att_readBlob_t;
/**
 * @brief   GATT initialization.
 *          !!!Note: this function is used to register ATT table to BLE Stack.
 * @param   none.
 * @return  none.
 */
void my_gatt_init(void);


#endif /* BLM_ATT_H_ */
