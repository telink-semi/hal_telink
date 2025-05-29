/********************************************************************************************************
 * @file    ext_hadm_rf.c
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
#include "ext_lib.h"
#include "ext_rf.h"
#include "stack/ble/controller/ble_controller.h"

#if FAST_SETTLE
extern _attribute_data_retention_sec_ rf_fast_settle_t *g_fast_settle_cal_val_ptr;
#endif

#if (HADM_PHASE_CONTINUITY)
_attribute_data_retention_ rf_cs_tx_cali_t tx_cs_cali;
_attribute_data_retention_ rf_cs_rx_cali_t rx_cs_cali;
_attribute_data_retention_ unsigned char cs_phase_continuity_flag = 0;


typedef struct __attribute__((packed))
{
    unsigned int dma_len;
    unsigned char header;
    unsigned char payload_len;
}ble_packet_header_t;


/**
 * @brief       This function is mainly used to get tx and rx calibration value by running a self-made stx and srx.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_settle_cali_init(void)
{
//    rf_mode_init();
//    rf_set_ble_1M_NO_PN_mode();

    reg_rf_ll_cmd = 0x80;//STOP_RF_STATE_MACHINE;
    rf_set_tx_rx_off ();
    rf_clr_irq_status(0xffff);//CLEAR_ALL_RFIRQ_STATUS;

//    rf_tx_settle_us(110);//adjust TX settle time
//    rf_set_rx_settle_time(85);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_set_tx_dma(0,128);
    unsigned char  ble_customer_tx_packet[16] __attribute__ ((aligned (4))) =
            {3,0,0,0,0xaa,0xaa,0x8e,0x89,0xbe,0xd6,0xaa,0x55,0x55,0xaa,0x00,0x00};
    ble_packet_header_t *ptr = (ble_packet_header_t*)ble_customer_tx_packet;
    ptr->dma_len = rf_tx_packet_dma_len(5);
//  for(unsigned char i=0;i<40;i++)
    {
        rf_set_chn(40);
        rf_start_fsm(FSM_STX,(void*)&ble_customer_tx_packet,stimer_get_tick());
        while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
        rf_clr_irq_status(FLD_RF_IRQ_TX);

        reg_rf_ll_cmd = 0x80;
        rf_clr_irq_status(0xffff);
//      fast_settle.cal_tbl[i] = rf_get_hpmc_cal_val();
    }

    //48M : 4.46us
    rf_cs_get_tx_cali_vlue(&tx_cs_cali);


    rf_start_fsm(FSM_SRX,NULL,stimer_get_tick());
    delay_us(85);//Wait for the rx packetization action to complete
//  rf_get_dcoc_cal_val(&fast_settle.dcoc_cal);

    reg_rf_ll_cmd = 0x80;
//
//    rf_set_tx_rx_off ();
//    rf_clr_irq_status(0xffff);
//    reg_rf_ll_cmd = 0x80;
//  rf_get_ldo_trim_val(&fast_settle.ldo_trim);

    //48M : 7.17us
    rf_cs_get_rx_cali_vlue(&rx_cs_cali);

//  reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;

}

/**
 *  @brief        This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_ // ble use
void rf_cs_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    write_reg8(0x1706e2 ,(ldo_trim.LDO_CAL_TRIM << 1) | 0x01);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,(ldo_trim.LDO_RXTXHF_TRIM << 2) | 0x03);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS
    write_reg8(0x1706e5 , ldo_trim.LDO_RXTXLF_TRIM);
    write_reg8(0x1706e6 ,(ldo_trim.LDO_PLL_TRIM << 2) | 0x03);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS
    write_reg8(0x1706e7 , ldo_trim.LDO_VCO_TRIM);
}


/**
/**
 *  @brief        This function is mainly used to set rccal Calibration-related values.
 *  @param[in]    rccal_cal    - rccal Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_ //ble use
void rf_cs_set_rccal_cal_val(rf_rccal_cal_t rccal_cal)
{

    write_reg8(0x1706c6,(rccal_cal.RCCAL_CODE << 1) & 0xfe);//open CBPF
    write_reg8(0x1706c6,(rccal_cal.CBPF_CCODE_L & 0x01 ) << 7 | (read_reg8(0x1706c6)&(~BIT(6))));//open CBPF
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_L & 0x06) >> 1 | read_reg8(0x1706c7));
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_H << 2 | (read_reg8(0x1706c7)|BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ //ble use
void rf_cs_get_ldo_trim_val(rf_ldo_trim_t *ldo_trim)
{
    ldo_trim->LDO_CAL_TRIM = read_reg8(0x1706ea) & 0x3f;
    ldo_trim->LDO_RXTXHF_TRIM = read_reg8(0x1706ec) & 0x3f;
    ldo_trim->LDO_RXTXLF_TRIM = ((read_reg8(0x1706ed) & 0x0f) << 2) + ((read_reg8(0x1706ec) & 0xc0) >> 6);
    ldo_trim->LDO_PLL_TRIM = read_reg8(0x1706ee) & 0x3f;
    ldo_trim->LDO_VCO_TRIM = ((read_reg8(0x1706ef) & 0x0f) << 2) + ((read_reg8(0x1706ee) & 0xc0) >> 6);
}

#if !SW_DCOC_EN
/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ //ble use
void rf_cs_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC = read_reg8(0x1706d8) & 0x3f;//DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC = read_reg8(0x1706da) & 0x3f;//DCOC_QDAC 0xda[5:0]
    dcoc_cal->DCOC_IADC_OFFSET = read_reg8(0x1706dc) & 0x7f;//DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal->DCOC_QADC_OFFSET = (read_reg8(0x1706dc) & 0x80) >> 7 |(read_reg8(0x1706dd) & 0x3f) << 1;//DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
}

/**
 *  @brief      This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_ //ble use
void rf_cs_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    write_reg8(0x1706d0,(dcoc_cal.DCOC_IDAC << 1) | 0x01);//DCOC_BYPASS_DAC
    write_reg8(0x1706d0,read_reg8(0x1706d0)|((dcoc_cal.DCOC_QDAC&0x01) << 7));
    write_reg8(0x1706d1,((dcoc_cal.DCOC_QDAC)&0x3e) >> 1);
    write_reg8(0x1706ce,(dcoc_cal.DCOC_IADC_OFFSET << 1) | 0x01);//DCOC_BYPASS_ADC
    write_reg8(0x1706cf,dcoc_cal.DCOC_QADC_OFFSET);
}

#endif

/**
 * @brief       This function is mainly used to set the sequence related to Fast Settle in cs.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_phase_continuity_en(void)//CCLK_96M, consume 49us
{
    #if (FAST_SETTLE)
        //rf_rx_fast_settle_dis
        write_reg8(0x1706e2, read_reg8(0x1706e2)&0xfe); //ldo cal bypass disable
        write_reg8(0x1706e4, read_reg8(0x1706e4)&0xfc); //ldo RXTXHF bypass disable
        write_reg8(0x1706e6, read_reg8(0x1706e6)&0xfc); //ldo RXTXLF bypass disable
        write_reg8(0x170629, read_reg8(0x170629)&0xf7);

        //rf_tx_fast_settle_dis
        write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe); //hpmc bypass disable
        write_reg8(0x1706e2,read_reg8(0x1706e2)&0xfe); //ldo cal bypass disable
        write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc); //ldo RXTXHF bypass disable
        write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc); //ldo RXTXLF bypass disable
        write_reg8(0x170629,read_reg8(0x170629)&0xef);
    #endif

    rf_cs_set_rx_cali_vlue(rx_cs_cali);
    rf_cs_set_tx_cali_vlue(tx_cs_cali);


    //seq_ldo_pll_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(3));    //LDO_PLL_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(3));    //LDO_PLL_PUP_OW

    //seq_ldo_vco_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(4));    //LDO_VCO_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(4));    //LDO_VCO_PUP_OW

    //seq_ldo_pll_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(3))); //LDO_PLL_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(3));    //LDO_PLL_FC_O

    //rf_seq_ldo_vco_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(4))); //LDO_VCO_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(4));    //LDO_VCO_FC_OW

    //seq_pd_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(0));    //PD_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(0));    //PD_PUP_OW

    //seq_pd_en_fcal_bias_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2))); //PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));    //PD_EN_FCAL_BIAS_OW

    //seq_xo_en_clk_ref_ow
    write_reg8(0x170770,read_reg8(0x170770)|BIT(3));    //XO_EN_CLK_REF
    write_reg8(0x170770,read_reg8(0x170770)|BIT(1));    //XO_EN_CLK_REF_OW

    //seq_vco_pup_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(0));    //VCO_PUP
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(0));    //VCO_PUP_OW

    //seq_lo_pup_vlo_fbk_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(6));    //LO_PUP_VLO_FBK
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(6));    //LO_PUP_VLO_FBK_OW

    //seq_fcal_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3))); //FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));    //FCAL_PUP_OW

    //_seq_fcal_set_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4))); //FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));    //FCAL_SET_OW

    //seq_fcal_run_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5))); //FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));    //FCAL_RUN_OW

    //seq_divn_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(6));    //DIVN_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(6));    //DIVN_PUP_OW

    //seq_divn_openloop_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(7))); //DIVN_OPENLOOP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(7));    //DIVN_OPENLOOP_OW

    //ldo_rxtxhf_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(1));    //LDO_RXTXHF_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(1));    //LDO_RXTXHF_PUP_OW

    //ldo_lv_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(0));    //LDO_LV_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(0));    //LDO_LV_PUP_OW

    //bg_pup_ow
    write_reg8(0x170766,read_reg8(0x170766)|BIT(0));    //BG_PUP
    write_reg8(0x170764,read_reg8(0x170764)|BIT(0));    //BG_PUP_OW

    //rf_mixer_pup_ow
    write_reg8(0x17077b,read_reg8(0x17077b)|BIT(3));    //RX_MIX_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(4));    //RX_MIX_PUP_OW

    //dsm_run
    write_reg8(0x170682,read_reg8(0x170682)|BIT(0));    //DSM_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(0));    //DSM_RUN_OW

    write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5))); //GFSK_AUTO
    write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));  //FREQ_COMP_EN
    write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));  //GFSK_EN

    write_reg8(0x170451, read_reg8(0x170451) | (BIT(1)));  //FREQ_COMP_AUTO //Jaguar need set,Tercel needn't

    //rf_hpm_cal_disable
    write_reg8(0x170688,read_reg8(0x170688)&(~BIT(3))); //TX_HPM_CAL_EN
    write_reg8(0x170686,read_reg8(0x170686)|BIT(3));    //TX_HPM_CAL_EN_OW

    //rf_seq_lo_pup_vlo_txfsk_ow
    write_reg8(0x170792,read_reg8(0x170792)|BIT(6));    //LO_PUP_VLO_TXFSK
    write_reg8(0x170790,read_reg8(0x170790)|BIT(6));    //LO_PUP_VLO_TXFSK_OW
    write_reg8(0x170792,read_reg8(0x170792)|BIT(7));    //LO_PUP_VLO_TXFSKDRV
    write_reg8(0x170790,read_reg8(0x170790)|BIT(7));    //LO_PUP_VLO_TXFSKDRV_OW

    //seq_lo_pup_vlo_rx_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(2));    //LO_PUP_VLO_RX
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(2));    //LO_PUP_VLO_RX_OW
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(3));    //LO_PUP_VLO_RXDRV
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(3));    //LO_PUP_VLO_RXDRV_OW


    rf_cs_settle_sequence_mode(RF_CS_SETTLE_SEQ_ON);

    cs_phase_continuity_flag = 1;
}

_attribute_ram_code_ void ble_rf_cs_phase_continuity_dis(unsigned char phase_en)
{
    rf_cs_settle_sequence_mode(RF_CS_SETTLE_SEQ_OFF);

    rf_cs_restore_cali_auto_run(phase_en);

#if (FAST_SETTLE)
    #if !SW_DCOC_EN
        rf_cs_set_dcoc_cal_val(rx_cs_cali.dcoc_cal);
    #endif
    rf_cs_set_ldo_trim_val(g_fast_settle_cal_val_ptr->ldo_trim);

    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_rx_fast_settle_en();
    rf_tx_fast_settle_en();

#endif

    cs_phase_continuity_flag = 0;
}


/**
 * @brief       This function is mainly used to enable the rx-related trim functions that are bypassed during channel sounding.
 * @param[in]   phase_en : Used to control whether the digital_MIX/GFSK/VCO is continuous or not. 1:Maintaining continuity;0:No longer continuous.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_restore_cali_auto_run(unsigned char phase_en)
{

    write_reg8(0x170681,read_reg8(0x170681)&0xf3);      //FCAL_DEBUG_RUN_OW //LDOT_DEBUG_RUN_OW
#if !SW_DCOC_EN
    write_reg8(0x170680,read_reg8(0x170680)&0xea);      //RCCAL_RUN_OW//RXDCOC_RUN_OW//DSM_RUN_OW
#else
    write_reg8(0x170680,read_reg8(0x170680)&0xfa);      //RCCAL_RUN_OW//RXDCOC_RUN_OW//DSM_RUN_OW
#endif

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);     //LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);     //LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);     //LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x1706ce, read_reg8(0x1706ce) & 0xfe);        //DCOC_BYPASS_ADC //no influence for sw dcoc enable.

    write_reg8(0x1706d0, read_reg8(0x1706d0) & 0xfe);        //DCOC_BYPASS_DAC //no influence for sw dcoc enable.

    write_reg8(0x1706c6,(read_reg8(0x1706c6)&0xbe));    //RCCAL_DBG1_0-->BYPASS//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0))); //RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&0xf5);      //LDO_RXTXHF_PUP_OW//LDO_RXTXHF_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&0xe7);      //LDO_PLL_FC_OW//LDO_VCO_FC_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW


    if(phase_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5))); //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));  //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));  //GFSK_EN

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)|BIT(0));    //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)|(BIT(5)));  //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)|(BIT(4)));  //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(0)));  //VCO_PUP_OW

        write_reg8(0x170788,0xc0);                          //PD_DIVN_FCAL_OW_CTRL 0x00->0xc0
                                                            //<0>:PD_PUP_OW
                                                            //<1>:PD_EN_PD_DRV_OW
                                                            //<2>:PD_EN_FCAL_BIAS_OW
                                                            //<3>:FCAL_PUP_OW
                                                            //<4>:FCAL_SET_OW
                                                            //<5>:FCAL_RUN_OW
                                                            //<6>:DIVN_PUP_OW      default 0 -> 1 open divn_pup overwrite
                                                            //<7>:DIVN_OPENLOOP_OW default 0 -> 1 open divn_openloop overwrite

        write_reg8(0x170760,read_reg8(0x170760)|(BIT(0)));  //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)|(BIT(0)));  //BG_PUP_OW

        write_reg8(0x170790,read_reg8(0x170790)|(BIT(6)));  //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(2)));  //LO_PUP_VLO_RX_OW

    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)|(BIT(5)));  //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)&(~BIT(1))); //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)&(~BIT(5))); //GFSK_EN

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

        write_reg8(0x170788,0x00);//PD_PUP_OW//PD_EN_PD_DRV_OW//PD_EN_FCAL_BIAS_OW//FCAL_PUP_OW//FCAL_SET_OW//FCAL_RUN_OW//DIVN_PUP_OW//DIVN_OPENLOOP_OW

        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

        write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW
    }

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

}

/**
 * @brief       This function is mainly used to set the preparation and enable of manual fcal(frequency calibration).
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_manual_fcal_start(void)
{
//  rf_seq_pd_en_pd_drv_ow(0);
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(1));//PD_EN_PD_DRV
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1)));//PD_EN_PD_DRV_OW

    write_reg8(0x170738,read_reg8(0x170738)|BIT(2));//BYPASS_CAL_CLK_GAT

//  rf_seq_pd_en_fcal_bias_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(2));//PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));//PD_EN_FCAL_BIAS_OW

//  rf_seq_fcal_pup_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(3));//FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));//FCAL_PUP_OW

//  rf_seq_fcal_set_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));//FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4)));//FCAL_SET_OW

//  rf_seq_fcal_run_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));//FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5)));//FCAL_RUN_OW

    write_reg8(0x170683,read_reg8(0x170683)|BIT(3));//FCAL_DEBUG_RUN
}


/**
 * @brief       This function is mainly used to set the relevant value after manual fcal(frequency calibration).
 * @return      none.
 * @note        The function needs to be called after the rf_manual_fcal_start call 22us.
 */
_attribute_ram_code_ //ble use
void rf_manual_fcal_done(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//FCAL_DEBUG_RUN
    write_reg8(0x170738,read_reg8(0x170738)&(~BIT(2)));//BYPASS_CAL_CLK_GAT

//  rf_seq_pd_en_fcal_bias_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));//PD_EN_FCAL_BIAS
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2)));

//  rf_seq_fcal_pup_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));//FCAL_PUP
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3)));

//  rf_seq_fcal_set_ow();
//  write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));//FCAL_SET_OW

//  rf_seq_fcal_run_ow();
//  write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));//FCAL_RUN_OW

//  rf_seq_pd_en_pd_drv_ow(1);
    write_reg8(0x170788,read_reg8(0x170788)|BIT(1));//PD_EN_PD_DRV_OW
}

/**
 * @brief       This function is mainly used to get the calibration value of the rx state that needs to be
 *              recorded in the cs function.
 * @param[out]  rx_cali -   Pointer to a structure that stores the value associated with the rx calibration.
 * @return      none.
 * @note        This function is usually called after a package has been received.
 */
_attribute_ram_code_ //ble use
void rf_cs_get_rx_cali_vlue(rf_cs_rx_cali_t *rx_cali)
{
    rf_cs_get_ldo_trim_val(&rx_cali->ldo_trim);
#if !SW_DCOC_EN
    rf_cs_get_dcoc_cal_val(&rx_cali->dcoc_cal);
#endif
    rf_cs_get_rccal_cal_val(&rx_cali->rccal_cal);
}

/**
 * @brief       This function is mainly used to get the calibration value of the tx state that needs to be
 *              recorded in the cs function.
 * @param[out]  rx_cali -   Pointer to a structure that stores the value associated with the tx calibration.
 * @return      none.
 * @note        This function is usually called after a package has been sent.
 */
_attribute_ram_code_ //ble use
void rf_cs_get_tx_cali_vlue(rf_cs_tx_cali_t *tx_cali)
{
    rf_cs_get_ldo_trim_val(&tx_cali->ldo_trim);
    extern unsigned short rf_get_hpmc_cal_val(void);
    tx_cali->tx_hpmc = rf_get_hpmc_cal_val();
}





/**
 *  @brief      This function is mainly used to get rccal Calibration-related values.
 *  @param[in]  rccal_cal   - rccal calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ //ble use
void rf_cs_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal)
{
    rccal_cal->RCCAL_CODE = read_reg8(0x1706ca)&0x1f;
    rccal_cal->CBPF_CCODE_L = read_reg8(0x1706ca)&0xe0 >> 5;
    rccal_cal->CBPF_CCODE_H = read_reg8(0x1706cb)&0x0f;
}

/**
 * @brief       This function is mainly used to enable LNA.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_lna_pup(void)
{
    write_reg8(0x17077a,read_reg8(0x17077a)|BIT(0));//RX_LNA_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(0));//RX_LNA_PUP_OW
}


/**
 * @brief       This function is mainly used for the disable hpmc trim function.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_dis_hpmc_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(5)));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(5));
}

/**
 *  @brief        This function is mainly used to set hpmc Calibration-related values.
 *  @param[in]  hpmc_gain  - hpmc Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_sec_noinline_ void rf_cs_set_hpmc_cal_val(unsigned short hpmc_gain)
{
    //The calibration value of hpmc is different at different frequency points,
    //So you need to reset it every time you switch channels.

    unsigned short tmp = read_reg16(0x1706f6);
    tmp = (tmp & 0xf001) | hpmc_gain | 0x0001;    //bit<1:11> 1111 0000 0000 0001    //HPMC_BYPASS
    write_reg16(0x1706f6,tmp);
}

/**
 * @brief       This function is mainly used for the disable ldo trim function.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_dis_ldo_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(2)));
    write_reg8(0x170681,read_reg8(0x170681)|BIT(2));
}

/**
 * @brief       This function is mainly used for the disable dcoc trim function.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_dis_dcoc_trim(void)
{
write_reg8(0x170682,read_reg8(0x170682)&(~BIT(4)));
write_reg8(0x170680,read_reg8(0x170680)|BIT(4));
}

/**
 * @brief       This function is mainly used for the disable rccal trim function.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_dis_rccal_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(2)));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(2));
}

/**
 * @brief       This function is mainly used for the disable fcal trim function.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_dis_fcal_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));
    write_reg8(0x170681,read_reg8(0x170681)|BIT(3));
}

/**
 * @brief       This function is mainly used to write the calibration value obtained through the rf_cs_get_rx_cali_vlue
 *              function to the corresponding register.
 * @param[in]   rx_cali     -   rx calibration value obtained by the rf_cs_get_rx_cali_vlue function.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_rx_cali_vlue(rf_cs_rx_cali_t rx_cali)
{
    rf_cs_dis_fcal_trim();
    rf_cs_set_ldo_trim_val(rx_cali.ldo_trim);
    rf_cs_dis_ldo_trim();
#if !SW_DCOC_EN
    rf_cs_set_dcoc_cal_val(rx_cali.dcoc_cal);
#endif
    rf_cs_dis_rccal_trim();
    rf_cs_set_rccal_cal_val(rx_cali.rccal_cal);
    rf_cs_dis_dcoc_trim();
    rf_lna_pup();
}

/**
 * @brief       This function is used to write the tx calibration value obtained by rf_cs_get_tx_cali_vlue to the
 *              corresponding register.
 * @param[in]   tx_cali     -   tx calibration value obtained by the rf_cs_get_tx_cali_vlue function.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_tx_cali_vlue(rf_cs_tx_cali_t tx_cali)
{
    rf_cs_dis_fcal_trim();
    rf_cs_set_ldo_trim_val(tx_cali.ldo_trim);
    rf_cs_set_hpmc_cal_val(tx_cali.tx_hpmc);
    rf_cs_dis_hpmc_trim();
}


/**
 * @brief       This function is used to enable or disable the corresponding sequence of shuttle in channel sounding mode; usually call
 *              this function before entering mode1/mode2 and pass the parameter RF_HADM_SETTLE_SEQ_ON to enable the corresponding sequence;
 *              and call the parameter RF_HADM_SETTLE_SEQ_OFF to disable the sequence after ending channel sounding.
 * @param[in]   on_off : Used to control whether to enable settle sequence in channel sounding RF_HADM_SETTLE_SEQ_OFF:off,RF_HADM_SETTLE_SEQ_ON:on
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_settle_sequence_mode(rf_cs_settle_seq_mode_e on_off)
{
    if (on_off == RF_CS_SETTLE_SEQ_ON)
    {
        //close hpmc and ldo trim,close hpmc(53us), ldotrim(4.5us),save 58us
        //Default settle time:108.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x17068a,0x00);  //sub-sequence1 start time:0
        write_reg8(0x17068b,0x08);  //sub-sequence2 start time:8us
        write_reg8(0x17068c,0x30);  //sub-sequence3 start time:48us
        write_reg8(0x17068d,0x31);  //sub-sequence4 start time:48.5us
        write_reg8(0x17068e,0x33);  //sub-sequence5 start time:51us
        write_reg8(0x17068f,0x30);  //sub-sequence6 start time:48us

        //RX: rx_ldo_trim (4.5us), rx_dcoc(40us)
        //RX Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x170690,0x00);  //sub-sequence1 start time:0us
        write_reg8(0x170691,0x09);  //sub-sequence2 start time:9us
        write_reg8(0x170692,0x09);  //sub-sequence3 start time:9us
        write_reg8(0x170693,0x1b);  //sub-sequence4 start time:27us
        write_reg8(0x170694,0x2d);  //sub-sequence5 start time:45us
        write_reg8(0x170695,0x2d);  //sub-sequence6 start time:45us
    }
    else if(on_off == RF_CS_SETTLE_SEQ_OFF)
    {
        //Default settle time:108.5us
        write_reg8(0x17068a,0x00);  //sub-sequence1 start time:0
        write_reg8(0x17068b,0x0d);  //sub-sequence2 start time:13us
        write_reg8(0x17068c,0x6a);  //sub-sequence3 start time:106us
        write_reg8(0x17068d,0x6b);  //sub-sequence4 start time:107us
        write_reg8(0x17068e,0x6e);  //sub-sequence5 start time:110us
        write_reg8(0x17068f,0x6a);  //sub-sequence6 start time:106us

        //RX Default settle time:85us
        write_reg8(0x170690,0x00);  //sub-sequence1 start time:0us
        write_reg8(0x170691,0x0d);  //sub-sequence2 start time:13us
        write_reg8(0x170692,0x0d);  //sub-sequence3 start time:13us
        write_reg8(0x170693,0x27);  //sub-sequence4 start time:43us
        write_reg8(0x170694,0x52);  //sub-sequence5 start time:82us
        write_reg8(0x170695,0x52);  //sub-sequence6 start time:82us
    }
}
#endif


_attribute_ram_code_ void ble_rf_channel_sounding_init(void)
{
    reg_rf_mode_ctrl0 |= FLD_RF_INFO_EXTENSION;
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_QWORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_QWORLD_WIDTH);

    write_reg8(0x170030, 0x3e); //enable tx timestamp

#if(CS_EBQ_TEST)
    write_reg8(0x170624, 0x4b); //PA_RAMP_MODE 1000ns fix EBQ mode2 rf timing issue -- yuexin sync with haili and xuqiang
#else
    write_reg8(0x170624, 0x49);
#endif
    //ble_rf_tx_channel_sounding_mode_en();
    BM_CLR(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);
    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 0));

    //ble_rf_rx_channel_sounding_mode_en(1, IQ_20_BIT_MODE); //sample rate 4MHz, IQ 20 bit
    //sample_interval_time: (1 + 1)*0.125us ---> 0.25us ---> 4MHz
    unsigned char interval = 1;
    rf_iq_data_mode_e suppmode = IQ_20_BIT_MODE;
    reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_IQ_SAMP_INTERVAL)) | (interval << 4));//The max sample rate is 4Mhz.
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE)) | ((suppmode&0x07) << 4));
    reg_rf_mode_ctrl0 |= FLD_RF_IQ_SAMP_EN;
}

_attribute_ram_code_ void ble_rf_channel_sounding_deinit(void)
{
    //ble_rf_tx_channel_sounding_mode_dis();
    BM_SET(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);
    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 2));

    reg_rf_mode_ctrl0 &= (~FLD_RF_IQ_SAMP_EN);//ble_rf_rx_channel_sounding_mode_dis();

    reg_rf_mode_ctrl0 &= (~FLD_RF_INFO_EXTENSION);
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_WORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_WORLD_WIDTH);

    reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;

    write_reg8(0x170030, 0x36); //disable tx timestamp
    write_reg8(0x170624, 0x49); //change PA_RAMP_MODE 250ns

    rf_agc_enable();
}

/**
 * @brief       This function is mainly used to initialize some parameter settings of channel sounding IQ sample.
 * @param[in]   sample_num  - Number of groups to sample IQ data.Value range 0x01~0xffff.
 * @param[in]   start_point - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                            at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                            starts at (start_point + 1) * 0.125us after sync.
 *                            Value range 0x00~0xff.
 * @param[in]   sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_cs_iq_sample_mode_e sample_mode)
{
    reg_rf_iq_samp_num = sample_num;
    reg_rf_iq_samp_start = start_point;
    if(sample_mode == RF_CS_IQ_SAMPLE_SYNC_MODE)
    {
        reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;
    }
    else
    {
        reg_rf_rxlatf &= (~FLD_RF_R_IQ_SAMP_MODE);
    }
}

_attribute_ram_code_ void ble_rf_set_manual_tx_mode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}


_attribute_ram_code_ void ble_rf_set_tx_modulation_index(rf_mi_value_e mi_value)//only support RF_MI_P0p00 and RF_MI_P0p50
{
    unsigned char modulation_index_low;
    unsigned char kvm_trim = 0;

    if(mi_value == RF_MI_P0p00){
        modulation_index_low = 0;
    }
    else if(mi_value == RF_MI_P0p50){
        modulation_index_low = 64;
    }
    else{
        return;
    }

    if(reg_rf_mode_cfg_tx1_0 & 0x01)
    {
        kvm_trim = 1;
    }

    reg_rf_radio_mode_cfg_rx2_0 = modulation_index_low;
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
}

/**
 * @brief       This function serves to set rf channel for CS.The actual channel set by this function is 2402 + chn.
 * @param[in]   chn   - That you want to set the channel as 2402 + chn.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_set_cs_channel(signed char chn)
{
    rf_set_chn(chn + 2);
}

/**
 * @brief       This function is mainly used to set the energy when sending a single carrier.
 * @param[in]   level       - The slice corresponding to the energy value.
 * @return      none.
 */
_attribute_ram_code_ void rf_cs_set_power_level_singletone(rf_power_level_e level)
{
    unsigned char value = 0;

    if(level&BIT(7))
    {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;// VANT
    }
    else
    {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
    }
    value = (unsigned char)level&0x3f;
    reg_rf_lnm_pa_ow_ctrl_val |= BIT(6);                            // TX_PA_PWR_OW  BIT6 set 1
    reg_rf_pa_ow_val = ((reg_rf_pa_ow_val&0x81)|(value<<1));        // TX_PA_PWR  BIT1 t0 BIT6 set value
}

/**
 * @brief       This function is mainly used to turn off the energy of the tone.
 * @return      none.
 * @note        After setting the tone energy with rf_set_power_level_singletone, you need to call
 *              rf_set_power_off_singletone to turn off the tone energy if you enter the send packet.
 */
_attribute_ram_code_ void rf_cs_set_power_off_singletone(void)
{
    write_reg8(0x17077c,(read_reg8(0x17077c)&0x81));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(6)));
}
/**
 * @brief       This function is mainly used for freeze AGC(Automatic Gain Control).
 * @return      none.
 * @note        This function needs to be called after ble_rf_agc_enable, otherwise there will be problems with rssi
 *              value exceptions.
 */
_attribute_ram_code_ //ble use
void rf_agc_disable()//TODO optimize execution time
{
    char gain_lat, lna_hgain, lna_lgain, lna_attn, cbpf_gain;
    reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;
#if 0
    gain_lat = (read_reg8(0x170059)>>4)&0x07;
#else
    gain_lat = (read_reg8(0x17045c))&0x07; // ble use
#endif
    write_reg8(0x170640,(read_reg8(0x170640)&0xe3)|((gain_lat&0x07)<<2));

    if(gain_lat == 0)
    {
        lna_hgain = 0;
        lna_lgain = 1;
        lna_attn  = 3;
        cbpf_gain = 0;
    }
    else if(gain_lat == 1)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 2;
        cbpf_gain = 1;
    }
    else if(gain_lat == 2)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 3)
    {
        lna_hgain = 3;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 4)
    {
        lna_hgain = 0xf;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 5)
    {
        lna_hgain = 0x3f;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 6)
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 0;
    }

    write_reg8(0x17077a,(read_reg8(0x17077a)&0x81)|(lna_hgain<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x02);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xfe)|(lna_lgain>>1));
    write_reg8(0x17077a,(read_reg8(0x17077a)&0x7f)|(lna_lgain<<7));
    write_reg8(0x170778,read_reg8(0x170778)|0x04);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xf9)|((lna_attn&0x03)<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x08);

    write_reg8(0x170782,(read_reg8(0x170782)&0xfd)|(cbpf_gain&0x01)<<1);
    write_reg8(0x170780,read_reg8(0x170780)|0x02);
}

/**
 * @brief       This function is mainly used for agc auto run.
 * @return      none.
 * @note        Call this function to enable agc auto tuning if you want to receive different energy packets correctly
 *              after calling ble_rf_agc_disable to disable agc auto tuning.
 */
_attribute_ram_code_ //ble use
void rf_agc_enable(void)//TODO optimize execution time
{
    reg_rf_radio_txrx_dbg1_0 &= (~FLD_RF_AGC_DISABLE);
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(1)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(2)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(3)));
    write_reg8(0x170780,read_reg8(0x170780)&(~BIT(1)));
}

/**
 * @brief     This function performs to set RF Access Code Threshold.
 * @param[in] threshold   cans be 0-32bits
 * @return    none.
 */
_attribute_ram_code_ void ble_rf_set_accessCodeThreshold(u8 threshold)
{
    write_reg8(0x17044e, threshold);
}

/**
 * @brief       This function is mainly used to set the antenna switching mode. Vulture support three different
 *              table lookup sequences.The setting here is just the order of the table lookup, and the content
 *              in the table is the number of the antenna to be switched to.The switching sequence of the antenna
 *              needs to be determined by the combination of the table look-up sequence and the antenna number in
 *              the table,so this function is usually used together with the rf_aoa_aod_ant_lut function.
 * @param[in]   pattern     - Enumeration of several different look-up table order modes.Refer to the corresponding
 *                            enumeration annotation for the meaning of the mode.
 * @return      none.
 */
_attribute_ram_code_
void rf_aoa_aod_ant_pattern(rf_ant_pattern_e pattern)
{
    reg_rf_man_ant_slot = ((reg_rf_man_ant_slot&(~FLD_RF_ANT_PAT))|pattern);
}

/**
 * @brief       This function is mainly used to set the number of antennas enabled by the multi-antenna board in the
 *              AOA/AOD function;the vulture series chips currently support up to 8 antennas for switching.By default,
 *              it is set to 8 antennas. After configuring the RF-related settings, you can set the number of enabled
 *              antennas, and this setting needs to be completed before sending and receiving packets.
 * @param[in]   ant_num     - The number of antennas, the value ranges from 1 to 8.
 * @return      none.
 */
_attribute_ram_code_
void rf_aoa_aod_set_ant_num(unsigned char ant_num)
{
    ant_num = (((ant_num - 1) & 0x07) << 4);
    reg_rf_rxsupp = ((reg_rf_rxsupp&(~FLD_RF_ANT_NUM))|ant_num);
}

/**
 * @brief       This function is used to set the antenna switching sequence table. The content in the table is the
 *              antenna sequence number that needs to be switched to when the position is found by the look-up table.
 *              Since determining the antenna switching sequence needs to determine the order of the table lookup and
 *              the setting of the table content, this function is usually used in conjunction with the function
 *              rf_aoa_aod_ant_pattern.
 * @param[in]   dat      - Antenna serial number written into the antenna switching sequence table.The value in the table
 *                       corresponds to the antenna number that needs to be switched to when it is found in the table.The
 *                       value range is 0 to 7.
 * @return      none.
 */
void rf_aoa_aod_ant_lut(unsigned char *dat)
{
    write_reg8(0x170068,((read_reg8(0x170068)&0xf0)|dat[0]));
    write_reg8(0x170068,((read_reg8(0x170068)&0x0f)|(dat[1]<<4)));
    write_reg8(0x170069,((read_reg8(0x170069)&0xf0)|dat[2]));
    write_reg8(0x170069,((read_reg8(0x170069)&0x0f)|(dat[3]<<4)));
    write_reg8(0x17006a,((read_reg8(0x17006a)&0xf0)|dat[4]));
    write_reg8(0x17006a,((read_reg8(0x17006a)&0x0f)|(dat[5]<<4)));
    write_reg8(0x17006b,((read_reg8(0x17006b)&0xf0)|dat[6]));
    write_reg8(0x17006b,((read_reg8(0x17006b)&0x0f)|(dat[7]<<4)));
    write_reg8(0x17006c,((read_reg8(0x17006c)&0xf0)|dat[8]));
    write_reg8(0x17006c,((read_reg8(0x17006c)&0x0f)|(dat[9]<<4)));

    write_reg8(0x17006d,((read_reg8(0x17006d)&0xf0)|dat[10]));
    write_reg8(0x17006d,((read_reg8(0x17006d)&0x0f)|(dat[11]<<4)));
    write_reg8(0x17006e,((read_reg8(0x17006e)&0xf0)|dat[12]));
    write_reg8(0x17006e,((read_reg8(0x17006e)&0x0f)|(dat[13]<<4)));
    write_reg8(0x17006f,((read_reg8(0x17006f)&0xf0)|dat[14]));
    write_reg8(0x17006f,((read_reg8(0x17006f)&0x0f)|(dat[15]<<4)));
}

/**
 * @brief       This function is used to set the antenna switching sequence table. The content in the table is the
 *              antenna sequence number that needs to be switched to when the position is found by the look-up table.
 *              Since determining the antenna switching sequence needs to determine the order of the table lookup and
 *              the setting of the table content, this function is usually used in conjunction with the function
 *              rf_aoa_aod_ant_pattern.
 * @param[in]   dat      - Antenna serial number written into the antenna switching sequence table.The value in the table
 *                       corresponds to the antenna number that needs to be switched to when it is found in the table.The
 *                       value range is 0 to 7.
 * @return      none.
 */
_attribute_ram_code_
_attribute_ram_code_ void ble_rf_cs_ant_lut(unsigned int dat)
{
    write_reg32(0x170068,dat);
}


_attribute_ram_code_
void rf_cs_txant_switch_mode(rf_cs_tx_ant_mode_e mode)
{
    reg_rf_rxchn = (reg_rf_rxchn&0xFC)|mode;
}

void rf_cs_rxant_switch_on(void)
{
    reg_rf_rxchn = (reg_rf_rxchn|BIT(2));
}

void rf_cs_rxant_switch_off(void)
{
    reg_rf_rxchn = (reg_rf_rxchn&(~BIT(2)));
}
_attribute_ram_code_ //ble use
void rf_cs_ant_switch_auto(void)
{
    reg_rf_man_ant_slot &= (~FLD_RF_ANT_SEL_MAN_EN);
}
_attribute_ram_code_ //ble use
void rf_cs_ant_switch_manual(void)
{
    reg_rf_man_ant_slot |= FLD_RF_ANT_SEL_MAN_EN;
}


/**
 * @brief       This function is mainly used to initialize the parameters related to cs antennas.
 * @param[in]   clk_mode    - Set whether the antenna-related clock is always on or only when switching antennas.
 * @param[in]   ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
 * @param[in]   ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @param[in]   ant_txoffset- Adjust the switching start point of the tx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @return      none.
 */
void rf_cs_ant_init(rf_cs_ant_clk_mode_e clk_mode,unsigned char ant_interval,unsigned char ant_rxoffset,unsigned char ant_txoffset)
{
    rf_cs_ant_clk_mode(clk_mode);
    rf_cs_set_ant_interval(ant_interval);
    rf_cs_set_rx_ant_offset(ant_rxoffset);
    rf_cs_set_tx_ant_offset(ant_txoffset);
}


/**
 * @brief        * This function initializes the GPIO pins used for antenna switching according to the specified configuration.
 *                 It disables the input and output functions of the GPIO pins, sets the pull-down resistance, and configures
 *                 the multiplexing function of the pins.
 * 
 * @param[in]   switch_ctrl    - Pointer to the antenna switch control structure array.
 * @param[in]   num            - Number of antenna switch pins
 * @return      none.
 */
void rf_cs_ant_switch_pin_init(rf_cs_ant_switch_ctrl *switch_ctrl, u8 num){

    for(int i = 0; i< num; i++){
        rf_cs_ant_switch_ctrl *pCtrl = &switch_ctrl[i];
        // set ANT config GPIO
        gpio_function_dis(pCtrl->pin);
        gpio_input_dis(pCtrl->pin);
        gpio_output_dis(pCtrl->pin);
        gpio_set_up_down_res(pCtrl->pin, GPIO_PIN_PULLDOWN_100K);

        gpio_set_mux_function(pCtrl->pin,  pCtrl->pin_fun);
    }
}

/**
 * @brief       This function is mainly used to set the antenna switching interval.
 * @param[in]   ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.Value range 0x00~0x1ff.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_ant_interval(unsigned short ant_interval)
{
    write_reg8(0x170035,ant_interval);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(0)))|(ant_interval>>8));
}

/**
 * @brief       This function is mainly used to set the starting position of the antenna switching at the rx-side.
 * @param[in]   ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 *              Value range 0x00~0x1ff.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_rx_ant_offset(unsigned short ant_rxoffset)
{
    write_reg8(0x17003a,ant_rxoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(2)))|((ant_rxoffset>>8)<<2));
    //write_reg8(0x170007,read_reg8(0x170007)|BIT(2));//rx_ant_switch_en
}

/**
 * @brief       This function is mainly used to set the starting position of the antenna switching at the tx-side.
 * @param[in]   ant_txoffset- Adjust the switching start point of the rx-side antenna,(ant_txoffset + 1)*0.125us.
 *              Value range 0x00~0x1ff.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_tx_ant_offset(unsigned short ant_txoffset)
{
    write_reg8(0x170039,ant_txoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(1)))|((ant_txoffset>>8)<<1));
    //write_reg8(0x170007,(read_reg8(0x170007)&0xfc)|0x02);//tx_ant_switch_en  check by lijing, needn't this setting
}

/**
 * @brief       This function is mainly used to set the clock working mode of the antenna.
 * @param[in]   clk_mode    - Open all the time or only when switching antennas.
 * @return      none.
 */
_attribute_ram_code_ //ble use
void rf_cs_ant_clk_mode(rf_cs_ant_clk_mode_e clk_mode)
{
    reg_rf_rxclk_auto = ((reg_rf_rxclk_auto&0xfe) | clk_mode);
}
