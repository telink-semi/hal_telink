/********************************************************************************************************
 * @file    PAwR_sync.h
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
#ifndef STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_
#define STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_

#define     PAWR_AUX_SYNC_SUBEVENT_ENCRYPTION  0

#define     PAWR_LAST_SUBEVENT_FLAG            BIT(7)  //subevent range from 0x00 to 0x7F, so BIT(7) can be used to indicate other function.

#define     LTV_LEN_OFFSET                     0
#define     LTV_TYPE_OFFSET                    1

#define     RSP_SLOT_EARLY_TICK                (100*SYSTEM_TIMER_TICK_1US)


#define     PAWR_EVT_SUBEVT_STORE_OFFSET       248


enum{
    NOT_FIND_ESL_ID_FLAG = BIT(7),
};


enum{
    PDA_SYNC_SUBEVT_PROP_INCLUDE_TXPOWER = BIT(6),
};

enum{
    PAWR_CONN_RTN_SUCCESS = 0,
    PAWR_CONN_RTN_FAIL    = BIT(0),
    PAWR_CONN_RTN_ADDR_NO_MATCH  = BIT(1),
};

/**
 *  @brief  Command Parameters for "7.8.126 LE Set Periodic Advertising Response Data command"
 */
typedef struct {
    u16     sync_handle;
    u16     req_event_count; // indicate PAwR event count. i.e. receive packet in the PAwR event count. ---paEventCounter

    u8      req_subevt_idx;  // indicate subevent. i.e. receive packet in the subevent idx.
    u8      rsp_subevt_idx;  // the subevent that the response shall be sent in
    u8      rsp_slot_idx;    // the response slot in which this response data is to be transmitted. note slot need to be in the response subevent.
    u8      rsp_data_len;

    u8      rsp_max_dataLen;
    u8      rsp_real_len;
    u16     rsp_slot_duration_us;

    u8      user_setRspData_flag;
    u8      u8_rsvd[3];

    u32     aux_sync_subevt_ind_headerTick;

    u8*     pRsp_data;
} setPDARspData_cmdParam_t;


typedef struct {
    u8      sync_subevt_num;
    u8      sync_subevt_prop;
    u8      set_syncSubevt_flag;
    u8      rsvd1[1];

    u8      sync_subevt[128]; //max subevent number is 128
} setPDASyncSubevt_cmdParam_t;


typedef struct{
    u32 dma_len;

    u8  type        :4;
    u8  rfu1        :1;
    u8  chan_sel    :1;
    u8  txAddr      :1;
    u8  rxAddr      :1;
    u8  rf_len;
    u8  ext_hdr_len :6;
    u8  adv_mode    :2;
    u8  ext_hdr_flg; //need to notice: this flag may not exist. because the following is optional.
    //8 Byte above

    //8B(AdvA_6 + CTE_Info_1 + tx_power 1) is enough
    u8  data[8];
}ll_auxSyncSubevtRsp_header_t;  //must 4B aligned, now 16B


ble_sts_t   blc_ll_initPAwRsync_module(int num_pawr_sync);
ble_sts_t   blc_ll_initPAwRsync_rspDataBuffer(u8 *pdaRspData, int maxLen_pdaRspData);

ble_sts_t   blc_hci_le_setPeriodicSyncSubevent(u16 sync_handle, perd_adv_prop_t pda_prop, u8 num_subevent, u8* pSubevent);
ble_sts_t   blc_hci_le_setPAwRsync_rspData( u16 sync_handle, u16 req_pdaEvtCnt, u8 req_subEvtCnt,
                                            u8 rsp_subEvtCnt, u8 rsp_slotIdx,   u8 rspDataLen,  u8* pRspData);




//the following need to update to version 2 from v1.
// LE Periodic Advertising Report event[v2]
// LE Periodic Advertising Sync Established event[v2]
// LE Periodic Advertising Sync Transfer Received event[v2]
#endif
