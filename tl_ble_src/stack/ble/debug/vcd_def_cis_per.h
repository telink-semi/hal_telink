/********************************************************************************************************
 * @file    vcd_def_cis_per.h
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
#ifndef VCD_DEF_CIS_PER_H_
#define VCD_DEF_CIS_PER_H_


#include "debug_cfg.h"

#if (VCD_DEFINE_SELECT == VCD_DEFINE_CIS_PER)


//log_event, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
#define         SLEV_timestamp              0  // SLEV 0, reserved, do not change it
#define         SLEV_irq_rx                 1
#define         SLEV_irq_tx                 2
#define         SLEV_irq_rfdone             3
#define         SLEV_irq_stimer             4

#define         SLEV_sche_rebuild           7
#define         SLEV_sche_slotRst           8


#define         SLEV_cis_rx                 15
#define         SLEV_cis_rx_rfLen           16

#define         SLEV_cis_tx_rfLen           17

#define         SLEV_cis_jump               18
#define         SLEV_cis0_sdu_cmplt         19
#define         SLEV_cis1_sdu_cmplt         20
#define         SLEV_cis0_rx_mic_fail       21
#define         SLEV_cis1_rx_mic_fail       22
#define         SLEV_cis_tx_padding         23


#define         SLEV_app0                   25
#define         SLEV_app1                   26
#define         SLEV_app2                   27
#define         SLEV_app3                   28
#define         SLEV_app4                   29
#define         SLEV_app5                   30

//log_tick, ID: 0~31, 0 is reserved for timeStamp, 1 ~ 31 is available
#define         SLET_timestamp              0   // SLET 0, reserved, do not change it


#define         SLET_app0                   20
#define         SLET_app1                   21
#define         SLET_app2                   22
#define         SLET_app3                   23
#define         SLET_app4                   24

//log_task, ID: 0~31, id0 is reserved,  1 ~ 31 is available
// 1-bit data:
#define         SL01_rsvd                   0
#define         SL01_IRQ                    1


#define         SL01_leg_adv                3

#define         SL01_eadv_ext_ind               4
#define         SL01_eadv_auxOrChain_ind        5
#define         SL01_eadv_aux_sync              6
#define         SL01_eadv_aux_chain             7

#define         SL01_aclp_0                 12
#define         SL01_aclp_1                 13

#define         SL01_cis_group              15
#define         SL01_cis0                   16
#define         SL01_cis1                   17

#define         SL01_app0                   20
#define         SL01_app1                   21
#define         SL01_app2                   22
#define         SL01_app3                   23
#define         SL01_app4                   24
#define         SL01_app5                   25
#define         SL01_app6                   26
#define         SL01_app7                   27
#define         SL01_app8                   28

#define         SL01_dbug0                  30
#define         SL01_dbug1                  31






// 8-bit data: cid0 - cid63
#define         SL08_rsvd                   0
#define         SL08_cis_snnesn             1


// 16-bit data: sid0 - sid63
#define         SL16_reserved               0
#define         SL16_cis_tx_header          1

#define         SL16_cis0_evtcnt            4
#define         SL16_cis1_evtcnt            5

#define         SL16_cis0_rxPldNum          8
#define         SL16_cis1_rxPldNum          9

#define         SL16_cis0_rxProPdu          12
#define         SL16_cis1_rxProPdu          13

#define         SL16_cis_rxPdu2Sdu_st       16

#define         SL16_cis0_txSetPldNum       17
#define         SL16_cis1_txSetPldNum       18

#define         SL16_cis0_txCurSendPldNum   30
#define         SL16_cis1_txCurSendPldNum   31

#define         SL16_cis0_txPrePldNum       34
#define         SL16_cis1_txPrePldNum       35

#define         SL16_cis0_txSdu_len         36
#define         SL16_cis1_txSdu_len         37

#define         SL16_cis0_rxSdu_len         42
#define         SL16_cis1_rxSdu_len         43

#define         SL16_eqb_testcase_seqNum    44

#define         SL16_dbug0                  62
#define         SL16_dbug1                  63



#endif //end of "VCD_DEFINE SELECT"


#endif /* VCD_DEF_CIS_PER_H_ */
