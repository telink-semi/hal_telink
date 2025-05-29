/********************************************************************************************************
 * @file    debug_cfg.h
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
#ifndef STACK_BLE_DEBUG_DEBUG_CFG_H_
#define STACK_BLE_DEBUG_DEBUG_CFG_H_


/*****************************************************************************************************************
                                                BLMS_ERR_DEBUG
*****************************************************************************************************************/

#define ERR_IGNORE             0
#define ERR_TRIGGER_CODE_STUCK 1
#define ERR_LOG_ON_SRAM        2


#ifndef BLT_ERR_PROCESS
    #define BLT_ERR_PROCESS ERR_LOG_ON_SRAM
#endif


#if (BLT_ERR_PROCESS == ERR_TRIGGER_CODE_STUCK || BLT_ERR_PROCESS == ERR_LOG_ON_SRAM)
void blt_ll_error_debug(u32 x);
    #define BLMS_ERR_DEBUG(en, x)  \
        if (en) {                  \
            blt_ll_error_debug(x); \
        }
#else
    #define BLMS_ERR_DEBUG(en, x)
#endif

#define BLMS_DEBUG_EN 1


extern u32 stkLog_mask;


/*****************************************************************************************************************
                                                debug
*****************************************************************************************************************/

#define DBG_SCHE_CAL_TIME_EN  0


#define ACL_MASTER_SCHE_DEBUG 0

#ifndef ACL_MASTER_INITIATE
    #define ACL_MASTER_INITIATE 0
#endif

#define SCHE_TIMING_IMPROVE_DBG_EN 0


#define DBG_SLAVE_CONN_UPDATE      0
#define DBG_MASTER_CONN_UPDATE     0
#define DBG_CONN_UPDATE            (DBG_SLAVE_CONN_UPDATE || DBG_MASTER_CONN_UPDATE)

#define DBG_HCI_TR                 0
#define DBG_HCI_FIFO               0

#define DBG_PM_LOGIC               0
#define DBG_PM_TIMING              0


//Extended ADV debug
#define DBG_EXTADV_LOGIC  0
#define DBG_EXTADV_TIMING 0
#define DBG_EXTADV_BUFFER 0

//Extended Scan debug
#define DBG_EXTSCAN_LOGIC    0
#define DBG_EXTSCAN_TIMING   0

#define DBG_AUXSCAN_LOGIC_QW 0

//periodic ADV  ebug
#define DBG_PRDADV_LOGIC  0
#define DBG_PRDADV_TIMING 0

//periodic ADV Sync debug
#define DBG_PDA_SYNC_LOGIC  0
#define DBG_PDA_SYNC_TIMING 0

//aoa_aod debug
#ifndef DBG_AOA_AOD_LOGIC
    #define DBG_AOA_AOD_LOGIC 0
#endif


#define DBG_BISNC_RX_PDU    0
#define DBG_BIS_SYNC_TIMING 0
#define DBG_BIS_BCST_LOGIC  0


#define CIS_DEBUG_EN        0 //dump log are in mainloop code 20230421
#define DBG_CIS_LOGIC       0 //no dump log

#define DBG_CIS_TIMING      0 //no dump log

#ifndef DBG_CIS_IRQ_TIMING_DUMPLOG
    #define DBG_CIS_IRQ_TIMING_DUMPLOG 0
#endif

#define DBG_CIS_SLAVE_TIMING 0

#define DBG_CIS_MASTER_LOGIC 0 //dump log all in mainloop code 20230421


#ifndef DBG_CIS_MASTER_IRQ_LOGIC_DUMPLOG
    #define DBG_CIS_MASTER_IRQ_LOGIC_DUMPLOG 0
#endif

#define DBG_CIS_MASTER_TIMING 0 //dump log all in mainloop code 20230421

#ifndef DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG
    #define DBG_CIS_MASTER_IRQ_TIMING_DUMPLOG 0
#endif

#define DBG_IAL_LOGIC           0

#define DBG_PCL_LOGIC           0

#define DBG_PAwR_ADV_LOGIC      0
#define DBG_PAwR_ADV_TIMING     0

#define DBG_PAwR_SYNC_LOGIC     0
#define DBG_PAwR_SYNC_TIMING    0

#define DBG_CS_LOGIC_EN         0
#define DBG_CS_INITIATOR_TIMING 0
#define DBG_CS_REFLECTOR_TIMING 0
#define DBG_CS_FLOW_EN          0
#ifndef DBG_CS_DATA_EN
    #define DBG_CS_DATA_EN 0
#endif
#ifndef DBG_CS_STEP_DATA_EN
    #define DBG_CS_STEP_DATA_EN 0
#endif

#define DBG_CS_SCH_INIT 0
#define DBG_CS_SCH_REFL 0


/*****************************************************************************************************************

                                                Dump message

*****************************************************************************************************************/
#ifndef UPPER_TESTER_DBG_EN
    #define UPPER_TESTER_DBG_EN 0
#endif

#ifndef UPPER_TESTER_HCI_LOG_EN
    #define UPPER_TESTER_HCI_LOG_EN 0
#endif


#ifndef STACK_DUMP_EN
    #define STACK_DUMP_EN 0
#endif

#ifndef IUT_HCI_LOG_EN
    #define IUT_HCI_LOG_EN 0
#endif

#ifndef HOST_HCI_ERR_LOG_EN
    #define HOST_HCI_ERR_LOG_EN 0
#endif

#ifndef BLC_LL_LOG_EN
    #define BLC_LL_LOG_EN 0
#endif


#ifndef CIS_FLOW_LOG_EN
    #define CIS_FLOW_LOG_EN 0
#endif

#ifndef LL_CTRL_LOG_EN
    #define LL_CTRL_LOG_EN 0
#endif

#ifndef DBG_DECRYPTION_ERR_EN
    #define DBG_DECRYPTION_ERR_EN 0
#endif

/* BLE smp trans.. log enable */
#ifndef SMP_DBG_EN
    #define SMP_DBG_EN 0
#endif

#ifndef TX_PUSH_DATA_LOG
    #define TX_PUSH_DATA_LOG 0
#endif

#ifndef RX_L2CAP_DATA_LOG
    #define RX_L2CAP_DATA_LOG 0
#endif

#ifndef DBG_HOST_LOG
    #define DBG_HOST_LOG 0
#endif

#ifndef DBG_L2CAP_BTSNOOP_LOG
    #define DBG_L2CAP_BTSNOOP_LOG 0
#endif

#ifndef DBG_GATTC_LOG
    #define DBG_GATTC_LOG 0
#endif

#ifndef DBG_GATTS_LOG
    #define DBG_GATTS_LOG 0
#endif

#ifndef DBG_GAPC_LOG
    #define DBG_GAPC_LOG 0
#endif

#ifndef DBG_BOUNDARY_RX
    #define DBG_BOUNDARY_RX 0
#endif

#ifndef DBG_LL_CTRL_LOG_EN
    #define DBG_LL_CTRL_LOG_EN 0
#endif


#ifndef DBG_IAL_EN
    #define DBG_IAL_EN 0
#endif


#ifndef DBG_PRVC_RL_EN
    #define DBG_PRVC_RL_EN 0
#endif

#ifndef DBG_PRVC_LEGADV_EN
    #define DBG_PRVC_LEGADV_EN 0
#endif

#ifndef DBG_PRVC_LEGSCAN_EN
    #define DBG_PRVC_LEGSCAN_EN 0
#endif

#ifndef DBG_PRVC_INIT_EN
    #define DBG_PRVC_INIT_EN 0
#endif

#ifndef DBG_PRVC_CONN_EN
    #define DBG_PRVC_CONN_EN 0
#endif

#ifndef DBG_PRVC_EXTADV_EN
    #define DBG_PRVC_EXTADV_EN 0
#endif

#ifndef DBG_PRVC_EXTSCAN_EN
    #define DBG_PRVC_EXTSCAN_EN 0
#endif

#ifndef DBG_EXTSCAN_REPORT
    #define DBG_EXTSCAN_REPORT 0
#endif

#ifndef DBG_EXTSCAN_ERR_PKT_EN
    #define DBG_EXTSCAN_ERR_PKT_EN 0
#endif

#ifndef DBG_SCAN_MON_ADV_EN
    #define DBG_SCAN_MON_ADV_EN 1
#endif

#ifndef DEB_CIG_MST_EN
    #define DEB_CIG_MST_EN 0
#endif

#ifndef DEB_CIG_SLV_EN
    #define DEB_CIG_SLV_EN 0
#endif

#ifndef DBG_CIS_DISCONN_EN
    #define DBG_CIS_DISCONN_EN 0
#endif

#ifndef DEB_BIG_BCST_EN
    #define DEB_BIG_BCST_EN 0
#endif

#ifndef DEB_BIG_SYNC_EN
    #define DEB_BIG_SYNC_EN 0
#endif

#ifndef DEB_BIG_SYNC_LL_DATA_EN
    #define DEB_BIG_SYNC_LL_DATA_EN 0
#endif

#ifndef DBG_ISO_TEST_EN
    #define DBG_ISO_TEST_EN 0
#endif


#ifndef DBG_CIS_TERMINATE
    #define DBG_CIS_TERMINATE 0
#endif


#ifndef DBG_CIS_1ST_AP_TIMING_EN
    #define DBG_CIS_1ST_AP_TIMING_EN 0
#endif


#ifndef DBG_NUM_COM_PKT
    #define DBG_NUM_COM_PKT 0
#endif


#ifndef DBG_CIS_PARAM
    #define DBG_CIS_PARAM 0
#endif

#ifndef DBG_CIS_CENTRAL_PARAM
    #define DBG_CIS_CENTRAL_PARAM 0
#endif

#ifndef DBG_SET_CIG_PARAMS
    #define DBG_SET_CIG_PARAMS 0
#endif

#ifndef DBG_CIS_TX_DATA
    #define DBG_CIS_TX_DATA 0
#endif

#ifndef DBG_CIS_RX_DATA
    #define DBG_CIS_RX_DATA 0
#endif

#ifndef DBG_CIS_RX_DATA_FLOW_EN
    #define DBG_CIS_RX_DATA_FLOW_EN 0
#endif

#ifndef DBG_CIS_TX_DATA_FLOW_EN
    #define DBG_CIS_TX_DATA_FLOW_EN 0
#endif


#ifndef DBG_HCI_CIS_TEST
    #define DBG_HCI_CIS_TEST 0
#endif


#ifndef DBG_SUBRATE_EN
    #define DBG_SUBRATE_EN 0
#endif


#ifndef DBG_LL_PAST_EN
    #define DBG_LL_PAST_EN 0
#endif

#ifndef DBG_LL_PCL_EN
    #define DBG_LL_PCL_EN 0
#endif

#ifndef DBG_LL_CC_EN
    #define DBG_LL_CC_EN 0
#endif


#ifndef DBG_CUSTOM_ACLC_TIMING
    #define DBG_CUSTOM_ACLC_TIMING 0
#endif


#ifndef DEB_BIG_SYNC_TIMESTAM_EN
    #define DEB_BIG_SYNC_TIMESTAM_EN 0
#endif


#ifndef DBG_OTA_WRITE_FW
    #define DBG_OTA_WRITE_FW 0
#endif

#ifndef DBG_SLAVE_CONN_UPDATE
    #define DBG_SLAVE_CONN_UPDATE 0
#endif

/*****************************************************************************************************************

                                                VCD

*****************************************************************************************************************/
#define VCD_DEFINE_DEFAULT   1

#define VCD_DEFINE_EXTENDED  5
#define VCD_DEFINE_BIS_SYNC  6

#define VCD_DEFINE_CIS       10
#define VCD_DEFINE_CIS_PER   11
#define VCD_DEFINE_CIS_CEN   12
#define VCD_FANQH_DEFINE_CIS 13

#define VCD_DEFINE_BIS       20

#define VCD_DEFINE_CS        21


#define VCD_DEFINE_SIHUI     30


#ifndef VCD_DEFINE_SELECT
    #define VCD_DEFINE_SELECT VCD_DEFINE_CIS_PER
#endif


/* BLE rf irq timing && log enable */
#ifndef SL_STACK_IRQ_TIMING_EN
    #define SL_STACK_IRQ_TIMING_EN 1
#endif

#ifndef SL_STACK_SCHE_TIMING_EN
    #define SL_STACK_SCHE_TIMING_EN 1
#endif

#ifndef SL_STACK_FSM_TIMING_EN
    #define SL_STACK_FSM_TIMING_EN 0
#endif


#ifndef SL_STACK_EXT_PRD_BASE_TIMING_EN
    #define SL_STACK_EXT_PRD_BASE_TIMING_EN 1
#endif


#ifndef SL_STACK_BIG_BCST_TIMING_EN
    #define SL_STACK_BIG_BCST_TIMING_EN 0
#endif


#ifndef SL_STACK_EXTSCAN_BASIC_TIMING_EN
    #define SL_STACK_EXTSCAN_BASIC_TIMING_EN 1
#endif


/* ACL connection */
#ifndef SL_STACK_ACL_BASIC_TIMING_EN
    #define SL_STACK_ACL_BASIC_TIMING_EN 1
#endif


#ifndef SL_STACK_CIS_BASIC_TIMING_EN
    #define SL_STACK_CIS_BASIC_TIMING_EN 1
#endif


#ifndef SL_STACK_CIS_RX_DATA_EN
    #define SL_STACK_CIS_RX_DATA_EN 1
#endif


#ifndef SL_STACK_CIS_TX_DATA_EN
    #define SL_STACK_CIS_TX_DATA_EN 1
#endif


#ifndef SL_STACK_BIS_SOURCE_TIMING_EN
    #define SL_STACK_BIS_SOURCE_TIMING_EN 0
#endif


#ifndef SL_STACK_BIS_SINK_TIMING_EN
    #define SL_STACK_BIS_SINK_TIMING_EN 1
#endif


#ifndef SL_STACK_BIS_RX_DATA_EN
    #define SL_STACK_BIS_RX_DATA_EN 0
#endif


#ifndef SL_STACK_CS_TIME_EN
    #define SL_STACK_CS_TIME_EN 0
#endif

#ifndef SL_STACK_CS_REFL_TIME_EN
    #define SL_STACK_CS_REFL_TIME_EN 0
#endif

/* controller,BQB,IUT, critical command & event */
#define SL_STACK_IUT_CMD_EVT 1


#define SL_STACK_ISO_DATA_EN 1


////////////////// CS debug start ///////////////////
#ifndef DBG_CS_LOG
    #define DBG_CS_LOG 1
#endif

#ifndef TTF_EN
    #define TTF_EN 0
#endif

#ifndef SUBEVENTLEN_ALG
    #define SUBEVENTLEN_ALG 0
#endif


#define DBG_CS_HCI_LOG_MASK_EN (stkLog_mask & STK_LOG_HCI_CS)
#define DBG_CS_LL_LOG_MASK_EN  (stkLog_mask & STK_LOG_LL_CS)
#define DBG_CS_LOG_EBQ_MASK_EN (stkLog_mask & STK_LOG_EBQ_CS)

#if DBG_CS_LOG
    #define BLC_CS_DBG(en, fmt, ...) tlkapi_printf(en, "[CS]" fmt "\n", ##__VA_ARGS__)
    #define CS_HCI_LOG(fmt, ...)     BLC_CS_DBG(DBG_CS_HCI_LOG_MASK_EN, "[HCI]" fmt, ##__VA_ARGS__)
    #define CS_LL_LOG(fmt, ...)      BLC_CS_DBG(DBG_CS_LL_LOG_MASK_EN, "[LL]" fmt, ##__VA_ARGS__)

    #define CS_EBQ_LOG(fmt, ...)     BLC_CS_DBG(DBG_CS_LOG_EBQ_MASK_EN, "[EBQ]" fmt, ##__VA_ARGS__)

    #define CS_EBQ_SEND_STRING       tlkapi_send_string_data

    #define CS_TEST_LOG(fmt, ...)    BLC_CS_DBG(DBG_CS_LOG_EBQ_MASK_EN, "[CS TEST CMD]" fmt, ##__VA_ARGS__)
    #define CS_TEST_SEND_STRING      tlkapi_send_string_data

#else
    #define BLC_CS_DBG(en, fmt, ...)
    #define CS_HCI_LOG(fmt, ...)
    #define CS_LL_LOG(fmt, ...)
    #define CS_EBQ_LOG(fmt, ...)
    #define CS_EBQ_SEND_STRING
    #define CS_TEST_LOG(fmt, ...)
    #define CS_TEST_SEND_STRING
#endif

#if TTF_EN
    #define DBG_LOG_TTF_MASK_EN (stkLog_mask & STK_LOG_TTF)
    #define TTF_LOG(fmt, ...)   tlkapi_printf(DBG_LOG_TTF_MASK_EN, "[TTF]" fmt "\n", ##__VA_ARGS__)
#else
    #define TTF_LOG(fmt, ...)
#endif

///////////////// CS debug end //////////////////


///////////////// HDT debug start //////////////
#ifndef DBG_HDT_LOG
    #define DBG_HDT_LOG 1
#endif

#define DBG_HDT_HCI_LOG_MASK_EN (stkLog_mask & STK_LOG_HCI_HDT)
#define DBG_HDT_LL_LOG_MASK_EN (stkLog_mask & STK_LOG_ll_HDT)

#if DBG_HDT_LOG
    #define HDT_HCI_LOG(fmt, ...)     BLC_CS_DBG(DBG_HDT_HCI_LOG_MASK_EN, "[HCI]" fmt, ##__VA_ARGS__)
    #define HDT_LL_LOG(fmt, ...)      BLC_CS_DBG(DBG_HDT_LL_LOG_MASK_EN, "[LL]" fmt, ##__VA_ARGS__)
#else
    #define HDT_HCI_LOG(fmt, ...)
    #define HDT_LL_LOG(fmt, ...)
#endif

///////////////// HDT debug end /////////////////
#endif /* STACK_BLE_DEBUG_DEBUG_CFG_H_ */
