/********************************************************************************************************
 * @file    past_stack.h
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
#ifndef PAST_STACK_H_
#define PAST_STACK_H_


#include "stack/ble/ble_stack.h"

typedef struct{
    u8          llid;
    u8          rf_len;
    u8          opcode;
    u16         id;
    sync_info_t syncInfo;
    u16         connEvtCnt;
    u16         lastPaEvtCnt;
    u8          sid  : 4;
    u8          aType: 1;
    u8          sca  : 3;
    u8          phy;
    u8          advA[6];
    u16         syncConnEvtCnt;
}rf_pkt_ll_periodic_sync_ind_t; //LL_PERIODIC_SYNC_IND


/////////////////// PAwR ///////////////////////
typedef struct{
    rf_pkt_ll_periodic_sync_ind_t pastInd;
    pawr_acad_t pawrTimingInfo;
}rf_pkt_ll_periodic_sync_wr_ind_t; //LL_PERIODIC_SYNC_WR_IND

typedef struct{
    u8          llid;
    u8          rf_len;
    u8          opcode;
    u16         id;
    sync_info_t syncInfo; //18B
    u16         connEvtCnt;
    u16         lastPaEvtCnt;
    u8          sid  : 4;
    u8          aType: 1;
    u8          sca  : 3;
    u8          phy;
    u8          advA[6];
    u16         syncConnEvtCnt;
    ////above is 36B and align 4B

    pawr_acad_t pawrAcadInfo;

}rf_pkt_ll_periodicSyncWrInd_t;
#endif /* PAST_STACK_H_ */
