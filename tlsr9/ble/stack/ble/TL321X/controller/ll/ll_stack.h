/********************************************************************************************************
 * @file    ll_stack.h
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
#ifndef LL_STACK_H_
#define LL_STACK_H_

#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_format.h"
#include "stack/ble/hci/hci_cmd.h"
#include "stack/ble/controller/whitelist/whitelist_stack.h"

#include "tl_common.h"
#include "drivers.h"
#include "../../debug/debug_cfg.h"


#define STATIC_ASSERT_MSG(cond, msg)                __attribute__((unused)) typedef char STATIC_ASSERT_MSG_(msg, __LINE__, __COUNTER__)[1 - 2 * !(cond)]

#define STATIC_ASSERT_MSG_(MSG, LINE, COUNTER)      STATIC_ASSERT_MSG__(MSG, LINE, COUNTER)
#define STATIC_ASSERT_MSG__(MSG, LINE, COUNTER)     static_assert##COUNTER##__FileOf_##MSG##__LineOf_##LINE

#define STATIC_ASSERT_FILE(exp, file_name)          STATIC_ASSERT_MSG(exp, file_name)


#ifndef         SDK_RELEASE_CHECK_EN
#define         SDK_RELEASE_CHECK_EN                                0
#endif


#define         DYNAMIC_SCHE_CAL_TIME_EN                            1  //must be 1 now, can not change

#ifndef         SCH_TASK_PRIORITY_IN_CB_EN
#define         SCH_TASK_PRIORITY_IN_CB_EN                          1
#endif

#ifndef         OPTIMIZE_INSERT_EMPTY_EN
#define         OPTIMIZE_INSERT_EMPTY_EN                            0
#endif

#ifndef         TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM
#define         TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM           0   //main_loop code in ram to workaround TIFS 150 uS variation
#endif

#ifndef         LL_CRC_CHECK_REGISTER_EN
#define         LL_CRC_CHECK_REGISTER_EN                            0
#endif

#ifndef         LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE
#define         LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE     0
#endif

#define         LL_FEATURE_BIT_NUMBER_ISOCHRONOUS_STREAM_HOST               32
#define         LL_FEATURE_BIT_NUMBER_CONNECTION_SUBRATING_HOST             38


#ifndef         LL_BIS_SNC_BV18C_BN6
#define         LL_BIS_SNC_BV18C_BN6                                0
#endif

#ifndef         LL_BIS_SYNC_TEST
#define         LL_BIS_SYNC_TEST                                    0
#endif

#ifndef         LL_DDI_ADV_BV45C
#define         LL_DDI_ADV_BV45C                                    0
#endif

#ifndef         PDA_SCAN_PENDING_FIX_EN
#define         PDA_SCAN_PENDING_FIX_EN                             0
#endif

#ifndef         SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM
#define         SUB_INTERVAL_VARIATION_WORKAROUND_MLP_CODE_IN_RAM   0
#endif

/*
 * Fix Pairing connection failure between telink ble slave RCU and Chrome OS problem
 * Refer to: http://192.168.48.49:8080/browse/BLE-50
 */
#ifndef         LL_PAUSE_ENC_FIX_EN
#define         LL_PAUSE_ENC_FIX_EN                                 1
#endif


#ifndef         BLE_LLMIC_CONCURRENT_EN
#define         BLE_LLMIC_CONCURRENT_EN                            0
#endif


/******************************* conn_config start ******************************************************************/

#define         CONN_MAX_NUM_C0_P1                                  1
#define         CONN_MAX_NUM_C0_P2                                  2
#define         CONN_MAX_NUM_C0_P4                                  3
#define         CONN_MAX_NUM_C1_P0                                  4
#define         CONN_MAX_NUM_C1_P1                                  5
#define         CONN_MAX_NUM_C1_P2                                  6
#define         CONN_MAX_NUM_C1_P4                                  7
#define         CONN_MAX_NUM_C2_P0                                  8
#define         CONN_MAX_NUM_C2_P2                                  9
#define         CONN_MAX_NUM_C2_P4                                  10
#define         CONN_MAX_NUM_C4_P0                                  11
#define         CONN_MAX_NUM_C4_P2                                  12
#define         CONN_MAX_NUM_C4_P4                                  13


#ifndef         CONN_MAX_NUM_CONFIG
#define         CONN_MAX_NUM_CONFIG                                 CONN_MAX_NUM_C4_P4
#endif

#if (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C0_P1)
    #define         LL_MAX_ACL_CEN_NUM                              0
    #define         LL_MAX_ACL_PER_NUM                              1

    #define         LL_ACL_CEN_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C0_P2)
    #define         LL_MAX_ACL_CEN_NUM                              0
    #define         LL_MAX_ACL_PER_NUM                              2

    #define         LL_ACL_CEN_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C0_P4)
    #define         LL_MAX_ACL_CEN_NUM                              0
    #define         LL_MAX_ACL_PER_NUM                              4

    #define         LL_ACL_CEN_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C1_P0)
    #define         LL_MAX_ACL_CEN_NUM                              1
    #define         LL_MAX_ACL_PER_NUM                              0

    #define         LL_ACL_PER_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C1_P1)
    #define         LL_MAX_ACL_CEN_NUM                              1
    #define         LL_MAX_ACL_PER_NUM                              1
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C1_P2)
    #define         LL_MAX_ACL_CEN_NUM                              1
    #define         LL_MAX_ACL_PER_NUM                              2
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C1_P4)
    #define         LL_MAX_ACL_CEN_NUM                              1
    #define         LL_MAX_ACL_PER_NUM                              4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C2_P0)
    #define         LL_MAX_ACL_CEN_NUM                              2
    #define         LL_MAX_ACL_PER_NUM                              0

    #define         LL_ACL_PER_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C2_P2)
    #define         LL_MAX_ACL_CEN_NUM                              2
    #define         LL_MAX_ACL_PER_NUM                              2
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C2_P4)
    #define         LL_MAX_ACL_CEN_NUM                              2
    #define         LL_MAX_ACL_PER_NUM                              4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C4_P0)
    #define         LL_MAX_ACL_CEN_NUM                              4
    #define         LL_MAX_ACL_PER_NUM                              0

    #define         LL_ACL_PER_EN                               0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C4_P2)
    #define         LL_MAX_ACL_CEN_NUM                              2
    #define         LL_MAX_ACL_PER_NUM                              4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_C4_P4)
    #define         LL_MAX_ACL_CEN_NUM                              4
    #define         LL_MAX_ACL_PER_NUM                              4
#else
    #error "unsupported CONN_MAX_NUM_CONFIG"
#endif


#define         LL_MAX_ACL_CONN_NUM                                 (LL_MAX_ACL_CEN_NUM + LL_MAX_ACL_PER_NUM)






#define         ACL_CONN_IDX_CEN0                                   0
#define         ACL_CONN_IDX_PER0                                   LL_MAX_ACL_CEN_NUM



#ifndef         LL_ACL_CEN_EN
#define         LL_ACL_CEN_EN                                       1
#endif

#ifndef         LL_ACL_PER_EN
#define         LL_ACL_PER_EN                                       1
#endif

#ifndef         LL_ASYNC_LEA_EN
#define         LL_ASYNC_LEA_EN                                     1
#endif



#ifndef BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
#if (LL_ACL_PER_EN)
    #define BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE                1
#else
    #define BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE                0
#endif
#endif

#define     BLS_PROC_BIS_SYNC_UPDATE_REQ_IN_IRQ_ENABLE              1 //must open


/******************************* conn_config end ******************************************************************/
#define         BLT_STRUCT_4B_ALIGN_CHECK_EN                    1   //keep enable all time, no need disable



//scheduler
#define         SCH_DEBUG_EN                                    1




#define         CRC_MATCH8(md,ms)   (md[0]==ms[0] && md[1]==ms[1] && md[2]==ms[2])



/* TianXiang found in 20230627, detail in Skype group "LE Audio SDK", search "cigs_start_tick", aound it
   root reason: create CIS command triggers some temp status variable,  ACL terminate only process CIS status clear for established CIS
                if CIS is in margin area between create command and established, ACL terminate not clear these status, lead to
                next command error, e.g. cisFlow_pending & cism_create_pending_msk & createCmd
   Actually we did not consider this margin area, so a lot of logic should be fixed, consider that code changed too much need test work
   and SDK test is nearby, SiHui only fix some obvious error 20230628, and will fix in depth when we got EBQ to test.
*/
#define         FIX_CIS_CREATE_CMD_ERR_DUE_TO_PREVIOUS_CREATE_NOT_CLEAR_WHEN_ACL_TERMINATE              1




/******************************* ll start ***********************************************************************/

/* with fast settle function, RX settle can be shorten by about 40uS, this is very useful for some case(e.g. BRX, CRX).
 * But it also bring some bad effect in some case, for example, legacy ADV RX2TX mode, send ADV packet then receive potential
 * conn_req/scan_req. RX anchor point is 150uS after TX done, so RX timing is very long, no need optimize. When RX settle is 40us,
 * the time range of 40~150us after TX done is OK for receive air packet in channel 37/38/39, so easier to receive unexpected packet
 * in air which means worse anti-interference ability and worse connectivity possibility to master when in bad environment.
 * So we need change RX settle to a bigger value in this case, and recover it after case over.
 *
 * optimize cases include below:
 * 1. legacy ADV: connectable ADV
 *
 */
#ifndef FASTSETTLE_RX_SHORTEN_BAD_EFFECT_FIX_EN
#define FASTSETTLE_RX_SHORTEN_BAD_EFFECT_FIX_EN        1
#endif




#define         ADV_LEGACY_MASK                                 BIT(0)
#define         ADV_EXTENDED_MASK                               BIT(1)
#define         SCAN_LEGACY_MASK                                BIT(2)
#define         SCAN_EXTENDED_MASK                              BIT(3)

#define         SET_RANDOM_ADDR_CMD_MASK                        BIT(4)
#define         SET_EXTSCAN_PARAM_CMD_MASK                      BIT(5)


#define         IS_LEGACY_ADV_VALID                             (blmsParam.hci_cmd_mask & ADV_LEGACY_MASK)
#define         SET_LEGACY_ADV_VALID                            (blmsParam.hci_cmd_mask |= ADV_LEGACY_MASK)

#define         IS_EXTENDED_ADV_VALID                           (blmsParam.hci_cmd_mask & ADV_EXTENDED_MASK)
#define         SET_EXTENDED_ADV_VALID                          (blmsParam.hci_cmd_mask |= ADV_EXTENDED_MASK)

#define         IS_LEGACY_SCAN_VALID                            (blmsParam.hci_cmd_mask & SCAN_LEGACY_MASK)
#define         SET_LEGACY_SCAN_VALID                           (blmsParam.hci_cmd_mask |= SCAN_LEGACY_MASK)

#define         IS_EXTENDED_SCAN_VALID                          (blmsParam.hci_cmd_mask & SCAN_EXTENDED_MASK)
#define         SET_EXTENDED_SCAN_VALID                         (blmsParam.hci_cmd_mask |= SCAN_EXTENDED_MASK)

#define         IS_LEGACY_CMD_EVT_VALID                         (blmsParam.hci_cmd_mask & (ADV_LEGACY_MASK | SCAN_LEGACY_MASK))
#define         IS_EXTENDED_CMD_EVT_VALID                       (blmsParam.hci_cmd_mask & (ADV_EXTENDED_MASK | SCAN_EXTENDED_MASK))



typedef struct{
    u32 dma_len;

    u8  type   :4;
    u8  rfu1   :1;
    u8  chan_sel:1;
    u8  txAddr :1;
    u8  rxAddr :1;

    u8  rf_len;             //LEN(6)_RFU(2)
    u8  initA[6];           //scanA
    u8  advA[6];            //
    u8  accessCode[4];      // access code
    u8  crcinit[3];
    u8  winSize;
    u16 winOffset;
    u16 interval;
    u16 latency;
    u16 timeout;
    u8  chm[5];

    u8  hop :5;
    u8  sca :3;
}rf_packet_connect_t;

typedef bool (*ll_adv_2_slave_callback_t)(rf_packet_connect_t *, bool);


typedef struct {
    u8      num;    //not used now
    u8      mask;   //not used now
    u8      wptr;
    u8      rptr;
    u8*     p;
}scan_fifo_t;

extern _attribute_aligned_(4) scan_fifo_t   scan_priRxFifo;
extern _attribute_aligned_(4) scan_fifo_t   scan_secRxFifo;

extern  ll_adv_2_slave_callback_t           ll_adv_2_slave_cb;

extern u8 glb_temp_rx_buff[];


#if (CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN || PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN)
    extern int flash_filterMac_address;
    extern int filter_mac_enable;
    extern u8  filter_mac_address[];
#endif


_attribute_aligned_(4)
typedef struct {
    u8      gLlChannelMap[5];
    u8      rsvd[3]; //align

    u32     hostMapUptCmdTick;
    u32     hostMapUptCmdPending; //BIT(0): master ACL0, BIT(1): master ACL1, etc.
} st_llm_hostChnClassUpt_t;

extern  _attribute_aligned_(4)  st_llm_hostChnClassUpt_t    blmhostChnClassUpt;
extern  const u16 scaPpmTbl[8];

#if (CONTROLLER_GEN_P256KEY_ENABLE)
extern _attribute_aligned_(4) smp_sc_key_t  smp_sc_key;
ble_sts_t   blt_ll_getP256pubKey(void);
ble_sts_t   blt_ll_generateDHkey (u8* remote_public_key, bool use_dbg_key);
void        blt_ll_procGetP256pubKeyEvent (void);
void        blt_ll_procGenDHkeyEvent (void);
#endif

/*
 *  RF_TX_Path_Compensation_Value: Convert 0.1dB to 1dB
 */
s8          blt_ll_getRfTxPathComp(void);

/*
 *  RF_RX_Path_Compensation_Value: Convert 0.1dB to 1dB
 */
s8          blt_ll_getRfRxPathComp(void);


bool        blt_ll_isResolvingListCommandAllowed(void);





_attribute_aligned_(4)
typedef struct {
    ll_resolv_list_t *pRslvlst_locRpa;
    u8      local_use_rpa;
    u8      peer_use_rpa;   //peer use RPA and local can resolve success to give corresponding IDA

    /* attention that: name is _pka_ida_, maybe a packet address or IDA
     * when first run, set pktA to it(maybe IDA or RPA)
     * if RPA resolve OK, will set true IDA(form RL) to it; if resolve Fail, still be pktA */
    u8      peer_pka_or_ida_type;
    u8      peer_pka_or_ida_addr[BLE_ADDR_LEN];
}ll_addr_t;
extern  _attribute_aligned_(4) ll_addr_t  bltAddr;


static inline void blt_ll_addr_clear_local_rpa_flag(void)
{
    bltAddr.local_use_rpa = 0;
}

static inline void blt_ll_addr_mark_local_rpa(ll_resolv_list_t *pRL)
{
    bltAddr.local_use_rpa = 1;
    bltAddr.pRslvlst_locRpa = pRL;
}

void blt_ll_addr_set_peer_address(u8, u8, u8*);


ble_sts_t   blc_hci_readLocalSupportedCommands(hci_readLocSupCmds_retParam_t *);
ble_sts_t   blc_hci_readLocalSupportedFeatures(hci_readLocSupFeatures_retParam_t *);


void        blc_ll_initFastSettle(u8 tx_fast_en,u8 rx_fast_en);

void blt_ll_record_identity_address(u8 type, u8 *addr);


/*************** begin ************* RF tx power setting ***************************************/
typedef enum{
    TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT,
    TX_POWER_STRATEGY_PCL,
}rf_tx_power_strategy_t;

_attribute_ram_code_ void blt_ll_set_tx_power_by_strategy(rf_tx_power_strategy_t pwrCtrl_flag, u8 pwrCtrl_power);
/*************** end  ************* RF tx power setting ***************************************/



/*************** begin ************* below APIs: PCL module used,
 * YaFei 20240223, API below merge from ll.h to ll_stack.h.  Temporarily User no need these APIs
 *
 * "blc ll_readSuppTxPower" & "blc ll_readRfPathComp" & "blc ll_writeRfPathComp" & "blc hci_le_readSuppTxPower" &
 * "blc hci_le_readRfPathComp" & "blc hci_le_writeRfPathComp" */
/**
 * @brief      this function is used to read the minimum and maximum transmit powers supported by the Controller
 *             across all supported PHYs.
 * @param[in]  *pMinTxPwr - Min_TX_Power
 * @param[in]  *pMaxTxPwr - Max_TX_Power
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_readSuppTxPower (s8 *pOutMinTxPwr, s8 *pOutMaxTxPwr);


/**
 * @brief      this function is used to read the RF Path Compensation Values parameter used in the Tx Power Level
 *             and RSSI calculation.
 * @param[in]  *pTxPathComp - RF_TX_Path_Compensation_Value
 * @param[in]  *pRxPathComp - RF_RX_Path_Compensation_Value
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_readRfPathComp(s16 *pOutTxPathComp, s16 *pOutRxPathComp);


/**
 * @brief      this function is used to indicate the RF path gain or loss between the RF transceiver and the
 *             antenna contributed by intermediate components.
 * @param[in]  txPathComp - RF_TX_Path_Compensation_Value
 * @param[in]  rxPathComp - RF_RX_Path_Compensation_Value
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_writeRfPathComp(s16 txPathComp, s16 rxPathComp);


/**
 * @brief      this function is used to read the minimum and maximum transmit powers supported by the Controller
 *             across all supported PHYs.
 * @param[in]  refer to 'hci_le_rdSuppTxPwrRetParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_readSuppTxPower(hci_le_rdSuppTxPwrRetParams_t *retPara);


/**
 * @brief      this function is used to read the RF Path Compensation Values parameter used in the Tx Power Level
 *             and RSSI calculation.
 * @param[in]  refer to 'hci_le_rdRfPathCompRetParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_readRfPathComp(hci_le_rdRfPathCompRetParams_t *retPara);


/**
 * @brief      this function is used to indicate the RF path gain or loss between the RF transceiver and the
 *             antenna contributed by intermediate components.
 * @param[in]  refer to 'hci_le_writeRfPathCompCmdParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_writeRfPathComp(hci_le_writeRfPathCompCmdParams_t *cmdPara);
/*************** end ************* below APIs: PCL module used, remove from ll.h to ll_stack.h */





/* remove from ll.h to ll_stack.h, special used
 * YaFei 20240223, "blt set_cus_chn_mask_t" &  "blc_ll_setCustomizedAdvertisingScanningChannelMask" &
 * "blc_ll_setCustomTxPowerEnable" merge from ll.h to ll_stack.h.  Temporarily User no need these */
typedef enum{
    ALLOW_LEG_CHN_SETTING = BIT(0),
    ALLOW_EXT_CHN_SETTING = BIT(1),
    ALLOW_LEG_EXT_CHN_SETTING = BIT(0) | BIT(1),
} blt_set_cus_chn_mask_t;
void        blc_ll_setCustomizedAdvertisingScanningChannelMask (blt_set_cus_chn_mask_t mask);

/* remove from ll.h to ll_stack.h, LE Audio proj special used */
/**
 * @brief      This function is used to enable custom TX power in specific PHY mode.
 * @param      en, 0x01: enable.
 *                 0x00: disable, default value.
 * @param[in]  tx_pwr_1m    - TX power in RF 1M PHY mode
 * @param[in]  tx_pwr_2m    - TX power in RF 2M PHY mode
 * @param[in]  tx_pwr_coded - TX power in RF Coded PHY mode
 * @return     none
 */
void        blc_ll_setCustomTxPowerEnable(u8 en, rf_power_level_index_e tx_pwr_1m,
                                                 rf_power_level_index_e tx_pwr_2m,
                                                 rf_power_level_index_e tx_pwr_coded);





/* SiHui 20240223
 * "ll feature_value_t" & "blcll_setHostFeature" merge from ll.h to ll_stack.h. application no need, only stack use */
typedef enum{
    LL_FEATURE_ENABLE   = 1,
    LL_FEATURE_DISABLE  = 0,
}ll_feature_value_t;

/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the ACL Central role is valid.
 * @param[in]  bit_number - Bit position in the FeatureSet.
 * @param[in]  bit_value - refer to the struct "ll_feature_value_t".
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_setHostFeature(u8 bit_number, ll_feature_value_t bit_value);
/******************************* ll end *************************************************************************/




/******************************* ll_system start ***********************************************************************/


enum {
    /* 1. common for all module, can not be repeated !!! */
    FLAG_CHECK_INIT                     =   BIT(31),
    FLAG_SCHEDULE_START                 =   BIT(30),
    FLAG_SCHEDULE_DONE                  =   BIT(29),
    FLAG_SCHEDULE_BUILD                 =   BIT(28),
    FLAG_INSERT_SCHTSK_CONFLICT         =   BIT(27),
    FLAG_SCHEDULE_SSLOT_RESET           =   BIT(26),/* now only periodic use*/

    FLAG_IRQ_RX                         =   BIT(25),
    FLAG_IRQ_TX                         =   BIT(24),


    FLAG_MODULE_MAINLOOP                =   BIT(23),
    FLAG_MODULE_RESET                   =   BIT(22),   //HCI reset
    FLAG_MODULE_SET_HOST_CHM            =   BIT(21),



    /* 2. define for modules
     *    can not be same in module internal, but can be same in different modules  */
    //ADV
    FLAG_SCHEDULE_SEND_EXTADV           =   BIT(20),
    FLAG_SCHEDULE_SEND_AUXADV           =   BIT(19),
    FLAG_SCHEDULE_EXTADV_BUILD          =   BIT(18),
    FLAG_SCHEDULE_LEGADV_BUILD          =   BIT(17),
    FLAG_EXTADV_SET_DATA_ADDR_CHANGE    =   BIT(16),
    FLAG_INSERT_AUXADV_SCHTSK_CONFLICT  =   BIT(15),


    //SCAN
    FLAG_SCHEDULE_PRICHN_SCAN_INSERT    =   BIT(20),
    FLAG_SCHEDULE_SECCHN_SCAN_INSERT    =   BIT(19),
    FLAG_SCAN_DATA_REPORT               =   BIT(18),
    FLAG_EXT_SCAN_DATA_TRUNCATED        =   BIT(17),
    FLAG_SCHEDULE_EXTSCAN_DISABLE       =   BIT(16),
    FLAG_EXT_SCAN_MAINLOOP_PEND_TASK    =   BIT(15),
    FLAG_SCHEDULE_PRICHN_SCAN_ALIGN_BUILD   =   BIT(14),
    FLAG_SCHEDULE_PRICHN_SCAN_ALIGN_TASK    =   BIT(13),

    //INIT
    FLAG_SCHEDULE_EXTINIT_CHECK_CONNRSP     =   BIT(19),


    //ACL
    FLAG_ACL_CONN_1                     =   BIT(20),
    FLAG_ACL_CONN_2                     =   BIT(19),
    FLAG_ACL_CONN_PARAM_UPDATE_CHECK    =   BIT(18),
    FLAG_ACL_SLAVE_CLEAR_SLEEP_LATENCY  =   BIT(17),
    FLAG_ACL_SLAVE_CHECK_UPDATE_CMD_DEC =   BIT(16),
    FLAG_ACL_SUBRATE_INSERT_CONTI_TASK  =   BIT(15),
    FLAG_ACL_SUBRATE_CONN_CB            =   BIT(14),
    FLAG_ACL_SUBRATE_RESET              =   BIT(13),
    FLAG_ACL_CONN_EXIT_CHECK            =   BIT(12),
    
    //PAST
    FLAG_PAST_INIT_AFT_ACL_CONN         =   BIT(20),
    FLAG_PAST_RCVD_PRD_SYNC_IND         =   BIT(19),


    //PCL
    FLAG_PCL_INIT_AFT_ACL_CONN          =   BIT(20),
    FLAG_PCL_INIT_AFT_CIS_CONN          =   BIT(19), /* IN LOOP */
    FLAG_PCL_MONITORING_ACL_RX_RSSI     =   BIT(18), /* FLAG_PCL_AUTOINIT_PCL_REQ_ACL */
    FLAG_PCL_MONITORING_CIS_RX_RSSI     =   BIT(17), /* FLAG_PCL_AUTOINIT_PCL_REQ_CIS */
    FLAG_PCL_MONITORING_PATH_LOSS       =   BIT(16),
    FLAG_PCL_PWR_CHG_AFT_PHY_UPT        =   BIT(15),
    FLAG_PCL_PWR_CHG_AFT_CIS_EST        =   BIT(14),


    //CHANNEL_CLASSIFICATION
    FLAG_CHNC_INIT_AFT_ACL_CONN         =   BIT(20),


    //PERD_ADV & PERD_ADV_SYNC
    FLAG_SCHEDULE_AUX_SYNCINFO_UPDATE       =   BIT(20),
    FLAG_SCHEDULE_PRDADV_PARAM_UPDATE       =   BIT(19),
    FLAG_SCHEDULE_PRDADV_TASK_BEGIN         =   BIT(18),
    FLAG_AUXSCAN_PRDADV_SYNCINFO            =   BIT(17),
    FLAG_PRDADV_SYNC_RX                     =   BIT(16),
    FLAG_PRDADV_DATA_REPORT                 =   BIT(15),
    FLAG_PRDADV_SYNC_ACAD_BIGINFOR          =   BIT(14),
    FLAG_PRDADV_SYNC_ACAD_CHMUPT            =   BIT(13),
    FLAG_PAWR_SYNC_RX_AUX_SYNC_SUBEVT_IND   =   BIT(12),
    FLAG_PAWR_SYNCINFO_AND_ACADSYNCTIMING   =   BIT(11),
    FLAG_PAWR_SYNC_RX_AUX_CONN_REQ          =   BIT(10),

    //PAWR-ADVERISTER
    FLAG_SCHEDULE_PAWRA_SLOT_START          =   BIT(20),
    FLAG_SCHEDULE_PAWRA_SLOT_POST           =   BIT(19),
    FLAG_INSERT_PAWRA_SLOT_TASK             =   BIT(18),
    FLAG_SCHEDULE_PAWRA_1ST_SUB             =   BIT(17),
    FLAG_SCHEDULE_PAWRA_CONN_REQ            =   BIT(16),
    FLAG_SCHEDULE_PAWRA_ADVDATA_UPT         =   BIT(15),


    //AoA & AoD
    FLAG_AOA_AOD_CONNECTIONLESS_IQ_REPORT   =   BIT(20),
    FLAG_AOA_AOD_CONNECTIONLESS_DATA_PROCESS=   BIT(19),
    FLAG_AOA_AOD_CONNECTION_MAINLOOP        =   BIT(18),


    ///////////////////// CIS start ////////////////////////
    FLAG_ACL_LOCAL_DISCONNECT           =   BIT(19),
    FLAG_ACL_IRQ_TERMINATE              =   BIT(18),    //rename later,
    FLAG_CIS_SCHEDULER_TASK             =   BIT(17),
    FLAG_SCHEDULE_CIG_SET1ST_AP         =   BIT(16),


    //CIG_MST
    FLAG_SCHEDULE_CIGMST_START          =   BIT(15),
    FLAG_SCHEDULE_CTX_START             =   BIT(14),
    FLAG_SCHEDULE_CTX_POST              =   BIT(13),
    FLAG_CIS_CREATE_CANCEL              =   BIT(12),
    FLAG_ACL_MLP_DISCONNECT_EVT         =   BIT(11),


    //CIG_SLV
    FLAG_SCHEDULE_CIGSLV_START          =   BIT(15),
    FLAG_SCHEDULE_CISSLV_START          =   BIT(14),
    FLAG_SCHEDULE_CISSLV_POST           =   BIT(13),
    FLAG_SCHEDULE_CIGSLV_BUILD          =   BIT(12),
    FLAG_SCHEDULE_CIGSLV_GET1ST_AP      =   BIT(11),
    ///////////////////// CIS end ////////////////////////




    //BIG_BCST
    FLAG_SCHEDULE_BIGBCST_START         =   BIT(20),
    FLAG_SCHEDULE_BISBCST_START         =   BIT(19),
    FLAG_SCHEDULE_BISBCST_POST          =   BIT(18),
    FLAG_SCHEDULE_BIGBCST_BUILD         =   BIT(17),
    FLAG_BIG_BRD_HANDLE_SEARCH          =   BIT(16),


    //BIG_SYNC
    FLAG_SCHEDULE_BIGSYNC_START         =   BIT(20),
    FLAG_SCHEDULE_BISSYNC_START         =   BIT(19),
    FLAG_SCHEDULE_BISSYNC_POST          =   BIT(18),
    FLAG_SCHEDULE_BIGSYNC_BUILD         =   BIT(17),
    FLAG_SCHEDULE_BISSYNC_RX            =   BIT(16),
    FLAG_BIS_SYNC_CHECK_UPDATE_CMD_DEC  =   BIT(15),
    FLAG_BIG_SYNC_HANDLE_SEARCH         =   BIT(14),
    FLAG_BIG_SYNC_BIGINFO_REPORT        =   BIT(13),

    //channel sounding
    FLAG_CS_ACL_CB_FLAG                 = BIT(20),
    FLAG_CS_SUBEVENT_START              = BIT(19), // csSubevent start
    FLAG_CS_SUBEVENT_POST               = BIT(18),


    FLAG_CS_STEP_INIT_STX_START         = BIT(17),
    FLAG_CS_STEP_INIT_SRX_START         = BIT(16),
    FLAG_CS_STEP_REFL_SRX_START         = BIT(15),
    FLAG_CS_STEP_REFL_STX_START         = BIT(14),
    FLAG_CS_STEP_REFL_STX_POST          = BIT(13),
    FLAG_CS_STEP_RX                     = BIT(12),
    FLAG_SCHEDULE_POLL                  = BIT(11),
    FLAG_STEP_POST                      = BIT(10),
    FLAG_CS_ACL_DISCONN_CB              = BIT(9),


    //SNIFFER
#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    FLAG_ACL_SNIFFER_SYNC               = BIT(20),
    FLAG_ACL_SNIFFER_SEEK               = BIT(19),
    FLAG_ACL_SNIFFER_SEEK_START         = BIT(18),
    FLAG_ACL_SNIFFER_SEEK_POST          = BIT(17),
    FLAG_ACL_SNIFFER_SEEK_RX            = BIT(16),
#endif



    FLAG_SCHEDULE_TASK_IDX_MASK         =   0x0F,  //0~15, 4 bit



};


void irq_system_timer(void);
void blt_ll_clear_prichn_scan_status(void);
void blt_ll_rx_start_tick_check(void);
u32  blt_ll_get_rx_packet_tick(u8 rf_len);
/******************************* ll_system end *************************************************************************/






/******************************* ll start **********************************************************************/


#define         BLMS_STATE_NONE                                 0

#define         BLMS_STATE_SCHE_START                           BIT(0)
#define         BLMS_STATE_SCHE_INSERT                          BIT(1)

#define         BLMS_STATE_PRICHN_SCAN_ALIGN                    BIT(2)

#define         BLMS_STATE_ADV                                  BIT(4)
#define         BLMS_STATE_EXTADV_S                             BIT(5)
#define         BLMS_STATE_EXTADV_E                             BIT(6)

#define         BLMS_STATE_PRICHN_SCAN_S                        BIT(7)  //primary channel scan start
#define         BLMS_STATE_PRICHN_SCAN_E                        BIT(8)  //primary channel scan end
#define         BLMS_STATE_SECCHN_SCAN_S                        BIT(9)  //secondary channel scan start
#define         BLMS_STATE_SECCHN_SCAN_E                        BIT(10) //secondary channel scan end
#define         BLMS_STATE_PDA_SYNC_S                           BIT(11) //periodic ADV sync start
#define         BLMS_STATE_PDA_SYNC_E                           BIT(12) //periodic ADV sync end

#define         BLMS_STATE_BTX_S                                BIT(13)
#define         BLMS_STATE_BTX_E                                BIT(14)
#define         BLMS_STATE_BRX_S                                BIT(15)
#define         BLMS_STATE_BRX_E                                BIT(16)


#define         BLMS_STATE_CIG_E                                BIT(18)
#define         BLMS_STATE_CTX_S                                BIT(19)  //CIS BTX Start
#define         BLMS_STATE_CTX_E                                BIT(20)  //CIS BTX End
#define         BLMS_STATE_CRX_S                                BIT(21)  //CIS BRX Start
#define         BLMS_STATE_CRX_E                                BIT(22)  //CIS BRX End


#define         BLMS_STATE_BIG_E                                BIT(25)
#define         BLMS_STATE_BBCST_S                              BIT(26)  //BIS BCST Start
#define         BLMS_STATE_BBCST_E                              BIT(27)  //BIS BCST End
#define         BLMS_STATE_BSYNC_S                              BIT(28)  //BIS SYNC Start
#define         BLMS_STATE_BSYNC_E                              BIT(29)  //BIS SYNC End

#define         BLMS_STATE_PAWRA_SLOT_GRP_E                     BIT(30)  //PAwR_A Rsp_Slots Group End
#define         BLMS_STATE_PAWRA_SLOT_S                         BIT(31)  //PAwR_A Slot Start
#define         BLMS_STATE_PAWRA_SLOT_E                         BIT(32)  //PAwR_A Slot End

#define         BLMS_STATE_PAWRS_SUB_S                          BIT(33)
#define         BLMS_STATE_PAWRS_SUB_E                          BIT(34)

#define         BLMS_STATE_CS_INIT_TX_S                         BIT(35)
#define         BLMS_STATE_CS_INIT_TX_E                         BIT(36)
#define         BLMS_STATE_CS_INIT_RX_S                         BIT(37)
#define         BLMS_STATE_CS_INIT_E                            BIT(38)

#define         BLMS_STATE_CS_REFL_STEP_S                       BIT(39)
#define         BLMS_STATE_CS_REFL_STEP_E                       BIT(40)
#define         BLMS_STATE_CS_SUBEVENT_E                        BIT(41)


#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #define     BLMS_STATE_SNIFS_SEEK_S                         BIT(42)  //ACL Sniffer Slave Seek Start
    #define     BLMS_STATE_SNIFS_SEEK_E                         BIT(43)  //ACL Sniffer Slave Seek End
    #define     BLMS_STATE_SNIFS_S                              BIT(44)  //ACL Sniffer Slave Sync Start
    #define     BLMS_STATE_SNIFS_E                              BIT(45)  //ACL Sniffer Slave Sync End

    #define     BLMS_STATE_SNIFM_SEEK_S                         BIT(46)  //ACL Sniffer Master Seek Start
    #define     BLMS_STATE_SNIFM_SEEK_E                         BIT(47)  //ACL Sniffer Master Seek End
    #define     BLMS_STATE_SNIFM_S                              BIT(48)  //ACL Sniffer Master Sync Start
    #define     BLMS_STATE_SNIFM_E                              BIT(49)  //ACL Sniffer Master Sync End

    #define BLMS_STATE_UPDATE_SCHEDULER         (blms_state & ( BLMS_STATE_SCHE_START | BLMS_STATE_SCHE_INSERT | BLMS_STATE_ADV | BLMS_STATE_EXTADV_E |  \
                                                            BLMS_STATE_PRICHN_SCAN_E | BLMS_STATE_SECCHN_SCAN_E | BLMS_STATE_PDA_SYNC_E | \
                                                            BLMS_STATE_BTX_E | BLMS_STATE_BRX_E | BLMS_STATE_CIG_E | BLMS_STATE_BIG_E | BLMS_STATE_PAWRA_SLOT_GRP_E | \
                                                            BLMS_STATE_PAWRS_SUB_E | BLMS_STATE_CS_SUBEVENT_E| BLMS_STATE_PRICHN_SCAN_ALIGN | \
                                                            BLMS_STATE_SNIFS_E | BLMS_STATE_SNIFS_SEEK_E | BLMS_STATE_SNIFM_E | BLMS_STATE_SNIFM_SEEK_E))//| BLMS_STATE_CRX_E
#else
    #define BLMS_STATE_UPDATE_SCHEDULER         (blms_state & ( BLMS_STATE_SCHE_START | BLMS_STATE_SCHE_INSERT | BLMS_STATE_ADV | BLMS_STATE_EXTADV_E |  \
                                                            BLMS_STATE_PRICHN_SCAN_E | BLMS_STATE_SECCHN_SCAN_E | BLMS_STATE_PDA_SYNC_E | \
                                                            BLMS_STATE_BTX_E | BLMS_STATE_BRX_E | BLMS_STATE_CIG_E | BLMS_STATE_BIG_E | BLMS_STATE_PAWRA_SLOT_GRP_E | \
                                                            BLMS_STATE_PAWRS_SUB_E | BLMS_STATE_CS_SUBEVENT_E| BLMS_STATE_PRICHN_SCAN_ALIGN))//| BLMS_STATE_CRX_E
#endif


#define BLMS_STATE_SHORT_TASK_START         (blms_state & ( BLMS_STATE_SECCHN_SCAN_S |  BLMS_STATE_PDA_SYNC_S | BLMS_STATE_BTX_S | BLMS_STATE_BRX_S | \
                                                            BLMS_STATE_CTX_S | BLMS_STATE_CRX_S | BLMS_STATE_BBCST_S | BLMS_STATE_BSYNC_S | \
                                                            BLMS_STATE_PAWRA_SLOT_S | BLMS_STATE_PAWRS_SUB_S | BLMS_STATE_CS_REFL_STEP_S))


//BLMS_STATE_BTX_E | BLMS_STATE_BRX_E
#define BLMS_STATE_CHECK_ACL_CONN_UPDATE_NEARBY         (blms_state & (BLMS_STATE_BTX_E | BLMS_STATE_BRX_E | BLMS_STATE_SECCHN_SCAN_E | \
                                                                BLMS_STATE_PDA_SYNC_E | BLMS_STATE_CIG_E | BLMS_STATE_BIG_E | BLMS_STATE_CS_SUBEVENT_E))



#define         BLMS_FLG_RF_CONN_DONE                           (FLD_RF_IRQ_CMD_DONE  | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT | FLD_RF_IRQ_RX_CRC_2)


#define         SYS_IRQ_TRIG_NEW_TASK                           0

#define         SYS_IRQ_TRIG_BTX_POST                           BIT(0)
#define         SYS_IRQ_TRIG_BRX_POST                           BIT(1)
#define         SYS_IRQ_TRIG_PRICHN_SCAN_POST                   BIT(2)
#define         SYS_IRQ_TRIG_SECCHN_SCAN_POST                   BIT(3)
#define         SYS_IRQ_TRIG_PDA_SYNC_POST                      BIT(4)  //periodic ADV sync

#define         SYS_IRQ_TRIG_PAWRA_SLOT_START                   BIT(5)  //PAwR-Advertiser - slot start
#define         SYS_IRQ_TRIG_PAWRA_SLOT_POST                    BIT(6)  //PAwR-Advertiser - slot end

#define         SYS_IRQ_TRIG_CTX_START                          BIT(7)
#define         SYS_IRQ_TRIG_CTX_POST                           BIT(8)
#define         SYS_IRQ_TRIG_CRX_START                          BIT(9)
#define         SYS_IRQ_TRIG_CRX_POST                           BIT(10)

#define         SYS_IRQ_TRIG_EXTADV_SEND                        BIT(12)

#define         SYS_IRQ_TRIG_BIG_POST                           BIT(16)
#define         SYS_IRQ_TRIG_BIS_TX_START                       BIT(17)
#define         SYS_IRQ_TRIG_BIS_TX_POST                        BIT(18)
#define         SYS_IRQ_TRIG_BIS_RX_START                       BIT(19)
#define         SYS_IRQ_TRIG_BIS_RX_POST                        BIT(20)

#define         SYS_IRQ_TRIG_SCHE_START                         BIT(24)
#define         SYS_IRQ_TRIG_SCHE_INSERT                        BIT(25)

#define         SYS_IRQ_TRIG_PAWRS_SUB_POST                     BIT(26)  //PAwR-sync subevent post

#define         SYS_IRQ_TRIG_CS_INIT_TX_START                   BIT(27)
#define         SYS_IRQ_TRIG_CS_INIT_SRX                        BIT(28)
#define         SYS_IRQ_TRIG_CS_STEP_POST                       BIT(29)
#define         SYS_IRQ_TRIG_CS_REFL_RX_START                   BIT(30)
#define         SYS_IRQ_TRIG_CS_REFL_TX_START                   BIT(31)
#define         SYS_IRQ_TRIG_CS_REFL_TX_POST                    BIT(32)

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #define     SYS_IRQ_TRIG_SNIFS_SEEK_POST                    BIT(33)
    #define     SYS_IRQ_TRIG_SNIFM_SEEK_POST                    BIT(34)
#endif

#define         STATE_CHANGE_LEG_ADV                            BIT(0)
#define         STATE_CHANGE_EXT_ADV                            BIT(1)
#define         STATE_CHANGE_LEG_SCAN                           BIT(2)
#define         STATE_CHANGE_EXT_SCAN                           BIT(3)
#define         STATE_CHANGE_INIT                               BIT(4)
#define         STATE_CHANGE_PRD_ADV                            BIT(5)
#define         STATE_CHANGE_PDA_SYNC                           BIT(6)
#define         STATE_CHANGE_PAWR_SYNC                          BIT(7)

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #define     STATE_CHANGE_ACL_SNIFS                          BIT(8)
    #define     STATE_CHANGE_ACL_SNIFM                          BIT(9)
#endif


_attribute_aligned_(4)
typedef struct {
    u8      macAddress_public[6];
    u8      macAddress_random[6];   //host may set this

    /* dynamically record identity address for all connections, including IDA from legacy ADV/extended ADV/legacy INIT/extended INIT
       attention that same potential risk as "conn_req_info": two connections very close, first connection callback event blocked by mainloop */
    u8      idenAdr_cur_addr[6];
    u8      idenAdr_cur_type;
}ll_mac_t;
extern _attribute_aligned_(4)  ll_mac_t  bltMac;




typedef union {
    struct{
        u8      leg_scan_en;     //legacy scan
        u8      leg_init_en;     //legacy initiate
        u8      ext_scan_en;     //extended scan
        u8      ext_init_en;     //extended initiate
    };
    struct{
        u16     leg_scan_init_en; //legacy scan or legacy initiate
        u16     ext_scan_init_en; //extended scan or extended initiate
    };
    u32 scn_init_en_pack;
}scn_init_en_t;

_attribute_aligned_(4)
typedef struct {

//0x00
    scn_init_en_t       scanInitEn_union;
    
    u16     state_chng;      //state change, set to 1 can only be execute in main_loop !!!
    u8      pda_syncing_flg;
    u8      rf_fsm_busy;   //RF state machine busy
    
    u8      create_connection;
    u8      connUptCmd_pending;
    u8      new_conn_forbidden;
    u8      connUpdHighAuthorityDis;

    u16     bisSyncRfLenMax;
    u8      newConn_forbidden_master;
    u8      newConn_forbidden_slave;
    
//0x10
    u8      sche_run_flag;
    u8      sdk_mainloop_flg;
    u8      max_master_num;
    u8      max_slave_num;
    
    u8      cur_master_num;
    u8      cur_slave_num;
    u8      connectEvt_mask;
    u8      disconnEvt_mask;
    
    u8      conupdtEvt_mask;  //conn_update
    u8      phyupdtEvt_mask;
    u8      getP256pubKeyEvtPending;
    u8      getP256pubKeystatus;
    
    u8      generateDHkeyEvtPending;
    u8      generateDHkeyStatus;
    u8      big_sche_build_pending;
    u8      perd_adv_sche_build_pending;
    
//0x20
    u8      acl_packet_length;  //7.8.2 LE Read Buffer Size command [v1]
    u8      hci_cmd_mask;
    u8      cig_slv_1st_sche_build_pending;
    u8      cig_mas_1st_sche_build_pending;
    
    u8      cis_create_pending;     //TODO: improve, u8 support 8 CIS most
    u8      delay_clear_rf_status;
    u8      cache_txfifo_used;
    u8      phytest_en;
    
    u8      legadv_en_strategy;
    u8      min_tolerance_us;   // < 256uS
    u8      dftTxPwrLvlIdx; //default used TX power level index ,used by HW
    s8      dftTxPwrLvl; //default used TX power level, 9~-23
    
    s8      rssiMin;
    u8      subrateUpdtEvt_mask;
    u8      ext_adv_en; //for Extended ADV,not use 1 bit like legacy adv. because need to consider advertising set.now stack support 4 advertising sets.
    u8      u8_rsvd0[1];
    
//0x30
    /* attention: variable below can only use 1 or 0, for "blc_hci_read Local Supported Commands" */
    u8      leg_adv_en      : 1;      //legacy ADV
    u8      stimer_irq_process_en : 1;
    u8      extScanModule_en: 1;
    u8      extInitModule_en: 1;
    u8      standard_hci_en : 1;
	u8		phy_test_en 	: 1;
    u8      reserved            : 3;
    
    u8      phy_2mCoded_en  : 1;
    u8      acl_master_en   : 1;
    u8      acl_slave_en    : 1;
    u8      cis_en          : 1;
    u8      cis_cen_en      : 1;
    u8      cis_per_en      : 1;
    u8      big_bcst_en     : 1;
    u8      big_sync_en     : 1;

    u8      bis_en          : 1;
    u8      iso_en          : 1;
    u8      iso_tx_en       : 1;
    u8      iso_rx_en       : 1;
    u8      pwr_ctrl_en     : 1; //PCL
    u8      past_en         : 1; //PAST feature
    u8      chncSup_en      : 1; //Classification feature
    u8      subrate_en      : 1;

    u8      pda_sync_en     : 1; //PERIODIC_ADVERTISING_SYNC feature
    u8      cte_connLess_en : 1; //Connectionless CTE feature
    u8      extAdvModule_en : 1;
    u8      prdAdvModule_en : 1;
    u8      advCodeingSel_en: 1;
    u8      prdAdvWr_en     : 1; //PAwR-Advertiser feature
    u8      prdSyncWr_en    : 1; //PAwR-Scanner feature
    u8      rfu_en          : 1;
    /* attention: variable above can only use 1 or 0, for "blc_hci_read Local Supported Commands" */

    u32     dly_start_tick_clr_rf_sts;
    u32     cis_1st_anchor_bSlot;

} st_ll_para_t;

extern _attribute_aligned_(4)   st_ll_para_t  blmsParam;


#if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
#define     SYNCINFOR_NOT_CHANGE                        0
#define     SYNCINFOR_VAILD_PENDING                     1
#define     SYNCINFOR_VAILD_COMPLETE                    2

#define     ACAD_NOT_CHANGE                             0
#define     ACAD_VALID_PENDING                          1
#define     ACAD_VALID_COMPLETE                         2


_attribute_aligned_(4)
typedef struct {
    u8 bigTask_timingStart;
    u8 syncInfor_changeCtrl;
    u8 acadInfor_changeCtrl;
    u8 rsvd0_u8;

    u32 auxAdv_sendNum;
    u32 pdaAdv_sendNum;

}bigExtAuxPda_conflictCtrl_t;

extern _attribute_aligned_(4) bigExtAuxPda_conflictCtrl_t bigExtAuxPda_conflictCtrl;
#endif



_attribute_aligned_(4)
typedef struct {
    u32 rx_irq_tick;
    u32 rx_header_tick;
    u32 rx_timeStamp;
    u8  crc_correct;
} rx_pkt_sts_t;

extern _attribute_aligned_(4)   rx_pkt_sts_t  bltRxPkt;




_attribute_aligned_(4)
typedef struct {
    u32     ll_aclRxFifo_set    : 1;
    u32     ll_aclTxMasFifo_set : 1;
    u32     ll_aclTxSlvFifo_set : 1;
    u32     ll_aclTxCacheFifo_set : 1;
    u32     hci_aclRxFifo_set   : 1;
    u32     hci_isoRxFifo_set   : 1;
    u32     ll_cisRxFifo_set    : 1;
    u32     ll_cisTxFifo_set    : 1;
    u32     ll_cisRxEvtFifo_set : 1;
    u32     ll_bisRxEvtFifo_set : 1;
    u32     ll_bisTxFifo_set    : 1;
    u32     ll_bisRxFifo_set    : 1;
    u32     rfu                 : 20; //attention !!!
}st_ll_temp_para_t;

extern _attribute_aligned_(4)   st_ll_temp_para_t  bltempParam;


extern  volatile    u64 blms_state;
extern  volatile    int blm_btxbrx_state;
extern  const u8  blms_tx_empty_packet[];
extern  volatile    u64 systick_irq_trigger;

extern  volatile u8     blc_adv_chn_ext_sel;
extern  volatile u8     blc_legadv_channel[];
extern  volatile u8     blc_extadv_channel[];
#if (LL_ASYNC_LEA_EN)
extern  volatile u8     blc_asyncAdv_channel[];
#endif


typedef     int (*ll_host_mainloop_callback_t)(void);
typedef     int (*ll_enc_done_callback_t)(u16 connHandle, u8 status, u8 enc_enable);
typedef     int (*ll_enc_pause_callback_t)(u16 connHandle);

typedef     int (*ll_conn_complete_handler_t)(u16 conn, u8 *p);
typedef     int (*ll_conn_terminate_handler_t)(u16 conn, u8 *p);
typedef     int (*blc_main_loop_phyTest_callback_t)(void);
typedef     int (*ll_iso_test_callback_t)(int flag, u16 handle, void *arg);

extern  ll_host_mainloop_callback_t             ll_host_main_loop_cb;
extern  ll_enc_done_callback_t                  ll_encryption_done_cb;
extern  ll_enc_pause_callback_t                 ll_encryption_pause_cb;
extern  ll_conn_complete_handler_t              ll_connComplete_handler;
extern  ll_conn_terminate_handler_t             ll_connTerminate_handler;
extern  blc_main_loop_phyTest_callback_t        blc_main_loop_phyTest_cb;



typedef     int (*ll_task_callback_t)(int);
typedef     int (*ll_task_callback_2_t)(int, void*p);
typedef     int (*ll_task_callback_3_t)(int, void*p0, void*p1);

typedef     int (*ll_ext_scan_truncate_pend_t)(void);

typedef     int (*hci_cmd_callback_t)(int, void *, void *);

typedef     int (*ll_prd_sync_pawr_sync_common_t)(sync_info_t* p0, unsigned char* p1);


extern  ll_task_callback_t                      ll_acl_conn_irq_task_cb;
extern  ll_task_callback_2_t                    ll_acl_conn_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_acl_slave_irq_task_cb;
extern  ll_task_callback_2_t                    ll_acl_master_irq_task_cb;

extern  ll_task_callback_2_t                    ll_cis_conn_irq_task_cb;
extern  ll_task_callback_2_t                    ll_cis_conn_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_cig_mst_irq_task_cb;
extern  ll_task_callback_2_t                    ll_cig_mst_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_cis_slv_irq_task_cb;
extern  ll_task_callback_t                      ll_cis_slv_mlp_task_cb;


extern  ll_task_callback_2_t                    ll_leg_adv_irq_task_cb;
extern  ll_task_callback_t                      ll_leg_adv_mlp_task_cb;
extern  ll_task_callback_2_t                    ll_ext_adv_irq_task_cb;
extern  ll_task_callback_2_t                    ll_ext_adv_mlp_task_cb;

extern  ll_task_callback_t                      ll_prichn_scan_irq_task_cb;

extern  ll_task_callback_t                      ll_leg_scan_mlp_task_cb;
extern  ll_task_callback_2_t                    ll_ext_scan_irq_task_cb;
extern  ll_task_callback_t                      ll_ext_scan_mlp_task_cb;


extern  ll_task_callback_t                      ll_init_mlp_task_cb;
extern  ll_task_callback_t                      ll_ext_init_irq_task_cb;

extern  ll_task_callback_2_t                    ll_prd_adv_irq_task_cb;
extern  ll_task_callback_2_t                    ll_prd_adv_mlp_task_cb;
extern  ll_task_callback_2_t                    ll_pawra_sub_irq_task_cb;
extern  ll_task_callback_2_t                    ll_pawra_rsp_irq_task_cb;
extern  ll_task_callback_2_t                    ll_pawra_mlp_task_cb;
extern  ll_task_callback_2_t                    ll_pda_sync_irq_task_cb;
extern  ll_task_callback_t                      ll_pda_sync_mlp_task_cb;

extern  ll_task_callback_3_t                    ll_pawr_sync_sub_irq_task_cb;
extern  ll_task_callback_t                      ll_pawr_sync_mlp_task_cb;
extern  ll_task_callback_t                      ll_pawr_sync_rspTx_irq_task_cb;

extern  ll_prd_sync_pawr_sync_common_t          ll_pda_sync_pawr_sync_common_cb;

extern  ll_task_callback_2_t                    ll_big_bcst_irq_task_cb;
extern  ll_task_callback_2_t                    ll_big_bcst_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_big_sync_irq_task_cb;
extern  ll_task_callback_2_t                    ll_big_sync_mlp_task_cb;

extern  ll_task_callback_t                      ll_secchn_scan_task_cb;

extern  ll_task_callback_t                      ll_aoa_aod_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_cis_map_update_cb;



typedef     int (*ll_rpa_tmo_mainloop_callback_t)(void);
typedef     int (*ll_isoal_mainloop_callback_t)(int flag);



typedef     int (*ll_irq_rx_callback_t)(void);

extern  ll_irq_rx_callback_t                    ll_irq_scan_rx_pri_chn_cb;
extern  ll_irq_rx_callback_t                    ll_irq_scan_rx_sec_chn_cb;


extern  ll_task_callback_2_t                    ll_chn_sounding_irq_task_cb;
extern  ll_task_callback_2_t                    ll_chn_sounding_mlp_task_cb;

extern  ll_task_callback_t                      ll_cs_initiator_irq_task_cb;
extern  ll_task_callback_t                      ll_cs_reflector_irq_task_cb;


#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    extern  ll_task_callback_t                  ll_acl_sniffer_slv_irq_task_cb;
    extern  ll_task_callback_t                  ll_acl_sniffer_slv_mlp_task_cb;
#endif

#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    extern  ll_task_callback_t                  ll_acl_sniffer_mst_irq_task_cb;
    extern  ll_task_callback_t                  ll_acl_sniffer_mst_mlp_task_cb;
#endif


void        blt_ll_registerHostMainloopCallback (ll_host_mainloop_callback_t cb);
void        blt_ll_registerConnectionEncryptionDoneCallback(ll_enc_done_callback_t cb);
void        blc_ll_registerConnectionEncryptionPauseCallback(ll_enc_pause_callback_t cb);
void        blt_ll_registerConnectionCompleteHandler(ll_conn_complete_handler_t  handler);
void        blt_ll_registerConnectionTerminateHandler(ll_conn_terminate_handler_t  handler);



u8          blt_ll_getCurrentState(void);
u8          blt_ll_getOwnAddrType(u16 connHandle);
u8*         blt_ll_getOwnMacAddr(u16 connHandle, u8 addr_type);

ble_sts_t   blc_ll_genRandomNumber(u8 *dest, u32 size);
ble_sts_t   blc_ll_encryptedData(u8*key, u8*plaintextData, u8* encryptedTextData);

void        blt_extAdv_terminateEvt(u8 connHandle, u8 advHandle, u8 terminateEvtNum, u8 connState);



/******************************* ll end ************************************************************************/






/******************************* llms_slot start ******************************************************************/

#define         SLOT_UPDT_SLAVE_CONN_CREATE                     BIT(0)
#define         SLOT_UPDT_SLAVE_CONN_UPDATE                     BIT(1)
#define         SLOT_UPDT_SLAVE_SYNC_DONE                       BIT(2)
#define         SLOT_UPDT_SLAVE_SSLOT_ADJUST                    BIT(3)

#define         SLOT_UPDT_SLAVE_CONNUPDATE_FAIL                 BIT(4)

#define         SLOT_UPDT_MASTER_CONN                           BIT(6)
#define         SLOT_UPDT_CONN_TERMINATE                        BIT(7)

#define         SLOT_UPDT_CIS_MASTER_CREATE                     BIT(8)
#define         SLOT_UPDT_CIS_MASTER_REMOVE                     BIT(10)
#define         SLOT_UPDT_CIS_MASTER_CHANGE                     BIT(11)
#define         SLOT_UPDT_CIS_SLAVE_TERMINATE                   BIT(12)

#define         SLOT_UPDT_SLOTTBL_RESCHED                       BIT(14)
#define         SLOT_UPDT_BIS_BCST_CREATE                       BIT(15)
#define         SLOT_UPDT_BIS_BCST_REMOVE                       BIT(16)
#define         SLOT_UPDT_BIS_BSYNC_CREATE                      BIT(17)
#define         SLOT_UPDT_BIS_BSYNC_REMOVE                      BIT(18)

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    #define     SLOT_UPDT_SNIF_SEEK_CREATE                      BIT(20)
    #define     SLOT_UPDT_SNIF_SEEK_FAIL                        BIT(21)
#endif

#define         SLOT_UPDT_EXT_SCAN_DISABLE                      BIT(25)
#define         SLOT_UPDT_ADV_SCAN_STATE_CHANGE                 BIT(26)
#define         SLOT_UPDT_EXT_ADV_DISABLE                       BIT(27)

#define         SLOT_UPDT_SLAVE_SUBRATE_STATE_CHANGE            BIT(28)
#define         SLOT_UPDT_MASTER_SUBRATE_STATE_CHANGE           BIT(29)

#define         SLOT_UPDT_CHANNEL_SOUNDING_STATE_CHANGE         BIT(30)




// 150us(T_ifs) + 352us(conn_req) = 502 us,  sub some margin 22(RX IRQ delay/irq_handler delay)
// real test data: 470 uS,  beginning is "u32 tick_now = rx_tick_now = clock_time();" in irq_acl_conn_rx
//                          ending is    "while( !HAL_GET_RF_TX_IRQ);" in
//                          "irq_handler" to "u32 tick_now = rx_tick_now = clock_time();" is 4 uS
#define         PKT_INIT_AFTER_RX_TICK                          ( 480 *SYSTEM_TIMER_TICK_1US)

//scan_req(12B)+150us+scan_rsp(6+31) = (12+10)*8 + 150 + (37+10)*8;;;10 = preamble+accessCode+mic+crc
#define         ACTIVE_SCAN_MAX_TICK                           (720*SYSTEM_TIMER_TICK_1US)//( 702*SYSTEM_TIMER_TICK_1US)


#ifndef HIGH_BANDWIDTH_BIS_ONLY
#define HIGH_BANDWIDTH_BIS_ONLY                                     0
#endif

// master: 30    mS * 4 , slave:  8.75 mS/10   mS/18.75 mS  276 uS, 20190921
// master: 30    mS * 4 , slave:  7.5  mS/10   mS/18.75 mS  310 uS, 20190921
// master: 31.25 mS * 4 , slave:  7.5  mS/10   mS/18.75 mS  339 uS, 20190921
#if HIGH_BANDWIDTH_BIS_ONLY
#define         SLOT_PROCESS_MAX_US                             50
#define         SLOT_PROCESS_MAX_TICK                           ( 50 *SYSTEM_TIMER_TICK_1US)//( 400 *SYSTEM_TIMER_TICK_1US)
#define         SLOT_PROCESS_MAX_SSLOT_NUM                      3 //100uS -> 5.12 sSlot
#else
#define         SLOT_PROCESS_MAX_US                             400
#define         SLOT_PROCESS_MAX_TICK                           ( 400 *SYSTEM_TIMER_TICK_1US)//( 400 *SYSTEM_TIMER_TICK_1US)
#define         SLOT_PROCESS_MAX_SSLOT_NUM                      21 //400uS -> 20.48 sSlot
#endif

//TODO(SiHui): optimize later, not use a constant value, use a variable which relative with how many task alive now



// test data 88 uS 20191014 by SiHui, consider application layer flash read or flash write timing, need add some margin
#define         SCAN_BOUNDARY_MARGIN_COMMON_TICK                ( 100 *SYSTEM_TIMER_TICK_1US + SLOT_PROCESS_MAX_TICK)
#define         SCAN_BOUNDARY_MARGIN_INIT_TICK                  ( PKT_INIT_AFTER_RX_TICK + SLOT_PROCESS_MAX_TICK)     // initiate timing + slot_update_rebuild_allocate running potential biggest time




#define         BOUNDARY_RX_RELOAD_TICK                         0  //new design, abandon all boundary RX(Eagle RX dma_len rewrite problem)



#define         SLOT_INDEX_INIT                                 0
#define         SLOT_INDEX_ALARM_LOW                            BIT(31)     //15 Days
#define         SLOT_INDEX_ALARM_HIGH                           0xFFFF0000  //30 Days        //(BIT(31)|BIT(30))   //23 Days





/*
 * NOENT: no encryption
 * ENCRT: encryption
 *
 *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *
 *  Coded PHY :    376 + (N*8+27)*S
 *               = 376 + ((rf_len+2)*8+27)*S
 *               = 376 + (rf_len*8+43)*S        // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
 *               = rf_len*S*8 + 43*S + 376
 *      S2    :  = rf_len*16 + 462
 *      S8    :  = rf_len*64 + 720
 */

#define         PAYLOAD_27B_NOENT_1MPHY_US                          296   // (27 + 10) * 8 = 296
#define         PAYLOAD_27B_NOENT_1MPHY_SSLOT_NUM                   15    // 296/19.53 = 15.15

#define         PAYLOAD_27B_ENCRT_1MPHY_US                          328   // (31 + 10) * 8 = 328

#define         PAYLOAD_27B_NOENT_2MPHY_US                          152   // (27 + 11) * 4 = 152
#define         PAYLOAD_27B_NOENT_2MPHY_SSLOT_NUM                   8     // 152/19.53 = 7.78

#define         PAYLOAD_27B_ENCRT_2MPHY_US                          168   // (31 + 11) * 4 = 168

#define         PAYLOAD_27B_NOENT_CODED_S2_US                       894   // 27*16 + 462 = 432 + 462 = 894
#define         PAYLOAD_27B_ENCRT_CODED_S2_US                       958   // 31*16 + 462 = 496 + 462 = 958

#define         PAYLOAD_27B_NOENT_CODED_S8_US                       2448  // 27*64 + 462 = 1728 + 720 = 2448
#define         PAYLOAD_27B_NOENT_CODED_S8_SSLOT_NUM                125   // 2448/19.53 = 125.34

#define         PAYLOAD_27B_ENCRT_CODED_S8_US                       2704  // 31*64 + 462 = 1984 + 720 = 2704



#define         PAYLOAD_27B_TIFS_27B_NOENT_1MPHY_US                 742   // (296*2 + 150) = 592 + 150 = 742
#define         PAYLOAD_27B_TIFS_27B_NOENT_1MPHY_SSLOT_NUM          40    // 806/19.53 = 37.99
#define         PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_US                 806   // (328*2 + 150) = 656 + 150 = 806
#define         PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_SSLOT_NUM          41    // 806/19.53 = 41.27

#define         PAYLOAD_27B_TIFS_27B_NOENT_2MPHY_US                 454   // (152*2 + 150) = 304 + 150 = 454
#define         PAYLOAD_27B_TIFS_27B_NOENT_2MPHY_SSLOT_NUM          23    // 454/19.53 = 23.24
#define         PAYLOAD_27B_TIFS_27B_ENCRT_2MPHY_US                 486   // (168*2 + 150) = 336 + 150 = 486
#define         PAYLOAD_27B_TIFS_27B_ENCRT_2MPHY_SSLOT_NUM          25    // 486/19.53 = 24.88



#define         PAYLOAD_27B_TIFS_27B_NOENT_CODED_S2_US              1938   // (894*2 + 150) = 1788 + 150 = 1938
#define         PAYLOAD_27B_TIFS_27B_NOENT_CODED_S2_SSLOT_NUM       99    // 1938/19.53 = 99.23
#define         PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S2_US              2066   // (958*2 + 150) = 1916 + 150 = 2066
#define         PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S2_SSLOT_NUM       106    // 2066/19.53 = 105.78

#define         PAYLOAD_27B_TIFS_27B_NOENT_CODED_S8_US              5046   // (2448*2 + 150) = 4896 + 150 = 5046
#define         PAYLOAD_27B_TIFS_27B_NOENT_CODED_S8_SSLOT_NUM       258    // 5046/19.53 = 258.37
#define         PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_US              5558   // (2704*2 + 150) = 5408 + 150 = 5558
#define         PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_SSLOT_NUM       284    // 5558/19.53 = 284.58


#define         PAYLOAD_27B_TIFS_EMPTY_NOENT_CODED_S8_US            3318   // (2448 + 150 + 720) = 3318
#define         PAYLOAD_27B_TIFS_EMPTY_ENCRT_CODED_S8_US            5558   // (2704 + 150 + 720) = 3514


extern const u16 pdu_27b_tifs_27b_us[3][2];
extern const u16 pdu_27b_tifs_27b_sslot[3][2];

/******************************* llms_slot end ********************************************************************/









/******************************* ll_schedule start ******************************************************************/

/* 20230720 by SiHui/QiuWei/HaoJie
 * for stuck code FF0A, SiHui thought if error US is very small(e.g. 10uS), set a post system tick for capture value,
 * no need stuck, STimer IRQ will trigger, next task will occur, and all task can endure some time delay margin.
 * But QiuWei propose that the STimer of a post capture value may not trigger IRQ, then it will wait for whole timer round,
 * e.g. B92, 24M clock, about 3 minute.
 * (1). HaoJie have a test environment, can trigger FF0A in about 10 minute. We add some debug, find that set post capture value
 * (FF0A happen, but do not stuck code) make about 3 minute waiting time for scheduler.  It seems that QiuWei is right.
 * (2). refer to SiHui's mail "System Timer/System Timer IRQ Capture" in 20191014. We know that for Kite, a post capture
 * value in a STimer IRQ handler can not trigger new IRQ quickly. QiuWei tested that Eagle is same situation. We can solve this
 * problem by using same method described in email. Actually, all B85m/B91m and later IC need consider this special problem.
 * (3) For HaoJie's environment, do a longer time test after code fix this problem. No problem in about one hour, we think that
 * this code fixing is valid.
 */
#ifndef FIX_STIMER_SET_CAPTURE_ERR
#define FIX_STIMER_SET_CAPTURE_ERR                              1
#endif



#define         SCHE_PRE_ALLOCATE_LEN_80MS                      1
#define         SCHE_PRE_ALLOCATE_LEN_120MS                     2
#define         SCHE_PRE_ALLOCATE_LEN_160MS                     3
#define         SCHE_PRE_ALLOCATE_LEN_240MS                     4

#if(LL_BIS_SNC_BV18C_BN6)
    #define         SCHE_PRE_ALLOCATE_MAX_LEN                   SCHE_PRE_ALLOCATE_LEN_240MS
#endif


#ifndef         SCHE_PRE_ALLOCATE_MAX_LEN
#define         SCHE_PRE_ALLOCATE_MAX_LEN                       SCHE_PRE_ALLOCATE_LEN_80MS
#endif


#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define     SCHE_PRE_ALLOCATE_BSLOT_NUM                     128
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    #define     SCHE_PRE_ALLOCATE_BSLOT_NUM                     192
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    #define     SCHE_PRE_ALLOCATE_BSLOT_NUM                     256
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    #define     SCHE_PRE_ALLOCATE_BSLOT_NUM                     384
#else
    #error "unsupported scheduler length!!!"
#endif


#ifndef         MAX_CONFLICT_NUM
#define         MAX_CONFLICT_NUM                                4
#endif



//task flag
//attention: can not be 0, 0 have other use
#define         TSKFLG_ACL_MASTER                               1
#define         TSKFLG_ACL_SLAVE                                2
#define         TSKFLG_CIG_MST                                  3
#define         TSKFLG_CIG_SLV                                  4
#define         TSKFLG_LEG_ADV                                  5
#define         TSKFLG_EXT_ADV                                  6
#define         TSKFLG_AUX_ADV                                  7
#define         TSKFLG_PERD_ADV                                 8
#define         TSKFLG_PRICHN_SCAN                              9       //primary channel scan
#define         TSKFLG_SECCHN_SCAN                              10      //secondary channel scan
#define         TSKFLG_PDA_SYNC                                 11
#define         TSKFLG_BIG_BCST                                 12
#define         TSKFLG_BIG_SYNC                                 13

#define         TSKFLG_PAWR_ADV                                 TSKFLG_PERD_ADV //PAwR-subevent_0 -t task
#define         TSKFLG_PAWRA_SUB                                14              //PAwR-subevent_n -t task
#define         TSKFLG_PAWRA_RSP                                15              //PAwR-rsp_slots task

#define         TSKFLG_PAWRS_SUB                                16
#define         TSKFLG_PAWRS_RSP                                17

#define         TSKFLG_CS                                       18 //channel sounding

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #define     TSKFLG_SNIFM_SEEK                               19 //sniffer monitor central
    #define     TSKFLG_SNIFS_SEEK                               20 //sniffer monitor peripheral
#endif

#define         TSKFLG_SCAN_ALIGN                               30


#define         TSKFLG_VALID_MASK                               0x7F
#define         TSKFLG_BSLOT_ALIGN                              BIT(7)

/*******************************************************************************
    00 ~ 03 :  ACL master
    04 ~ 07 :  ACL slave
    08      :  CIG master
    09 ~ 10 :  CIG slave
    11      :  Leg ADV
    12 ~ 15 :  Ext_ADV      0x2000 0x1000 0x080
    16 ~ 19 :  Aux_ADV
    20 ~ 21 :  Periodic ADV
    22 ~ 23 :  PAwR ADV Subevent
    24 ~ 25 :  PAwR ADV Rsp
    26      :  Primary channel Scan(leg_scan & Ext_Scan)
    27 ~ 30 :  secondary channel Scan
    31 ~ 34 :  Periodic ADV Sync
    35 ~ 36 :  BIS Bcst
    37      :  BIS Sync
    38 ~ 39 :  PAwR Sync Subevent
    40 ~ 41 :  PAwR Sync Rsp
    42 ~ 43 :  Channel Sounding
 ******************************************************************************/
//max task number
#define         TSKNUM_ACL_MASTER                               LL_MAX_ACL_CEN_NUM
#define         TSKNUM_ACL_SLAVE                                LL_MAX_ACL_PER_NUM

#define         TSKNUM_CIG_MST                                  1
#define         TSKNUM_CIG_SLV                                  2

#define         TSKNUM_LEG_ADV                                  1
#define         TSKNUM_EXT_ADV                                  4
#define         TSKNUM_AUX_ADV                                  4
#define         TSKNUM_PERD_ADV                                 2
#define         TSKNUM_PAWRA_SUB                                2 //for PAwR-Advertiser
#define         TSKNUM_PAWRA_RSP                                2 //for PAwR-Advertiser
#define         TSKNUM_PRICHN_SCAN                              1


#define         TSKNUM_SECCHN_SCAN                              4 //for multiple set LL/DDI/SCN/BV-25-C
#define         TSKNUM_PDA_SYNC                                 2 //PERIODIC_ADV_SYNC
#define         TSKNUM_BIG_BCST                                 2
#define         TSKNUM_BIG_SYNC                                 1

#define         TSKNUM_PAWRS_SUB                                2 //must be same as TSKNUM_PDA_SYNC
#define         TSKNUM_PAWRS_RSP                                2

#define         TSKNUM_CS                                       2


#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #define     TSKNUM_SNIFM_SEEK                               (LL_RSSI_SNIFFER_MASTER_ENABLE ? LL_MAX_ACL_CEN_NUM : 0)
    #define     TSKNUM_SNIFS_SEEK                               (LL_RSSI_SNIFFER_SLAVE_ENABLE ? LL_MAX_ACL_PER_NUM : 0)

    #define     TSKNUM_MAX                                      (TSKNUM_ACL_MASTER + TSKNUM_ACL_SLAVE + TSKNUM_CIG_MST + \
                                                                 TSKNUM_CIG_SLV + TSKNUM_LEG_ADV + TSKNUM_EXT_ADV + TSKNUM_AUX_ADV + \
                                                                 TSKNUM_PERD_ADV + TSKNUM_PAWRA_SUB + TSKNUM_PAWRA_RSP + TSKNUM_PAWRS_SUB + TSKNUM_PAWRS_RSP +\
                                                                 TSKNUM_PRICHN_SCAN + TSKNUM_SECCHN_SCAN + TSKNUM_PDA_SYNC + \
                                                                 TSKNUM_BIG_BCST + TSKNUM_BIG_SYNC +TSKNUM_PAWRS_SUB + TSKNUM_PAWRS_RSP + TSKNUM_CS + \
                                                                 TSKNUM_SNIFM_SEEK + TSKNUM_SNIFS_SEEK )
#else
    #define     TSKNUM_MAX                                      (TSKNUM_ACL_MASTER + TSKNUM_ACL_SLAVE + TSKNUM_CIG_MST + \
                                                                 TSKNUM_CIG_SLV + TSKNUM_LEG_ADV + TSKNUM_EXT_ADV + TSKNUM_AUX_ADV + \
                                                                 TSKNUM_PERD_ADV + TSKNUM_PAWRA_SUB + TSKNUM_PAWRA_RSP + TSKNUM_PAWRS_SUB + TSKNUM_PAWRS_RSP +\
                                                                 TSKNUM_PRICHN_SCAN + TSKNUM_SECCHN_SCAN + TSKNUM_PDA_SYNC + \
                                                                 TSKNUM_BIG_BCST + TSKNUM_BIG_SYNC +TSKNUM_PAWRS_SUB + TSKNUM_PAWRS_RSP + TSKNUM_CS)
#endif

//task offset
#define         TSKOFT_ACL_CONN                                 0
#define         TSKOFT_ACL_MASTER                               0                                               //00~03
#define         TSKOFT_ACL_SLAVE                                ( TSKNUM_ACL_MASTER )                           //04~07
#define         TSKOFT_CIG_MST                                  ( TSKOFT_ACL_SLAVE      + TSKNUM_ACL_SLAVE )    //08~08
#define         TSKOFT_CIG_SLV                                  ( TSKOFT_CIG_MST        + TSKNUM_CIG_MST )      //09~10
#define         TSKOFT_LEG_ADV                                  ( TSKOFT_CIG_SLV        + TSKNUM_CIG_SLV )      //11
#define         TSKOFT_EXT_ADV                                  ( TSKOFT_LEG_ADV        + TSKNUM_LEG_ADV )      //12~15
#define         TSKOFT_AUX_ADV                                  ( TSKOFT_EXT_ADV        + TSKNUM_EXT_ADV )      //16~19
#define         TSKOFT_PERD_ADV                                 ( TSKOFT_AUX_ADV        + TSKNUM_AUX_ADV )      //20~21
#define         TSKOFT_PAWRA_SUB                                ( TSKOFT_PERD_ADV       + TSKNUM_PERD_ADV )     //22~23
#define         TSKOFT_PAWRA_RSP                                ( TSKOFT_PAWRA_SUB      + TSKNUM_PAWRA_SUB )    //24~25
#define         TSKOFT_PRICHN_SCAN                              ( TSKOFT_PAWRA_RSP      + TSKNUM_PAWRA_RSP )    //26~27
#define         TSKOFT_SECCHN_SCAN                              ( TSKOFT_PRICHN_SCAN    + TSKNUM_PRICHN_SCAN )  //28~28
#define         TSKOFT_PDA_SYNC                                 ( TSKOFT_SECCHN_SCAN    + TSKNUM_SECCHN_SCAN )  //29~31
#define         TSKOFT_BIG_BCST                                 ( TSKOFT_PDA_SYNC       + TSKNUM_PDA_SYNC )     //32~33
#define         TSKOFT_BIG_SYNC                                 ( TSKOFT_BIG_BCST       + TSKNUM_BIG_BCST )     //34-34
#define         TSKOFT_PAWRS_SUB                                ( TSKOFT_BIG_SYNC       + TSKNUM_BIG_SYNC )     //35~36
#define         TSKOFT_PAWRS_RSP                                ( TSKOFT_PAWRS_SUB      + TSKNUM_PAWRS_SUB)     //37~39
#define         TSKOFT_CS                                       ( TSKOFT_PAWRS_RSP      + TSKNUM_PAWRS_RSP)     //40~42

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
#define         TSKOFT_SNIFM_SEEK                               ( TSKOFT_CS             + TSKNUM_CS )           //43~46
#define         TSKOFT_SNIFS_SEEK                               ( TSKOFT_SNIFM_SEEK     + TSKNUM_SNIFM_SEEK )   //47~50
#endif


//task mask begin
#define         TSKMSK_ACL_CONN_0                               BIT(0)
#define         TSKMSK_ACL_MASTER_0                             BIT(0)
#define         TSKMSK_ACL_SLAVE_0                              BIT(LL_MAX_ACL_CEN_NUM)
#define         TSKMSK_CIG_MASTER_0                             BIT(TSKOFT_CIG_MST)
#define         TSKMSK_CIG_SLAVE_0                              BIT(TSKOFT_CIG_SLV)
#define         TSKMSK_EXT_ADV_0                                BIT(TSKOFT_EXT_ADV)
#define         TSKMSK_AUX_ADV_0                                BIT(TSKOFT_AUX_ADV)
#define         TSKMSK_PERD_ADV_0                               BIT(TSKOFT_PERD_ADV)
#define         TSKMSK_PAWRA_SUB_0                              BIT(TSKOFT_PAWRA_SUB) //pawr
#define         TSKMSK_PAWRA_RSP_0                              BIT(TSKOFT_PAWRA_RSP) //pawr
#define         TSKMSK_SECCHN_SCAN_0                            BIT(TSKOFT_SECCHN_SCAN)
#define         TSKMSK_PDA_SYNC_0                               BIT(TSKOFT_PDA_SYNC)
#define         TSKMSK_BIG_BCST_0                               BIT(TSKOFT_BIG_BCST)
#define         TSKMSK_BIG_SYNC_0                               BIT(TSKOFT_BIG_SYNC)
#define         TSKMSK_PAWRS_SUB_0                              BIT(TSKOFT_PAWRS_SUB) //pawr sync
#define         TSKMSK_PAWRS_RSP_0                              BIT(TSKOFT_PAWRS_RSP)
#define         TSKMSK_CS_0                                     BIT(TSKOFT_CS)//channel sounding

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
#define         TSKMSK_SNIFM_SEEK_0                             BIT(TSKOFT_SNIFM_SEEK)
#define         TSKMSK_SNIFS_SEEK_0                             BIT(TSKOFT_SNIFS_SEEK)
#endif


//task mask
#define         TSKMSK_ACL_CONN_ALL                             ((1<<LL_MAX_ACL_CONN_NUM) - 1)
#define         TSKMSK_ACL_MASTER_ALL                           ((1<<LL_MAX_ACL_CEN_NUM) - 1)
#define         TSKMSK_ACL_SLAVE_ALL                            BIT_RNG(TSKOFT_ACL_SLAVE,       TSKOFT_CIG_MST - 1)
#define         TSKMSK_CIG_MASTER_ALL                           BIT_RNG(TSKOFT_CIG_MST,         TSKOFT_CIG_SLV - 1)
#define         TSKMSK_CIG_SLAVE_ALL                            BIT_RNG(TSKOFT_CIG_SLV,         TSKOFT_LEG_ADV - 1)
#define         TSKMSK_LEG_ADV                                  BIT(TSKOFT_LEG_ADV)  //task number is 1 forever
#define         TSKMSK_EXT_ADV_ALL                              BIT_RNG(TSKOFT_EXT_ADV,         TSKOFT_AUX_ADV - 1)    //0x00 00 f0 00
#define         TSKMSK_AUX_ADV_ALL                              BIT_RNG(TSKOFT_AUX_ADV,         TSKOFT_PERD_ADV - 1)   //0x00 0f 00 00
#define         TSKMSK_PERD_ADV_ALL                             BIT_RNG(TSKOFT_PERD_ADV,        TSKOFT_PAWRA_SUB - 1)  //0x00 30 00 00
#define         TSKMSK_PAWRA_SUB_ALL                            BIT_RNG(TSKOFT_PAWRA_SUB,       TSKOFT_PAWRA_RSP - 1)
#define         TSKMSK_PAWRA_RSP_ALL                            BIT_RNG(TSKOFT_PAWRA_RSP,       TSKOFT_PRICHN_SCAN - 1)
#define         TSKMSK_PRICHN_SCAN                              BIT(TSKOFT_PRICHN_SCAN) //task number is 1 forever       00 00 00 40
#define         TSKMSK_SECCHN_SCAN_ALL                          BIT_RNG(TSKOFT_SECCHN_SCAN,     TSKOFT_PDA_SYNC - 1) //00 00 00 78
#define         TSKMSK_PDA_SYNC_ALL                             BIT_RNG(TSKOFT_PDA_SYNC,        TSKOFT_BIG_BCST - 1)
#define         TSKMSK_BIG_BCST_ALL                             BIT_RNG(TSKOFT_BIG_BCST,        TSKOFT_BIG_SYNC - 1)
#define         TSKMSK_BIG_SYNC_ALL                             BIT_RNG(TSKOFT_BIG_SYNC,        TSKOFT_PAWRS_SUB - 1)
#define         TSKMSK_PAWRS_SUB_ALL                            BIT_RNG(TSKOFT_PAWRS_SUB,       TSKOFT_PAWRS_RSP - 1)
#define         TSKMSK_PAWRS_RSP_ALL                            BIT_RNG(TSKOFT_PAWRS_RSP,       TSKOFT_CS - 1)

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
#define         TSKMSK_CS_ALL                                   BIT_RNG(TSKOFT_CS,              TSKOFT_SNIFM_SEEK - 1)
#define         TSKMSK_SNIFM_SEEK_ALL                           BIT_RNG(TSKOFT_SNIFM_SEEK,      TSKOFT_SNIFS_SEEK - 1)
#define         TSKMSK_SNIFS_SEEK_ALL                           BIT_RNG(TSKOFT_SNIFS_SEEK,      TSKNUM_MAX - 1)
#else
#define         TSKMSK_CS_ALL                                   BIT_RNG(TSKOFT_CS,              TSKNUM_MAX - 1)
#endif




_attribute_aligned_(4)
typedef struct sch_task_t{
    //u16, 65536*20uS = 1.3S most;
    //s32, 65536*65536/2/625 = 3435973 unit most, 3435973*19.5uS=67 S most
    //     for Eagle,  SSLOT_TICK_NUM = 625/2,  65536*65536/2/625  = 3435973 unit most, 3435973*19.53=67 S most
    //     for Jaguar, SSLOT_TICK_NUM = 1875/4, 65536*65536/2/1875 = 1145324 unit most, 1145324*19.53=22.368 S most
    s32  begin; //right align
    s32  end;   //left align

    u8   scheTask_oft;  // 0~63, 6 bit enough
    u8   scheTask_idx;  //max 4 now, 4 master/4 slave/4 CIS, consider future 16 master, 16 or 32 ?
    u8   scheTask_flg;  //5 bit is enough
    u8   taskFifo_idx;  //used in ADV now

    u8   cover_other;  //cover other task
    u8   subrate_evt_flag;
    u8   pawr_last_subevt;
    u8   pawr_subevt_idx;

    u32 cs_subevent_seqNum;
    u16 cs_procCnt;
    u8  cs_oft;
    u8  rsvd8;

    struct sch_task_t  *next;
} sch_task_t;

extern sch_task_t   bltSche_header;







/********************************************************************************************
 * 0.625 mS slot
 * 0.000625 S * 2^32 = 0.000625 S*65536*65536 = 2684354 S = 745 Hours = 31 Days
 *******************************************************************************************/

//big   slot: 625 uS
//small slot: 625 uS/32 = 19.53uS

/* Duration time switch */
#define BSLOT_DUR_2_SSLOT_DUR(bSlot_dur)        ((bSlot_dur)<<5)
#define SSLOT_DUR_2_BSLOT_DUR(sSlot_dur)        ((sSlot_dur)>>5)
#define BSLOT_DUR_2_TICKS_DUR(bSlot_dur)        ((bSlot_dur)*SYSTEM_TIMER_TICK_625US)
#define SSLOT_DUR_2_TICKS_DUR(sSlot_dur)        ((sSlot_dur)*SSLOT_TICK_NUM)
#define TICKS_DUR_2_SSLOT_DUR(ticks_dur)        (((s32)(ticks_dur))*SSLOT_TICK_REVERSE)
#define TICKS_DUR_2_BSLOT_DUR(ticks_dur)        (((s32)(ticks_dur))/SYSTEM_TIMER_TICK_625US)
/* Absolute time switch */
#define BSLOT_ABS_2_SSLOT_ABS(bSlot_abs)        (((bSlot_abs) - bltSche.bSlot_idx_start)<<5)
#define SSLOT_ABS_2_BSLOT_ABS(sSlot_abs)        (bltSche.bSlot_idx_start + ((sSlot_abs)>>5))
#define SSLOT_ABS_2_TICKS_ABS(sSlot_abs)        (bltSche.sSlot_tick_start + (sSlot_abs)*SSLOT_TICK_NUM)
#define BSLOT_ABS_2_TICKS_ABS(bSlot_abs)        (bltSche.bSlot_tick_start + ((bSlot_abs) - bltSche.bSlot_idx_start)*SYSTEM_TIMER_TICK_625US)
//#define TICKS_ABS_2_SSLOT_ABS(ticks_abs)      (((u32)(ticks_abs) - bltSche.sSlot_tick_start)*SSLOT_TICK_REVERSE)
// only for Eagle and Jaguar,  Eagle's clock is 16M, Jaguar's clock is 24M, if want to support other chip, need to modify this definition;
#define TICKS_ABS_2_SSLOT_ABS(ticks_abs)        ((SYSTICK_NUM_PER_US == 16)? \
                                                    (((ticks_abs - bltSche.sSlot_tick_start)*2 + 313)/625):\
                                                    (((ticks_abs - bltSche.sSlot_tick_start)*4 + 938)/1875))

#define SSLOT_US_NUM                    625/32  //attention: not use "()" for purpose !!!
#define SSLOT_US_REVERSE                32/625  //attention: not use "()" for purpose !!!


_attribute_aligned_(4)
typedef struct{
//0x00
    u8  sSlot_idx_reset; //attention: 2 is special for CIS slave
    u8  addTsk_idx;      //task index of add task
    u8  build_index;
#if(!LL_BIS_SNC_BV18C_BN6)
    u8  bSlot_maxLen;
#else
    u8  rsvd0_u8;
    u32 bSlot_maxLen;
#endif

    u8  sSlot_diff_irq;
    u8  task_type_num;
    u8  sSlot_diff_next;
    u8  sche_process_en;

    u8  lklt_taskNum; //link list task number
    u8  abandon_taskNum;
    u8  insertTsk_flag;  //task flag of insert task
    u8  insertTsk_idx;   //task index of insert task

    u8  immediate_task; //not use now
    u8  recovered_task;
    u8  cal_time_en;
    u8  sche_pro_sslot_num;  //scheduler process sSlot number

    u8  task_rebuild;
    u8  u8_rsvd[3];

#if (DYNAMIC_SCHE_CAL_TIME_EN)
    //6~7
    u32  sche_tick_begin;
    u32  sche_process_us;
    #if (DBG_SCHE_CAL_TIME_EN)
        u32  sche_max_us;
    #endif
#endif

//8~13
    s32 sSlot_endIdx_dft;   // right align
    u32 update;
    u64 task_en;    //u32 => u64
    u64 task_mask;  //u32 => u64

//14~17
    u32 bSlot_idx_start;
    u32 bSlot_idx_next;
    u32 bSlot_idx_irq_real;
    u32 bSlot_tick_start;
//18~21
    u32 bSlot_tick_irq_real;
    s32 sSlot_idx_next;
    s32 sSlot_idx_past;
    s32 sSlot_idx_irq_real;

//22~25
    u32 sSlot_tick_start;
    u32 sSlot_tick_next;
    u32 sSlot_tick_irq;
    u32 sSlot_tick_irq_real;

//26~27
    u32 bSlot_endIdx_dft;   // right align
    s32 sSlot_endIdx_maxPri;// right align


//28~32
    u32 system_irq_tick;
    s32 lastTsk_endSslot;   //left align
    u32 lastTsk_endBslot;   //left align
    u32 lastTsk_endTick;    //not used now. end tick of last task on link_list
    u32 lklt_endTick;       //not used now

//33~36
    sch_task_t  *pTask_head;
    sch_task_t  *pTask_pre;  //now not used
    sch_task_t  *pTask_next;
    sch_task_t  *pTask_cur;

//37
    u32 sch_end_tick;
}sch_man_t;

extern sch_man_t        bltSche;




/* consider: scheduler update happens, a new task added, cost more time */
#define SCHE_NEW_TASK_MARGIN_US         50

#define GET_BSLOT_IDX(stimer_tick)      (bltSche.bSlot_idx_irq_real + (stimer_tick - bltSche.bSlot_tick_irq_real)/SYSTEM_TIMER_TICK_625US)



#define  FUTURE_TASK_MAX_NUM         (TSKNUM_AUX_ADV + TSKNUM_SECCHN_SCAN + TSKNUM_PAWRA_RSP)

_attribute_aligned_(4)
typedef struct {
    u8  task_flg;  //scheTask_flg
    u8  task_oft;
    u8  u8_rsvd[2];

    u32 tick_s;
    u32 tick_e;
}future_task_e;

_attribute_aligned_(4)
typedef struct {
    future_task_e task_tbl[FUTURE_TASK_MAX_NUM];

    u8  number;
}ll_future_task_t;
extern ll_future_task_t bltFutTask;




#define PRI_TASK_NUM                        TSKNUM_MAX

typedef signed short    pri_data_t;

typedef enum{
    TASK_PRIORITY_LOW           =   10,

    TASK_PRIORITY_MID           =   100,





    TASK_PRIORITY_AUX_SCAN_DFT  =   190,
    TASK_PRIORITY_PDA_SYNCING_DFT   =   195,

    TASK_PRIORITY_PDA_SYNCED_FIRST = 200,
    TASK_PRIORITY_HIGH_THRES    =   220,

    TASK_PRIORITY_AUX_ADV       =   10, //225,
//  TASK_PRIORITY_AUX_SCAN_DFT  =   225,

    TASK_PRIORITY_PERD_ADV_DFT  =   228,



    TASK_PRIORITY_CONN_CREATE   =   230,

    TASK_PRIORITY_CONN_UPDATE   =   240,
//  TASK_PRIORITY_PDA_SYNCING_DFT   =   240,

    TASK_PRIORITY_BIG_BCST_DFT  =   250,

    TASK_PRIORITY_MAX           =   500,  // bigger than TASK_PRIORITY_CONN_UPDATE + CONN_INTERVAL_100MS
}task_pri_t;


typedef struct pri_mng_t{
    pri_data_t  pri_now[PRI_TASK_NUM];
    pri_data_t  pri_cal[PRI_TASK_NUM];
    pri_data_t  priMax_value;
    u8  step_final[PRI_TASK_NUM];
    u8  step_intvl[PRI_TASK_NUM];
//  u8  step_set[PRI_TASK_NUM];
    u8  priMax_index;  //now not used
    u16 csctvAbandonCnt[PRI_TASK_NUM];
} pri_mng_t;

extern pri_mng_t bltPri;




static inline void blt_ll_setSchedulerTaskPriority(u8 task_offset, pri_data_t pri)
{
    bltPri.pri_now[task_offset] = pri;
}
void blt_ll_incSchedulerTaskPriority(u8 task_offset, int inc);
void blt_ll_incSchedulerTaskCalPriority  (u8 task_offset, int inc);


int     blt_ll_mainloop_startScheduler(void);
void    blt_ll_irq_startScheduler(void);
int     blt_ll_updateScheduler(void);
void    blt_ll_reset_bSlot_idx(void);
void    blt_ll_proc_bSlot_idx_overflow(void);


void    blt_ll_procStateChange(void);

void    blt_ll_calculate_sSlot_next(u32 next_tick);

int     blt_ll_addTask2ExistLinklist( sch_task_t *pStart_schTsk, int task_num_max);

int     blt_ll_addTask2AbandonTaskLinklist( sch_task_t *pStart_schTsk, int task_num);

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif
static inline void blt_sche_addTaskMask(u64 tskmsk){
    bltSche.task_mask |= (tskmsk);
}

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif
static inline void blt_sche_removeTaskMask(u64 tskmsk){
    bltSche.task_mask &= ~(tskmsk);
}

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif
static inline void blt_sche_enableTask(u64 tskmsk){
    bltSche.task_en |= (tskmsk);
}

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif
static inline void blt_sche_disableTask(u64 tskmsk){
    bltSche.task_en &= ~(tskmsk);
}

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif

static inline void blt_sche_addUpdate(u64 updt){
    bltSche.update |= (updt);
}

#if OS_COMPILE_OPTIMIZE_EN
_always_inline
#endif
static inline void blt_sche_setSystemIrqTrigger(u32 trigger){
    systick_irq_trigger = trigger;
}



typedef enum {
    CHECK_EXPIRE_ID_CIS_PER = 0,
    CHECK_EXPIRE_ID_CIS_CEN,
    CHECK_EXPIRE_ID_ACL_CONN,
    CHECK_EXPIRE_ID_MAX,
}chk_exp_id;  //check expire ID



typedef int (*func_chk_exp_isr_t) (void);


typedef struct chk_exp_str
{
    u8      en;
    u8      rsvd[3];

    func_chk_exp_isr_t func_exp;
}chk_exp_str; //check expire structure


typedef struct chk_exp_mng
{
    u8      chkExp_msk;
    u8      rsvd[3];

    chk_exp_str chk_exp_tbl[CHECK_EXPIRE_ID_MAX];
} chk_exp_mng; //check expire management

extern chk_exp_mng blt_chkExp;


static inline void blt_ll_init_check_expire_task(u8 id, func_chk_exp_isr_t func_cb)
{
    blt_chkExp.chk_exp_tbl[id].func_exp = func_cb;
}

static inline void blt_ll_enable_check_expire_task(u8 id)
{
    blt_chkExp.chkExp_msk |= BIT(id);
    blt_chkExp.chk_exp_tbl[id].en = 1;
}

static inline void blt_ll_disable_check_expire_task(u8 id)
{
    blt_chkExp.chkExp_msk &= ~BIT(id);
    blt_chkExp.chk_exp_tbl[id].en = 0;
}







typedef enum {
    REAL_TIME_TASK_CIS_SUD_IN   = 0,
    REAL_TIME_TASK_CIS_SUD_OUT,
    REAL_TIME_TASK_MAX,
}rt_task_id;  //real time task ID



typedef int (*func_rt_task_isr_t) (void);


typedef struct rt_task_str
{
    u8      en;
    u8      rsvd[1];
    u16     task_us;

    func_rt_task_isr_t func_rt;
}rt_task_str; //real time task structure


typedef struct rt_task_mng
{

    u8      rtTsk_msk;

    rt_task_str rt_task_tbl[REAL_TIME_TASK_MAX];
} rt_task_mng; //real time task management

extern rt_task_mng blt_rtTask;


static inline void blt_ll_init_irq_rt_task(u8 id, func_chk_exp_isr_t func_cb, u16 task_us)
{
    blt_rtTask.rt_task_tbl[id].func_rt = func_cb;
    blt_rtTask.rt_task_tbl[id].task_us = task_us;
}


static inline void blt_ll_enable_irq_rt_task(u8 id)
{
    blt_rtTask.rtTsk_msk |= BIT(id);
    blt_rtTask.rt_task_tbl[id].en = 1;
}

static inline void blt_ll_disable_irq_rt_task(u8 id)
{
    blt_rtTask.rtTsk_msk &= ~BIT(id);
    blt_rtTask.rt_task_tbl[id].en = 0;
}



/******************************* ll_schedule end ********************************************************************/



















/******************************* ll_pm start ******************************************************************/
#ifndef         BLMS_PM_ENABLE
#define         BLMS_PM_ENABLE                                  1
#endif

#define         ACL_SLAVE_PM_LATENCY_EN                         1




#define         PPM_IDX_200PPM                                  2
#define         PPM_IDX_300PPM                                  3
#define         PPM_IDX_400PPM                                  4
#define         PPM_IDX_500PPM                                  5
#define         PPM_IDX_600PPM                                  6
#define         PPM_IDX_700PPM                                  7
#define         PPM_IDX_800PPM                                  8
#define         PPM_IDX_900PPM                                  9
#define         PPM_IDX_1000PPM                                 10
#define         PPM_IDX_1100PPM                                 11
#define         PPM_IDX_1200PPM                                 12
#define         PPM_IDX_1300PPM                                 13
#define         PPM_IDX_1400PPM                                 14
#define         PPM_IDX_1500PPM                                 15


#define         WKPTASK_INVALID             0xFF

_attribute_aligned_(4)
typedef struct {
    u8      pm_inited;
    u8      sleep_allowed;
    u8      deepRt_en;
    u8      deepRet_type;

    u8      wakeup_src;
    u8      gpio_early_wkp;
    u8      slave_no_sleep;
    u8      slave_idx_calib;

    u8      appWakeup_en;
    u8      appWakeup_flg;
    u8      sleep_enter_flag;
    u8      pm_entered;

    u8      wkpTsk_oft;
    u8      sys_ppm_index;
    u16     Wakeup_early_us;//default:30us

    u16     sleep_mask;
    u16     user_latency;


    u32     deepRet_thresTick;
    u32     deepRet_earlyWakeupTick;
    u64     sleep_taskMask; //u32 -> u64
    u32     next_task_tick;
    u32     next_adv_tick;
    u32     wkpTsk_tick;
    u32     current_wakeup_tick; //The system wake-up tick of the actual transfer of the cpu_sleep_wakeup function.

    u32     appWakeup_tick;




    sch_task_t  *pTask_wakeup;

    sch_task_t  wkpTsk_fifo; //latency wake_up task fifo

}st_llms_pm_t;
extern st_llms_pm_t  blmsPm;




typedef     int (*ll_module_pm_callback_t)(void);
typedef     void (*ll_module_pmOs_callback_t)(int);
extern ll_module_pm_callback_t  ll_module_pm_cb;

#ifndef MCU_STALL_EN
#define MCU_STALL_EN     0
#endif

#if (MCU_STALL_EN)
extern      volatile  u8     pm_mcuStall_allowFlag;
#endif


int blt_sleep_process(void);
/******************************* ll_pm end ********************************************************************/












/******************************* ll_misc start ******************************************************************/
/* this access code algorithm v1 is now only used for BQB, SDK do not use it */
u32         blt_ll_connCalcAccessAddr_v1(void);
u32         blt_ll_connCalcAccessAddr_v2(void);

int         blt_calBit1Number(u32 dat);
int         blt_calBit1Number_16bit(u32 dat);

int         blt_debug_hex_2_dec_display(int src_data);
unsigned int zuixiao_gongbeishu(unsigned int, unsigned int, int);



/* for BQB test, manual add feature bit */
void    blc_ll_addFeature_0(u32 feat_mask);
void    blc_ll_removeFeature_0(u32 feat_mask );
void    blc_ll_addFeature_1(u32 feat_mask);
void    blc_ll_removeFeature_1(u32 feat_mask);


/******************************* ll_misc end ********************************************************************/




/******************************* ll_aes_ccm start ******************************************************************/
#define AES_BLOCK_SIZE     16


//#define       SUCCESS         0
enum {
    AES_SUCC = SUCCESS,
    AES_NO_BUF,
    AES_FAIL,
};


typedef struct {
    u32     pkt;
    u8      dir;
    u8      iv[8];
} ble_crypt_nonce_t;


typedef struct {
    u64                 enc_pno;
    u64                 dec_pno;
    u8                  ltk[16];
    u8                  sk[16];         //session key
    ble_crypt_nonce_t   nonce;
    u8                  st;
    u8                  enable;         //1: slave enable; 2: master enable
    u8                  mic_fail;
} ble_crypt_para_t;


struct CCM_FLAGS_TAG {
    union {
        struct {
            u8 L : 3;
            u8 M : 3;
            u8 aData :1;
            u8 reserved :1;
        } bf;
        u8 val;
    };
};

typedef struct CCM_FLAGS_TAG ccm_flags_t;


typedef struct {
    union {
        u8 A[AES_BLOCK_SIZE];
        u8 B[AES_BLOCK_SIZE];
    } bf;

    u8 tmpResult[AES_BLOCK_SIZE];
    u8 newAstr[AES_BLOCK_SIZE];
} aes_enc_t;


enum{
    CRYPT_NONCE_TYPE_ACL = 0,
    CRYPT_NONCE_TYPE_CIS = 1,
    CRYPT_NONCE_TYPE_BIS = 2,
};


extern volatile int aes_enc_dec_busy;

void    aes_ll_ccm_encryption_init (u8 *ltk, u8 *skdm, u8 *skds, u8 *ivm, u8 *ivs, ble_crypt_para_t *pd);
void    aes_ll_ccm_encryption(llPhysChnPdu_t *pllPhysChnPdu, u8 role, u8 ll_type, ble_crypt_para_t *pd);
int     aes_ll_ccm_decryption(llPhysChnPdu_t *pllPhysChnPdu, u8 role, u8 ll_type, ble_crypt_para_t *pd);
/******************************* ll_aes_ccm end ******************************************************************/




/******************************* ll_device start ******************************************************************/


typedef enum {
    MASTER_DEVICE_INDEX_0     = 0,

    SLAVE_DEVICE_INDEX_0      = 0,
    SLAVE_DEVICE_INDEX_1      = 1,
    SLAVE_DEVICE_INDEX_2      = 2
}local_dev_ind_t;   //local device index


/******************************* ll_device end ********************************************************************/





/******************************* HCI start ******************************************************************/
ble_sts_t       blc_hci_le_getLocalSupportedFeatures(hci_le_readLocSupFeature_retParam_t*);

/******************************* HCI end ********************************************************************/








//scan interval 50mS, legacy interval 200mS, then scan change channel every 200mS, scan too slow if peer device ADV on single channel 37
#define         PRICHN_SCAN_SMALL_INTV_WITH_ANOTHER_BIG_INTV_ADV_RESOLVE_EN         0

#define         ANOTHER_BIG_INTV_EXTENDED_ADV                                       1  //1: extended, 1: legacy


#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define     LEGADV_THRES        ADV_INTERVAL_80MS
#else
    #error "add code for scan_align_task!!!"
#endif

_attribute_aligned_(4)
typedef struct {
    u8  legadv_alloc;
    u8  legadv_sSLot_durn;  //u8 is enough
    u8  extAdv_num;
    u8  extAdv_legacyMode;

    s32 legadv_sSlot;

    u16 legadv_int;
    volatile u16 legadv_int_thres;  //Customer setup threshold

    volatile u8 extAdv_num_thres;   //Customer setup extAdv_num threshold
    volatile u8 scanTask_Policy;    //0x00 Execute according to the set parameters
                            //0x01 Must execute
    u8  u8_rsvd[2];
}ll_ad_scan_t;

extern  _attribute_aligned_(4) ll_ad_scan_t  bltAdScn;











/****************************** (ble1m,2m,500k,125k)RF RX/TX packet format ********************************************
RF RX packet format:
  b0          b3    b4         b5       b6   b(5+w) b(6+w) b(8+w) b(9+w) b(12+w)  b(13+w)    b(14+w)  b(15+w)                      b(16+w)
*---------------*---------*-----------*------------*------------*---------------*-------------------*----------*--------------------------------------------------*
|  DMA_len(4B)  | type(1B)| Rf_len(1B)| payload(wB)|   CRC(3B)  | time_stamp(4B)|  Fre_offset(2B)   | Rssi(1B) |           pkt status indicator(1B)               |
| (b0,b1 valid) |        Header       |   Payload  |            |               |                   | rssi-110 |[0]:crc err;[1]:sfd err;[2]:ll err;[4]:pwr err;   |
|               |<--           PDU              -->|            |               |                   |          |[4]:long range 125k;[6:5]:N/A;[7]:Telink NACK ind |
*---------------*----------------------------------*------------*---------------*-------------------*----------*--------------------------------------------------*
|<--- 4byte --->|<------ 2 byte ----->|<- Rf_len ->|<- 3 byte ->|<----------------------------------- 8 byte ---------------------------------------------------->|
note:       b4       ->  type(1B): llid(2bit) nesn(1bit) sn(1bit) md(1bit).
we can see: DMA_len     =   rx[0] = w(Rf_len)+13 = rx[5]+13.
            CRC_OK      =   DMA_buffer[rx[0]+3] == 0x00 ? True : False.

******
RF TX packet format:
 b0          b3      b4         b5       b6   b(5+w)
*---------------*----------*-----------*------------*
|  DMA_len(4B)  | type(1B) | Rf_len(1B)| payload(wB)|
| (b0,b1 valid) |         Header       |   Payload  |
|               |<--               PDU           -->|
*---------------*-----------------------------------*
note:       b4      ->  type(1B): llid(2bit) nesn(1bit) sn(1bit) md(1bit).Here type only means that llid, other bit is automatically populated when sent by hardware
we can see: DMA_len = rx[0]= w(Rf_len) + 2.
**********************************************************************************************************************/



/***********************************(DLE and MTU buffer size formula)*************************************************
Note: DLE only contains the len of payload (maxTxOct/maxRxOct 251 bytes)
    1. ACL Tx Data buffer len = 4(DMA_len) + 2(BLE header) + maxTxOct + 4(MIC) = maxTxOct + 10
    2. ACL RX Data buffer len = 4(DMA_len) + 2(BLE header) + maxRxOct + 4(MIC) + 3(CRC) + 8(ExtraInfo)  = maxRxOct + 21

    MTU contains ATT exclusive L2cap_Length(2) and CID(2)
    1. MTU Tx buffer len = DMA(4) + Header(2)  + L2cap_Header(4) + MTU = MTU + 10
    2. MTU Rx buffer len = DMA(4) + Header(2) + + L2cap_Header(4) + MTU + MTU + 10
    //todo DMA and Header should not include in MTU buff, Just use DMA field to hold packed len
*********************************************************************************************************************/





/************************************** Link Layer pkt format *********************************************************
Link Layer pak format(BLE4.2 spec):
*-------------*-------------------*-------------------------------*-------------------*
| preamble(1B)| Access Address(4B)|          PDU(2~257B)          |      CRC(3B)      |
|             |                   |  Header(2B) | payload(0~255B) |                   |
*-------------*-------------------*-------------------------------*-------------------*
1.ADV Channel, payload:0~37bytes = 6bytes AdvAdd + [maximum 31bytes adv packet payload]
2.Data Channel, payload:0~255bytes = 0~251bytes + 4bytes MIC(may include MIC feild)[The payload in ble4.2 can reach 251 bytes].
  Protocol overhead: 10bytes(preamble\Access Address\Header\CRC) + L2CAP header 4bytes = 14bytes, all LL data contains 14 bytes of overhead,
  For att, opCode is also needed, 1bytes + handle 2bytes = 3bytes, 251-4-3=[final 247-3bytes available to users].
******
Link Layer pak format(BLE4.0\4.1 spec):
*-------------*-------------------*-------------------------------*-------------------*
| preamble(1B)| Access Address(4B)|          PDU(2~39B)           |      CRC(3B)      |
|             |                   |  Header(2B) | payload(0~37B)  |                   |
*-------------*-------------------*-------------------------------*-------------------*
1.ADV Channel, payload:0~37bytes = 6bytes AdvAdd + [maximum 31bytes adv packet payload]
2.Data Channel, payload:0~31bytes = 0~27bytes + 4bytes MIC(may include MIC feild)[The payload in ble4.0/4.1 is 27 bytes].
  Protocol overhead: 10bytes(preamble\Access Address\Header\CRC) + L2CAP header 4bytes = 14bytes,all LL data contains 14 bytes of overhead,
  For att, opCode is also needed, 1bytes + handle 2bytes = 3bytes, 27-4-3=[final 23-3bytes available to users] This is why the default mtu size is 23 in the ble4.0 protocol.
**********************************************************************************************************************/


/*********************************** Advertising channel PDU : Header *************************************************
Header(2B):[Advertising channel PDU Header](BLE4.0\4.1 spec):
*--------------*----------*------------*-------------*-------------*----------*
|PDU Type(4bit)| RFU(2bit)| TxAdd(1bit)| RxAdd(1bit) |Length(6bits)| RFU(2bit)|
*--------------*----------*------------*-------------*-------------*----------*
public (TxAdd = 0) or random (TxAdd = 1).
**********************************************************************************************************************/


/******************************************* Data channel PDU : Header ************************************************
Header(2B):[Data channel PDU Header](BLE4.2 spec):(BLE4.0\4.1 spec):
*----------*-----------*---------*----------*----------*-------------*----------*
|LLID(2bit)| NESN(1bit)| SN(1bit)| MD(1bit) | RFU(3bit)|Length(5bits)| RFU(3bit)|
*----------*-----------*---------*----------*----------*-------------*----------*
******
Header(2B):[Data channel PDU Header](BLE4.2 spec):
*----------*-----------*---------*----------*----------*------------------------*
|LLID(2bit)| NESN(1bit)| SN(1bit)| MD(1bit) | RFU(3bit)|       Length(8bits)    |
*----------*-----------*---------*----------*----------*------------------------*
start    pkt:  llid 2 -> 0x02
continue pkt:  llid 1 -> 0x01
control  pkt:  llid 3 -> 0x03
***********************************************************************************************************************/


/*********************************** DATA channel PDU ******************************************************************
*------------------------------------- ll data pkt -------------------------------------------*
|             |llid nesn sn md |  pdu-len   | l2cap_len(2B)| chanId(2B)|  opCode(1B)|data(xB) |
| DMA_len(4B) |   type(1B)     | rf_len(1B) |       L2CAP header       |       value          |
|             |          data_header         |                        payload                  |
*-------------*-----------------------------*-------------------------------------------------*
*--------------------------------- ll control pkt ----------------------------*
| DMA_len(4B) |llid nesn sn md |  pdu-len   | LL Opcode(1B) |  CtrData(0~22B) |
|             |   type(1B)     | rf_len(1B) |               |      value      |
|             |          data_header         |            payload              |
*-------------*-----------------------------*---------------------------------*
***********************************************************************************************************************/



#endif /* LL_STACK_H_ */
