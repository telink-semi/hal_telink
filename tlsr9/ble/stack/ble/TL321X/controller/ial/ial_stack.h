/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef IAL_STACK_H_
#define IAL_STACK_H_

#if (LL_FEATURE_ENABLE_ISO)

#include "stack/ble/controller/ll/iso/iso.h"
#include "stack/ble/controller/ll/iso/iso_stack.h"
#include "stack/ble/controller/ll/iso/bis_stack.h"
#include "stack/ble/controller/ll/iso/cis_stack.h"


/******************************* Macro & Enumeration & Structure Definition for Stack Begin, user can not use!!!!  *****/
#define         SPILT_SDU2PDU_PRE_PROCESS_US        2000


typedef enum{
    UNFRAMED_START,
    UNFRAMED_CONTINUE,
    UNFRAMED_END,
    UNFRAMED_COMPLETE,
    UNFRAMED_INVALID,
}ll_iso_unframe_type_t;


typedef enum{
    SDU_STATE_NEW,
    SDU_STATE_CONTINUE,
}ll_iso_sdu_unframe_state_t;

typedef struct{
    u8 sc   :1;
    u8 cmplt:1;
    u8 RFU  :6;

    u8 length;

    u32 time_offset:24;
}iso_framed_segmHdr_t;



/************************
 * sdu buff manage
 */
typedef struct{
    u8 *in_fifo_b;
    u8 *out_fifo_b;

    u8 in_fifo_num;
    u8 in_fifo_mask;
    u8 out_fifo_num;
    u8 out_fifo_mask;

    u16 max_in_fifo_size;
    u16 max_out_fifo_size;

}iso_sdu_mng_t;

extern iso_sdu_mng_t sdu_mng;




extern iso_sdu_mng_t sduCisMng;
extern iso_sdu_mng_t sduBisMng;




int     blt_ial_interrupt_task (int flag);



int     blc_hci_iso_data_loop(void);


int     blt_ial_bisBroadcast_mainloop(ll_bis_t *pBis);
int     blt_ial_bisSync_mainloop(void);
int     blt_ial_isochronous_testMode_mainloop(void);


ble_sts_t   blt_ial_reassembleCisPdu2Sdu(ll_cis_conn_t*, iso_rx_evt_t*);

ble_sts_t blt_ial_bisSync_reassemblePdu2Sdu(ll_bis_t *pBis, bis_rx_pdu_t *pBisPdu);


ble_sts_t blt_bis_splitSdu2FramedPdu(u16 bisHandle, u8*pNumOfCmpPkt);




/**
 * @brief      This function is used to segmentation SDU to one Framed PDUs.
 * @param[in]  cis_connHandle
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t blt_cis_splitSdu2FramedPdu(ll_cis_conn_t *,  u8*);


/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  cis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t blt_cis_splitSdu2UnframedPdu(ll_cis_conn_t *, sdu_packet_t *, u8*);




/**
 * @brief      This function is used to fragmentation SDU to one or more Unframed PDUs.
 * @param[in]  bis_connHandle
 * @param[in]  sdu  point to sdu buff
 * @return      Status - 0x00: command succeeded; IAL_ERR_SDU_LEN_EXCEED_SDU_MAX
 *                       LL_ERR_INVALID_PARAMETER: command failed
 */
ble_sts_t   blt_bis_splitSdu2UnframedPdu(u16 bis_connHandle, sdu_packet_t *sdu, u8 *pNumOfCmpPkt);

#endif


#endif
