/********************************************************************************************************
 * @file    ext_rf.c
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
#include "../../lib/include/rf.h"
#include "../../dma.h"
#include "../../stimer.h"

#include "ext_lib.h"
#include "ext_rf.h"
#include "common/config/user_config.h"

#define   TXADDR                0xC0013000

#define   BLE_TXDMA_DATA        (0x140800 + 0x84)      //0x140884
#define   BLE_RXDMA_DATA        (0x140800 + 0x80)      //0x140880

_attribute_data_retention_sec_ signed char ble_txPowerLevel = 0; /* <<TX Power Level>>: -127 to +127 dBm */

extern dma_config_t rf_tx_dma_config;
extern dma_config_t rf_rx_dma_config;

//RF BLE Minimum TX Power LVL (unit: 1dBm)
const char  ble_rf_min_tx_pwr   = -23; /* -23dBm */
//RF BLE Maximum TX Power LVL (unit: 1dBm)
const char  ble_rf_max_tx_pwr   = 9;   /*  +9dBm */
//RF BLE Current TX Path Compensation (s16-1280 ~ 1280, unit: 0.1 dB)
_attribute_data_retention_  signed short ble_rf_tx_path_comp = 0;
//RF BLE Current RX Path Compensation (s16-1280 ~ 1280, unit: 0.1 dB)
_attribute_data_retention_  signed short ble_rf_rx_path_comp = 0;

//Current RF RX DMA buffer point for BLE
_attribute_data_retention_ unsigned char *ble_curr_rx_dma_buff = NULL;

_attribute_data_retention_sec_ ext_rf_t blt_extRF;



/******************************* ext_aoa start ******************************************************************/
/**
 * @brief       This function serve to set the antenna switch sequence.
 * @param[in]   ant_num - the number of antenna.
 * @return      none.
 */
void set_antenna_num(unsigned char ant_num)
{
    ant_num = ((ant_num & 0x07) << 4);
    write_reg8(0x140838,(read_reg8(0x140838)&0x0f)|ant_num);
}

void triangle_all_open(void)
{
    /*
     * RF ALL OPEN
     */
    write_reg8(0x140316,(read_reg8(0x140316)&0xf1));

    write_reg8(0x140334,(read_reg8(0x140334)&0xf3)|0x54);

//  sub_wr(0x59e, 0, 6, 6); //d[6], act as gpio
//  sub_wr(0x58e, 0, 1, 0); //b[1:0], act as gpio
//  sub_wr(0x5af, 2, 5, 4); //d[6], antsel_0
//  sub_wr(0x5aa, 2, 1, 0) ;//b[0], antsel_1
//  sub_wr(0x5aa, 2, 3, 2) ;//b[1], antsel_2
//  sub_wr(0x438, 7, 7, 4); //ant_num logical num
}

/**
 * @brief       This function enables the sending and receiving functions of AOA/AOD in ordinary format packets or ADV format packets.
 * @param[in]   mode - AOA/AOD broadcast package or normal package trx mode.
 * @return      none.
 */
void rf_set_aoa_aod_trx_mode(rf_aoa_aod_mode_e mode)
{
    write_reg8(0x140838,((read_reg8(0x140838) & 0xf0) | mode));
}

/**
 * @brief       This function is used to calibrate AOA, AOD sampling frequency offset.
 * @param[in]   sample_locate - This time is the time of a single switch or a single sample slot.
 * @return      none.
 */
void rf_adjust_ant_sample_offset(unsigned char sample_locate)
{
    write_reg8(0x14083b,sample_locate);
}

/**
 * @brief       This function is used to set the AOD/AOA sampling time.
 * @param[in]   sample_time - The default value is 0x2d, add 1 means sample point delay 0.125us.
 * @return      none.
 * @note        Attention:It should be noted that in normal mode, AOD can set 1us or 2us slot time according to CTE information,
 *              while AOA is 2us slot in normal mode; at the same time, AOD can be manually set to 1us slot through this function.
 */
void aoa_set_sample_slot_time(sample_slot_time_e sample_time)
{
    if(sample_time <= 3)
    {
        write_reg8 (0x14083c, (( (read_reg8(0x14083c)&0xcf) | (sample_time << 4) )));
        write_reg8(0x14083e,(read_reg8(0x14083e)&0xfc));
    }
    else
    {
        write_reg8 (0x14083c, ( read_reg8(0x14083c)&0xcf));
        write_reg8(0x14083e,(read_reg8(0x14083e)&0xfc) | (sample_time - 3));
    }
}
/******************************* ext_aoa end ********************************************************************/

_attribute_ram_code_
void ble_rf_set_tx_dma(unsigned char fifo_dep, unsigned char fifo_byte_size)
{
    /*
     * FLD_RF_TX_MULTI_EN must enable
     * 1. FLD_RF_TX_MULTI_EN = 0, whatever reg_rf_bb_tx_chn_dep, can not send OK
     * 2. FLD_RF_TX_MULTI_EN = 1, reg_rf_bb_tx_chn_dep = 0, 2, 3, both OK
     */
    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);//u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk
    reg_rf_bb_tx_chn_dep = fifo_dep;//tx_chn_dep = 2^2 =4 (have 4 fifo)
    reg_rf_bb_tx_size   = fifo_byte_size;//tx_idx_addr = {tx_chn_adr*bb_tx_size,4'b0}// in this setting the max data in one dma buffer is 0x20<<4.And the The product of fifo_dep and bytesize cannot exceed 0xfff

    dma_config(DMA0,&rf_tx_dma_config);//solve dma_chn_dis(DMA0) cause read_num_en reset to 0
    dma_set_address(DMA0, TXADDR, BLE_TXDMA_DATA);   // TXADDR=0xc0013000;
}


_attribute_ram_code_
void ble_rf_set_rx_dma(unsigned char *buff, unsigned char fifo_byte_size)
{
    ble_curr_rx_dma_buff = buff;

    buff +=4;

    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en

    //TODO: check with Qiangkai
    reg_rf_rx_wptr_mask = 0; //rx_wptr_real=rx_wptr & mask:After receiving 4 packets,the address returns to original address.mask value must in (0x01,0x03,0x07,0x0f)
    reg_rf_bb_rx_size = fifo_byte_size;//rx_idx_addr = {rx_wptr*bb_rx_size,4'h0}// in this setting the max data in one dma buffer is 0x20<<4.

//  dma_config(DMA1,&rf_rx_dma_config);   // reg_dma_ctrl(DMA1) = 0xc0aa1200;

    dma_set_address(DMA1, BLE_RXDMA_DATA, (unsigned int)convert_ram_addr_cpu2bus(buff));
    reg_dma_size(DMA1)=0xffffffff;

}

#if (BLC_PM_DEEP_RETENTION_MODE_EN)
_attribute_ram_code_
#endif
void ble_rx_dma_config(void){
    dma_config(DMA1,&rf_rx_dma_config);
}


_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = mode;

    if(tx_addr){
        dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(tx_addr));
    }
}

/**
 * @brief      This function serves to reset baseband.this function is same as driver rf_emi_reset_baseband.
 * but rf_emi_reset_baseband is not ram code.
 * @return     none
 */
_attribute_ram_code_
void ble_rf_reset_baseband(void)
{
    reg_rst3 &= (~FLD_RST3_ZB);               // reset baseband
    reg_rst3 |= (FLD_RST3_ZB);                // clr baseband
}

_attribute_ram_code_ //must be RamCode
void rf_set_ble_channel (signed char chn_num)
{
    signed char ble_chn_num = 0;
    write_reg8 (0x14080d, chn_num);
    if (chn_num < 11)
        ble_chn_num = chn_num + 2;

    else if (chn_num < 37)
        ble_chn_num = chn_num + 3;

    else if (chn_num == 37)
        ble_chn_num = 1;

    else if (chn_num == 38)
        ble_chn_num = 13;

    else if (chn_num == 39)
        ble_chn_num = 40;

    ble_chn_num = ble_chn_num << 1;
    rf_set_chn(ble_chn_num);
}


#if (SW_DCOC_EN)
/**
 * @brief   This function serves to get agc gain, User API.
 * @return  agc gain(3 bit)
 */
unsigned char rf_get_agc_gain(void)
{
    return ((read_reg8(0x140e40)&0x1c)>>2);
}

/**
 *  @brief      This function is mainly used to get dcoc dac values, User API.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
void rf_get_dcoc_dac_val(rf_dcoc_iq_dac_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC = read_reg8(0x140ed8) & 0x3f;//DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC = read_reg8(0x140eda) & 0x3f;//DCOC_QDAC 0xda[5:0]
}
#endif




_attribute_ram_code_
void rf_mode_optimize_init(void)
{

        #if (SW_DCOC_EN)
            extern void rf_sw_dcoc_cal(void);
            rf_sw_dcoc_cal();
        #endif

            //To modify DCOC parameters, default:0x15bb
            write_reg8(0x140ed2,0x9b);//DCOC_SFIIP DCOC_SFQQP
            write_reg8(0x140ed3,0x19);//DCOC_SFQQ

            //Setting for blanking
        #if RF_RX_SHORT_MODE_EN
            write_reg8(0x140c7b,0x0e);          //default :0xf6;BLANK_WINDOW
            write_reg8(0x140c79,0x38);//BIT[3] RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix
                                      //per floor issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
        #else
            write_reg8(0x140c7b,0xfe);
            write_reg8(0x140c79,0x08);//RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix per floor
                                      //issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
        #endif

            //To set AGC thresholds
            write_reg16(0x140e4a,0x090e);       //default:0x0689;POW_000_001,POW_001_010_H
        //  write_reg16(0x140e4e,0x0f09);       //default:0x0f09;POW_100_101 ,POW_101_100_L,POW_101_100_H;
            write_reg32(0x140e54,0x080c090e);   //default:0x078c0689,POW_001_010_L,POW_001_010_H,POW_011_100_L,POW_011_100_H
        //  write_reg16(0x140e58,0x0f09);       //default: 0x0f09;POW_101_100_L,POW_101_100_H
            //For optimum preamble detection
            write_reg16(0x140c76,0x7350);       //default:0x7357;FREQ_CORR_CFG2_0,FREQ_CORR_CFG2_1
        #if RF_RX_SHORT_MODE_EN
            write_reg16(0x14083a,0x6586);       //default:0x2d4e;rx_ant_offset  rx_dly(0x140c7b,0x140c79,0x14083a,0x14083b),samp_offset
        #endif
            analog_write_reg8(0x8b,0x04);       //default:0x06;FREQ_CORR_CFG2_1

}

#if BLC_PM_EN
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void rf_drv_ble_init(void)
{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   1. merge from driver function "rf_mode_init"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    rf_mode_optimize_init();
    
    rf_ble_set_1m_phy();


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   3. setting for BLE by BLE_Team
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
    write_reg8(0x140830, 0x36);         //default:0x3c;disable tx timestamp en, add by LiBiao

//  //copy from QingHua's code
//  write_reg32(0x80140860, 0x5f4f4434);  //grx_3~0
//  write_reg16(0x80140864, 0x766b);      //grx_5~4

//  write_reg8(0x80140a06, 0x00);       //LL_RXWAIT, default 0x0009    only involved in BTX/BRX/RX2TX
//  write_reg8(0x80140a0c, 0x50);       //LL_RXSTL   default 0x0095
//  write_reg8(0x80140a0e, 0x00);       //LL_TXWAIT, default 0x0009    only involved in BTX/BRX/TX2RX
//  write_reg8(0x80140a10, 0x00);       //LL_ARD,    default 0x0063


    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;        //coded phy accesscode triggle mode: manual mode

}



void rf_switchPhyTestMode(rf_mode_e mode)
{
    if(mode == RF_MODE_BLE_1M)
    {
        write_reg8(0x401, 0x00);    //PN disable
        write_reg8(0x402, 0x46);
        write_reg8(0x404, 0xd5);    //PN disable

        #if (SW_DCOC_EN)
            //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
            //Restore the secondary filter to initial state (turn off)
            write_reg8(0x140e7a,read_reg8(0x140e7a)|0x20);//bit<5>:BYPASS_RRC_BLE   default 1, Turn off the secondary filter
        #endif
    }
    else if(mode == RF_MODE_BLE_2M)
    {
        write_reg8(0x401, 0x00);    //PN disable
        write_reg8(0x404, 0xc5);    //PN disable

        #if (SW_DCOC_EN)
            //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
            //Restore the secondary filter to initial state (turn off)
            write_reg8(0x140e7a,read_reg8(0x140e7a)|0x20);//bit<5>:BYPASS_RRC_BLE   default 1, Turn off the secondary filter
        #endif
    }
    else if(mode == RF_MODE_LR_S2_500K)
    {
        write_reg8(0x401, 0x00);

        #if (SW_DCOC_EN)
            //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
            //But this will lead to a narrowing of the RX packet receiving bandwidth and a decrease in frequency offset performance.(Modified by chenxi,confirmed by yuya at 20240407)
            write_reg8(0x140e7a,read_reg8(0x140e7a)&0xdf);//bit<5>:BYPASS_RRC_BLE   default 1,->0 Turn on the secondary filter
        #endif
    }
    else if(mode == RF_MODE_LR_S8_125K)
    {
        write_reg8(0x401, 0x00);

        #if (SW_DCOC_EN)
            //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
            //But this will lead to a narrowing of the RX packet receiving bandwidth and a decrease in frequency offset performance.(Modified by chenxi,confirmed by yuya at 20240407)
            write_reg8(0x140e7a,read_reg8(0x140e7a)&0xdf);//bit<5>:BYPASS_RRC_BLE   default 1,->0 Turn on the secondary filter
        #endif
    }

    write_reg32 (0x800408, 0x29417671); //accesscode: 1001-0100 1000-0010 0110-1110 1000-1110   29 41 76 71
    write_reg8 (0x800405, read_reg8(0x405)|0x80); //todo register have changed

}


/*
 * brief:If already know the DMA length value,this API can calculate the real RF length value that is easier for humans to understand.
 * param: dma_len -- the value calculated by this macro "rf_tx_packet_dma_len"
 * return: 0xFFFFFFFE --- error;
 *         other value--- success;
 */
u32 rf_cal_rfLenFromDmaLen(u32 dma_len){

    u32 dmaLen_LSB = dma_len&0x0003FFFFF;
    u16 dmaLen_rmd = (dma_len&0xFFC00000)>>22;

    u32 realRfLen_min = (dmaLen_LSB<<2) - 3;
    int i = 0;
    for(i=0; i<4; i++){

       u32 realRfLen = realRfLen_min + i;
       if( (realRfLen%4) == dmaLen_rmd){
           return realRfLen;
       }
    }

    return 0xFFFFFFFE; //error, not run here.
}







#if (FAST_SETTLE)

_attribute_data_retention_ Fast_Settle fast_settle_1M;
_attribute_data_retention_ Fast_Settle fast_settle_2M;
_attribute_data_retention_ Fast_Settle fast_settle_S2;
_attribute_data_retention_ Fast_Settle fast_settle_S8;

#endif //end of FAST_SETTLE
