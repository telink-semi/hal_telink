/********************************************************************************************************
 * @file    phy.c
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


_attribute_ble_data_retention_ _attribute_aligned_(4) ll_phy_t bltPHYs = {
    /* Does not depend on whether the function is registered: blc_ll_init2MPhyCodedPhy_feature */
    .cur_llPhy       = BLE_PHY_1M,             //default 1M
    .cur_own_CI      = LE_CODED_S8,            //default S8
    .cur_peer_CI     = LE_CODED_S8,            //default S8
    .tx_stl_adv      = TX_STL_ADV_SET_1M,
    .tx_stl_tifs     = TX_STL_TIFS_SET_1M,
    .own_oneByte_us  = 8,                      //default 1M
    .peer_oneByte_us = 8,                      //default 1M
    .extra_preamble  = PRMBL_EXTRA_1M,         //default extra preamble account for 5
    .TIFS_offset_us  = 190 - TX_STL_TIFS_REAL_1M - HW_DELAY_1M,
    .prmb_ac_us      = 40 + AD_CONVERT_DLY_1M, //default 1Mt timing: 5(preamble 1B + access_code 4B) * 8 = 40
};


#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)

/**************************************************************************************************************************
 *     PHYs           timing(uS)
 *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
 *
 *  Coded PHY :    376 + (N*8+27)*S
 *               = 376 + ((rf_len+2)*8+27)*S
 *               = 376 + (rf_len*8+43)*S        // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
 *               = rf_len*S*8 + 43*S + 376
 *      S2    :  = rf_len*16 + 462
 *      S8    :  = rf_len*64 + 720
 *
 *       Empty packet time                  rf_len = 27 packet time         rf_len = 255 packet time
 *    1M    PHY     :    80 uS                      296 uS                          2120 uS
 *    2M    PHY     :    44 uS                      152 uS                          1064 uS
 *    COded PHY S2  :   462 uS                      894 uS                          4542 uS
 *    COded PHY S8  :   720 uS                     2448 uS                         17040 uS
 *************************************************************************************************************************/


/*******************************************************************************************************************************
 *  tx sequence          CMD trigger -> tx settle -> preamble ->access code -> PDU -> CRC
 *
 * in each ConnInterval, slave will calculate the TxTimestamp by master, TxTimestamp(master) = RxTimestamp - accessCode - preamble
 *
 *  PHYs            time(accessCode + preamble)
 *  1M      PHY     5*8 = 40us
 *  2M      PHY     (2*8 + 4*8)/2 = 24us
 *  coded   PHY     80us + 256us = 336us
 *
 *******************************************************************************************************************************/

_attribute_ble_data_retention_ ll_phy_switch_callback_t           ll_phy_switch_cb           = NULL;
_attribute_ble_data_retention_ ll_coded_phy_ind_detect_callback_t ll_coded_phy_ind_detect_cb = NULL;

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
#if (BLE_S2_S8_NEW_PATH)
    u8 tx_stl_auto_mode[5] = {0, TX_STL_AUTO_MODE_1M, TX_STL_AUTO_MODE_2M, TX_STL_AUTO_MODE_CODED_S2, TX_STL_AUTO_MODE_CODED_S8};
    #if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
    u8 tx_rxPathDly_extraPreamble[5]  = {0, RX_PATH_DLY_EXTRA_PREAMBLE_1M, RX_PATH_DLY_EXTRA_PREAMBLE_2M, RX_PATH_DLY_EXTRA_PREAMBLE_S2, RX_PATH_DLY_EXTRA_PREAMBLE_S8}; //150-125;150-133;150-124
    #endif
#else
    u8 tx_stl_auto_mode[4] = {0, TX_STL_AUTO_MODE_1M, TX_STL_AUTO_MODE_2M, TX_STL_AUTO_MODE_CODED};
    #if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
    u8 tx_rxPathDly_extraPreamble[4]  = {0, RX_PATH_DLY_EXTRA_PREAMBLE_1M, RX_PATH_DLY_EXTRA_PREAMBLE_2M, RX_PATH_DLY_EXTRA_PREAMBLE_CODED}; //150-125;150-133;150-124
    #endif
#endif

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u8    tx_stl_btx_1st_pkt[4] = {0, TX_STL_BTX_1ST_PKT_SET_1M, TX_STL_BTX_1ST_PKT_SET_2M, TX_STL_BTX_1ST_PKT_SET_CODED};


///////////////////multiple master and multiple slave////////////////////
_attribute_ble_data_retention_ llms_conn_phy_update_callback_t llms_conn_phy_update_cb = NULL; ///blt_ll_updateConnPhy
_attribute_ble_data_retention_ llms_conn_phy_switch_callback_t llms_conn_phy_switch_cb = NULL; ///blt_ll_switchConnPhy

///////////////////end of multiple master and multiple slave///////////////


_attribute_ram_code_ int blt_phy_getRfPacketTime_us(int rf_len, le_phy_type_t phy, le_coding_ind_t ci)
{
    if (phy == BLE_PHY_1M) {
        return (rf_len + 10) * 8;
    } else if (phy == BLE_PHY_2M) {
        return (rf_len + 11) * 4;
    } else {
        if (ci == LE_CODED_S8) {
            return (rf_len * 64 + 720); //376 + (rf_len*8+43)*8 = 376 + 344 + rf_len*64 = 720 + rf_len*64
        } else {                        //LE_CODED_S2
            return (rf_len * 16 + 462); // 376 + (rf_len*8+43)*2; //
        }
    }
}

_attribute_ram_code_ void rf_ble_set_coded_phy_ind(le_coding_ind_t own_coding_ind)
{
    bltPHYs.cur_own_CI = own_coding_ind;

    if (own_coding_ind == LE_CODED_S2) {
        rf_ble_set_coded_phy_s2();
        bltPHYs.own_oneByte_us  = 16;
        bltPHYs.peer_oneByte_us = 16;
        bltPHYs.TIFS_offset_us  = 276 - TX_STL_TIFS_REAL_CODED - HW_DELAY_CODED;
    } else { // S8
        rf_ble_set_coded_phy_s8();
        bltPHYs.own_oneByte_us  = 64;
        bltPHYs.peer_oneByte_us = 64;
        bltPHYs.TIFS_offset_us  = 534 - TX_STL_TIFS_REAL_CODED - HW_DELAY_CODED;
    }
}
    #if FAST_SETTLE
extern _attribute_data_retention_sec_ rf_fast_settle_t *g_fast_settle_cal_val_ptr;
    #endif
_attribute_ram_code_ void rf_ble_switch_phy(le_phy_type_t phy, le_coding_ind_t own_coding_ind)
{
    if (phy != bltPHYs.cur_llPhy) {
        bltPHYs.cur_llPhy = phy;
        if (phy == BLE_PHY_1M) //BLE1M 01
        {
            rf_ble_set_1m_phy();
    #if FAST_SETTLE
            g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_1M;
    #endif
            bltPHYs.tx_stl_adv     = TX_STL_ADV_SET_1M;
            bltPHYs.tx_stl_tifs    = TX_STL_TIFS_SET_1M;
            bltPHYs.own_oneByte_us = bltPHYs.peer_oneByte_us = 8;
            bltPHYs.TIFS_offset_us                           = 190 - TX_STL_TIFS_REAL_1M - HW_DELAY_1M;
            bltPHYs.prmb_ac_us                               = 40 + AD_CONVERT_DLY_1M; //timing: 5(preamble 1B + access_code 4B) * 8 = 40
        }
#if (!ESL_RAM_OPTIMIZATION)
        else if (phy == BLE_PHY_2M) //BLE2M 10
        {
            rf_ble_set_2m_phy();
    #if FAST_SETTLE
            g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_2M;
    #endif
            bltPHYs.tx_stl_adv     = TX_STL_ADV_SET_2M;
            bltPHYs.tx_stl_tifs    = TX_STL_TIFS_SET_2M;
            bltPHYs.own_oneByte_us = bltPHYs.peer_oneByte_us = 4;
            bltPHYs.TIFS_offset_us                           = 170 - TX_STL_TIFS_REAL_2M - HW_DELAY_2M;
            bltPHYs.prmb_ac_us                               = 24 + AD_CONVERT_DLY_2M; //timing: 6(preamble 2B + access_code 4B) * 4 = 24
        } else                                                                         //LE coded PHY
        {
            rf_ble_set_coded_phy_common();
            rf_ble_set_coded_phy_ind(own_coding_ind);
    #if FAST_SETTLE
            if (own_coding_ind == LE_CODED_S2) {
                g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_S2;
            } else {
                g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_S8;
            }
    #endif
            bltPHYs.tx_stl_adv  = TX_STL_ADV_SET_CODED;
            bltPHYs.tx_stl_tifs = TX_STL_TIFS_SET_CODED;
            bltPHYs.prmb_ac_us  = 336 + AD_CONVERT_DLY_CODED; //timing: preamble(10B uncode) + access_code(4B S8) + AD convert delay

            //TODO:When the sys clock is 24/32/48M , it must be delayed, but it is strange not to know why.Add by tyf 190813
            sleep_us(5);
        }
#endif //(!ESL_RAM_OPTIMIZATION)
    }
#if (!ESL_RAM_OPTIMIZATION)
    else if (phy == BLE_PHY_CODED && own_coding_ind != bltPHYs.cur_own_CI) { //phy == bltPHYs.cur_llPhy, but own_coding_ind not same
        rf_ble_set_coded_phy_ind(own_coding_ind);
    #if FAST_SETTLE
        if (own_coding_ind == LE_CODED_S2) {
            g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_S2;
        } else {
            g_fast_settle_cal_val_ptr = (rf_fast_settle_t *)&fast_settle_S8;
        }
    #endif
    }
#endif //(!ESL_RAM_OPTIMIZATION)
}

_attribute_ram_code_ void blt_coded_phy_detect_peer_code_phy_indication(u8 rf_len)
{
    //Hardware bug, rx_pck's status field bit4 always zero.
    //u8 peer_coded_phy_ci = (raw_pkt[DMA_RFRX_OFFSET_STATUS(raw_pkt)] & BIT(4)) ? LE_CODED_S8 : LE_CODED_S2; //S2 or S8    8*125k  2*500k
    //tlkapi_send_string_data(0, "rx_status", &raw_pkt[DMA_RFRX_OFFSET_STATUS(raw_pkt)], 1);

    /*
         * Coded PHY :     376 + (N*8+27)*S
         *               = 376 + ((rf_len+2)*8+27)*S
         *               = 376 + (rf_len*8+43)*S     // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
         *               = rf_len*S*8 + 43*S + 376
         *      S2    :  = rf_len*16 + 462
         *      S8    :  = rf_len*64 + 720
         */
    u32 rx_start_tick = bltRxPkt.rx_timeStamp - bltPHYs.prmb_ac_us * SYSTEM_TIMER_TICK_1US;

    //The following code uses software calculation to get the CI value.
    if ((s32)(bltRxPkt.rx_irq_tick - rx_start_tick) >= (s32)(rf_len * 64 + 720) * SYSTEM_TIMER_TICK_1US) {
        bltPHYs.cur_peer_CI = LE_CODED_S8;

        bltPHYs.peer_oneByte_us = 64;
        bltPHYs.TIFS_offset_us  = 534 - TX_STL_TIFS_REAL_CODED - HW_DELAY_CODED;
    } else {
        bltPHYs.cur_peer_CI = LE_CODED_S2;

        bltPHYs.peer_oneByte_us = 16;
        bltPHYs.TIFS_offset_us  = 276 - TX_STL_TIFS_REAL_CODED - HW_DELAY_CODED;
    }
}

_attribute_noinline_ void blc_ll_init2MPhyCodedPhy_feature(void)
{
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_2M_PHY << 8 | LL_FEATURE_ENABLE_LE_CODED_PHY << 11);
    blmsParam.phy_2mCoded_en = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"

    ll_phy_switch_cb           = rf_ble_switch_phy;
    ll_coded_phy_ind_detect_cb = blt_coded_phy_detect_peer_code_phy_indication;
    llms_conn_phy_update_cb    = blt_ll_updateConnPhy;
    llms_conn_phy_switch_cb    = blt_ll_switchConnPhy;
}
#endif
#if (LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT)
_attribute_noinline_ void blc_ll_initHdtPhy_feature(void)
{
    LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT << 27);
    blmsParam.phy_hdt_en = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"

}


#endif // end of (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
