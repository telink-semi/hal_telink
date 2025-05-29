/********************************************************************************************************
 * @file    vcd_def_extended.h
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

#ifndef VCD_DEF_EXTENDED_H_
#define VCD_DEF_EXTENDED_H_


#include "debug_cfg.h"

#if (VCD_DEFINE_SELECT == VCD_DEFINE_EXTENDED)


    //log_event, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
    #define SLEV_timestamp            0 // SLEV 0, reserved, do not change it
    #define SLEV_irq_rx               1
    #define SLEV_irq_tx               2
    #define SLEV_irq_rfdone           3
    #define SLEV_irq_stimer           4

    #define SLEV_sche_rebuild         5
    #define SLEV_sche_slotRst         6

    #define SLEV_PAWR_SYNC_DEBUG0     7
    #define SLEV_PAWR_SYNC_DEBUG1     8
    #define SLEV_PAWR_SYNC_DEBUG2     9
    #define SLEV_PAWR_SYNC_DEBUG3     10
    #define SLEV_PAWR_SYNC_DEBUG4     13
    #define SLEV_PAWR_SYNC_DEBUG5     14


    #define SLEV_PAWR_SYNC_RX         11
    #define SLEV_PAWR_SYNC_TASK_BUILD 12

    #define SLEV_eadv_ext_build       25
    #define SLEV_eadv_aux_build       26
    #define SLEV_eadv_aux_insert      27
    #define SLEV_pda_adv_build        28
    #define SLEV_bigBcst_build        29

    #define SLEV_bigScan_build        15
    #define SLEV_pdaScan_build        16
    #define SLEV_auxscanFutrTsk_add   17


    #define SLEV_primary_rx_extAdv    18
    #define SLEV_primary_rx_legAdv    19
    #define SLEV_second_rx_adv        20
    #define SLEV_bsync_rev            21


    //log_tick, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
    #define SLET_timestamp 0 // SLET 0, reserved, do not change it


    //log_task, ID: 0~31, id0 is reserved,  1 ~ 31 is available
    // 1-bit data:
    #define SL01_rsvd                  0
    #define SL01_IRQ                   1


    #define SL01_scn_prichn            4
    #define SL01_pdachn_scn            5
    #define SL01_scn_secchn            6
    #define SL01_big_sync              7
    #define SL01_bsync0                8

    #define SL01_PAWR_SYNC_build       9


    #define SL01_eadv_ext_ind          11 //primary channel
    #define SL01_eadv_auxOrChain_ind   12
    #define SL01_prd_adv               13

    #define SL01_big_bcst              20
    #define SL01_bis_bcst              21


    #define SL01_ext_scan_endis        22
    #define SL01_ext_adv_endis         23
    #define SL01_pda_adv_endis         24


    #define SL01_PADVB_ACAD_PAWR       25
    #define SL01_PAWR_SYNC_SUB         26


    #define SL01_PAWR_SYNC_RSP_SLOT_TX 28


    // 8-bit data: cid0 - cid63
    #define SL08_rsvd             0
    #define SL08_pdaSync_conflict 1
    #define SL08_bisSync_conflict 2
    #define SL08_bisBcst_conflict 3
    #define SL08_auxAdv_conflict  4
    #define SL08_pdaAdv_conflict  5

    #define SL08_PAWR_SUBEVT_IDX  6

    #define SL08_PDA_JUMP_CNT     7
    #define SL08_SUBEVT_JUMP_CNT  8

    #define SL08_SUB_PDA_JUMP_CNT 9


    // 16-bit data: sid0 - sid63
    #define SL16_rsvd         0
    #define SL16_PAWR_EVT_CNT 1
    #define SL16_PAwR_chnIdx  2
    #define SL16_PDA_EVT_CNT  3


#endif //end of "VCD_DEFINE SELECT"

#endif /* VCD_DEF_H_ */
