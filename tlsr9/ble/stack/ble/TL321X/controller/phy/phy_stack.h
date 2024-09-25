/********************************************************************************************************
 * @file    phy_stack.h
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
#ifndef PHY_STACK_H_
#define PHY_STACK_H_


#include "tl_common.h"
#include "stack/ble/hci/hci_cmd.h"


/******************************* phy start *************************************************************************/

#define         RFLEN_255_1MPHY_US                              2120
#define         RFLEN_255_2MPHY_US                              1064
#define         RFLEN_255_CODEDPHY_S2_US                        4542
#define         RFLEN_255_CODEDPHY_S8_US                        17040


typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u8  tx_phys;
    u8  rx_phys;
}rf_pkt_ll_phy_req_rsp_t;   //phy_req, phy_rsp

typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u8  m_to_s_phy;
    u8  s_to_m_phy;
    u8  instant0;
    u8  instant1;
}rf_pkt_ll_phy_update_ind_t;   //phy_req, phy_rsp

typedef struct {
    u8  dft_tx_prefer_phys;
    u8  dft_rx_prefer_phys;
    u8  dft_prefer_phy;
    u8  dft_CI;

    //for Extended ADV
    u8  cur_llPhy;  //"le_phy_type_t"    1:1M    2:2M   3:Coded
    u8  cur_own_CI;  //TX cur_coding_ind
    u8  cur_peer_CI; //IRQ variable, current Peer CI
    u8  tx_stl_adv;

    u8  tx_stl_tifs;
    u8  peer_oneByte_us; //1M: 8uS;  2M: 4uS; Coded S2: 16uS; Coded S8: 64uS
    u8  own_oneByte_us; //1M: 8uS;  2M: 4uS; Coded S2: 16uS; Coded S8: 64uS
    u8  extra_preamble; //extra preamble numbers
    /* T1 definition
     * timing after "access_code" to packet tail
     * 1M:       (rf_len+5)*8          = rf_len*8 + 40          5 = 2(header)+3(CRC)
     * 2M:       (rf_len+5)*4          = rf_len*4 + 20          5 = 2(header)+3(CRC)
     * Coded S8: rf_len*64 + 720 - 336 = rf_len*64 + 384        336: preamble 80uS + accesscode 256uS
     * Coded S2: rf_len*16 + 462 - 336 = rf_len*16 + 126        336: preamble 80uS + accesscode 256uS
     *
     * timing after "access_code" = rf_len*oneByte_us + T1, then T1 is :
     * 1M:        T1 = 40
     * 2M:        T1 = 20
     * Coded S8:  T1 = 384
     * Coded S2:  T1 = 126
     *
     * T2 definition
     * T2 = other_switch_delay, include RX to TX switch, TX settle to TX sending switch time
     *      not very big, maybe just several uS, less than 10uS
     *
     *
     * TX_trigger_tick  = hal_rf_get_rx_timestamp() + (rf_len*oneByte_us + TIFS_offset_us) *SYSTEM_TIMER_TICK_1US;
     *
     *
     * AD_convert_delay + LL_TX_STL_TIFS + TX_compensation = 150
     *
     * AD_convert_delay is different for different MCUs and different PHYs
     *
     *
     * 1M:    TX_compensation = 150 - TX_STL_TIFS_REAL_1M - AD_convert_delay_1M
     * 2M:    TX_compensation = 150 - TX_STL_TIFS_REAL_2M - AD_convert_delay_2M
     * Coded: TX_compensation = 150 - TX_STL_TIFS_REAL_CODED - AD_convert_delay_Coded
     *
     * TIFS_offset_us
     * 1M:       TIFS_offset_us = TX_compensation + T1  = 190 - TX_STL_TIFS_REAL_1M     - AD_CONVERT_DLY_1M
     * 2M:       TIFS_offset_us = TX_compensation + T1  = 170 - TX_STL_TIFS_REAL_2M     - AD_CONVERT_DLY_2M
     * Coded S8: TIFS_offset_us = TX_compensation + T1 = 534 - TX_STL_TIFS_REAL_CODED - AD_CONVERT_DLY_CODED
     * Coded S2: TIFS_offset_us = TX_compensation + T1 = 276 - TX_STL_TIFS_REAL_CODED - AD_CONVERT_DLY_CODED
     */
    u16 TIFS_offset_us;

    /* preamble + access_code:  1M: 5*8=40uS;  2M: 6*4=24uS;  Coded: 80+256=336uS */
    /* AD_convert_delay : timing cost on RF analog to digital convert signal process:
     *                  Eagle   1M: 20uS       2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
     *                  Jaguar  1M: 20uS       2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
     *                  data is come from Xuqiang.Zhang
     *
     *  prmb_ac_us + AD_convert_delay:
     *           1M: 40 + 20 = 60 uS
     *           2M: 24 + 10 = 34 uS
     *        Coded: 336 + 14 = 350 uS
     * */
    u16 prmb_ac_us; //
}ll_phy_t;

typedef enum {
    BLE_PHY_NONE        = 0x00, //different from BLE_PHY_1M/BLE_PHY_2M/BLE_PHY_CODED
} le_phy_ext_type_t;

typedef enum{
    LE_CI_NONE = 0,  //when setting 1M/2M PHY, use this to distinguish, for code readability
    LE_CODED_S2 = 2,
    LE_CODED_S8 = 8,
}le_coding_ind_t;

//do not support Asymmetric PHYs, conn_phys = tx_phys & rx_phys
typedef struct {
    u8  conn_prefer_phys;  // conn_prefer_phys = tx_prefer_phys & rx_prefer_phys
    u8  conn_cur_phy;      //
    u8  conn_next_phy;     //
    u8  conn_cur_CI;       // CI: coding_ind

    u8  conn_next_CI;
    u8  phy_req_trigger;  // 1: means current device triggers phy_req, due to API "blc_ll_setPhy" called by Host or Application
    u8  phy_req_pending;
    u8  phy_update_pending;

    u32 conn_updatePhy;

    u8 conn_last_phy;
    u8 align[3];

    #if 0
        u8  tx_prefer_phys;     //phy set
        u8  rx_prefer_phys;
        u8  tx_next_phys;
        u8  rx_next_phys;

        u8  cur_tx_phy;     //phy using
        u8  cur_rx_phy;
        u16 rsvd;
    #endif

}ll_conn_phy_t;

typedef int (*llms_conn_phy_update_callback_t)(u16 connHandle);
typedef int (*llms_conn_phy_switch_callback_t)(u16 connHandle);
typedef void (*ll_phy_switch_callback_t)(le_phy_type_t, le_coding_ind_t);

typedef void (*ll_coded_phy_ind_detect_callback_t)(u8);
extern ll_coded_phy_ind_detect_callback_t ll_coded_phy_ind_detect_cb;

extern  llms_conn_phy_update_callback_t llms_conn_phy_update_cb; ///blt_ll_updateConnPhy
extern  llms_conn_phy_switch_callback_t llms_conn_phy_switch_cb; ///blt_ll_switchConnPhy
extern  ll_phy_switch_callback_t        ll_phy_switch_cb;

extern const u8   tx_stl_auto_mode[4];
extern const u8   tx_stl_btx_1st_pkt[4];


extern _attribute_aligned_(4) ll_phy_t      bltPHYs;

int  blt_phy_getRfPacketTime_us(int rf_len, le_phy_type_t phy, le_coding_ind_t ci);
void rf_ble_switch_phy(le_phy_type_t phy, le_coding_ind_t own_coding_ind);
void blt_ll_set_peer_codePhy_CI(le_coding_ind_t coding_ind);
void blt_ll_phy_param_reset(void);;
/******************************* phy end ***************************************************************************/






/******************************* phy_test start *************************************************************************/
    int       blt_phyTest_main_loop(void);


/**
* @brief   This function is used to receive and parse HCI instructions for sending tests in V1 format.
* @param   hci_le_transmitterTestV1_cmdParam_t:V1 HCI sends the parameter resolution struct of the test command;
* @return  ble_sts_t:Returns the instruction completion status.
*/
ble_sts_t blt_phyTest_hci_setTransmitterTest_V1(hci_le_transmitterTestV1_cmdParam_t *param);

/**
* @brief   This function is used to receive and parse HCI instructions for receive tests in V1 format.
* @param   hci_le_transmitterTestV1_cmdParam_t:V1 HCI parameter resolution struct of the receive test command;
* @return  ble_sts_t:Returns the instruction completion status.
*/
ble_sts_t blt_phyTest_hci_setReceiverTest_V1(hci_le_receiverTestV1_cmdParam_t *param);

/**
* @brief   This function is used to receive and parse HCI instructions for transmit tests in V2 format.
* @param   hci_le_transmitterTestV1_cmdParam_t:V2 HCI parameter resolution struct of the transmit test command;
* @return  ble_sts_t:Returns the instruction completion status.
*/
ble_sts_t blt_phyTest_hci_setTransmitterTest_V2(hci_le_transmitterTestV2_cmdParam_t *param);

/**
* @brief   This function is used to receive and parse HCI instructions for receive tests in V2 format.
* @param   hci_le_transmitterTestV1_cmdParam_t:V2 HCI parameter resolution struct of the receive test command;
* @return  ble_sts_t:Returns the instruction completion status.
*/
ble_sts_t blt_phyTest_hci_setReceiverTest_V2(hci_le_receiverTestV2_cmdParam_t *param);

ble_sts_t blt_phyTest_setTestEnd(u8 *pkt_num);


    //void blc_phy_preamble_length_set(unsigned char len);
    void blt_InitPhyTestDriver(rf_mode_e rf_mode);


/******************************* phy_test end ***************************************************************************/



#endif /* PHY_STACK_H_ */


