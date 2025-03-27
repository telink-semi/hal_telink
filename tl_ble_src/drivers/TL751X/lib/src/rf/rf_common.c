/********************************************************************************************************
 * @file    rf_common.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/rf/rf_common.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"
#include "stimer.h"
#include "core.h"


/**********************************************************************************************************************
 *                                         RF global constants                                                        *
 *********************************************************************************************************************/
/**
 * @brief The table of rf power level.
 * @note   Attention:
 *          (1)The tx power values in the table are based on VDDRF1 1.8V and VDDRF2 1.8V tests
 *          (2)The power values in the table are for reference only, and the specific power values are subject to actual testing
 *          (3)At present, the configuration method of tx power is not the final version and will be updated in the future
 *
 */
const rf_power_level_e rf_power_Level_list[30] =
{
     /*HP*/
     RF_POWER_P10p00dBm,
     RF_POWER_P9p53dBm,
     RF_POWER_P8p51dBm,
     RF_POWER_P7p53dBm,
     RF_POWER_P6p51dBm,
     RF_POWER_P5p42dBm,
     RF_POWER_P4p92dBm,
     RF_POWER_P4p48dBm,
     RF_POWER_P3p62dBm,
     RF_POWER_P2p52dBm,
     RF_POWER_P1p51dBm,
     RF_POWER_P0p50dBm,
     RF_POWER_P0p00dBm,
     RF_POWER_N0p51dBm,
     RF_POWER_N1p38dBm,
     RF_POWER_N2p39dBm,
     RF_POWER_N3p35dBm,
     /*LP*/
     RF_POWER_N4p26dBm,
     RF_POWER_N9p07dBm,
     RF_POWER_N14p12dBm,
     RF_POWER_N18p56dBm,
     RF_POWER_N23p25dBm,
};



static rf_status_e s_rf_trxstate = RF_MODE_TX;
rf_mode_e   g_rfmode;

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

/*********************************************onca rf init*******************************************************/

/**
 * @brief      This function serves to calibrate the adpll.
 * @return     none.
 */
void rf_calib_adpll(void)
{

    unsigned short kdco[11];
    unsigned short kdtc[11];
    unsigned short kdco_avg=0;
    unsigned short kdtc_avg=0;

    write_reg8(CSEMDIGADDR+0x01f,read_reg8(CSEMDIGADDR+0x01f)&(~BIT(4)));
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg8(CSEMADPLLADDR + 0x07B, 0x9e);    //DMA
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg8(CSEMDIGADDR+ 0x320,0x15);        //COMMANDS
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk*1000);//Delay 1ms for ADPLL DMA ready,
    write_reg8(CSEMDIGADDR+ 0x320,0x16); //COMMANDS

    for(int k=0;k<11;k++){
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        write_reg8(CSEMADPLLADDR+ 0x006,k<<3);//CHANNEL_REG
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        write_reg8(0xd4170016,k<<3);
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        write_reg8(CSEMDIGADDR+ 0x320,0x12); //COMMANDS
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk*100);
        //while((read8(CSEMADPLLADDR + 0x07B)&0x04)==0);//DMA
//      core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk*1000);//Delay 1ms for calibration ready
        write_reg8(CSEMDIGADDR+ 0x320,0x13);//COMMANDS
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        kdco[k] =(unsigned short)read_reg16(CSEMADPLLADDR+ 0x0b0);//KDCO_HF
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        kdco_avg += kdco[k];
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        kdtc[k] =(unsigned short)read_reg16(CSEMADPLLADDR+ 0x0b2);//KDTC_OUT
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        kdtc_avg += kdtc[k];
    }
    kdtc_avg /= 11;
    kdco_avg /= 11;
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg16(CSEMADPLLADDR+ 0x038, kdco_avg);//KDCO_CAL_HF_IC
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg16(CSEMADPLLADDR+ 0x048, kdtc_avg);//KDTC_CAL_IC
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    for(int k=0;k<11;k++){
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        write_reg8(CSEMADPLLADDR+ 0x03c+k,0x1f&(kdco[k]-kdco_avg));//KDCO_LUT_COEFS
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        write_reg8(CSEMADPLLADDR+ 0x050+k,0x1f&(kdtc[k]-kdtc_avg));//KDTC_LUT_COEFS
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    }
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    reg_rf_kdco_cal0 &= (~FLD_RF_KDCO_CAL_TX_E);//KDCO_CAL_TX_E
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    reg_rf_kdtc_cal0 &= (~FLD_RF_KDTC_CAL_E);//KDTC_CAL_E
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    reg_rf_kdco_lut |= FLD_RF_KDCO_LUT_E;
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    reg_rf_kdtc_lut |=FLD_RF_KDTC_LUT_E;
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    reg_rf_kdco_cal0 &=(~FLD_RF_KDCO_CAL_RX_E);
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg8(CSEMADPLLADDR + 0x07B, 0x00);
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
    write_reg8(CSEMDIGADDR+0x01f,read_reg8(CSEMDIGADDR+0x01f)|(BIT(4)));
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
}

static const unsigned char TELOSR2_MAIN_REGS[] = {
     0x00, 0x00, 0x00, 0x00,   0x1f, 0x00, 0x00, 0x01,   0x00, 0x00, 0x00, 0x01,   0x00, 0x00, 0x00, 0x00,   // 0x00x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x10,   // 0x01x
     0x2f, 0x00, 0x01, 0x0c,   0x18, 0x02, 0x21, 0x0e,   0x3c, 0x0f, 0x18, 0x0d,   0x7b, 0x00, 0x8b, 0x00,   // 0x02x
     0x8e, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x10,   // 0x03x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0xfc, 0x00,   0x00, 0x00, 0xc3, 0x00,   // 0x04x
     0xf0, 0x10, 0x00, 0x4f,   0x00, 0x0c, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   // 0x05x
     0xc6, 0x0f, 0x00, 0x00,   0x20, 0x00, 0x00, 0x00,   0x11, 0x03, 0x00, 0x03,   0x00, 0x00, 0xf0, 0x00,   // 0x06x
     0x9b, 0x00, 0x0d, 0x04,   0x87, 0x00, 0x0e, 0x0b,   0x80, 0x0b, 0x31, 0x01,   0x17, 0x70, 0x00, 0x34,   // 0x07x
     0x01, 0x02, 0x00, 0xfe,   0xfd, 0x01, 0x06, 0x07,   0x00, 0xf2, 0xea, 0xf3,   0x13, 0x43, 0x6e, 0x7f,   // 0x08x
     0x00, 0x68, 0x00, 0xf0,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x02,   0x05, 0x0c, 0x18, 0x2a,   // 0x09x
     0x3d, 0x4e, 0x5a, 0x60,   0x02, 0x05, 0x01, 0x01,   0x00, 0x03, 0x00, 0x00,   0xbc, 0x82, 0x0c, 0x38,   // 0x0ax
//     0x10, 0x00, 0xff, 0x01,   0x0c, 0x04, 0x3f, 0x00,   0xff, 0xff, 0xff, 0x03,   0x00, 0x08, 0x08, 0x00,   // 0x0bx default configuration
     0x10, 0x00, 0xff, 0x01,   0x0c, 0x04, 0x00, 0x00,   0xff, 0xff, 0xff, 0x03,   0x00, 0x0f, 0x0f, 0x00,   // 0x0bx

     0x00, 0x10, 0x20, 0x30,   0x00, 0x00, 0x00, 0x00,   0x09, 0x3f, 0x3f, 0x13,   0x3f, 0x27, 0x0d, 0x04,   // 0x0cx
     0x18, 0x19, 0x1f, 0x15,   0x3f, 0x1e, 0x3f, 0x3f,   0x3f, 0x3f, 0x3f, 0x1c,   0x3f, 0x22, 0x20, 0x3f,   // 0x0dx
     0x1f, 0x0b, 0x08, 0x3f,   0x1d, 0x16, 0x0a, 0x28,   0x29, 0x3f, 0x1f, 0x24,   0x25, 0x1a, 0x3f, 0x2a,   // 0x0ex
     0x07, 0x23, 0x2b, 0x00,   0x02, 0x01, 0xe0, 0x50,   0x26, 0x48, 0x66, 0x80,   0x96, 0xaa, 0xba, 0xc8,   // 0x0fx
     0xd3, 0xdd, 0xe5, 0xec,   0xf1, 0xf6, 0xfb, 0x01,   0x10, 0x20, 0x30, 0x40,   0x50, 0x60, 0x70, 0x80,   // 0x10x
     0x90, 0xa0, 0xb0, 0xc0,   0xd0, 0xe0, 0xf0, 0x19,   0x10, 0x20, 0x30, 0x40,   0x50, 0x60, 0x70, 0x80,   // 0x11x
     0x90, 0xa0, 0xb0, 0xc0,   0xd0, 0xe0, 0xf0, 0x04,   0x10, 0x20, 0x30, 0x40,   0x50, 0x60, 0x70, 0x80,   // 0x12x
     0x90, 0xa0, 0xb0, 0xc0,   0xd0, 0xe0, 0xf0, 0x07,   0x90, 0x00, 0x98, 0x00,   0xa0, 0x00, 0xa8, 0x00,   // 0x13x
     0xb0, 0x00, 0xb8, 0x00,   0xc0, 0x00, 0xc8, 0x00,   0xd0, 0x00, 0xd8, 0x00,   0xe0, 0x00, 0x00, 0x00,   // 0x14x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   // 0x15x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x05, 0x00, 0x00, 0x00,   0x00, 0x93, 0x00, 0x00,   // 0x16x
     0x00, 0x00, 0x28, 0x01,   0x80, 0xc1, 0x48, 0x19,   0x80, 0xc1, 0x48, 0x01,   0x83, 0x91, 0x00, 0x00,   // 0x17x
     0x12, 0x63, 0x00, 0x00,   0x12, 0x63, 0x00, 0x22,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   // 0x18x
     0x00, 0x00, 0x0c, 0x0c,   0x30, 0x3c, 0x0c, 0x0c,   0x30, 0x0c, 0x00, 0x03,   0x00, 0x00, 0xab, 0x0e,   // 0x19x
     0x44, 0xd0, 0xa2, 0x20,   0x03, 0x16, 0x00, 0x00,   0x0c, 0x00, 0x00, 0x00,   0x04, 0x00, 0x00, 0x01,   // 0x1ax
     0x37, 0x00, 0x3f, 0x1e,   0x53, 0x08, 0x19, 0x00,   0x20, 0x10, 0x80, 0x00,   0x12, 0xf0, 0xde, 0xed,   // 0x1bx
     0x1f, 0x64, 0xca, 0xfe,   0x00, 0x00, 0x1f, 0x00,   0x21, 0x74, 0xca, 0xfe,   0x00, 0x00, 0x22, 0x27,   // 0x1cx
     0x27, 0x09, 0x28, 0x03,   0x28, 0x10, 0xb8, 0x1d,   0x18, 0x08, 0x08, 0x12,   0x27, 0x27, 0x61, 0x00,   // 0x1dx
     0x01, 0x00, 0x9c, 0x03,   0x00, 0x00, 0x00, 0x12,   0x22, 0x3b, 0x55, 0x27,   0x01, 0x01, 0x15, 0x00,   // 0x1ex
     0xe7, 0x45, 0x03, 0x03,   0x8f, 0x00, 0x05, 0x0a,   0x1f, 0x31, 0x00, 0x50,   0x00, 0x20, 0x10, 0x02,   // 0x1fx
     0x3e, 0x20, 0x10, 0x00,   0x14, 0x20, 0x10, 0x00,   0x20, 0xf0, 0x10, 0x00,   0x07, 0x7f, 0x23, 0x49,   // 0x20x
     0x15, 0x7f, 0x7f, 0x2f,   0x2d, 0x4b, 0x36, 0x45,   0x38, 0x6e, 0x4c, 0x7f,   0x7f, 0x7f, 0x7f, 0x02,   // 0x21x
     0x68, 0x6d, 0x6c, 0x45,   0x7f, 0x3c, 0x41, 0x4a,   0x47, 0x45, 0x4d, 0x04,   0x03, 0x66, 0x5c, 0x3d,   // 0x22x
     0x4b, 0x59, 0x5b, 0x54,   0x7f, 0x3b, 0x42, 0x30,   0x2e, 0x32, 0x29, 0x7f,   0x40, 0x4e, 0x7f, 0x19,   // 0x23x
     0x13, 0x09, 0x51, 0x17,   0x46, 0x0d, 0x3c, 0x1d,   0x25, 0x1b, 0x46, 0x3e,   0x46, 0x1f, 0x0b, 0x5d,   // 0x24x
     0x46, 0x4c, 0x46, 0x54,   0x4c, 0x66, 0x48, 0x4c,   0x7f, 0x7f, 0x21, 0x4c,   0x5c, 0x64, 0x7f, 0x7f,   // 0x25x
     0x7f, 0x7f, 0x7f, 0x66,   0x5d, 0x46, 0x2c, 0x00,   0x2b, 0x7f, 0x69, 0x69,   0x2f, 0x49, 0xb0, 0x55,   // 0x26x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   // 0x27x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   // 0x28x
     0xe3, 0x00, 0x00, 0x00,   0x25, 0x00, 0xdf, 0x11,   0x2d, 0x03, 0x80, 0x00,   0x55, 0x55, 0x55, 0x00,   // 0x29x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x50,   0x00, 0x00, 0x00, 0x00,   // 0x2ax
     0x00, 0x00, 0x00, 0x00,   0x01, 0x02, 0x01, 0x03,   0x00, 0x20, 0x00, 0x00,   0x0f, 0x02, 0x00, 0x0c,   // 0x2bx
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x1e, 0x18,   0x1c, 0x02, 0x01, 0x03,   // 0x2cx
     0x01, 0x01, 0x03, 0x03,   0x24, 0x00, 0x00, 0x07,   0x01, 0x50, 0x8a, 0x02,   0x8a, 0x02, 0x00, 0x00,   // 0x2dx
     0x00, 0x00, 0x1c, 0x08,   0x00, 0x00, 0x00, 0x07,   0x33, 0x50, 0x18, 0x01,   0x13, 0x00, 0x37, 0xf7,   // 0x2ex
     0x80, 0x24, 0x05, 0x0c,   0x00, 0x00, 0x00, 0x00,   0x04, 0x01, 0x00, 0x00,   0x02, 0x25, 0x01, 0x3c,   // 0x2fx
     0x00, 0x00, 0x6c, 0x00,   0x00, 0x28, 0x16, 0x02,   0x00, 0x00, 0x00, 0x00
};
static const int telosr2_main_regs_len = sizeof(TELOSR2_MAIN_REGS)/sizeof(TELOSR2_MAIN_REGS[0]);

void rf_csem_dig_setup_csem(const unsigned char *telosr2_main_regs, const int telosr2_main_regs_len1)
{
    for (int i = 0;  i <telosr2_main_regs_len1; i++)
        {
            write_reg8(CSEMDIGADDR+ 0x000+i, telosr2_main_regs[i]);
        }
}

static const unsigned char TELOSR2_ADPLL_REGS[] = {
     0x55, 0x05, 0x19, 0x00,   0x55, 0x15, 0x13, 0x2c,   0xaa, 0x02, 0x00, 0x02,   0x89, 0x73, 0x24, 0x33,   // 0x00x
     0x1e, 0xe9, 0x91, 0x1c,   0x07, 0x1f, 0x1e, 0x1f,   0x5b, 0x53, 0x01, 0x00,   0x01, 0x00, 0x00, 0x00,   // 0x01x
     0x00, 0x01, 0x07, 0x11,   0x1f, 0x31, 0x47, 0x61,   0x7f, 0x03, 0x07, 0x00,   0x00, 0xf8, 0xe5, 0xcf,   // 0x02x
     0xc1, 0xc2, 0xdd, 0x18,   0x7f, 0x00, 0x10, 0x01,   0xc8, 0x00, 0x8f, 0x09,   0x00, 0x00, 0x00, 0x00,   // 0x03x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0xfa, 0x00, 0x60, 0x01,   0xb5, 0x1f, 0x47, 0x01,   // 0x04x
     0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x00, 0x3b,   0x88, 0x15, 0x21, 0x5f,   // 0x05x
     0x7d, 0x3f, 0x02, 0x00,   0xfa, 0x01, 0x04, 0x00,   0xb6, 0x00, 0x02, 0xa0,   0x76, 0x00, 0x07, 0xde,   // 0x06x
     0x50, 0x01, 0x01, 0x01,   0x1d, 0x13, 0x2a, 0x0f,   0x04, 0x08, 0xb3, 0x00,   0xc8, 0x03, 0x04, 0x19,   // 0x07x
     0x00, 0x00, 0x00, 0x00
};
static const int telosr2_adpll_regs_len = sizeof(TELOSR2_ADPLL_REGS)/sizeof(TELOSR2_ADPLL_REGS[0]);
void rf_set_csem_adpll_new(const unsigned char *telosr2_adpll_regs,const int telosr2_adpll_regs_len1)
{
    for (int i = 0; i < telosr2_adpll_regs_len1; i++)
        {
            core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
            write_reg8(CSEMADPLLADDR+i, telosr2_adpll_regs[i]);
            core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);
        }
}

static const unsigned char TELOSR2_RXPH_CODE[] = {
     0x71, 0xb2, 0x1a, 0x3f,     0x35, 0x07, 0x79, 0x71,     0xb1, 0x18, 0x32, 0x07,     0x2f, 0x31, 0x08, 0x20,   // 0x0x
     0x44, 0x1b, 0x2c, 0x44,     0x1b, 0x37, 0xe1, 0x1b,     0x46, 0xff, 0xa4, 0x1b,     0x63, 0x01, 0x06, 0x2b,   // 0x1x
     0x30, 0x08, 0x2b, 0x1e,     0x02, 0x1b, 0x31, 0x13,     0x1b, 0x33, 0xe2, 0xa7,     0x01, 0x06, 0x2b, 0x1a,   // 0x2x
     0x0f, 0x1b, 0x37, 0xe1,     0x1b, 0x16, 0x06, 0x72,     0xaa, 0x1a, 0x00, 0xa3,     0x17, 0xbf, 0xa5, 0x31,   // 0x3x
     0x08, 0x2b, 0x17, 0x1c,     0x84, 0x1b, 0x6a, 0x20,     0x1c, 0x1a, 0x1b, 0x2a,     0x52, 0x1c, 0x06, 0x1b,   // 0x4x
     0x46, 0x25, 0x1c, 0x10,     0x30, 0x08, 0x5b, 0x72,     0xaa, 0x1a, 0x01, 0xaf,     0x1b, 0x4c, 0x15, 0x1b,   // 0x5x
     0x46, 0xf7, 0x1c, 0x10,     0x1b, 0x18, 0x0b, 0x1e,     0x01, 0x34, 0x08, 0x2b,     0x30, 0x07, 0x74, 0x1b,   // 0x6x
     0x67, 0xa1, 0x06, 0x2b,     0x1b, 0x67, 0xa2, 0x06,     0x2b, 0x1a, 0x27, 0x71,     0xb1, 0x1a, 0x0f, 0x1b,   // 0x7x
     0x37, 0xe1, 0x06, 0x2b
};

static const int telosr2_rxph_code_len = sizeof(TELOSR2_RXPH_CODE)/sizeof(TELOSR2_RXPH_CODE[0]);
void rf_set_sram_rxph(const unsigned char *telosr2_rxph_code,const int telosr2_rxph_code_len1)
{
    for (int i = 0; i < telosr2_rxph_code_len1; i++)
        {
            write_reg8(CSEMDIGADDR + 0xB40 + i, telosr2_rxph_code[i]);
        }
}

static const unsigned char TELOSR2_TXPH_CODE[] = {
     0x40, 0xa7, 0x36, 0x07,     0x75, 0x32, 0x07, 0x17,     0x48, 0x31, 0x07, 0x10,     0x30, 0x08, 0x10, 0x4a,   // 0x0x
     0x37, 0x07, 0x6b, 0x02,     0xa0, 0x06, 0x10, 0x41,     0x16, 0x76, 0x37, 0x07,     0x6b, 0x02, 0xa0, 0x31,   // 0x1x
     0x08, 0x1a, 0x12, 0x08,     0x1a, 0x73, 0x20, 0x18,     0x17, 0x16, 0x08, 0x37,     0x07, 0x6b, 0x02, 0xa0,   // 0x2x
     0x12, 0x08, 0x2b, 0x42,     0xa1, 0x43, 0xa0, 0x44,     0x09, 0x00, 0xa2, 0x16,     0x05, 0x04, 0x12, 0x08,   // 0x3x
     0x3d, 0x16, 0x05, 0x45,     0x30, 0x08, 0x49, 0x46,     0x51, 0x34, 0x07, 0x50,     0x04, 0x12, 0x08, 0x4c,   // 0x4x
     0x37, 0x07, 0x63, 0x02,     0x34, 0x07, 0x5a, 0xa0,     0x06, 0x50, 0x30, 0x07,     0x60, 0xa1, 0x06, 0x50,   // 0x5x
     0xa2, 0x06, 0x50, 0x09,     0x00, 0xa5, 0xab, 0xab,     0x4c, 0xa4, 0x00, 0x09,     0x00, 0x1f, 0x02, 0xa3,   // 0x6x
     0x4e, 0xa0, 0x4f, 0xa0,     0x00, 0xa7, 0x36, 0x08,     0x6b, 0x06, 0x75
};

static const int telosr2_txph_code_len = sizeof(TELOSR2_TXPH_CODE)/sizeof(TELOSR2_TXPH_CODE[0]);


void rf_set_sram_txph(const unsigned char *telosr2_txph_code,const int telosr2_txph_code_len1)
{
    for (int i = 0; i < telosr2_txph_code_len1; i++)
        {
            write_reg8(CSEMDIGADDR+ 0xA80+i, telosr2_txph_code[i]);
        }
}


static const unsigned char TELOSR2_SEQ_CODE[] = {
     0x00, 0x6e, 0x75, 0x20,     0x7e, 0x62, 0x02, 0x52,     0xb9, 0x22, 0x52, 0xc1,     0x02, 0x0b, 0x54, 0x8a,   // 0x0x
     0x73, 0x20, 0x40, 0xa8,     0x7e, 0x62, 0x00, 0x00,     0x0b, 0x67, 0x62, 0xb9,     0x22, 0x62, 0xc1, 0x02,   // 0x1x
     0x00, 0x52, 0xbe, 0x03,     0x32, 0x0a, 0x76, 0x31,     0x0a, 0x2d, 0x30, 0x0a,     0x73, 0x5e, 0x75, 0x20,   // 0x2x
     0x52, 0xe0, 0x1f, 0x0b,     0x54, 0x52, 0xb9, 0x1d,     0x73, 0x20, 0x42, 0x00,     0x73, 0x20, 0x43, 0x62,   // 0x3x
     0xb9, 0x1d, 0x62, 0xe0,     0x1f, 0x0b, 0x67, 0x6e,     0x03, 0x01, 0x62, 0xf0,     0x01, 0x62, 0xbe, 0x03,   // 0x4x
     0x73, 0x20, 0xf0, 0x00,     0x53, 0x24, 0x00, 0x5e,     0x7c, 0x7f, 0x8f, 0x5e,     0x7c, 0x80, 0x82, 0x5e,   // 0x5x

     0x18, 0x10, 0x7e, 0xa0,     0x10, 0x81, 0x0d, 0x7e,     0xa0, 0x20, 0x6e, 0x18,     0x10, 0x81, 0x6e, 0x7c,   // 0x6x
     0xff, 0x81, 0x0d, 0x5e,     0x03, 0x01, 0x52, 0xf0,     0x01, 0x09, 0x2d, 0x5e,     0x7c, 0x7f, 0x8a, 0x5e,   // 0x7x
     0x7c, 0x80, 0x82, 0x7e,     0xa0, 0x11, 0x8a, 0x73,     0x20, 0x42, 0x00, 0x0b,     0x67, 0x00, 0x7e, 0x62,   // 0x8x
     0x02, 0x00
};

static const int telosr2_seq_code_len = sizeof(TELOSR2_SEQ_CODE)/sizeof(TELOSR2_SEQ_CODE[0]);


void rf_set_sram_seq(const unsigned char *telosr2_seq_code,const int telosr2_seq_code_len1)
{

    write_reg8 (CSEMDIGADDR+ 0x022, 0x01);        //seq_seqs_0_addr
    write_reg8 (CSEMDIGADDR+ 0x024, 0x18);        //seq_seqs_1_addr
    write_reg8 (CSEMDIGADDR+ 0x026, 0x21);        //seq_seqs_2_addr
    write_reg8 (CSEMDIGADDR+ 0x028, 0x3c);        //seq_seqs_3_addr
    write_reg8 (CSEMDIGADDR+ 0x02a, 0x18);        //seq_seqs_4_addr
    write_reg8 (CSEMDIGADDR+ 0x02c, 0x7b);        //seq_seqs_5_addr
    write_reg8 (CSEMDIGADDR+ 0x02e, 0x8b);        //seq_seqs_6_addr
    write_reg8 (CSEMDIGADDR+ 0x030, 0x8e);        //seq_seqs_7_addr

    for (int i = 0; i < telosr2_seq_code_len1; i++)
        {
            write_reg8(CSEMDIGADDR+ 0x9C0+i, telosr2_seq_code[i]);
        }
}


static const unsigned char TELOSR2_AGC_CODE[] = {
    0x6b, 0x3f, 0xe9, 0xe1,     0x4e, 0xdd, 0xd5, 0xef,     0xc1, 0x82, 0x1f, 0xf9,     0x8a, 0x9f, 0xf9, 0x8f,         // 0x000
    0xff, 0xaf, 0xa5, 0x3e,     0x54, 0x50, 0x85, 0x56,     0x7c, 0x55, 0xe8, 0xa2,     0x4e, 0x5b, 0x8a, 0xfe,         // 0x010
    0x8f, 0x8f, 0x5d, 0xc7,     0xc4, 0x9e, 0xea, 0x34,     0x45, 0xfd, 0xa5, 0x2e,     0xf9, 0x9a, 0x2f, 0xf8,         // 0x020
    0x8a, 0xff, 0xaf, 0x6a,     0x2c, 0xf9, 0x91, 0x1f,     0xf8, 0x31, 0xaa, 0xf9,     0x99, 0x3f, 0xf8, 0x80,         // 0x030
    0x8f, 0xf9, 0x9c, 0x4f,     0xf8, 0x82, 0x9f, 0xf5,     0x01, 0x0a, 0xc0, 0xd1,     0x2f, 0xf9, 0x81, 0xaf,         // 0x040
    0xf9, 0x89, 0x9f, 0xf7,     0xe9, 0x5e, 0x53, 0x04,     0x65, 0x58, 0xc7, 0x45,     0x83, 0x75, 0x58, 0xfd,         // 0x050
    0xec, 0x0e, 0x54, 0xd4,     0x0f, 0xc0, 0x83, 0x0f,     0xf8, 0x98, 0xcf, 0xf9,     0x00, 0x1c, 0xfd, 0xfa,         // 0x060
    0xfa, 0xf8, 0x32, 0x3a,     0x14, 0x05, 0xfc, 0xf9,     0xe2, 0x58, 0xa0, 0xd8,     0x3f, 0xd0, 0x98, 0xcf,         // 0x070
    0xf9, 0x80, 0x4f, 0xf8,     0x9f, 0x6f, 0xc7, 0x12,     0x1a, 0xfd, 0xfa, 0xfa,     0xf8, 0x30, 0x21, 0xc0,         // 0x080
    0x4c, 0xf0, 0xdd, 0x00,     0xfd, 0xf9, 0x05, 0x8a,     0xfd, 0xd1, 0xaf, 0xaf,     0x8f, 0x4f, 0xe0, 0x02,         // 0x090
    0xec, 0x05, 0xdf, 0x4d,     0xd0, 0x9f, 0x5f, 0xa0,     0xd8, 0x1f, 0xfd, 0xfa,     0x5a, 0xa0, 0x1c, 0xce,         // 0x0a0
    0xd0, 0x00, 0x8c, 0xe1,     0x08, 0x0d, 0xc0, 0x94,     0x4d, 0x8f, 0x00, 0x0c,     0xd9, 0xf0, 0x08, 0xc0,         // 0x0b0
    0x83, 0x3d, 0x8f, 0x00,     0x1c, 0xfd, 0x8f, 0xaf,     0xa7, 0xc8, 0x9d, 0x06,     0xcf, 0x8d, 0x8f, 0x9f,         // 0x0c0
    0x1f, 0x51, 0x05, 0x8a,     0xfd, 0xd1, 0xff, 0xf8,     0x3a, 0xca, 0xdc, 0x85,     0xf0, 0xdc, 0xfc, 0x18,         // 0x0d0
    0x51, 0x09, 0x0a, 0xc0,     0x82, 0xaf, 0xe6, 0x1a,     0x5d, 0xa0, 0x00, 0xac,     0x17, 0x04, 0xac, 0xd1,         // 0x0e0
    0x09, 0x0a, 0xc0, 0x3a,     0x41, 0xc0, 0x1a, 0xdd,     0xa0, 0x00, 0xac, 0xf8,     0x00, 0x0c, 0x00, 0x00,         // 0x0f0
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x100
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x110
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x120
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x130
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x140
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x150
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x160
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,         // 0x170
    0x00, 0x00, 0x00, 0x00,     0x00, 0x00, 0x00, 0x00,     0x32, 0x00, 0x00, 0x00,     0x00, 0x44, 0x00, 0x00,         // 0x180
    0x00, 0x00, 0x00, 0x00,     0x49, 0x5a, 0x66, 0x83,     0x8e, 0x71, 0x00, 0x98,     0x00, 0x00, 0x00, 0x00,         // 0x190
    0x00, 0x01, 0x00, 0x00,     0x23, 0x10, 0x10, 0x32,     0x0d, 0x00, 0x00, 0x00,         // 0x1a0
};

static const int telosr2_agc_code_len = sizeof(TELOSR2_AGC_CODE)/sizeof(TELOSR2_AGC_CODE[0]);

void rf_set_sram_agc(const unsigned char *telosr2_agc_code,const int telosr2_agc_code_len1)
{
    for (int i = 0; i < telosr2_agc_code_len1; i++)
        {
            write_reg8(CSEMDIGADDR + 0xC00 + i, telosr2_agc_code[i]);
        }
}

/**
 * @brief      This function serves to set tx power lut character
 * @return     none.
 */
void rf_set_tx_power_lut_character(void)
{
    write_reg8 (CSEMDIGADDR+ 0x139,(read_reg8(CSEMDIGADDR+ 0x139)&0xfc)|0x01);          //dig_tx_top_tx_pow_lut_0_pmu
    write_reg8 (CSEMDIGADDR+ 0x138, 0x54);              //dig_tx_top_tx_pow_lut_0_power

    write_reg8 (CSEMDIGADDR+ 0x13b,(read_reg8(CSEMDIGADDR+ 0x13b)&0xfc)|0x01);          //dig_tx_top_tx_pow_lut_1_pmu
    write_reg8 (CSEMDIGADDR+ 0x13a, 0x6e);              //dig_tx_top_tx_pow_lut_1_power

    write_reg8 (CSEMDIGADDR+ 0x13d,(read_reg8(CSEMDIGADDR+ 0x13d)&0xfc)|0x01);           //dig_tx_top_tx_pow_lut_2_pmu
    write_reg8 (CSEMDIGADDR+ 0x13c, 0x89);              //dig_tx_top_tx_pow_lut_2_power

    write_reg8 (CSEMDIGADDR+ 0x13f,(read_reg8(CSEMDIGADDR+ 0x13f)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_3_pmu
    write_reg8 (CSEMDIGADDR+ 0x13e, 0xa4);              //dig_tx_top_tx_pow_lut_3_power

    write_reg8 (CSEMDIGADDR+ 0x141,(read_reg8(CSEMDIGADDR+ 0x141)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_4_pmu
    write_reg8 (CSEMDIGADDR+ 0x140, 0xc0);              //dig_tx_top_tx_pow_lut_4_power

    write_reg8 (CSEMDIGADDR+ 0x143,(read_reg8(CSEMDIGADDR+ 0x143)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_5_pmu
    write_reg8 (CSEMDIGADDR+ 0x142, 0xc4);              //dig_tx_top_tx_pow_lut_5_power

    write_reg8 (CSEMDIGADDR+ 0x145,(read_reg8(CSEMDIGADDR+ 0x145)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_6_pmu
    write_reg8 (CSEMDIGADDR+ 0x144, 0xc9);              //dig_tx_top_tx_pow_lut_6_power

    write_reg8 (CSEMDIGADDR+ 0x147,(read_reg8(CSEMDIGADDR+ 0x147)&0xfc)|0x01);              //dig_tx_top_tx_pow_lut_7_pmu
    write_reg8 (CSEMDIGADDR+ 0x146, 0xce);              //dig_tx_top_tx_pow_lut_7_power

    write_reg8 (CSEMDIGADDR+ 0x149,(read_reg8(CSEMDIGADDR+ 0x149)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_8_pmu
    write_reg8 (CSEMDIGADDR+ 0x148, 0xd3);              //dig_tx_top_tx_pow_lut_8_power

    write_reg8 (CSEMDIGADDR+ 0x14b,(read_reg8(CSEMDIGADDR+ 0x14b)&0xfc)|0x01);             //dig_tx_top_tx_pow_lut_9_pmu
    write_reg8 (CSEMDIGADDR+ 0x14a, 0xda);              //dig_tx_top_tx_pow_lut_9_power

    write_reg8 (CSEMDIGADDR+ 0x14d,(read_reg8(CSEMDIGADDR+ 0x14d)&0xfc));           //dig_tx_top_tx_pow_lut_10_pmu
    write_reg8 (CSEMDIGADDR+ 0x14c, 0xa6);              //dig_tx_top_tx_pow_lut_10_power

    write_reg8 (CSEMDIGADDR+ 0x14f,(read_reg8(CSEMDIGADDR+ 0x14f)&0xfc));           //dig_tx_top_tx_pow_lut_11_pmu
    write_reg8 (CSEMDIGADDR+ 0x14e, 0xab);              //dig_tx_top_tx_pow_lut_11_power

    write_reg8 (CSEMDIGADDR+ 0x151,(read_reg8(CSEMDIGADDR+ 0x151)&0xfc));           //dig_tx_top_tx_pow_lut_12_pmu
    write_reg8 (CSEMDIGADDR+ 0x150, 0xb0);              //dig_tx_top_tx_pow_lut_12_power

    write_reg8 (CSEMDIGADDR+ 0x153,(read_reg8(CSEMDIGADDR+ 0x153)&0xfc));           //dig_tx_top_tx_pow_lut_13_pmu
    write_reg8 (CSEMDIGADDR+ 0x152, 0xb6);              //dig_tx_top_tx_pow_lut_13_power

    write_reg8 (CSEMDIGADDR+ 0x155,(read_reg8(CSEMDIGADDR+ 0x155)&0xfc));           //dig_tx_top_tx_pow_lut_14_pmu
    write_reg8 (CSEMDIGADDR+ 0x154, 0xbd);              //dig_tx_top_tx_pow_lut_14_power

    write_reg8 (CSEMDIGADDR+ 0x157,(read_reg8(CSEMDIGADDR+ 0x157)&0xfc));            //dig_tx_top_tx_pow_lut_15_pmu
    write_reg8 (CSEMDIGADDR+ 0x156, 0xc2);              //dig_tx_top_tx_pow_lut_15_power

    write_reg8 (CSEMDIGADDR+ 0x159,(read_reg8(CSEMDIGADDR+ 0x159)&0xfc));            //dig_tx_top_tx_pow_lut_16_pmu
    write_reg8 (CSEMDIGADDR+ 0x158, 0xc6);              //dig_tx_top_tx_pow_lut_16_power

    write_reg8 (CSEMDIGADDR+ 0x15b,(read_reg8(CSEMDIGADDR+ 0x15b)&0xfc));            //dig_tx_top_tx_pow_lut_17_pmu
    write_reg8 (CSEMDIGADDR+ 0x15a, 0xcb);              //dig_tx_top_tx_pow_lut_17_power

    write_reg8 (CSEMDIGADDR+ 0x15d,(read_reg8(CSEMDIGADDR+ 0x15d)&0xfc));            //dig_tx_top_tx_pow_lut_18_pmu
    write_reg8 (CSEMDIGADDR+ 0x15c, 0xd0);              //dig_tx_top_tx_pow_lut_18_power

    write_reg8 (CSEMDIGADDR+ 0x15f,(read_reg8(CSEMDIGADDR+ 0x15f)&0xfc));            //dig_tx_top_tx_pow_lut_19_pmu
    write_reg8 (CSEMDIGADDR+ 0x15e, 0xd6);              //dig_tx_top_tx_pow_lut_19_power

    write_reg8 (CSEMDIGADDR+ 0x161,(read_reg8(CSEMDIGADDR+ 0x161)&0xfc));            //dig_tx_top_tx_pow_lut_20_pmu
    write_reg8 (CSEMDIGADDR+ 0x160, 0xdd);              //dig_tx_top_tx_pow_lut_20_power

    write_reg8 (CSEMDIGADDR+ 0x163,(read_reg8(CSEMDIGADDR+ 0x163)&0xfc));            //dig_tx_top_tx_pow_lut_21_pmu
    write_reg8 (CSEMDIGADDR+ 0x162, 0xe2);              //dig_tx_top_tx_pow_lut_21_power

    write_reg8 (CSEMDIGADDR+ 0x165,(read_reg8(CSEMDIGADDR+ 0x165)&0xfc));            //dig_tx_top_tx_pow_lut_22_pmu
    write_reg8 (CSEMDIGADDR+ 0x164, 0xe6);              //dig_tx_top_tx_pow_lut_22_power

    write_reg8 (CSEMDIGADDR+ 0x167,(read_reg8(CSEMDIGADDR+ 0x167)&0xfc));            //dig_tx_top_tx_pow_lut_23_pmu
    write_reg8 (CSEMDIGADDR+ 0x166, 0xeb);              //dig_tx_top_tx_pow_lut_23_power


    //HP mode use lut 0
   reg_rf_ana_trx_rf_pa_pmu_lut_0_0 &= 0xfc; //BIT(0):ana_trx_rf_pa_pmu_lut_0_lp ,BIT(1):ana_trx_rf_pa_pmu_lut_0_ldo_lp
   reg_rf_ana_trx_rf_pa_pmu_lut_0_1  = 32;   //ana_trx_rf_pa_pmu_lut_0_pa_vref

   //LP mode use lut 1
   reg_rf_ana_trx_rf_pa_pmu_lut_1_0 |=0x03;//BIT(0):ana_trx_rf_pa_pmu_lut_1_lp,BIT(1):ana_trx_rf_pa_pmu_lut_1_ldo_lp
   reg_rf_ana_trx_rf_pa_pmu_lut_1_1  = 18; //ana_trx_rf_pa_pmu_lut_1_pa_vref


    //HP LUT
    write_reg8 (CSEMDIGADDR+ 0x0f8, 0x2e);       //dig_tx_top_pa_linearize_lut_0_coef_0
    write_reg8 (CSEMDIGADDR+ 0x0f9, 0x5b);       //dig_tx_top_pa_linearize_lut_0_coef_1
    write_reg8 (CSEMDIGADDR+ 0x0fa, 0x83);       //dig_tx_top_pa_linearize_lut_0_coef_2
    write_reg8 (CSEMDIGADDR+ 0x0fb, 0xa4);       //dig_tx_top_pa_linearize_lut_0_coef_3
    write_reg8 (CSEMDIGADDR+ 0x0fc, 0xc0);       //dig_tx_top_pa_linearize_lut_0_coef_4
    write_reg8 (CSEMDIGADDR+ 0x0fd, 0xc0);       //dig_tx_top_pa_linearize_lut_0_coef_5
    write_reg8 (CSEMDIGADDR+ 0x0fe, 0xe9);       //dig_tx_top_pa_linearize_lut_0_coef_6
    write_reg8 (CSEMDIGADDR+ 0x0ff, 0xfa);       //dig_tx_top_pa_linearize_lut_0_coef_7
    write_reg8 (CSEMDIGADDR+ 0x100, 0xfb);       //dig_tx_top_pa_linearize_lut_0_coef_8
    write_reg8 (CSEMDIGADDR+ 0x101, 0xfb);       //dig_tx_top_pa_linearize_lut_0_coef_9
    write_reg8 (CSEMDIGADDR+ 0x102, 0xfe);       //dig_tx_top_pa_linearize_lut_0_coef_10
    write_reg8 (CSEMDIGADDR+ 0x103, 0xfe);       //dig_tx_top_pa_linearize_lut_0_coef_11
    write_reg8 (CSEMDIGADDR+ 0x104, 0xff);       //dig_tx_top_pa_linearize_lut_0_coef_12
    write_reg8 (CSEMDIGADDR+ 0x105, 0xff);       //dig_tx_top_pa_linearize_lut_0_coef_13
    write_reg8 (CSEMDIGADDR+ 0x106, 0xff);       //dig_tx_top_pa_linearize_lut_0_coef_14

    //LP LUT
    write_reg8 (CSEMDIGADDR+ 0x108, 0x30);       //dig_tx_top_pa_linearize_lut_1_coef_0
    write_reg8 (CSEMDIGADDR+ 0x109, 0x57);       //dig_tx_top_pa_linearize_lut_1_coef_1
    write_reg8 (CSEMDIGADDR+ 0x10a, 0x78);       //dig_tx_top_pa_linearize_lut_1_coef_2
    write_reg8 (CSEMDIGADDR+ 0x10b, 0x93);       //dig_tx_top_pa_linearize_lut_1_coef_3
    write_reg8 (CSEMDIGADDR+ 0x10c, 0xa9);       //dig_tx_top_pa_linearize_lut_1_coef_4
    write_reg8 (CSEMDIGADDR+ 0x10d, 0xbb);       //dig_tx_top_pa_linearize_lut_1_coef_5
    write_reg8 (CSEMDIGADDR+ 0x10e, 0xc9);       //dig_tx_top_pa_linearize_lut_1_coef_6
    write_reg8 (CSEMDIGADDR+ 0x10f, 0xd5);       //dig_tx_top_pa_linearize_lut_1_coef_7
    write_reg8 (CSEMDIGADDR+ 0x110, 0xdf);       //dig_tx_top_pa_linearize_lut_1_coef_8
    write_reg8 (CSEMDIGADDR+ 0x111, 0xe7);       //dig_tx_top_pa_linearize_lut_1_coef_9
    write_reg8 (CSEMDIGADDR+ 0x112, 0xed);       //dig_tx_top_pa_linearize_lut_1_coef_10
    write_reg8 (CSEMDIGADDR+ 0x113, 0xf2);       //dig_tx_top_pa_linearize_lut_1_coef_11
    write_reg8 (CSEMDIGADDR+ 0x114, 0xf7);       //dig_tx_top_pa_linearize_lut_1_coef_12
    write_reg8 (CSEMDIGADDR+ 0x115, 0xf9);       //dig_tx_top_pa_linearize_lut_1_coef_13
    write_reg8 (CSEMDIGADDR+ 0x116, 0xfd);       //dig_tx_top_pa_linearize_lut_1_coef_14


    reg_rf_dig_tx_top_tx_pow |=BIT(0);          //dig_tx_top_tx_pow_use_lut
    reg_rf_ana_trx_rf_pa_pmu |=BIT(0);          //ana_trx_rf_pa_pmu_use_lut


    write_reg8 (CSEMDIGADDR+ 0x0f7, 0x00);              //dig_tx_top_tx_pow_pa_power

}

/**
 * @brief     This function serves to initiate information of RF.
 * @return     none.
 */
void rf_mode_init(void)
{
    //According to the RF power-up timing requirements, it takes at least 5us to power up normally,
    //so when configuring the power-up timing, you need to enable VDDD_LDO (PM_TOP:0x39 BIT<0>) first, then baseband power-up
    //(add by chenxi.wang, confirmed by xuqiang 202401030)
    analog_write_reg8(0x39, 0x01);//vddd ldo enable
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4 |=FLD_RST4_AHB1;
    reg_clk_en4 |=FLD_CLK4_HCLK1_EN;

    reg_n22_rst = 0xfffc;
    reg_n22_clk_en = 0xfffc;

/********************************mdm base set**********************************/
    //  itrxdm_init
    reg_rf_csemdig_refe_block_en |= FLD_RF_PTATS_THIN_E;//Enable PTAT 0V9.
    write_reg16(reg_rf_csemdig_refe_config,(reg_rf_csemdig_refe_config&0xf00f)|(0x05<<4));//Reference voltage tuning. 127 steps of 2mV
    reg_rf_csemdig_refe_config |= FLD_RF_ISOLATE_MISC_B;
    reg_rf_csemdig_ana_trx_adpll_dyn |=BIT(0);//ana_trx_adpll_rst_b:ADPLL active low reset for local digital.

    //  csem_dig_setup
    rf_csem_dig_setup_csem((const unsigned char*)TELOSR2_MAIN_REGS,telosr2_main_regs_len);
    rf_set_csem_adpll_new((const unsigned char*)TELOSR2_ADPLL_REGS,telosr2_adpll_regs_len);

    rf_set_sram_rxph((const unsigned char*)TELOSR2_RXPH_CODE,telosr2_rxph_code_len);
    rf_set_sram_txph((const unsigned char*)TELOSR2_TXPH_CODE,telosr2_txph_code_len);
    rf_set_sram_seq((const unsigned char*)TELOSR2_SEQ_CODE,telosr2_seq_code_len);
    rf_set_sram_agc((const unsigned char*)TELOSR2_AGC_CODE,telosr2_agc_code_len);

    reg_rf_csemdig_refe_config |=FLD_RF_ISOLATE_MISC_B;
    //if tx
    reg_rf_refe_xtal3 &=(~FLD_RF_REFE_XTAL_XO_E_B);
    reg_rf_dig_clk_ctrl &=(~FLD_RF_DIG_CLK_CTRL_SEL_CK_DIG_NXO);
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk*1000);//delay 1ms for xtal
/********************************mdm base set**********************************/

    rf_clr_irq_mask(FLD_RF_IRQ_SYNC);//The default interrupt mask in RF is open.
                                    //Close the interrupt mask in the initialization code and reopen it when in use

/********************************adpll_cal_v2**********************************/
//For adpll calibration
    rf_calib_adpll();
    rf_clr_irq_status(FLD_RF_IRQ_SYNC);//The FLD_RF_IRQ_SYNC interrupt is pulled during the RF initialization ADPLL calibration phase.
    rf_set_tx_power_lut_character();
    reg_rf_dig_seq_cfg = 0x00;//EDR2/EDR3 set 0x10
                              //BLE/Longrange/BR set 0x00
    reg_rf_dig_custom_iface |=FLD_RF_DIG_USE_CUSTOM_IFACE;
/********************************adpll_cal_v2**********************************/

/********************************linklayer setting**********************************/

    rf_set_tx_rx_settle_time(43);
    reg_rf_ll_ctrl3 =(reg_rf_ll_ctrl3&0x0f)|0x30;//tx en dly
    reg_rf_ll_tx_en_ctrl4 = (reg_rf_ll_tx_en_ctrl4&0xf8)|0x01;//tx en dly high
/********************************linklayer setting**********************************/
    rf_update_internal_cap(0x5d);//Currently using 6PF capacitor recommendation by default.
    reg_rf_mdm_dig_irq_mask |=BIT(6);//dig_irq_ctrl_tx_end_sig
                                     //Enable tx end interrupt, if not enabled, tx end status will not be pulled up

}


/**
 * @brief      This setting serve to set the configuration of Tx DMA.
 */
__attribute__((section(".data")))
rf_dma_config_t rf_tx_dma_config={
    .dst_req_sel= 8,//tx req.(must 8)
    .src_req_sel=0,
    .dst_addr_ctrl=DMA_ADDR_FIX,
    .src_addr_ctrl=DMA_ADDR_INCREMENT,//increment.
    .dstmode=DMA_HANDSHAKE_MODE,//handshake.
    .srcmode=DMA_NORMAL_MODE,
    .dstwidth=DMA_CTR_WORD_WIDTH,//must word.
    .srcwidth=DMA_CTR_WORD_WIDTH,//must word.
    .src_burst_size=0,//must 0.
    .vacant_bit=0,
    .read_num_en=1,
    .priority=0,
    .write_num_en=0,
    .auto_en=1,//must 1.
};

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_
void rf_set_tx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk.
    rf_dma_config(RF_TX_DMA,&rf_tx_dma_config);
    rf_dma_set_dst_address(RF_TX_DMA,reg_rf_txdma_adr);
}

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth        - tx chn deep,fifo_depth range: 0~5,Number of fifo=2^fifo_depth.
 * @param[in] fifo_byte_size    - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return    none.
 */
_attribute_ram_code_
void rf_set_tx_dma(unsigned char fifo_dep,unsigned short fifo_byte_size)
{
    rf_set_tx_dma_config();
    rf_set_tx_dma_fifo_num(fifo_dep);
    rf_set_tx_dma_fifo_size(fifo_byte_size);

}


/**
 * @brief      This setting serve to set the configuration of Rx DMA.
 */
__attribute__((section(".data")))
rf_dma_config_t rf_rx_dma_config={
        .dst_req_sel= 0,//tx req.
        .src_req_sel=9,//must 9
        .dst_addr_ctrl=0,
        .src_addr_ctrl=DMA_ADDR_FIX,//increment.
        .dstmode=DMA_NORMAL_MODE,
        .srcmode=DMA_HANDSHAKE_MODE,//handshake.
        .dstwidth=DMA_CTR_WORD_WIDTH,//must word.
        .srcwidth=DMA_CTR_WORD_WIDTH,//must word.
        .src_burst_size=0,//must 0.
        .vacant_bit=0,
        .read_num_en=0,
        .priority=0,
        .write_num_en=1,
        .auto_en=1,//must 1.
    };


/**
 * @brief       This function serve to rx dma config
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_
void rf_set_rx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en.
    rf_dma_config(RF_RX_DMA,&rf_rx_dma_config);
    rf_dma_set_src_address(RF_RX_DMA,reg_rf_rxdma_adr);
    rf_dma_set_size(RF_RX_DMA,0xFFFFFC,RF_DMA_WORD_WIDTH);

}

/**
 * @brief      This function serves to rx dma setting.
 * @param[in]  buff - This parameter is the first address of the received data buffer, which must be 4 bytes aligned, otherwise the program will enter an exception.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *             -# you need to leave 4bytes of space for the length information.
 *             -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem
 *             For example, the actual received data length is 5bytes, the minimum value of the actual buffer size that the user needs to set is 12bytes, and the calculation of 12bytes is explained as follows::
 *             4bytes (length information) + 5bytes (data) + 3bytes (the number of additional bytes to prevent out-of-bounds)
 * @param[in]  wptr_mask       - This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 *                               The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @param[in]  fifo_byte_size  - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return     none.
 */
_attribute_ram_code_
void rf_set_rx_dma(unsigned char *buff,unsigned char wptr_mask,unsigned short fifo_byte_size)
{
    rf_set_rx_dma_config();
    rf_set_rx_buffer(buff);
    rf_set_rx_dma_fifo_num(wptr_mask);
    rf_set_rx_dma_fifo_size(fifo_byte_size);
}

/**
 * @brief       This function serves to RF trigger stx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger tx after tick delay.
 * @return      none.
 */
_attribute_ram_code_
void rf_start_stx  (void* addr,  unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;
}


/**
 * @brief     This function serves to trigger srx on.
 * @param[in] tick  - Trigger rx receive packet after tick delay.
 * @return    none.
 */
_attribute_ram_code_
void rf_start_srx(unsigned int tick)
{
    write_reg32 (0xd4170228, 0x0fffffff);                   // first timeout.
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    write_reg8(0xd4170200, 0x86);
}


/**
 * @brief       This function serves to RF trigger stx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger tx send packet after tick delay.
 * @return      none.
 */
_attribute_ram_code_
void rf_start_stx2rx  (void* addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    write_reg32(0xd4170218, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    write_reg8  (0xd4170200, 0x87); // single tx2rx.
}


volatile unsigned char  g_single_tong_freqoffset = 0;//for eliminate single carrier frequency offset.
/**
 * @brief       This function serves to set rf channel for all mode.The actual channel set by this function is 2402+chn.
 * @param[in]   chn   - That you want to set the channel as 2402+chn.
 * @return      none.
 */
_attribute_ram_code_
void rf_set_chn(signed char chn)
{
    reg_rf_trx_chn = chn ;
}



/**
 * @brief       This function serves to get rssi.
 * @return      rssi value.
 */
signed char rf_get_rssi(void)
{
    return (((signed char)(read_reg8(0xd417137a))) - 110);//this function can not tested on fpga
}

/**
 * @brief       This function serves to set RF Rx manual on.
 * @return      none.
 * @note        The default setting for the rx continue mode interval in manual mode is 14us.
 */
_attribute_ram_code_
void rf_set_rxmode(void)
{
    write_reg8(0xd417024f,read_reg8(0xd417024f)|0x70);//bit<3~7>: rx ramp down delay in continue mode 14us
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    write_reg8(0xd417022b,read_reg8(0xd417022b)|BIT(7));//set continue mode.
    reg_rf_ll_ctrl0 |= FLD_RF_R_RX_EN_MAN;//rx enable.
    reg_rf_rxmode |= FLD_RF_RX_ENABLE;//bb rx enable.

}

/**
 * @brief       This function serves to get the right fifo packet.
 * @param[in]   fifo_num   - the number of fifo set in dma.
 * @param[in]   fifo_dep   - deepth of each fifo set in dma.
 * @param[in]   addr       - address of rx packet.
 * @return      the next rx_packet address.
 */
unsigned char* rf_get_rx_packet_addr(int fifo_num,int fifo_dep,void* addr)
{
    unsigned char rptr;
    rptr = read_reg8(0xd41708f5);
    unsigned char * raw_pkt =(unsigned char *)((unsigned char*)addr + (rptr & (fifo_num-1)) * (fifo_dep));
    write_reg8(0xd41708f5,0x40);
    return raw_pkt;
}

/**
 * @brief       This function serves to set RF Tx mode.
 * @return      none.
 * @note        TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 *              (TX manual mode has a bug)
 */
void rf_set_txmode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}


/**
 * @brief       This function serves to set RF Tx packet address to DMA src_addr.
 * @param[in]   addr   - The packet address which to send.
 * @return      none.
 */
void rf_tx_pkt(void* addr)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_bb_dma_ctr0(0) |= 0x01;
}

/**
 * @brief       This function serves to disable pn of ble mode.
 * @return      none.
 */
void rf_pn_disable(void)
{
    reg_rf_tx_mode2 &= (~FLD_RF_V_PN_EN);
}


/**
 * @brief       This function serves to judge RF Tx/Rx state.
 * @param[in]   rf_status   - Tx/Rx status.
 * @param[in]   rf_channel  - This param serve to set frequency channel(2402+rf_channel) .
 * @return      Whether the setting is successful(-1:failed;else success).
 */
int rf_set_trx_state(rf_status_e rf_status, signed char rf_channel)
{
      int err = 0;

      reg_rf_ll_ctrl0 = 0x45;           // reset tx/rx state machine.
      rf_set_chn(rf_channel);

    if (rf_status == RF_MODE_TX) {
        rf_set_txmode();
        s_rf_trxstate = RF_MODE_TX;
    }
    else if (rf_status == RF_MODE_RX) {
        rf_set_rxmode();
        s_rf_trxstate = RF_MODE_RX;
    }
    else if(rf_status == RF_MODE_OFF){
        rf_set_tx_rx_off();
        s_rf_trxstate = RF_MODE_OFF;
    }
    else if (rf_status == RF_MODE_AUTO) {
        reg_rf_ll_cmd = 0x80;       //stop cmd.
        reg_rf_ll_ctrl3 = 0x29;     // reg0xd4170216 pll_en_man and tx_en_dly_en  enable.
        reg_rf_rxmode |= (~FLD_RF_RX_ENABLE);   //rx disable.
        reg_rf_ll_ctrl0 &=0xce;         //reg0xd4170202 disable rx_en_man and tx_en_man.
        s_rf_trxstate = RF_MODE_AUTO;
    }
    else {
        err = -1;
    }
    return  err;

}


/**
 * @brief       This function serves to set RF power level.
 * @param[in]   level    - The power level to set.
 * @return      none.
 */
void rf_set_power_level (rf_power_level_e level)
{
    reg_rf_tx_power = (unsigned char)(level & 0x3F);
}

/**
 * @brief       This function serves to set RF power through select the level index.
 * @param[in]   idx      - The index of power level which you want to set.
 * @return      none.
 */
void rf_set_power_level_index(rf_power_level_index_e idx)
{
    unsigned char level = 0;

    if(idx < sizeof(rf_power_Level_list)/sizeof(rf_power_Level_list[0]))
    {
        level = rf_power_Level_list[idx];
        reg_rf_tx_power = (unsigned char)(level & 0x3F);
    }
}


/**
 * @brief       This function serves to update the value of internal cap.
 * @param[in]   value   - The value of internal cap which you want to set.
 * @return      none.
 * @note        Attention:
 *             (1)Adjusting the capacitance value may cause abnormal operation of the crystal oscillator!!!
 *             (2)There are three types of crystal oscillator load capacitors used on ONCA: 12pF, 8pF, 6pF
 *                The recommended load capacitance trim center values for these three types of crystal oscillators are:
 *                12pF:0xe6, 8pF:0x8d, 6pF:0x5d
 *                (Adjustable range: 12pF:0xe1<= value <=0xeb,  8pF:0x88<= value <=0x92,  6pF:0x58<= value <= 0x62)
 */
void rf_update_internal_cap(unsigned char value)
{
    write_reg8(0xd4171302,value);//reg_xo_freq_tri,Xtal frequency trimming.
                                 //When using RF, configure this register to adjust the internal capacitance value
}

/**
 * @brief       This function serves to get RF status.
 * @return      RF Rx/Tx status.
 */
rf_status_e rf_get_trx_state(void)
{
    return s_rf_trxstate;
}

/**
 * @brief   This function serve to change the length of preamble.
 * @param[in]   len     -The value of preamble length.Set the register bit<0>~bit<4>.
 * @return      none
 */
void rf_set_preamble_len(unsigned char len)
{
    len = len&0x1f;
    write_reg8(0xd4170002,(read_reg8(0xd4170002)&0xe0)|len);
}

/**
 * @brief   This function serve to set the length of access code.
 * @param[in]   byte_len    -   The value of access code length.
 * @return      none
 */
void rf_set_access_code_len(unsigned char byte_len)
{
    unsigned char temp;
    temp = byte_len & 0x07;
    write_reg8(0xd4170005,(read_reg8(0xd4170005)&0xf8)|temp);
}

/**
 * @brief       This function serves to RF trigger srx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger rx receive packet after tick delay.
 * @return      none.
 */
void rf_start_srx2tx  (void* addr, unsigned int tick)
{
    write_reg32 (0xd4170228, (read_reg32(0xd4170228)&0xff000000)|0xffffff);                 // first timeout
    write_reg32(0xd4170218, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    write_reg8(0xd4170216, read_reg8(0xd4170216) | 0x04);   // Enable cmd_schedule mode
    write_reg16 (0xd4170200, 0x3f88);                               // single rx2tx
}

/**
 * @brief       This function is used to judge whether there is a CRC error in the received packet through hardware.
 *              For the same packet, the value of this bit is consistent with the CRC flag bit in the packet.
 * @param[in]   none.
 * @return      none.
 */
unsigned char rf_get_crc_err(void)
{
    return  (reg_rf_dec_err & 0x10);
}

/**
 * @brief      This function serves to reset RF digital logic states.
 * @return     none
 * @note       (1)The rf_dma_reset interface needs to be called before this interface is called.
 *             (2)This function requires setting reset zb, rstl_bb, and reset modem.
 *                It is used to clear RF related state machines, IRQ states, and digital internal logic states.
 */
_attribute_ram_code_sec_noinline_ void rf_clr_dig_logic_state(void)
{
    reg_n22_rst &=~(FLD_RST0_ZB|(FLD_RST1_RST_BB<<8));
    write_reg8(CSEMDIGADDR+ 0x320,0xF0);//COMMANDS for reset
    write_reg8(CSEMDIGADDR+ 0x320,0x41);//COMMANDS for Stop TX
    write_reg8(CSEMDIGADDR+ 0x320,0x51);//COMMANDS for ADPLL mod stop
    write_reg8(CSEMDIGADDR+ 0x320,0x71);//COMMANDS for RCCO CAL reset
    write_reg8(CSEMDIGADDR+ 0x320,0x43);//COMMANDS for Stop RX
    write_reg8(CSEMDIGADDR+ 0x320,0x65);//COMMANDS for reset AGC
    write_reg8(CSEMDIGADDR+ 0x320,0xF0);//COMMANDS for reset
    reg_n22_rst |= (FLD_RST0_ZB|(FLD_RST1_RST_BB<<8));
}

/**
 * @brief      This function is used to restore the rf related registers to their default values.
 * @return     none
 * @note       (1)After calling this interface, all configured interfaces of rf need to be called again.
 *             (2)After calling this interface, RF DMA configurations need to be reconfigured.
 */
_attribute_ram_code_sec_noinline_ void rf_reset_register_value(void)
{
    reg_n22_rst0 &= ~FLD_RST0_ZB_PON;
    reg_n22_rst0 |= FLD_RST0_ZB_PON;
}

