/********************************************************************************************************
 * @file    phy_test.c
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

/**
 * @brief   PHY test mode enable or disable
 */
#if (LL_FEATURE_SUPPORT_PHY_TEST_MODE)


#define         PHY_DEBUG_FLAG                          0
#define         PHY_CMD_SETUP                           0
#define         PHY_CMD_RX                              1
#define         PHY_CMD_TX                              2
#define         PHY_CMD_END                             3


#define         PKT_TYPE_PRBS9                          0
#define         PKT_TYPE_0X0F                           1
#define         PKT_TYPE_0X55                           2
#define         PKT_TYPE_0XFF                           3

#define         PKT_TYPE_HCI_PRBS9                      0
#define         PKT_TYPE_HCI_0X0F                       1
#define         PKT_TYPE_HCI_0X55                       2
#define         PKT_TYPE_HCI_PRBS15                     3
#define         PKT_TYPE_HCI_0XFF                       4
#define         PKT_TYPE_HCI_0X00                       5
#define         PKT_TYPE_HCI_0XF0                       6
#define         PKT_TYPE_HCI_0XAA                       7


#define TX_PKT_SHARE_SAVE_RAM       1


enum{
    PHY_EVENT_STATUS     = 0,
    PHY_EVENT_PKT_REPORT = 0x8000,
};

enum{
    PHY_STATUS_SUCCESS   = 0,
    PHY_STATUS_FAIL      = 0x0001,
};


typedef struct {
    u8 cmd;
    u8 tx_start;
    u16 pkts;

    u32 tick_tx;
    u32 interval;
}phy_data_t;


static union pkt_length_u
{
    u8 len;
    struct len_t
    {
        u8 low:6;
        u8 upper:2;

    }l;
}pkt_length;

typedef struct{
    unsigned int len;        // data max 252
    unsigned char data[254];
}uart_phy_t;


_attribute_data_retention_  phy_data_t  blt_phyTest;

_attribute_data_retention_ unsigned char rxpara_flag  = 1;

static rf_mode_e phy_test_mode = RF_MODE_BLE_1M;


#if TX_PKT_SHARE_SAVE_RAM //save ramcode
    _attribute_data_retention_  u8 *pkt_phytest;
#else
    u8      pkt_phytest [64] = {
            39, 0, 0, 0,
            0, 37,
            0, 1, 2, 3, 4, 5, 6, 7
    };
#endif

u8  phytest_rx_fifo[288];
u8  phytest_tx_fifo[288];

void blc_phy_initPhyTest_module(void)
{
    blmsParam.phy_test_en = 1;
    blc_main_loop_phyTest_cb = blt_phyTest_main_loop;
}


u8  phyTest_Channel (u8 chn)
{
    if (chn == 0)
    {
        return 37;
    }
    else if (chn < 12)
    {
        return chn - 1;
    }
    else if (chn == 12)
    {
        return 38;
    }
    else if (chn < 39)
    {
        return chn - 2;
    }
    else
    {
        return 39;
    }
}

void phyTest_PRBS9 (u8 *p, int n)
{
    //PRBS9: (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100)
    u16 x = 0x1ff;
    for (int i=0; i<n; i++)
    {
        u8 d = 0;
        for (int j=0; j<8; j++)
        {
            if (x & 1)
            {
                d |= BIT(j);
            }
            x = (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100);
        }
        *p++ = d;
    }
}

void phyTest_PRBS15 (u8 *p, int n)
{
    u16 x = 0x7fff;
    for (int i=0; i<n; i++)
    {
        u8 d = 0;
        for (int j=0; j<8; j++)
        {
            if (x & 1)
            {
                d |= BIT(j);
            }
            x = (x >> 1) | (((x<<13) ^ (x<<14)) & 0x4000);
        }
        *p++ = d;
    }
}

unsigned int blt_getPktInterval(unsigned char payload_len, rf_mode_e mode)
{
    unsigned int total_len,byte_time=8;
    unsigned char preamble_len;
    unsigned int total_time;

    if(mode == RF_MODE_BLE_1M )//1m
    {
        preamble_len = reg_rf_preamble_trail & 0x1f ;
        total_len = preamble_len + 4 + 2 + payload_len +3; // preamble + access_code + header + payload + crc
        byte_time = 8;
        return (((byte_time * total_len + 249  + 624)/625)*625);
    }
    else if(mode == RF_MODE_BLE_2M)//2m
    {
        preamble_len = reg_rf_preamble_trail & 0x1f ;
        total_len = preamble_len + 4 + 2 + payload_len +3; // preamble + access_code + header + payload + crc
        byte_time = 4;
        return (((byte_time * total_len + 249  + 624)/625)*625);
    }
    else if(mode == RF_MODE_LR_S2_500K) // s=2
    {
        byte_time = 2;  //2us/bit
        total_time = (80 + 256 + 16 + 24) + (16 + payload_len*8 + 24 +3)*byte_time; // preamble + access_code + coding indicator + TERM1 + header + payload + crc + TERM2
        return (((total_time + 249  + 624)/625)*625);
    }
    else if(mode == RF_MODE_LR_S8_125K  )//s=8
    {
        byte_time = 8;  //8us/bit
        total_time = (80 + 256 + 16 + 24) + (16 + payload_len*8 + 24 +3)*byte_time; // preamble + access_code + coding indicator + TERM1 + header + payload + crc + TERM2
        return (((total_time + 249  + 624)/625)*625);
    }
    return 0;
}


static void set_rf_rxCalibrationFunction(u8 en,u8 rx_chn)
{
    (void)en; //unused, remove warning
#if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92) || (MCU_CORE_TYPE == MCU_CORE_TL751X))
    delay_us(30);
    unsigned char freq = phyTest_Channel(rx_chn);
    if(rxpara_flag == 1)
    {
        rf_set_rxpara();
        rxpara_flag = 0;
    }

    if(freq == 10 || freq == 21 || freq == 33)
    {
        rf_ldot_ldo_rxtxlf_bypass_en();
    }
    else
    {
        rf_ldot_ldo_rxtxlf_bypass_dis();
    }
#else
    (void)rx_chn; //unused, remove warning
#endif
}

ble_sts_t blt_phyTest_hci_setReceiverTest_V1 (hci_le_receiverTestV1_cmdParam_t *param)
{
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }

    blt_phyTest.pkts = 0;
    rf_set_tx_rx_off();
    rf_set_tx_rx_off_auto_mode();
    u8 rx_chn = param->rxChn;   //frequency
    rf_set_ble_channel ( phyTest_Channel(rx_chn) );
    rf_set_1st_rx_timeout(0x0fffffff);
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
    rf_start_fsm(FSM_SRX, NULL, reg_system_tick+50);
    set_rf_rxCalibrationFunction(1,rx_chn);
    rf_set_rx_settle_time(85);
    rf_set_rx_maxlen(0xff);//max length is 255
    blt_phyTest.cmd = PHY_CMD_RX;
    return BLE_SUCCESS;
}


ble_sts_t blt_phyTest_2wireUart_setTransmitterTest_V1(u8 tx_chn, u8 length, u8 pkt_type,rf_mode_e mod)
{
    unsigned int transLen;
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }


    if (pkt_type == PKT_TYPE_PRBS9)
    {
        pkt_phytest[4] = 0;
        phyTest_PRBS9 (pkt_phytest + 6, length);
    }
    else if (pkt_type == PKT_TYPE_0X0F)
    {
        pkt_phytest[4] = 1;
        memset (pkt_phytest + 6, 0x0f, length);
    }
    else if(pkt_type == PKT_TYPE_0X55)
    {
        pkt_phytest[4] = 2;
        memset (pkt_phytest + 6, 0x55, length);
    }
    else if(pkt_type == PKT_TYPE_0XFF)
    {
        if(mod == RF_MODE_LR_S2_500K || mod == RF_MODE_LR_S8_125K)
        {
            pkt_phytest[4] = 4;
            memset (pkt_phytest + 6, 0xff, length);
        }
    }
    transLen = length + 2;
    transLen = rf_tx_packet_dma_len(transLen);
    pkt_phytest[3] = (transLen >> 24)&0xff;
    pkt_phytest[2] = (transLen >> 16)&0xff;
    pkt_phytest[1] = (transLen >> 8)&0xff;
    pkt_phytest[0] = transLen & 0xff;
    pkt_phytest[5] = length;
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
    rf_set_ble_channel ( phyTest_Channel(tx_chn) );
    rf_set_tx_rx_off_auto_mode();
    rf_clr_tx_rptr(0);
    blt_phyTest.interval = blt_getPktInterval(length,mod);
    /* DMA reset & reset baseband, special situation, debug ongoing, SiHui and QiuWei will process later */
    #if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
        dma_chn_dis(DMA0);  /* reset RF TX DMA channels */
    #endif
    rf_set_tx_dma(0, 288);

    blt_phyTest.pkts = 0;
    blt_phyTest.tx_start = 1;
    blt_phyTest.cmd = PHY_CMD_TX;

    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_hci_setTransmitterTest_V1(hci_le_transmitterTestV1_cmdParam_t *param)
{
    unsigned int transLen;
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }
    u8 pkt_type = param->pktPayload;
    if (pkt_type == PKT_TYPE_HCI_PRBS9)
    {
        pkt_phytest[4] = 0;
        phyTest_PRBS9 (pkt_phytest + 6, param->testDataLen);
    }
    else if (pkt_type == PKT_TYPE_HCI_0X0F)
    {
        pkt_phytest[4] = 1;
        memset (pkt_phytest + 6, 0x0f, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_0X55)
    {
        pkt_phytest[4] = 2;
        memset (pkt_phytest + 6, 0x55, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_PRBS15)
    {
        pkt_phytest[4] = 3;
        phyTest_PRBS15 (pkt_phytest + 6, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XFF)
    {
        pkt_phytest[4] = 4;
        memset (pkt_phytest + 6, 0xff, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_0X00)
    {
        pkt_phytest[4] = 5;
        memset (pkt_phytest + 6, 0x00, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XF0)
    {
        pkt_phytest[4] = 6;
        memset (pkt_phytest + 6, 0xf0, param->testDataLen);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XAA)
    {
        pkt_phytest[4] = 7;
        memset (pkt_phytest + 6, 0xaa, param->testDataLen);
    }
    transLen =  param->testDataLen + 2;
    transLen = rf_tx_packet_dma_len(transLen);
    pkt_phytest[3] = (transLen >> 24)&0xff;
    pkt_phytest[2] = (transLen >> 16)&0xff;
    pkt_phytest[1] = (transLen >> 8)&0xff;
    pkt_phytest[0] = transLen & 0xff;
    pkt_phytest[5] = param->testDataLen;
    pkt_length.len = param->testDataLen;
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
    rf_set_ble_channel ( phyTest_Channel(param->txChn) );
    rf_clr_tx_rptr(0);
    /* DMA reset & reset baseband, special situation, debug ongoing, SiHui and QiuWei will process later */
    #if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
        dma_chn_dis(DMA0);  /* reset RF TX DMA channels */
    #endif
    rf_set_tx_dma(0,288);
    blt_phyTest.interval = blt_getPktInterval(transLen,RF_MODE_BLE_1M);
    blt_phyTest.pkts = 0;
    blt_phyTest.tx_start = 1;
    blt_phyTest.cmd = PHY_CMD_TX;
    return BLE_SUCCESS;
}
ble_sts_t blt_phyTest_hci_setReceiverTest_V2(hci_le_receiverTestV2_cmdParam_t *param)
{
    u8 rx_chn = param->rxChn;
    u8 phy_mode = param->phys;
    u8 modulation_index = param->modulationIndex;
    rf_mode_init();
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }

    my_dump_str_data(PHY_DEBUG_FLAG, "phy_test rx v2", 0, 0);

    blt_phyTest.pkts = 0;
    rf_set_ble_channel ( phyTest_Channel(rx_chn) );
    if(phy_mode == 0x01)
    {
        blt_InitPhyTestDriver(RF_MODE_BLE_1M);
    }
    else if(phy_mode == 0x02)
    {
        blt_InitPhyTestDriver(RF_MODE_BLE_2M);
    }
    else if(phy_mode == 0x03)
    {
        blt_InitPhyTestDriver(RF_MODE_LR_S8_125K);
    }
    else if(phy_mode == 0x04)//s=2
    {
        blt_InitPhyTestDriver(RF_MODE_LR_S2_500K);
    }
    if(modulation_index == 0x00)
    {

    }
    else if(modulation_index == 0x01)
    {

    }
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }
    rf_set_tx_rx_off();
    rf_set_tx_rx_off_auto_mode();
    rf_set_ble_channel ( phyTest_Channel(rx_chn) );
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;
    rf_start_fsm(FSM_SRX, NULL, reg_system_tick+50);
    set_rf_rxCalibrationFunction(1,rx_chn);
    rf_set_rx_settle_time(85);
    rf_set_rx_maxlen(0xff);//max length is 255
    blt_phyTest.cmd = PHY_CMD_RX;
    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_hci_setTransmitterTest_V2(hci_le_transmitterTestV2_cmdParam_t *param)
{
    unsigned int transLen;
    u8 tx_chn = param->txChn;
    u8 length = param->testDataLen;
    u8 pkt_type = param->pktPayload;
    u8 phy_mode = param->phy;
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }

    my_dump_str_data(PHY_DEBUG_FLAG, "phy_test tx v2", 0, 0);
    rf_mode_init();
    if(phy_mode == 0x01)
    {
        blt_InitPhyTestDriver(RF_MODE_BLE_1M);
        blt_phyTest.interval = blt_getPktInterval(length,RF_MODE_BLE_1M);
    }
    else if(phy_mode == 0x02)
    {
        blt_InitPhyTestDriver(RF_MODE_BLE_2M);
        blt_phyTest.interval = blt_getPktInterval(length,RF_MODE_BLE_2M);
    }
    else if(phy_mode == 0x03)
    {
        blt_InitPhyTestDriver(RF_MODE_LR_S8_125K);
        blt_phyTest.interval = blt_getPktInterval(length,RF_MODE_LR_S8_125K);
    }
    else if(phy_mode == 0x04)
    {
        blt_InitPhyTestDriver(RF_MODE_LR_S2_500K);
        blt_phyTest.interval = blt_getPktInterval(length,RF_MODE_LR_S2_500K);
    }
    if (pkt_type == PKT_TYPE_HCI_PRBS9)
    {
        pkt_phytest[4] = 0;
        phyTest_PRBS9 (pkt_phytest + 6, length);
    }
    else if (pkt_type == PKT_TYPE_HCI_0X0F)
    {
        pkt_phytest[4] = 1;
        memset (pkt_phytest + 6, 0x0f, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_0X55)
    {
        pkt_phytest[4] = 2;
        memset (pkt_phytest + 6, 0x55, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_PRBS15)
    {
        pkt_phytest[4] = 3;
        phyTest_PRBS15 (pkt_phytest + 6, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XFF)
    {
        pkt_phytest[4] = 4;
        memset (pkt_phytest + 6, 0xff, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_0X00)
    {
        pkt_phytest[4] = 5;
        memset (pkt_phytest + 6, 0x00, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XF0)
    {
        pkt_phytest[4] = 6;
        memset (pkt_phytest + 6, 0xf0, length);
    }
    else if(pkt_type == PKT_TYPE_HCI_0XAA)
    {
        pkt_phytest[4] = 7;
        memset (pkt_phytest + 6, 0xaa, length);
    }
    transLen = length + 2;
    transLen = rf_tx_packet_dma_len(transLen);
    pkt_phytest[3] = (transLen >> 24)&0xff;
    pkt_phytest[2] = (transLen >> 16)&0xff;
    pkt_phytest[1] = (transLen >> 8)&0xff;
    pkt_phytest[0] = transLen & 0xff;
    pkt_phytest[5] = length;
    pkt_length.len = length;
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_TX_ON);  }
    rf_set_ble_channel ( phyTest_Channel(tx_chn) );
    rf_clr_tx_rptr(0);

    /* DMA reset & reset baseband, special situation, debug ongoing, SiHui and QiuWei will process later */
    #if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
        dma_chn_dis(DMA0);  /* reset RF TX DMA channels */
    #endif
    rf_set_tx_dma(0,288);
    blt_phyTest.pkts = 0;
    blt_phyTest.tx_start = 1;
    blt_phyTest.cmd = PHY_CMD_TX;
    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_hci_setTransmitterTest_V4(hci_le_transmitterTestV4_cmdParam_t *param)
{
  //CTE_Length,
  //CTE_Type,
  //Switching_Pattern_Length,
  //Antenna_IDs[i]

  //power
  s8 TX_Power_Level = param->antennaId[param->CTELen];
  if(TX_Power_Level > 20 || TX_Power_Level < -127)
  {
    return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
  }
  rf_power_level_index_e index = rf_ble_get_tx_pwr_idx(TX_Power_Level);
  rf_set_power_level_index(index);
  if(blc_rf_pa_cb){ blc_rf_pa_cb(PA_TYPE_TX_ON);  }
  return blt_phyTest_hci_setTransmitterTest_V2((hci_le_transmitterTestV2_cmdParam_t*)param);
}


ble_sts_t blt_phyTest_setTestEnd (u8 *pkt_num)
{
    if(!blmsParam.phytest_en){  //must set phy mode
        blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
    }

    my_dump_str_data(PHY_DEBUG_FLAG, "phy_test end", &blt_phyTest.pkts, 2);

    if(blt_phyTest.cmd == PHY_CMD_RX)
    {
        pkt_num[0] = U16_LO(blt_phyTest.pkts);
        pkt_num[1] = U16_HI(blt_phyTest.pkts);
    }
    else
    {
        //The Num_Packets for a transmitter test shall be reported as 0x0000.
        pkt_num[0] = 0;
        pkt_num[1] = 0;
    }


    rf_set_tx_rx_off ();
    rf_set_tx_rx_off_auto_mode(); //    STOP_RF_STATE_MACHINE;
    STOP_RF_STATE_MACHINE;

    /* DMA reset & reset baseband, special situation, debug ongoing, SiHui and QiuWei will process later */
#if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
    dma_chn_dis(DMA0);  /* reset RF TX DMA channels */
#endif
    delay_us(30);
    CLEAR_ALL_RFIRQ_STATUS;
#if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92) || (MCU_CORE_TYPE == MCU_CORE_TL751X))
    rf_ldot_ldo_rxtxlf_bypass_dis();
#endif
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }
    blt_phyTest.pkts = 0;  //clear
    blmsParam.phytest_en = 0;

    blt_phyTest.cmd = PHY_CMD_END;
    return BLE_SUCCESS;
}

ble_sts_t blt_phyTest_setReset(void)
{

    STOP_RF_STATE_MACHINE;

    /* DMA reset & reset baseband, special situation, debug ongoing, SiHui and QiuWei will process later */
    #if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92))
        dma_chn_dis(DMA0);  /* reset RF TX DMA channels */
    #endif

    rf_set_tx_rx_off ();
    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }
#if ((MCU_CORE_TYPE == MCU_CORE_B91) || (MCU_CORE_TYPE == MCU_CORE_B92) || (MCU_CORE_TYPE == MCU_CORE_TL751X))
    rf_ldot_ldo_rxtxlf_bypass_dis();
#endif
    CLEAR_ALL_RFIRQ_STATUS;
//  blmsParam.phytest_en = 0;
    blt_phyTest.pkts = 0;

    return BLE_SUCCESS;
}


ble_sts_t blc_phy_setPhyTestEnable (u8 en)
{
    u32 r = irq_disable();

    blt_InitPhyTestDriver(RF_MODE_BLE_1M);
    if(en && !blmsParam.phytest_en)
    {
        DBG_CHN0_HIGH;
        systimer_irq_disable();
        systimer_clr_irq_status();
        rf_clr_irq_mask(0XFFFF);
        CLEAR_ALL_RFIRQ_STATUS;

        blms_state = BLMS_STATE_NONE;

        rf_access_code_comm(0x29417671);
        rf_set_ble_crc_value(0x555555);
        ble_rf_set_rx_dma(phytest_rx_fifo, 255);
        rf_set_1st_rx_timeout(1000000); //EBQ delay send rx ,  65536
        rf_mode_init();
        rf_set_ble_1M_NO_PN_mode();
        rf_pn_disable();
        rf_set_preamble_len(5);
        #if TX_PKT_SHARE_SAVE_RAM
            //todo why not use blt_txfifo
            pkt_phytest = phytest_tx_fifo;
            *(u32 *)pkt_phytest = 39;
            pkt_phytest[4] = 0;
            pkt_phytest[5] = 37;
        #endif

        DBG_CHN0_LOW;
        my_dump_str_data(PHY_DEBUG_FLAG, "phytest_en", 0, 0);
    }

    else if(!en && blmsParam.phytest_en)
    {
        start_reboot();  //clear all status
        my_dump_str_data(PHY_DEBUG_FLAG, "phytest_dis", 0, 0);
    }



    blmsParam.phytest_en = en;
    my_dump_str_data(PHY_DEBUG_FLAG, "phytest_init", 0, 0);

    irq_restore(r);

    return BLE_SUCCESS;
}

bool      blc_phy_isPhyTestEnable(void)
{
    return blmsParam.phytest_en;
}


int blc_phytest_cmd_handler (u8 *p, int n)
{
    (void)n; //unused, remove warning

    //Commands and Events are sent most significant byte (MSB) first, followed
    //by the least significant byte (LSB).
    blt_phyTest.cmd = p[0] >> 6;
    u16 phy_event = 0;


    if (blt_phyTest.cmd == PHY_CMD_SETUP)       //reset
    {
        u8 ctrl = p[0]&0x3f;


        if(ctrl==0)
        {
            u8 para = (p[1] >> 2)&0x3f;
            if(para==0)
            {
                pkt_length.l.upper =0;
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_FAIL;
            }
            blc_phy_setPhyTestEnable(BLC_PHYTEST_ENABLE);
        }
        else if(ctrl== 1)
        {
            u8 para = (p[1] >> 2)&0x3f;
            if(para <= 3)
            {
                pkt_length.l.upper = para &0x03;
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_FAIL;
            }
        }
        else if(ctrl==2)
        {
            u8 para = (p[1] >> 2)&0x3f;
            if(para==1)//BLE 1M
            {
                blt_InitPhyTestDriver(RF_MODE_BLE_1M);
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else if(para==2)//BLE 2M
            {
                blt_InitPhyTestDriver(RF_MODE_BLE_2M);
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else if(para==3)//s=8
            {
                blt_InitPhyTestDriver(RF_MODE_LR_S8_125K);
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else if(para==4)//s=2
            {
                blt_InitPhyTestDriver(RF_MODE_LR_S2_500K);
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_FAIL;
            }
        }
        else if(ctrl==3)
        {
            u8 para = (p[1] >> 2)&0x3f;
            (void)para; //unused, remove warning
            //TODO standard modulation / stable modulation
            phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
        }
        else if(ctrl==4)
        {
            u8 para = (p[1] >> 2)&0x3f;
            (void)para; //unused, remove warning
            phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS | BIT(1) | BIT(2);
        }
        else if(ctrl==5)
        {
            u8 para = (p[1] >> 2)&0x3f;
            if(para==0)
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS | (251<<1);
            }
            else if(para==1)
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS | (17040 << 1);
            }
            else if(para==2)
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS | (251<<1);
            }
            else if(para==3)
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS | (17040 << 1);
            }
            else
            {
                phy_event = PHY_EVENT_STATUS | PHY_STATUS_FAIL;
            }
        }
        else if(ctrl==9)
        {
            s8 para = (s8)p[1];
            if(para<=20 && para>=-127)
            {
              rf_power_level_index_e index = rf_ble_get_tx_pwr_idx(para);
              rf_set_power_level_index(index);
              phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
            }
            else
            {
              phy_event = PHY_EVENT_STATUS | PHY_STATUS_FAIL;
            }
        }
        blt_phyTest_setReset();
    }
    else if (blt_phyTest.cmd == PHY_CMD_RX) //rx
    {
        u8 chn =  p[0] & 0x3f;
        pkt_length.l.low  = (p[1] >> 2) & 0x3f;

        blt_phyTest_hci_setReceiverTest_V1((hci_le_receiverTestV1_cmdParam_t *)&chn);
        phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
    }
    else if (blt_phyTest.cmd == PHY_CMD_TX) //tx
    {
        u8 chn =  p[0] & 0x3f;
        u8 pkt_type = p[1] & 0x03;
        pkt_length.l.low =  (p[1] >> 2) & 0x3f;

        blt_phyTest_2wireUart_setTransmitterTest_V1(chn, pkt_length.len, pkt_type, phy_test_mode);
        phy_event = PHY_EVENT_STATUS | PHY_STATUS_SUCCESS;
    }
    else  if(blt_phyTest.cmd == PHY_CMD_END)                //end
    {
        u16 pkt_num;
        phy_event = PHY_EVENT_PKT_REPORT | blt_phyTest.pkts;
        blt_phyTest_setTestEnd((u8 *)&pkt_num);
    }

    u8 returnPara[2] = {phy_event>>8, phy_event};
    blc_hci_send_event(HCI_FLAG_EVENT_PHYTEST_2_WIRE_UART, returnPara, 2);


    return 0;
}



_attribute_ram_code_ int blt_phyTest_main_loop(void)
{
    //phytest depend on blc_hci_rx_handler/blc_hci_tx_handler, so it must before phytest
    //------------------   HCI -------------------------------
    ///////// RX //////////////
    if (blc_hci_rx_handler)
    {
        blc_hci_rx_handler ();
    }
    ///////// TX //////////////
    if (blc_hci_tx_handler)
    {
        blc_hci_tx_handler ();
    }


    if (blt_phyTest.cmd == PHY_CMD_TX)
    {
        if (clock_time_exceed(blt_phyTest.tick_tx, blt_phyTest.interval) || blt_phyTest.tx_start)//pkt_interval
        {
            STOP_RF_STATE_MACHINE;                      // stop SM
            CLEAR_ALL_RFIRQ_STATUS;
            if(blt_phyTest.tx_start)
            {
                blt_phyTest.tx_start = 0;
            }
            rf_set_tx_rx_off_auto_mode();
            blt_phyTest.tick_tx =  reg_system_tick;
            rf_start_fsm(FSM_STX,pkt_phytest, reg_system_tick);
        }
    }


    else if (blt_phyTest.cmd == PHY_CMD_RX)
    {
        if (rf_get_irq_status(FLD_RF_IRQ_RX))
        {
            my_dump_str_data(PHY_DEBUG_FLAG, "phy_rx", &blt_phyTest.pkts, 2);
            CLEAR_ALL_RFIRQ_STATUS;
            u8 * raw_pkt = (u8 *)phytest_rx_fifo;
            if ( RF_BLE_RF_PAYLOAD_LENGTH_OK(raw_pkt) && RF_BLE_RF_PACKET_CRC_OK(raw_pkt) && (REG_ADDR8(REG_BASEBAND_BASE_ADDR + 0x40)&0xf0)==0)
            {
                blt_phyTest.pkts++;
            }

            rf_set_tx_rx_off();
            rf_set_tx_rx_off_auto_mode();
            rf_set_1st_rx_timeout(0x0fffffff);

            rf_start_fsm(FSM_SRX, NULL,reg_system_tick+50);
            rf_set_rx_settle_time(85);
        }
    }
    else if (blt_phyTest.cmd == PHY_CMD_END){
    }

    return 0;
}

void blt_InitPhyTestDriver(rf_mode_e rf_mode)
{

    if(rf_mode == RF_MODE_BLE_1M)
    {
        if(phy_test_mode != RF_MODE_BLE_1M)
        {
            my_dump_str_data(PHY_DEBUG_FLAG, "phy_test 1M mode", 0, 0);
            rf_ble_switch_phy(BLE_PHY_1M,0); //rf_ble_set_1m_phy();
            phy_test_mode = RF_MODE_BLE_1M;
        }
    }
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    else if(rf_mode == RF_MODE_BLE_2M)
    {
        if(phy_test_mode != RF_MODE_BLE_2M)
        {
            my_dump_str_data(PHY_DEBUG_FLAG, "phy_test 2M mode", 0, 0);
            rf_ble_switch_phy(BLE_PHY_2M,0);
            phy_test_mode = RF_MODE_BLE_2M;
        }
    }
    else if(rf_mode == RF_MODE_LR_S2_500K)
    {
        if(phy_test_mode != RF_MODE_LR_S2_500K)
        {
            my_dump_str_data(PHY_DEBUG_FLAG, "phy_test coded_s2 mode", 0, 0);
            rf_ble_switch_phy(BLE_PHY_CODED,LE_CODED_S2);
            phy_test_mode = RF_MODE_LR_S2_500K;
        }
    }
    else if(rf_mode == RF_MODE_LR_S8_125K)
    {
        if(phy_test_mode != RF_MODE_LR_S8_125K)
        {
            my_dump_str_data(PHY_DEBUG_FLAG, "phy_test coded_s8 mode", 0, 0);
            rf_ble_switch_phy(BLE_PHY_CODED,LE_CODED_S8);
            phy_test_mode = RF_MODE_LR_S8_125K;
        }
    }
#endif

    if(blmsParam.phytest_en)
    {
        rf_pn_disable();
    }
}

#endif

