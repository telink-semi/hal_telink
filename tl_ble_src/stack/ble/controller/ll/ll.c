/********************************************************************************************************
 * @file    ll.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"


_attribute_ble_data_retention_ volatile u64 blms_state;
_attribute_ble_data_retention_ volatile u64 systick_irq_trigger;

_attribute_ble_data_retention_ volatile u8 blc_adv_chn_ext_sel   = ALLOW_LEG_EXT_CHN_SETTING; /* 1: legacy, 2: extended, 3: all */
_attribute_ble_data_retention_ volatile u8 blc_extadv_channel[3] = {37, 38, 39};
_attribute_ble_data_retention_ volatile u8 blc_legadv_channel[3] = {37, 38, 39};
#if (LL_ASYNC_LEA_EN)
_attribute_ble_data_retention_ volatile u8 blc_asyncAdv_channel[3] = {37, 38, 39};
#endif

_attribute_ble_data_retention_ u32 LL_FEATURE_MASK_0 = LL_FEATURE_MASK_BASE0;
_attribute_ble_data_retention_ u32 LL_FEATURE_MASK_1 = LL_FEATURE_MASK_BASE1;

_attribute_ble_data_retention_ _attribute_aligned_(4) ll_mac_t bltMac;

#if (CONTROLLER_GEN_P256KEY_ENABLE)

typedef struct
{
    //Private and DHkey use the different buffer, avoid private key content be overwritten by dhkey
    u8 sc_sk_own[32];  //  own  private key[32]
    u8 sc_dhk_own[32]; //  own  DHKey[32]
    u8 sc_pk_own[64];  //  own  public  key[64]
    u8 sc_pk_peer[64]; // peer  public  key[64]
} ll_ecdh_key_t;

_attribute_ble_data_retention_ _attribute_aligned_(4) ll_ecdh_key_t ll_ecdh_key;
#endif

_attribute_ble_data_retention_ _attribute_aligned_(4) st_ll_para_t blmsParam;

#if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
_attribute_ble_data_retention_ _attribute_aligned_(4) bigExtAuxPda_conflictCtrl_t bigExtAuxPda_conflictCtrl;
#endif

_attribute_ble_data_retention_ _attribute_aligned_(4) rx_pkt_sts_t bltRxPkt;

_attribute_ble_data_retention_ _attribute_aligned_(4) ll_addr_t bltAddr;

// Channel map
_attribute_ble_data_retention_ _attribute_aligned_(4) st_llm_hostChnClassUpt_t blmhostChnClassUpt = {
    .gLlChannelMap        = {0xff, 0xff, 0xff, 0xff, 0x1f}, //dft Link layer channel map: All channel are useful
    .hostMapUptCmdTick    = 0,
    .hostMapUptCmdPending = 0,
};

/*! \brief      SCA PPM table. */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u16 scaPpmTbl[8] = {
    500, /* SCA_MASTER_SLAVE_251_500_PPM */
    250, /* SCA_MASTER_SLAVE_151_250_PPM */
    150, /* SCA_MASTER_SLAVE_101_150_PPM */
    100, /* SCA_MASTER_SLAVE_76_100_PPM */
    75,  /* SCA_MASTER_SLAVE_51_75_PPM  */
    50,  /* SCA_MASTER_SLAVE_31_50_PPM  */
    30,  /* SCA_MASTER_SLAVE_21_30_PPM  */
    20   /* SCA_MASTER_SLAVE_0_20_PPM  */
};

_attribute_ble_data_retention_ _attribute_aligned_(4) st_ll_temp_para_t bltempParam; //attention: temp_use, no need retention even PM enable


/* when use this RX buffer, remember to limit RX DMA data not exceed 64 Byte !!! */
_attribute_ble_data_retention_ u8 glb_temp_rx_buff[64]; //TODO: for retention data optimize, can use data_no_init section later


/* attention:
 * first mark use "cur_phy - 1"
 * first mark use "cur_phy - 1"
 * */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u16 pdu_27b_tifs_27b_us[3][2] = {
    {PAYLOAD_27B_TIFS_27B_NOENT_1MPHY_US,       PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_US},
    {PAYLOAD_27B_TIFS_27B_NOENT_2MPHY_US,       PAYLOAD_27B_TIFS_27B_ENCRT_2MPHY_US},
    {PAYLOAD_27B_TIFS_27B_NOENT_CODED_S8_US,    PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_US},
};

/* attention:
 * first mark use "cur_phy - 1"
 * first mark use "cur_phy - 1"
 * */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u16 pdu_27b_tifs_27b_sslot[3][2] = {
    {PAYLOAD_27B_TIFS_27B_NOENT_1MPHY_SSLOT_NUM,       PAYLOAD_27B_TIFS_27B_ENCRT_1MPHY_SSLOT_NUM},
    {PAYLOAD_27B_TIFS_27B_NOENT_2MPHY_SSLOT_NUM,       PAYLOAD_27B_TIFS_27B_ENCRT_2MPHY_SSLOT_NUM},
    {PAYLOAD_27B_TIFS_27B_NOENT_CODED_S8_SSLOT_NUM,    PAYLOAD_27B_TIFS_27B_ENCRT_CODED_S8_SSLOT_NUM},
};


/* Different process for different MCU: set RX DMA FIFO and RX threshold **********
 * * u8 blms_tx_empty_packet[6] = {2, 0, 0, 0, 1, 0} for Kite/Vulture;
 * ((u32 *)blms_tx_empty_packet) = rf_tx_packet_dma_len(2) = 0x02 00 00 00
 *
 *  u8 blms_tx_empty_packet[6] = {0x01, 0x00, 0x80, 0x00, 0x01, 0x00} for B91/B92 and more MCU
 * ((u32 *)blms_tx_empty_packet) = rf_tx_packet_dma_len(2) = 0x00 80 00 01
 *
 */

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u8 blms_tx_empty_packet[6] = {
    U32_BYTE0(rf_tx_packet_dma_len(2)),
    U32_BYTE1(rf_tx_packet_dma_len(2)),
    U32_BYTE2(rf_tx_packet_dma_len(2)),
    U32_BYTE3(rf_tx_packet_dma_len(2)),
    0x01,
    0x00};


_attribute_ble_data_retention_ ll_host_mainloop_callback_t ll_host_main_loop_cb     = NULL;
_attribute_ble_data_retention_ ll_enc_done_callback_t      ll_encryption_done_cb    = NULL;
_attribute_data_retention_ ll_enc_pause_callback_t         ll_encryption_pause_cb   = NULL;
_attribute_ble_data_retention_ ll_conn_complete_handler_t  ll_connComplete_handler  = NULL;
_attribute_ble_data_retention_ ll_conn_terminate_handler_t ll_connTerminate_handler = NULL;


_attribute_ble_data_retention_ ll_adv_2_slave_callback_t ll_adv_2_slave_cb = NULL;


_attribute_ble_data_retention_ blc_main_loop_phyTest_callback_t blc_main_loop_phyTest_cb = NULL;


_attribute_ble_data_retention_ ll_push_fifo_handler_t ll_push_tx_fifo_handler = NULL;


_attribute_ble_data_retention_ blt_event_callback_t blt_p_event_callback;

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #include "acl_conn/acl_sniffer/acl_sniffer.h"
_attribute_ble_data_retention_ volatile blt_event_callback_t blt_event_func[BLT_SNIFFER_EV_MAX_NUM] = {0};
#else
_attribute_ble_data_retention_ volatile blt_event_callback_t blt_event_func[BLT_EV_MAX_NUM] = {0};
#endif

_attribute_ble_data_retention_ user_irq_handler_cb_t usr_irq_handler_cb = NULL;


#if (CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN || PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN)
_attribute_ble_data_retention_ int flash_filterMac_address;
_attribute_ble_data_retention_ int filter_mac_enable     = 0;
_attribute_ble_data_retention_ u8  filter_mac_address[6] = {};
#endif

#if (BLMS_PM_ENABLE) /** If enable BLE PM, need to add this code */
_attribute_ram_code_ 
#endif 
void blt_event_callback_func (u8 e, u8 *p, int n)
{
    if (blt_event_func[e]) {
        blt_event_func[e](e, p, n);
    }
}

void blc_ll_registerTelinkControllerEventCallback(u8 e, blt_event_callback_t p)
{
    blt_event_func[e] = p;
}

void blc_ll_register_user_irq_handler_cb(user_irq_handler_cb_t cb)
{
    if (cb) {
        usr_irq_handler_cb = cb;
    }
}

#if (BLC_PM_DEEP_RETENTION_MODE_EN)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    void
    blc_ll_initBasicMCU(void)
{
    systimer_set_irq_capture(clock_time() + BIT(29));

    systimer_clr_irq_status();
    zb_rt_irq_enable();

#if (HW_AES_CCM_ALG_EN)
    reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
#endif

#ifdef MCU_CORE_N22_ENABLE
    clic_interrupt_vector_en(IRQ_ZB_RT);
    clic_set_priority(IRQ_ZB_RT, 2);
    clic_interrupt_vector_en(IRQ_SYSTIMER);
    clic_set_priority(IRQ_SYSTIMER, 2);
    clic_set_threshold(IRQ_PRI_NUM0);

    //clic_preempt_feature_en();
#else
    plic_set_priority(IRQ_ZB_RT, 2);
    plic_set_priority(IRQ_SYSTIMER, 2);
    plic_set_threshold(IRQ_PRI_NUM0);

    flash_plic_preempt_config(1, 1);
    plic_preempt_feature_en(CORE_PREEMPT_PRI_MODE0);
#endif
    HAL_BLE_STACK_RF_IRQ_MASK_SET;

    /* enable:  RX_FIRST_TIMEOUT_EN + RX_TIMEOUT_EN + CRC_2_EN
     * disable: FSM_TIMEOUT_EN */
    //  FSM_TIMEOUT_DISABLE;
    reg_rf_ll_ctrl_1 = FLD_RF_RX_FIRST_TIMEOUT_EN | FLD_RF_RX_TIMEOUT_EN | FLD_RF_CRC_2_EN;

    //rf_ble_set_rx_timeout(0x00F9); //default value is f9, no need set


    /* SiHui: very special, only B91 B92 need this code, other MCU do not need !
     * when developing B91 BLE at 2018, found that dma_config(DMA1...) do not need execute very time, execute onece
     * at power_on & deepsleep retention wake_up is enough, so we add a new function "ble_rx_dma_config"  to optimize this
     * saving running time and SRAM space.
     * B92 also use this method.
     * For later new IC, we do not this optimization anymore, because:
     * 1. new MCU may be have different design or software implementation,
     * 2. for more colleagues developing BLE stack, optimization is also burden and risk, very easy to make mistake. */
    ble_rx_dma_config();

    //ske enable,for aes-ecb module.Need evaluate whether it can be deleted in the future.
    HAL_SKE_ENABLE;
}

/**
 * @brief      for user to initialize default RF TX power level index
 * @param      rfPwrLvlIdx - refer to 'rf_power_level_index_e'
 * @return     none
 */
void blc_ll_setDefaultTxPowerLevel(rf_power_level_index_e rfPwrLvlIdx)
{
    if (blmsParam.pwr_ctrl_en) {
        s8 rfTxPower = rf_ble_get_tx_pwr_level(rfPwrLvlIdx);

        /* update rfPwrLvlIdx */
        rfPwrLvlIdx = rf_ble_get_tx_pwr_idx(rfTxPower);

        /* re-map from users setting */
        blmsParam.dftTxPwrLvl = rfTxPower;
    }
    blmsParam.dftTxPwrLvlIdx = rfPwrLvlIdx;

    /* set the TX_PWR to the value that was actually set in HW */
    rf_set_power_level_index(rfPwrLvlIdx);
}

/**
 * @brief       API to encrypt plaintextData to encryptedTextData.
 * @param[in]   key - 128 bit key for the encryption of the data, little--endian.
 * @param[in]   128 bit data block that is requested to be encrypted, little--endian.
 * @param[out]  128 bit encrypted data block, little--endian.
 * @return      PKE_SUCCESS(success), other(error).
 */
ble_sts_t blc_ll_encryptedData(u8 *key, u8 *plaintextData, u8 *encryptedTextData)
{
    /* Core 5.2 Spec | Vol 4, Part E page 1886, 5.2 Section
     * Unless noted otherwise, all parameter values are sent and received in little-endian
     * format (i.e. for multi-octet parameters the rightmost (Least Significant Octet) is
     * transmitted first). */

    /* Sample data refer to Core 5.2 Spec | Vol 6, Part C page 3078
     HCI_LE_Encrypt (length 0x20)--C command
        Pars (LSO to MSO) bf 01 fb 9d 4e f3 bc 36 d8 74 f5 39 41 38 68 4c 13 02 f1 e0 df
        ce bd ac 79 68 57 46 35 24 13 02
        Key (16-octet value MSO to LSO): 0x4C68384139F574D836BCF34E9DFB01BF
        Plaintext_Data (16-octet value MSO to LSO): 0x0213243546576879acbdcedfe0f10213
        HCI_Command_Complete (length 0x14)--C event
        Pars (LSO to MSO) 02 17 20 00 66 c6 c2 27 8e 3b 8e 05 3e 7e a3 26 52 1b ad 99
        Num_HCI_Command_Packets: 0x02
        Command_Opcode (2-octet value MSO to LSO): 0x2017
        Status: 0x00
        Encrypted_Data (16-octet value MSO to LSO): 0x99ad1b5226a37e3e058e3b8e27c2c666
     */

    aes_encryption_le(key, plaintextData, encryptedTextData);

    return BLE_SUCCESS;
}

/**
 * @brief       This function is used to provide random number generator for Host use
 * @param[out]  dest: The address where the random number is stored
 * @param[in]   size: Output random number size, unit byte
 * @return      0:  success
 *              -1: failure
 */
ble_sts_t blc_ll_genRandomNumber(u8 *dest, u32 size)
{
    generateRandomNum(size, dest);

    return BLE_SUCCESS;
}

void blc_ll_initStandby_module(u8 *public_adr)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_ll_para_t)), ll);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(sch_man_t)), ll);
    //STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_mac_t)), ll);
#endif

    /* RF TX Power level default setting */
    blmsParam.dftTxPwrLvl    = 0;
    blmsParam.dftTxPwrLvlIdx = RF_POWER_P0dBm;
    smemcpy(bltMac.macAddress_public, public_adr, BLE_ADDR_LEN);

    blt_p_event_callback = blt_event_callback_func;

    bltSche.pTask_head       = &bltSche_header;
    bltSche.pTask_head->next = NULL;

#if (LL_FEATURE_ENABLE_PRIVACY)
    blt_ll_initResolvingList();
#else
    #warning "privacy feature must be enable, because peer device may use RPA"
#endif


#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    mlDevMng.cur_dev_idx = DEFAULT_DEVICE_INDEX;
#endif

#if (CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN || PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN)
    if (blc_flash_capacity == FLASH_SIZE_1M) {
        flash_filterMac_address = 0xF0000;
    } else if (blc_flash_capacity == FLASH_SIZE_2M) {
        flash_filterMac_address = 0x1F0000;
    } else if (blc_flash_capacity == FLASH_SIZE_4M) {
        flash_filterMac_address = 0x3F0000;
    } else if (blc_flash_capacity == FLASH_SIZE_16M) {
        flash_filterMac_address = 0xFF0000;
    }
    flash_read_page(flash_filterMac_address, 6, filter_mac_address);
    if (filter_mac_address[0] != 0xFF || filter_mac_address[5] != 0xFF) {
        filter_mac_enable = 1;
        tlkapi_send_string_data(filter_mac_enable, "[FILTER] Connect Mac filter enable", filter_mac_address, 6)
    }
#endif

#if FAST_SETTLE
    blc_ll_initFastSettle(1, 1);
#endif


#if (SDK_RELEASE_CHECK_EN)
    #if (CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN || PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN)
        #error "MAC_FILTER_EN can not be 1 !!!"
    #endif

    #if (BLT_ERR_PROCESS == ERR_TRIGGER_CODE_STUCK)
        #error "BLT_ERR_PROCESS can not be  ERR_TRIGGER_CODE_STUCK !!!"
    #endif

    #if (DEBUG_GPIO_ENABLE || DEBUG_SIHUI_GPIO_ENABLE || DEBUG_QIUWEI_GPIO_ENABLE || DEBUG_FANQH_GPIO_ENABLE || \
         DEBUG_YAFEI_GPIO_ENABLE || DEBUG_TIANXIANG_GPIO_ENABLE || DEBUG_CS_GPIO_ENABLE || DEBUG_HDT_GPIO_ENABLE)
        #error "GPIO debug can not be disable"
    #endif

    #if (TLKAPI_USE_INTERNAL_SPECIAL_UART_TOOL)
        #error "TLKAPI_USE_INTERNAL_SPECIAL_UART_TOOL must be 0 !!!"
    #endif

    #if (LL_RSSI_SNIFFER_MODE_ENABLE)
        #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
            #error "DEBUG_SNIFFER_REPORT_INSTANT_EN can not be 1 !!!"
        #endif
        #if (DEBUG_SNIFFER_NULL_POINTER_EN)
            #error "DEBUG_SNIFFER_NULL_POINTER_EN can not be 1 !!!"
        #endif
    #endif

    #if (LL_CS_SNIFFER_MODE_ENABLE)
        #if (DBG_CS_DATA_EN)
            #error "DBG_CS_DATA_EN can not be 1 !!!"
        #endif
    #endif
#endif


#if (0)
    tlkapi_send_string_u32s(1, "ADV_SET_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(st_ext_adv_t)), 0, 0, 0);
    tlkapi_send_string_u32s(1, "PERD_ADV_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(st_prd_adv_t)), 0, 0, 0);

    tlkapi_send_string_u32s(1, "BIG_BCST_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(ll_big_bcst_t)), 0, 0, 0);
    tlkapi_send_string_u32s(1, "BIG_SYNC_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(ll_big_sync_t)), 0, 0, 0);
    tlkapi_send_string_u32s(1, "BIS_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(ll_bis_t)), 0, 0, 0);

    tlkapi_send_string_u32s(1, "CIS_SLV_PARAM_LEN", blt_debug_hex_2_dec_display(sizeof(ll_cis_slv_t)), 0, 0, 0);
    tlkapi_send_string_u32s(1, "CIG_PARAM_LEN", blt_debug_hex_2_dec_display(sizeof(ll_cig_mst_t)), 0, 0, 0);
    tlkapi_send_string_u32s(1, "CIS_CONN_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(ll_cis_conn_t)), 0, 0, 0);

    tlkapi_send_string_u32s(1, "CS_PARAM_LENGTH", blt_debug_hex_2_dec_display(sizeof(cs_config_t)), 0, 0, 0);
#endif
}

void blc_ll_setCustomizedAdvertisingScanningChannel(u8 chn0, u8 chn1, u8 chn2)
{
    if (blc_adv_chn_ext_sel & ALLOW_LEG_CHN_SETTING) {
        blc_legadv_channel[0] = chn0;
        blc_legadv_channel[1] = chn1;
        blc_legadv_channel[2] = chn2;
        tlkapi_printf((stkLog_mask & STK_LOG_LL_CMD), "leg chn %d-%d-%d ", chn0, chn1, chn2);
    }

    if (blc_adv_chn_ext_sel & ALLOW_EXT_CHN_SETTING) {
        blc_extadv_channel[0] = chn0;
        blc_extadv_channel[1] = chn1;
        blc_extadv_channel[2] = chn2;
        tlkapi_printf((stkLog_mask & STK_LOG_LL_CMD), "ext chn %d-%d-%d ", chn0, chn1, chn2);
    }
}

void blc_ll_setCustomizedAdvertisingScanningChannelMask(blt_set_cus_chn_mask_t mask)
{
    blc_adv_chn_ext_sel = mask;
}

ble_sts_t blc_ll_readBDAddr(u8 *addr)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_BD_ADDR", &bltMac.macAddress_public, 6);

    smemcpy(addr, bltMac.macAddress_public, BLE_ADDR_LEN);
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_writeBDAddr(u8 *addr)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Write_BD_ADDR", addr, 6);

    smemcpy(bltMac.macAddress_public, addr, BLE_ADDR_LEN);
    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setRandomAddr(u8 *randomAddr)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Random_Addr", randomAddr, 6);

    if (blmsParam.create_connection || blmsParam.scanInitEn_union.leg_scan_en || blmsParam.leg_adv_en) {
        /* TP/CON/INI/BV-01-C Test that the IUT responds with Command Disallowed to an LE Set
        Random Address command when initiating*/
        return HCI_ERR_CMD_DISALLOWED;
    }

    smemcpy(bltMac.macAddress_random, randomAddr, BLE_ADDR_LEN);
    blmsParam.hci_cmd_mask |= SET_RANDOM_ADDR_CMD_MASK;

    return BLE_SUCCESS;
}



#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif 
int blt_ll_set_interval_level (u8 task_offset, u16 interval) // interval unit: 1.25 mS
{
    /*********************************************************************************************************************
 interval:       6      7        8         9       10        11         12       13       14        15         16
 interval_mS   7.5mS  8.75mS   10mS    11.25mS    12.5mS    13.75mS    15mS   16.25mS    17.5mS    18.75mS    20mS
 step:           2      2        2         2       2         2          3        3         3         3          4


 interval:       16~19          20~23         24~27         28~31   32~35   36~39                   80
 interval_mS    20~23.75mS     25~28.75mS    30~33.75mS                                            100mS
 step:             4              5             6             7       8       9                     20

 step = interval/4 + 1
 ********************************************************************************************************************/
    if (interval < CONN_INTERVAL_10MS) {
        bltPri.step_intvl[task_offset] = 2;
    } else if (interval > CONN_INTERVAL_200MS) {
        bltPri.step_intvl[task_offset] = 30;
    } else if (interval > CONN_INTERVAL_100MS) {
        bltPri.step_intvl[task_offset] = 20;
    } else {
        bltPri.step_intvl[task_offset] = (interval >> 2);
    }
    //bltPri.step_intvl[task_offset] = interval < 8 ? 2: (interval>>2);  //max 255 -> 1.275 S
    bltPri.step_final[task_offset] = bltPri.step_intvl[task_offset];

    return 1;
}


#if (CONTROLLER_GEN_P256KEY_ENABLE)
/*
 * @brief  API to Get p-256 Public Key.
 *
 * */
ble_sts_t blt_ll_getP256pubKey(void)
{
    if (!blmsParam.getP256pubKeyEvtPending) {
        blmsParam.getP256pubKeyEvtPending = 1;
        blt_ecc_init(); /*  */

        //All ecc pub/priv keys are big--endian
        blmsParam.getP256pubKeystatus = blt_ecc_gen_key_pair(ll_ecdh_key.sc_pk_own, ll_ecdh_key.sc_sk_own, ECC_use_secp256r1, FALSE);
    }

    return BLE_SUCCESS;
}

/*
 * @brief  API to Generate DHKey.
 *
 * */
ble_sts_t blt_ll_generateDHkey(u8 *remote_public_key, bool use_dbg_key)
{
    if (!blmsParam.generateDHkeyEvtPending) {
        blmsParam.generateDHkeyEvtPending = 1;

        u8 status = 0; //dft: Invalid Parameters

        //0: Use the generated private key
        if (use_dbg_key == TRUE || use_dbg_key == FALSE) {
            //0x01: Use the debug private key
            if (use_dbg_key == TRUE) {
                smemcpy(ll_ecdh_key.sc_sk_own, (u8 *)blt_ecc_dbg_priv_key256, 32);
            }

            /* Core 5.2 Spec | Vol 4, Part E page 1886, 5.2 Section
             * Unless noted otherwise, all parameter values are sent and received in little-endian
             * format (i.e. for multi-octet parameters the rightmost (Least Significant Octet) is
             * transmitted first). */

            swapN(remote_public_key, 32);      //little-endain to big-endian
            swapN(remote_public_key + 32, 32); //little-endain to big-endian

            status = blt_ecc_gen_dhkey(remote_public_key, ll_ecdh_key.sc_sk_own, ll_ecdh_key.sc_dhk_own, ECC_use_secp256r1);
        }

        blmsParam.generateDHkeyStatus = status;
    }

    return BLE_SUCCESS;
}

_attribute_noinline_ void blt_ll_procGetP256pubKeyEvent(void)
{
    /* If the controller does not support SC, you can comment it out here so
     * that the ll_ecdh_key variable will not be stored in RAM: save 169bytes */
    #if (1)
    blmsParam.getP256pubKeyEvtPending = 0;

    /* Core 5.2 Spec | Vol 4, Part E page 1886, 5.2 Section
     * Unless noted otherwise, all parameter values are sent and received in little-endian
     * format (i.e. for multi-octet parameters the rightmost (Least Significant Octet) is
     * transmitted first). */

    if (hci_le_eventMask & HCI_LE_EVT_MASK_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE) {
        swapN(ll_ecdh_key.sc_pk_own, 32);      //big-endain to little-endian
        swapN(ll_ecdh_key.sc_pk_own + 32, 32); //big-endain to little-endian
        hci_le_readLocalP256KeyComplete_evt(ll_ecdh_key.sc_pk_own, blmsParam.getP256pubKeystatus);
    }
    #endif
}

_attribute_noinline_ void blt_ll_procGenDHkeyEvent(void)
{
    /* If the controller does not support SC, you can comment it out here so
     * that the ll_ecdh_key variable will not be stored in RAM: save 169bytes*/
    #if (1)
    blmsParam.generateDHkeyEvtPending = 0;

    if (hci_le_eventMask & HCI_LE_EVT_MASK_GENERATE_DHKEY_COMPLETE) {
        swapN(ll_ecdh_key.sc_dhk_own, 32); //big-endain to little-endian
        hci_le_generateDHKeyComplete_evt(ll_ecdh_key.sc_dhk_own, blmsParam.generateDHkeyStatus);
    }
    #endif
}
#endif

/******************************************************************************************************************************
 *
 *  HCI
 *
 *****************************************************************************************************************************/
ble_sts_t blc_hci_reset(void)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] HCI_Reset", 0, 0);

    u32 r = irq_disable(); //disable IRQ in beginning

    //log_event_irq(SL_STACK_IUT_CMD_EVT, SLEV_hci_reset); //use _irq cause here IRQ disabled already

    //stop RF first, leave some time for clear some status
    u32 begin_tick = clock_time();
    rf_set_tx_rx_off();
    STOP_RF_STATE_MACHINE;

    extern void blt_hal_reset_baseband(void);
    blt_hal_reset_baseband();


    u8 dftLlChannelMap[5] = {0xff, 0xff, 0xff, 0xff, 0x1f}; //dft Link layer channel map: All channel are useful
    smemcpy(blmhostChnClassUpt.gLlChannelMap, dftLlChannelMap, 5);
    blmhostChnClassUpt.hostMapUptCmdPending = 0;
    blmhostChnClassUpt.hostMapUptCmdTick    = 0;

    blmsParam.phytest_en   = 0;
    blmsParam.hci_cmd_mask = 0;

    //RF BLE Current TX Path Compensation (s16 -1280 ~ 1280, unit: 0.1 dB)
    ble_rf_tx_path_comp = 0;
    //RF BLE Current RX Path Compensation (s16 -1280 ~ 1280, unit: 0.1 dB)
    ble_rf_rx_path_comp = 0;


    LL_FEATURE_MASK_1 &= ~LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS;
    LL_FEATURE_MASK_1 &= ~LL_FEATURE_MASK_CONNECTION_SUBRATING_HOST;


    hci_eventMask    = HCI_EVT_MASK_DISCONNECTION_COMPLETE;
    hci_le_eventMask = 0x0000001F;


    if (ll_acl_conn_mlp_task_cb) {
        ll_acl_conn_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_acl_conn_mainloop_task()
    }

#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    if (ll_cis_conn_mlp_task_cb) {
        ll_cis_conn_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_cis_conn_mainloop_task
    }
#endif


    //TODO: move to acl module
    if (ll_init_mlp_task_cb) {
        ll_init_mlp_task_cb(FLAG_MODULE_RESET); // blt_init_mainloop_task
    }


    if (ll_leg_adv_mlp_task_cb) {
        ll_leg_adv_mlp_task_cb(FLAG_MODULE_RESET); // blt_leg_adv_mainloop_task
    }

#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
    if (ll_ext_adv_mlp_task_cb) {
        ll_ext_adv_mlp_task_cb(FLAG_MODULE_RESET, NULL); // blt_ext_adv_mainloop_task
    }
#endif


    if (ll_leg_scan_mlp_task_cb) {
        ll_leg_scan_mlp_task_cb(FLAG_MODULE_RESET); // blt_leg_scan_mainloop_task
    }


#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
    if (ll_ext_scan_mlp_task_cb) {
        ll_ext_scan_mlp_task_cb(FLAG_MODULE_RESET); // blt_ext_scan_mainloop_task
    }
#endif


#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
    if (ll_prd_adv_mlp_task_cb) {
        ll_prd_adv_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_prd_adv_mainloop_task
    }
#endif


#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
    if (ll_big_bcst_mlp_task_cb) {
        ll_big_bcst_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_big_bcst_mainloop_task()
    }
#endif


#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    if (ll_big_sync_mlp_task_cb) {
        ll_big_sync_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_big_sync_mainloop_task()
    }

#endif

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
    if (ll_pda_sync_mlp_task_cb) {
        ll_pda_sync_mlp_task_cb(FLAG_MODULE_RESET); //blt_pda_sync_mainloop_task
    }
#endif


#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    if (ll_acl_subrate_mlp_task_cb) { //bltsubrate_mainloop_task
        ll_acl_subrate_mlp_task_cb(FLAG_MODULE_RESET, NULL);
    }
#endif

#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    if (ll_aoa_aod_mlp_task_cb) {
        ll_aoa_aod_mlp_task_cb(FLAG_MODULE_RESET); //blt_aoa_aod_mainloop_task
    }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    if (ll_chn_sounding_mlp_task_cb) {
        ll_chn_sounding_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_cs_mainloop_task
    }
#endif

#if (LL_FEATURE_ENABLE_MONITORING_ADVERTISERS)
    if (ll_mon_adv_mlp_task_cb) {
        ll_mon_adv_mlp_task_cb(FLAG_MODULE_RESET, NULL);  //blt_mon_adv_mainloop_task
    }
#endif


#if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    if (ll_acl_sniffer_mst_mlp_task_cb) {
        ll_acl_sniffer_mst_mlp_task_cb(FLAG_MODULE_RESET); // blt_acl_sniffer_mst_mainloop_task() blt_ll_reset_acl_sniffer_mst()
    }
#endif

#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    if (ll_acl_sniffer_slv_mlp_task_cb) {
        ll_acl_sniffer_slv_mlp_task_cb(FLAG_MODULE_RESET); //blt_acl_sniffer_slv_mainloop_task() blt_ll_reset_acl_sniffer_slv()
    }
#endif

    //LL/CON/CEN/BV-54-C    [Responding to PHY Update Procedure - Packet Time Restrictions, LE Coded]
    //LL/CON/CEN/BV-53-C    [Initiating PHY Update Procedure - Packet Time Restrictions, LE Coded]
    //LL/CON/PER/BV-56-C    [Responding to PHY Update Procedure - Packet Time Restrictions, LE Coded]
    //LL/CON/PER/BV-55-C    [Initiating PHY Update Procedure - Packet Time Restrictions, LE Coded]
    bltHci_rxAclfifo.rptr = bltHci_rxAclfifo.wptr = 0;

    bltSche.task_en         = 0;
    bltSche.task_mask       = 0;
    bltSche.update          = 0;
    blmsParam.sche_run_flag = 0;
    blmsParam.state_chng    = 0;
    blmsParam.hci_cmd_mask  = 0;

#if (ONLY_FOR_EBQ_TEST_LATER_REMOVE)
    bigExtAuxPda_conflictCtrl.syncInfor_changeCtrl = SYNCINFOR_NOT_CHANGE;
#endif

    systimer_set_irq_capture(clock_time() + BIT(29));
    systimer_clr_irq_status();
    systimer_irq_disable();


    blc_ll_clearWhiteList();
    blt_ll_clearResolvingList();

    while (!clock_time_exceed(begin_tick, 30))
        ;                   //30 uS
    CLEAR_ALL_RFIRQ_STATUS; //clear all RF IRQ


    irq_restore(r);

    systimer_clr_irq_status();

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setHostFeature(u8 bit_number, ll_feature_value_t bit_value)
{
    /*If Bit_Number specifies a feature bit that is not controlled by the Host, the
    Controller shall return the error code Unsupported Feature or Parameter Value
    (0x11).*/
#if (BLUETOOTH_VER < BLUETOOTH_VER_5_2)
    return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_2)
    if (bit_number != LL_FEATURE_BIT_NUMBER_ISOCHRONOUS_STREAM_HOST) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_3)
    if (!(bit_number == LL_FEATURE_BIT_NUMBER_ISOCHRONOUS_STREAM_HOST ||
          bit_number == LL_FEATURE_BIT_NUMBER_CONNECTION_SUBRATING_HOST)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }
#endif

    /*If the Host issues this command while the Controller has an established ACL,
    the Controller shall return the error code Command Disallowed (0x0C).*/
    if (ll_acl_conn_mlp_task_cb && ll_acl_conn_mlp_task_cb(FLAG_ACL_CONN_EXIT_CHECK, NULL)) { //blt_acl_conn_mainloop_task
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (bit_value == LL_FEATURE_ENABLE) {
        LL_FEATURE_MASK_1 |= BIT(bit_number - 32);
    } else if (bit_value == LL_FEATURE_DISABLE) {
        LL_FEATURE_MASK_1 &= ~BIT(bit_number - 32);
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_getLocalSupportedFeatures(hci_le_readLocSupFeature_retParam_t *pRetParam)
{
    pRetParam->status = BLE_SUCCESS;

    smemcpy(pRetParam->le_features, &LL_FEATURE_MASK_0, 4);
    smemcpy(pRetParam->le_features + 4, &LL_FEATURE_MASK_1, 4);

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Local_Sup_Features", pRetParam->le_features, 8);

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_setHostChannel(u8 *pChm)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Host_Chn", pChm, 5);

    u32 chanMapLow  = (u32)pChm[0] + ((u32)pChm[1] << 8) + ((u32)pChm[2] << 16) + ((u32)pChm[3] << 24);
    u32 chanMapHigh = (u32)pChm[4] & 0x1F;
    u8  cnt         = blt_calBit1Number(chanMapLow) + blt_calBit1Number(chanMapHigh);

    if (cnt < 2 || (pChm[4] & 0xe0) != 0) {
        return HCI_ERR_CMD_DISALLOWED;
    }

#if 0
    /*The interval between two successive commands sent shall be at least one second. */
    if(blmhostChnClassUpt.hostMapUptCmdTick){
        if(clock_time_exceed(blmhostChnClassUpt.hostMapUptCmdTick, 1000*1000)){
            blmhostChnClassUpt.hostMapUptCmdTick = 0;
        }
        else{
            return HCI_ERR_CMD_DISALLOWED;
        }
    }

    blmhostChnClassUpt.hostMapUptCmdTick = clock_time()|1;
#endif

    if (!smemcmp(blmhostChnClassUpt.gLlChannelMap, pChm, 5)) { // do nothing if the host set same channel map
        return BLE_SUCCESS;
    } else {
        /* Refer to <<Core5.2>> | Vol 4, Part E. <<7.8.19 LE Set Host Channel Classification command>>
         * This command shall only be used when the local device supports the Master
         * role, or supports extended advertising in the Advertising state, or supports the
         * Isochronous Broadcaster role.
         */

        /*
         * In <<Core5.3>>, the Role requirements have been removed.
         */

        //Change LL channel map, it'll cause channel map update procedure to start
        smemcpy(blmhostChnClassUpt.gLlChannelMap, pChm, 5);
        blmhostChnClassUpt.hostMapUptCmdPending = 0; //clear

        /* This command shall only be used when the local device supports the Master
        * role, supports extended advertising in the Advertising state, or supports the
        * Isochronous Broadcaster role. */

        //ACL Master or ACL Center
#if (LL_ACL_CEN_EN)
        if (ll_acl_master_mlp_task_cb) {
            ll_acl_master_mlp_task_cb(FLAG_MODULE_SET_HOST_CHM, pChm); //blt_acl_master_mainloop_task()
        }
#endif
        //Periodic Adv
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
        if (ll_prd_adv_mlp_task_cb) {
            ll_prd_adv_mlp_task_cb(FLAG_MODULE_SET_HOST_CHM, pChm); //blt_prd_adv_mainloop_task()
        }
#endif
        //BIS Broadcast
#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
        if (ll_big_bcst_mlp_task_cb) {
            ll_big_bcst_mlp_task_cb(FLAG_MODULE_SET_HOST_CHM, pChm); //blt_big_bcst_mainloop_task()
        }
#endif
        //ACL Slave or ACL Peripheral
#if (LL_ACL_PER_EN)
        if (ll_acl_slave_mlp_task_cb) {
            ll_acl_slave_mlp_task_cb(FLAG_MODULE_SET_HOST_CHM, pChm); //blt_acl_slave_mainloop_task()
        }
#endif
    }

    return BLE_SUCCESS;
}

init_err_t blt_contr_checkControllerInitialization(void)
{
    u32 init_status = 0;

    /* Check ACL connection buffer. */
    if (ll_acl_conn_mlp_task_cb) {                                    // blt_acl_conn_mainloop_task
        init_status = ll_acl_conn_mlp_task_cb(FLAG_CHECK_INIT, NULL); //blt_ll_checkAclInit
        if (init_status) {
            return init_status;
        }
    }

    /* Check HCI ACL buffer -- blc_ll_initHciAclDataFifo called */
    if (bltempParam.hci_aclRxFifo_set) {
        if (!bltHci_rxAclfifo.num || !bltHci_rxAclfifo.size || !blmsParam.acl_packet_length ||
            bltHci_rxAclfifo.size < (u32)(blmsParam.acl_packet_length + 4)) { //user set 0, or not POW_OF_2
            return HCI_ACL_DATA_BUF_PARAM_INVALID;
        }
    }

    if (blmsParam.max_master_num > LL_MAX_ACL_CEN_NUM) {
        return LL_ACL_CONN_NUM_NOT_SUPPORTED;
    }

    if (blmsParam.max_slave_num > LL_MAX_ACL_PER_NUM) {
        return LL_ACL_CONN_NUM_NOT_SUPPORTED;
    }


#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    if (ll_cis_conn_mlp_task_cb) {
        init_status = ll_cis_conn_mlp_task_cb(FLAG_CHECK_INIT, NULL); //blt_cis_conn_mainloop_task  blt_ll_checkCisInit
        if (init_status) {
            return init_status;
        }
    }
#endif


#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
    if (ll_big_bcst_mlp_task_cb) {
        init_status = ll_big_bcst_mlp_task_cb(FLAG_CHECK_INIT, NULL); //blt_ll_checkBisBroadcastInit
        if (init_status) {
            return init_status;
        }
    }
#endif

#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    if (ll_big_sync_mlp_task_cb) {
        init_status = ll_big_sync_mlp_task_cb(FLAG_CHECK_INIT, NULL); //blt_ll_checkBisSyncInit
        if (init_status) {
            return init_status;
        }
    }
#endif

    //TODO: check HCI TX/RX buffer with HCI ACL data buffer


    // Check if system clock is too slow based on different parameters
    if ((blmsParam.max_master_num == 0) && (blmsParam.max_slave_num == 1)) {
        if (sys_clk.cclk < 24) {
            // If there are no master devices, only one slave device, and the system clock is below 24 MHz,
            // return clock too slow error (specific condition for low clock with single slave)
            return SYS_ERR_CLK_TOO_SLOW;
        }
    } else if (sys_clk.cclk < 32) {
        // If system clock is below 32 MHz, return clock too slow error (general check)
        return SYS_ERR_CLK_TOO_SLOW;
    }

#if MCU_CORE_TYPE == MCU_CORE_TL323X
    #warning "only used for TL323X FPGA, actual chip will fix it!"
#endif


    /* add a module enable value if do not written in sub_module */
    blmsParam.iso_en = blmsParam.cis_en || blmsParam.bis_en;

    blmsParam.iso_tx_en = blmsParam.cis_en || blmsParam.big_bcst_en;
    blmsParam.iso_rx_en = blmsParam.cis_en || blmsParam.big_sync_en;


    return INIT_SUCCESS;
}

init_err_t blc_contr_checkControllerInitialization(void)
{
    u32 error_code = blt_contr_checkControllerInitialization();
    //  if(error_code){
    //      my_dump_str_data(STACK_DUMP_EN, "user init ERROR", &error_code, 1);
    //      BLMS_ERR_DEBUG(BLMS_DEBUG_EN, 0x12345678);
    //  }


    return error_code;
}

/*
 * 7.8.74 LE Read Transmit Power command
 */
ble_sts_t blc_ll_readSuppTxPower(s8 *pOutMinTxPwr, s8 *pOutMaxTxPwr)
{
    assert(pOutMinTxPwr != NULL);
    assert(pOutMaxTxPwr != NULL);

    *pOutMinTxPwr = ble_rf_min_tx_pwr;
    *pOutMaxTxPwr = ble_rf_max_tx_pwr;

    return BLE_SUCCESS;
}

/*
 *  RF_TX_Path_Compensation_Value: Convert 0.1dB to 1dB
 */
s8 blt_ll_getRfTxPathComp(void)
{
    s16 rfTxMinPathComp0p1dBm = -1280; //in 0.1dB units
    s8  rfTxMinPathComp1dBm   = -128;  //in 1dB units
    s16 pathCompUnsigned      = ble_rf_tx_path_comp - rfTxMinPathComp0p1dBm;

    return ((s8)(pathCompUnsigned / 10) + rfTxMinPathComp1dBm);
}

/*
 *  RF_RX_Path_Compensation_Value: Convert 0.1dB to 1dB
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION) //for RISC-V IRQ priority
_attribute_ram_code_
#endif
s8   blt_ll_getRfRxPathComp(void) //In IRQ may be used , optimize latter
{
    s16 rfRxMinPathComp0p1dBm = -1280; //in 0.1dB units
    s8  rfRxMinPathComp1dBm   = -128;  //in 1dB units
    s16 pathCompUnsigned      = ble_rf_rx_path_comp - rfRxMinPathComp0p1dBm;

    return ((s8)(pathCompUnsigned / 10) + rfRxMinPathComp1dBm);
}

/*
 * 7.8.75 LE Read RF Path Compensation command
 */
ble_sts_t blc_ll_readRfPathComp(s16 *pOutTxPathComp, s16 *pOutRxPathComp)
{
    assert(pOutTxPathComp != NULL);
    assert(pOutRxPathComp != NULL);

    /*
     * ble_rf_tx_path_comp \ ble_rf_rx_path_comp:
     * The default values for the RF path compensation are vendor-specific.
     */

    /*
     * Radiative Tx power level =
     *      Tx power level at RF transceiver output + RF_TX_Path_Compensation_Value
     */
    *pOutTxPathComp = ble_rf_tx_path_comp;
    /*
     * The RF_RX_Path_Compensation_Value parameter shall be used by the
     * Controller to calculate the RSSI value reported to the Host.
     */
    *pOutRxPathComp = ble_rf_rx_path_comp;

    return BLE_SUCCESS;
}

/*
 * 7.8.76 LE Write RF Path Compensation command
 */
ble_sts_t blc_ll_writeRfPathComp(s16 txPathComp, s16 rxPathComp)
{
    //Compensation_Value Range: -128.0 dB (0xFB00) to 128.0 dB (0x0500)
    if ((txPathComp < -1280 || txPathComp > 1280) ||
        (rxPathComp < -1280 || rxPathComp > 1280)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * The RF_TX_Path_Compensation_Value parameter shall be used by the Controller
     * to calculate the radiative Tx power level used in HCI commands, HCI events,
     * Advertising physical channel PDUs, and Link Layer Control PDUs using the
     * following equation:
     *      Radiative Tx power level = Tx power level at RF transceiver output + RF_TX_Path_Compensation_Value
     */
    ble_rf_tx_path_comp = txPathComp;
    ble_rf_rx_path_comp = rxPathComp;

    /*
     * If the command leads to a change in the local radiative transmit power level for
     * an LE ACL connection, then the Controller shall generate an HCI_LE_Transmit_Power_Reporting
     * event if local reporting is enabled and initiate a Link Layer Power Change Indication procedure
     * if remote reporting is enabled. TODO:
     *
     */

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_readSuppTxPower(hci_le_rdSuppTxPwrRetParams_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Transmit_Power", 0, 0);
    s8 minTxPwrLvl, maxTxPwrLvl;
    retPara->status      = blc_ll_readSuppTxPower(&minTxPwrLvl, &maxTxPwrLvl);
    retPara->minTxPwrLvl = minTxPwrLvl;
    retPara->maxTxPwrLvl = maxTxPwrLvl;

    return retPara->status;
}

ble_sts_t blc_hci_le_readRfPathComp(hci_le_rdRfPathCompRetParams_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Read_Rf_Path_Compensation", 0, 0);
    s16 txPathComp, rxPathComp;
    retPara->status     = blc_ll_readRfPathComp(&txPathComp, &rxPathComp);
    retPara->txPathComp = txPathComp;
    retPara->rxPathComp = rxPathComp;

    return retPara->status;
}

ble_sts_t blc_hci_le_writeRfPathComp(hci_le_writeRfPathCompCmdParams_t *cmdPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Write_Rf_Path_Compensation", 0, 0);
    return blc_ll_writeRfPathComp(cmdPara->txPathComp, cmdPara->rxPathComp);
}

/**
 * This command shall not be used when address resolution is enabled in the Controller and:
 * (1)Advertising (other than periodic advertising) is enabled,
 * (2)Scanning is enabled, or an HCI_LE_Create_Connection, HCI_LE_Extended_Create_Connection,
 *   or HCI_LE_Periodic_Advertising_Create_Sync command is outstanding.
 * This command may be used at any time when address resolution is disabled in the Controller.
 * @return int 0: not allowed. 1: allowed.
 */
bool blt_ll_isResolvingListCommandAllowed(void)
{
    int rc = 1;
/**
     * These codes are only for BQB test, Can not be used when not in BQB test. Modified by SunWei, confirmed by SiHui.
     */
#if (BQB_TEST_EN)
    u32 r = irq_disable();

    if (blmsParam.leg_adv_en || blmsParam.ext_adv_en || blmsParam.scanInitEn_union.leg_scan_en || blmsParam.scanInitEn_union.leg_init_en) {
        rc = 0;
    }

    //legacy init and extended init share the variable initiate_going
    if (bltScn.initiate_going || blmsParam.pda_syncing_flg) {
        rc = 0;
    }

    irq_restore(r);
#endif
    return rc;
}


#if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
_attribute_ram_code_sec_noinline_ void blt_ll_record_identity_address(u8 type, u8 *addr)
{
    bltMac.idenAdr_cur_type = type;
    smemcpy(bltMac.idenAdr_cur_addr, addr, BLE_ADDR_LEN);
}
#endif


#if FAST_SETTLE
extern _attribute_data_retention_sec_ rf_fast_settle_t *g_fast_settle_cal_val_ptr;
#endif
_attribute_ram_code_sec_noinline_ void blt_ll_addr_set_peer_address(u8 rpa_resolve_ok, u8 peer_adrType, u8 *peer_addr)
{
#if (LL_FEATURE_ENABLE_PRIVACY)
    bltAddr.peer_use_rpa = rpa_resolve_ok;
#endif
    bltAddr.peer_pka_or_ida_type = peer_adrType;
    smemcpy(bltAddr.peer_pka_or_ida_addr, peer_addr, BLE_ADDR_LEN);
}

u32 blc_ll_checkBleTaskIsIdle(void)
{
    //not recommended use
    return blmsParam.sche_run_flag;
}

bool blc_ll_isBleTaskIdle(void)
{
    return !(blmsParam.sche_run_flag || blmsParam.state_chng);
}


#if (FAST_SETTLE)
//Get RF calibration values
void blc_ll_initFastSettle(u8 tx_fast_en, u8 rx_fast_en)
{
    if (tx_fast_en || rx_fast_en) {
        /* get 1M fast settle calib value */
        u32 clock_now;
        STOP_RF_STATE_MACHINE;
        rf_set_tx_rx_off();
        CLEAR_ALL_RFIRQ_STATUS;

        rf_ble_set_tx_settle(110 - PRMBL_EXTRA_1M * 8); //attention:here tx settle time use no fast settle value
        ble_rf_set_tx_dma(0, 17);
        HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
        HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;

        u8  tmp_tx_buff[] = {8, 0, 0, 0, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
        u32 dma_len       = rf_tx_packet_dma_len(8);
        tmp_tx_buff[0]    = U32_BYTE0(dma_len);
        tmp_tx_buff[1]    = U32_BYTE1(dma_len);
        tmp_tx_buff[2]    = U32_BYTE2(dma_len);
        tmp_tx_buff[3]    = U32_BYTE3(dma_len);

        for (unsigned char chn = 0; chn <= 80; chn++) {
            rf_set_chn(chn); //here, fast_settle.tx_fast_en can not be enable !!!
            clock_now = clock_time();
            rf_start_fsm(FSM_STX, (void *)tmp_tx_buff, clock_now);
            while (!clock_time_exceed(clock_now, 110))
                ;
            rf_tx_fast_settle_get_cal_val(TX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_1M);
            STOP_RF_STATE_MACHINE;
            rf_set_tx_rx_off();
            CLEAR_ALL_RFIRQ_STATUS;

            if (!tx_fast_en) {
                break;
            }
        }

        rf_set_rx_settle_time(85);                    //adjust RX settle time
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
        rf_start_fsm(FSM_SRX, NULL, clock_time());
        delay_us(85);                                 //Wait for the rx packetization action to complete
        rf_set_tx_rx_off_auto_mode();                 //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_set_chn(chn);
            ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
            rf_start_fsm(FSM_SRX, NULL, clock_time());
            delay_us(85);                                 //Wait for the rx packetization action to complete

            rf_rx_fast_settle_get_cal_val(RX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_1M);

            rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
            rf_clr_irq_status(FLD_RF_IRQ_ALL);
        }


#if (!ESL_RAM_OPTIMIZATION)
        /* get 2M fast settle calib value */
        rf_ble_set_2m_phy();
        STOP_RF_STATE_MACHINE;
        rf_set_tx_rx_off();
        CLEAR_ALL_RFIRQ_STATUS;

        rf_ble_set_tx_settle(110 - PRMBL_EXTRA_2M * 8); //attention:here tx settle time use no fast settle value
        ble_rf_set_tx_dma(0, 17);
        HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
        HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;


        dma_len        = rf_tx_packet_dma_len(8);
        tmp_tx_buff[0] = U32_BYTE0(dma_len);
        tmp_tx_buff[1] = U32_BYTE1(dma_len);
        tmp_tx_buff[2] = U32_BYTE2(dma_len);
        tmp_tx_buff[3] = U32_BYTE3(dma_len);

        for (unsigned char chn = 0; chn <= 80; chn++) {
            rf_set_chn(chn); //here, fast_settle.tx_fast_en can not be enable !!!
            clock_now = clock_time();
            rf_start_fsm(FSM_STX, (void *)tmp_tx_buff, clock_now);
            while (!clock_time_exceed(clock_now, 110))
                ;
            rf_tx_fast_settle_get_cal_val(TX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_2M);
            STOP_RF_STATE_MACHINE;
            rf_set_tx_rx_off();
            CLEAR_ALL_RFIRQ_STATUS;

            if (!tx_fast_en) {
                break;
            }
        }

        rf_set_rx_settle_time(85);                    //adjust RX settle time
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
        rf_start_fsm(FSM_SRX, NULL, clock_time());
        delay_us(85);                                 //Wait for the rx packetization action to complete
        rf_set_tx_rx_off_auto_mode();                 //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_set_chn(chn);
            ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
            rf_start_fsm(FSM_SRX, NULL, clock_time());
            delay_us(85);                                 //Wait for the rx packetization action to complete

            rf_rx_fast_settle_get_cal_val(RX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_2M);

            rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
            rf_clr_irq_status(FLD_RF_IRQ_ALL);
        }


        /* get S2 fast settle calib value */
        rf_ble_set_coded_phy_common();
        rf_ble_set_coded_phy_s2();
        STOP_RF_STATE_MACHINE;
        rf_set_tx_rx_off();
        CLEAR_ALL_RFIRQ_STATUS;

        rf_ble_set_tx_settle(110 - PRMBL_EXTRA_Coded * 8); //attention:here tx settle time use no fast settle value
        ble_rf_set_tx_dma(0, 17);
        HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
        HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;


        dma_len        = rf_tx_packet_dma_len(8);
        tmp_tx_buff[0] = U32_BYTE0(dma_len);
        tmp_tx_buff[1] = U32_BYTE1(dma_len);
        tmp_tx_buff[2] = U32_BYTE2(dma_len);
        tmp_tx_buff[3] = U32_BYTE3(dma_len);

        for (unsigned char chn = 0; chn <= 80; chn++) {
            rf_set_chn(chn); //here, fast_settle.tx_fast_en can not be enable !!!
            clock_now = clock_time();
            rf_start_fsm(FSM_STX, (void *)tmp_tx_buff, clock_now);
            while (!clock_time_exceed(clock_now, 110))
                ;
            rf_tx_fast_settle_get_cal_val(TX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_S2);
            STOP_RF_STATE_MACHINE;
            rf_set_tx_rx_off();
            CLEAR_ALL_RFIRQ_STATUS;

            if (!tx_fast_en) {
                break;
            }
        }

        rf_set_rx_settle_time(85);                    //adjust RX settle time
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
        rf_start_fsm(FSM_SRX, NULL, clock_time());
        delay_us(85);                                 //Wait for the rx packetization action to complete
        rf_set_tx_rx_off_auto_mode();                 //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_set_chn(chn);
            ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
            rf_start_fsm(FSM_SRX, NULL, clock_time());
            delay_us(85);                                 //Wait for the rx packetization action to complete

            rf_rx_fast_settle_get_cal_val(RX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_S2);

            rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
            rf_clr_irq_status(FLD_RF_IRQ_ALL);
        }

        /* get S8 fast settle calib value */
        rf_ble_set_coded_phy_common();
        rf_ble_set_coded_phy_s8();
        STOP_RF_STATE_MACHINE;
        rf_set_tx_rx_off();
        CLEAR_ALL_RFIRQ_STATUS;

        rf_ble_set_tx_settle(110 - PRMBL_EXTRA_Coded * 8); //attention:here tx settle time use no fast settle value
        ble_rf_set_tx_dma(0, 17);
        HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
        HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;


        dma_len        = rf_tx_packet_dma_len(8);
        tmp_tx_buff[0] = U32_BYTE0(dma_len);
        tmp_tx_buff[1] = U32_BYTE1(dma_len);
        tmp_tx_buff[2] = U32_BYTE2(dma_len);
        tmp_tx_buff[3] = U32_BYTE3(dma_len);

        for (unsigned char chn = 0; chn <= 80; chn++) {
            rf_set_chn(chn); //here, fast_settle.tx_fast_en can not be enable !!!
            clock_now = clock_time();
            rf_start_fsm(FSM_STX, (void *)tmp_tx_buff, clock_now);
            while (!clock_time_exceed(clock_now, 110))
                ;
            rf_tx_fast_settle_get_cal_val(TX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_S8);
            STOP_RF_STATE_MACHINE;
            rf_set_tx_rx_off();
            CLEAR_ALL_RFIRQ_STATUS;

            if (!tx_fast_en) {
                break;
            }
        }

        rf_set_rx_settle_time(85);                    //adjust RX settle time
        ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
        rf_start_fsm(FSM_SRX, NULL, clock_time());
        delay_us(85);                                 //Wait for the rx packetization action to complete
        rf_set_tx_rx_off_auto_mode();                 //STOP_RF_STATE_MACHINE;
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_set_chn(chn);
            ble_rf_set_rx_dma((u8 *)glb_temp_rx_buff, 4); //Switch dma rx buffer to ADV's dma rx buffer
            rf_start_fsm(FSM_SRX, NULL, clock_time());
            delay_us(85);                                 //Wait for the rx packetization action to complete

            rf_rx_fast_settle_get_cal_val(RX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_S8);

            rf_set_tx_rx_off_auto_mode(); //STOP_RF_STATE_MACHINE;
            rf_clr_irq_status(FLD_RF_IRQ_ALL);
        }

#endif //(!ESL_RAM_OPTIMIZATION)
    }
        g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_1M;
        for (unsigned char chn = 4; chn <= 80; chn += 10) {
            rf_tx_fast_settle_set_cal_val(TX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_1M);
            rf_rx_fast_settle_set_cal_val(RX_FAST_SETTLE_LEVEL, chn, (rf_fast_settle_t *)&fast_settle_1M);
        }

        STOP_RF_STATE_MACHINE;
        rf_set_tx_rx_off();
        CLEAR_ALL_RFIRQ_STATUS;

        rf_fast_settle_config(TX_FAST_SETTLE_LEVEL, RX_FAST_SETTLE_LEVEL);
        if (tx_fast_en) {
            rf_set_tx_settle_time(TX_FAST_SETTLE_TIME);
            rf_tx_fast_settle_en();
        }

        if (rx_fast_en) {
            rf_set_rx_settle_time(RX_FAST_SETTLE_TIME);
            rf_rx_fast_settle_en();
        }

        CLEAR_ALL_RFIRQ_STATUS;
        HAL_BLE_STACK_RF_IRQ_MASK_SET;


    rf_ble_set_1m_phy();
    /* make sure "fast_settle.tx_fast_en" set after "rf_set ble_channel" above,
         * because  "rf_set ble_channel" internal will judge if fast settle prepared OK */
    fast_settle_1M.tx_fast_en = tx_fast_en;
    fast_settle_1M.rx_fast_en = rx_fast_en;
}
#endif

///////////////////////////////////////////////////////////////////////

#if 1
/**
     * TODO If we want to be compatible with PCL and customer specific, code logic will be very complex.
     *      Currently we have not come up with a good solution to solve that. So we decide not to implement this for now.
     *      That will not cause any problems. i.e disable PCL and customer specification function.
     *      Later we will design it carefully. SIHUI/QIUWEI/YAFEI
     */
_attribute_ram_code_ void blt_ll_set_tx_power_by_strategy(rf_tx_power_strategy_t pwrCtrl_flag, u8 pwrCtrl_power)
{
    (void)pwrCtrl_flag;
    (void)pwrCtrl_power;
}
#else
extern unsigned char                       txPower_index;
volatile _attribute_data_retention_sec_ u8 pcl_txPower_index_backup = 0xFF;
volatile _attribute_data_retention_sec_ u8 app_txPower_index_backup = 0xFF;

/**
 * @brief It is used to set rf tx power.
 *        Note: must be ram code.
 * @param index -- the index that set rf power.
 */
_attribute_ram_code_ void blt_ll_set_power_level_index(u8 index)
{
    if ((pcl_txPower_index_backup != 0xFF) && (pcl_txPower_index_backup != index)) {
        rf_set_power_level_index(index);
    }
}

/**
 * @brief This API is used to set rf tx power. Compatible with power control & customer specification & normal setting.
 *        Note: must be ram code.
 *        case 1: When PCL module is initiated and the first parameter pwrCtrl_flag is TX_POWER_STRATEGY_PCL,
 *                the second parameter pwrCtrl_power will be used to set rf tx power.
 *
 *        case 2: When PCL module is initiated and the first parameter pwrCtrl_flag is not TX_POWER_STRATEGY_PCL, NOT use pwrCtrl_power to set rf power.
 *                2.1. if customer specific the different phy tx power, will use the power index that customer specific.
 *                2.2. if customer do not specific tx power, will use the latest power index(rf_set_power_level_index) to set rf power.
 *
 *        case 3: When PCL module is not initiated
 *                3.1. if customer specific the different phy tx power, will use the power index that customer specific.
 *                3.2. if customer do not specific tx power, will use the latest power index(rf_set_power_level_index) to set rf power.
 *
 * @param pwrCtrl_flag  -- indicate whether use PCL. Because not all power setting need to use PCL power.
 *                         such as advertise or scan don't use PCL power even though enable PCL function.
 * @param pwrCtrl_power -- the power index to set where need to use PCL power.
 */
_attribute_ram_code_ void blt_ll_set_tx_power_by_strategy(rf_tx_power_strategy_t pwrCtrl_flag, u8 pwrCtrl_power)
{
    if (blmsParam.pwr_ctrl_en && (pwrCtrl_flag == TX_POWER_STRATEGY_PCL)) {
        app_txPower_index_backup = txPower_index;

        blt_ll_set_power_level_index(pwrCtrl_power);

        pcl_txPower_index_backup = txPower_index;
        txPower_index            = app_txPower_index_backup;
    } else {
    #if (0)
            //TODO: consider different PHY customized power
    #endif
        {
            blt_ll_set_power_level_index(app_txPower_index_backup);
        }
    }
}
#endif


_attribute_ram_code_ bool blc_ll_isRfBusy(void)
{
    if (blm_btxbrx_state) {
        return true;
    }
    if (blmsParam.rf_fsm_busy) {
        return true;
    }
    return false;
}

ble_status_t blc_ll_getBleCurrentState(void)
{
    ble_status_t current_status_mask;
    current_status_mask = BLE_STATUS_IDLE;

    if (bltSche.task_mask & TSKMSK_LEG_ADV) {
        current_status_mask |= BLE_STATUS_ADVERTISING;
    }

    if (bltSche.task_mask & TSKMSK_PRICHN_SCAN) {
        current_status_mask |= BLE_STATUS_SCANNING;
    }

    if ((bltSche.task_mask & TSKMSK_ACL_MASTER_ALL) || (bltSche.task_mask & TSKMSK_ACL_SLAVE_ALL)) {
        current_status_mask |= BLE_STATUS_CONNECTED;
    }

    if (bltSche.task_mask & TSKMSK_EXT_ADV_ALL) {
        current_status_mask |= BLE_STATUS_EXT_ADVERTISING;
    }

    if (blmsParam.create_connection == CONNECT_REQ_GOING) {
        current_status_mask |= BLE_STATUS_INITIATING;
    }


    // If not indicated, also add
    if (bltSche.task_mask && (current_status_mask == BLE_STATUS_IDLE)) {
        current_status_mask |= BLE_STATUS_UNKNOWN;
    }

    return current_status_mask;
}

#ifdef BLC_ZEPHYR_BLE_INTEGRATION
u32 blc_ll_checkBleRfFsmIsBusy(void)
{
    return blmsParam.rf_fsm_busy;
}
#endif
