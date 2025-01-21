/********************************************************************************************************
 * @file    cs_sniffer_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef CS_SNIFFER_STACK_H_
#define CS_SNIFFER_STACK_H_

#if (LL_CS_SNIFFER_MODE_ENABLE)


_attribute_aligned_(4) typedef struct __attribute__((packed))
{
    u8  event_code;
    u8  status;
    u16 snifHandle;
    u8  event_data[1];      //non-fixed length
} cs_sniffer_event_param_t; //cs sniffer trigger parameters

_attribute_aligned_(4) typedef struct __attribute__((packed))
{
    u8 totalNodeNum;  //main node number and all sub node number, range from 1 to 7
    u8 curNodeIdx;    //MainNode curNodeIdx must be fixed to 0, SubNode curNodeIdx must be from 1 to 6
    u8 curProcedureCountIdx;
    u8 rsvd8;
} cs_sniffer_param_t; //cs sniffer common parameters

extern _attribute_aligned_(4) volatile cs_sniffer_param_t csSniffer_param;

#endif /* end of LL_CS_SNIFFER_MODE_ENABLE */

#endif /* CS_SNIFFER_STACK_H_ */
