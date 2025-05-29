/********************************************************************************************************
 * @file    hdt_phy_test.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "hdt_phy_test.h"
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
typedef struct
{
    u8  cmd;
    u8  tx_start;
    u16 pkts;

    u32 tick_tx;
    u32 interval;

} phy_data_t;

static hdt_rf_mode_e hdt_phy_test_mode = RF_MODE_BLE_HDT_SHORT_FORMAT;
extern _attribute_data_retention_ phy_data_t blt_phyTest;

void blt_InitHdtPhyTestDriver(hdt_rf_mode_e mode)
{
    hdt_phy_test_mode = mode;
}

ble_sts_t blc_phy_setHdtPhyTestEnable(u8 en)
{
    u32 r = irq_disable();


    irq_restore(r);

    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_hci_setReceiverTest_V3(hci_le_receiverTestV3_cmdParam_t *param)
{
    u8 rx_chn           = param->rxChn;
    u8 phy_mode         = param->phy;
    u8 modulation_index = param->modulationIndex;

    #define PHY_TEST_CMD_HDT_RX        5  //PHY_CMD_HDT_RX
    #define PHY_TEST_CMD_RX            1  //PHY_CMD_RX

    if (phy_mode != BLE_PHY_HDT){
        blt_phyTest_hci_setReceiverTest_V2(param);
    }
    else{
        HDT_HCI_LOG("phy_test rx v3");
        blt_phyTest.pkts = 0;

        //hdt rf init      ->rf_mode_init();
        if (!blmsParam.phytest_en) { //must set phy mode
            //blc_hdtphy_setPhyTestEnable        ->blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
        }

        //hdt driver init        ->blt_InitPhyTestDriver(RF_MODE_LR_S2_500K);

        if (modulation_index == 0x00) {
        } else if (modulation_index == 0x01) {
        }
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        }
        //hdt tx_rx off         ->rf_set_tx_rx_off_auto_mode();rf_set_tx_rx_off();
        //hdt rf set chn        ->rf_set_ble_channel(phyTest_Channel(rx_chn));
        //hdt rx timeout        ->reg_rf_ll_rx_fst_timeout = 0x0fffffff;
        //hdt rx machine        ->rf_start_fsm(FSM_SRX, NULL, reg_system_tick + 50);
        //hdt rx calib          ->set_rf_rxCalibrationFunction(1, rx_chn);
        //hdt_rx settle         ->rf_set_rx_settle_time(85);
        //hdt_rx_max_len        ->rf_set_rx_maxlen(0xff); //max length is 255
        blt_phyTest.cmd = PHY_TEST_CMD_HDT_RX;
    }

    #undef PHY_TEST_CMD_HDT_RX
    #undef PHY_TEST_CMD_RX

    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_hci_setTransmitterTest_V5(hci_le_transmitterTestV5_cmdParam_t *param)
{
    hci_le_transmitterTestV5_remain_cmdParam_t *p = (hci_le_transmitterTestV5_remain_cmdParam_t*) (&param->antenna_ids[param->switching_pattern_len]);
    //power
    s8 TX_Power_Level = p->txPowerLevel;
    if ((TX_Power_Level != 0x7E)&&(TX_Power_Level != 0x7F)&&(TX_Power_Level > 20 || TX_Power_Level < -127)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    //for GFSK PHY
    if(param->phy != BLE_PHY_HDT){

        rf_power_level_index_e index    = rf_ble_get_tx_pwr_idx(TX_Power_Level);
        rf_set_power_level_index(index);
        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }
#if 0//todo missing parameters
        hdt_testParam.hdtRateInd    = HDT_RATE_2M;
        hdt_testParam.hdtPfi        = HDT_PKT_FORMAT0_SHORT_FORMAT;
        hdt_testParam.hdtHeaderlen  = 0x01;
        hdt_testParam.hdtPhyIntvl   = 0x00;
        for(int i = 0; i< 4; i++){
            hdt_testParam.hdtNumBlocks[i]  = 0x00;
            hdt_testParam.hdtBlockSize[i]  = 0x01FF;
            hdt_testParam.hdtFinalBlockSize[i] = 0x01FF;
        }
#endif
        return blt_phyTest_hci_setTransmitterTest_V2((hci_le_transmitterTestV2_cmdParam_t *)param);
    }
    //for HDT PHY
    else{
        if ((p->hdtPfi ==  HDT_PKT_FORMAT1) && (p->hdtRateInd == HDT_RATE_SHORT_FORMAT)) {
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
        #define PHY_TEST_CMD_HDT_TX        4
        #define PHY_TEST_CMD_HDT_RX        5

        //rf power setting
        //rf hdt phy switch
        if (!blmsParam.phytest_en) { //must set hdt phy mode
            blc_phy_setHdtPhyTestEnable(BLC_PHYTEST_ENABLE);
        }
        HDT_HCI_LOG("hdt phy_test tx v5");

        //hdt_rf_mode_int ->rf_mode_init();
        //hdt phy driver

        if (p->hdtRateInd == HDT_RATE_2M) {
            //blt_InitHdtPhyTestDriver(HDT_RATE_2M);
            //blt_phyTest.interval = blt_getPktInterval(p->hdtHeaderlen, HDT_RATE_2M);
        }else{
            //...
        }

        if(p->hdtPfi ==  HDT_PKT_FORMAT0_SHORT_FORMAT){
            if(p->hdtRateInd == HDT_RATE_SHORT_FORMAT){
                //short format
                //ignore p->hdtBlockSize;p->hdtFinalBlockSize;p->hdtHeaderlen;p->hdtNumBlocks;p->hdtPhyIntvl;
            }
            else{
                //format 0
                //p->hdtHeaderlen : PDU header + payload length
                //ignore p->hdtBlockSize;p->hdtFinalBlockSize;p->hdtNumBlocks;p->hdtPhyIntvl;

            }
        }else if(p->hdtPfi ==  HDT_PKT_FORMAT1){
            //format 1
            //p->hdtHeaderlen : PDU header length only
        }

        //payload type : prbs9 ...
        //assign packet content

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_TX_ON);
        }
        // set ble hdt chn     -> rf_set_ble_channel(phyTest_Channel(tx_chn));
        //dma reset

        blt_phyTest.pkts        = 0;
        blt_phyTest.tx_start    = 1;
        blt_phyTest.cmd         = PHY_TEST_CMD_HDT_TX;

        #undef PHY_TEST_CMD_HDT_TX
        #undef PHY_TEST_CMD_HDT_RX
    }

    return BLE_SUCCESS;
}

#endif
