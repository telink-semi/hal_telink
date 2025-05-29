/********************************************************************************************************
 * @file    analog_afe1v_aon_reg.h
 *
 * @brief   This is the header file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef ANALOG_AFE1V_AON_REG_H
#define ANALOG_AFE1V_AON_REG_H
#include "soc.h"



#define areg_aon_0x05               0x05
enum{
    FLD_32K_RC_PD           = BIT(0),
    FLD_32K_XTAL_PD         = BIT(1),
    FLD_24M_RC_PD           = BIT(2),
    FLD_48M_XTL_PD          = BIT(3),
    FLD_PLL_ALL_3V          = BIT(4),
    FLD_VBAT_LCLDO_0P8      = BIT(5),
    FLD_PL_VBAT_LDO_3V      = BIT(6),
    FLD_ANA_LDO             = BIT(7),
};

#define areg_aon_0x06               0x06
enum{
    FLD_EN_CURLIMIT         = BIT(0),
    FLD_RFVDD_SELECT        = BIT(1),
    FLD_PD_LC_COMP_3V       = BIT(2),//power down of low current comparator
    FLD_PD_VBAT_SW          = BIT(3),//power down of bypass switch(VBAT LDO)
    FLD_PD_LDO_DCORE        = BIT(4),//power down of digital core ldo
    FLD_PD_LDO_SRAM         = BIT(5),//power down of ram ldo
    //RSVD
    FLD_DIG_RET_PD          = BIT(7),//power down of retention  ldo
};

#define areg_aon_0x07               0x07
enum{
    FLD_DCDC_EN_0P8         = BIT(0),
    FLD_DCDC_HIZ_0P8        = BIT(1),
    FLD_DCDC_IDLE_0P8       = BIT(2),
    FLD_DCDC_ISON_0P8       = BIT(3),
    FLD_DCDC_OEN1_0P8       = BIT(4),
    FLD_DCDC_OEN2_0P8       = BIT(5),
    FLD_DCDC_OEN3_0P8       = BIT(6),
    FLD_DCDC_OEN4_0P8       = BIT(7),
};

#define areg_aon_0x08               0x08
enum{
    FLD_PD_LDO_AVDD1        = BIT(0),
    FLD_PD_LDO_AVDD2        = BIT(1),
    FLD_PD_LDO_DVDD1        = BIT(2),
    FLD_PD_LDO_DVDD2        = BIT(3),
    FLD_PD_LCLDO_AVDD1      = BIT(4),
    FLD_PD_LCLDO_AVDD2      = BIT(5),
    FLD_PD_LCLDO_DVDD1      = BIT(6),
    FLD_PD_LCLDO_DVDD2      = BIT(7),
};

#define areg_aon_0x09               0x09
enum{
    FLD_PD_LCLDO_ANA            = BIT(0),
    FLD_PD_VDD_CORE             = BIT(1),
    FLD_PD_VDD_RAM              = BIT(2),
    FLD_PD_VDD_RETRAM           = BIT(3),
    FLD_PD_POWER_BBPLL_AUDIO    = BIT(4),
    FLD_PD_POWER_BBPLL          = BIT(5),
    FLD_EN_BYPASS_LDODIG_3V     = BIT(6),
    FLD_EN_BYPASS_LDORAM_3V     = BIT(7),
};

#define areg_aon_0x2a               0x2a
enum{
    FLD_AUTO_PD_32K_RC          = BIT(0),
    FLD_AUTO_PD_32K_XTAL        = BIT(1),
    //RSVD
    FLD_AUTO_PD_48M_XTAL        = BIT(3),
    FLD_AUTO_PD_PL_ALL          = BIT(4),
    FLD_AUTO_PD_VBAT_LCLDO_0P8  = BIT(5),
    FLD_AUTO_PD_PL_VBAT_LDO_3V  = BIT(6),
    FLD_AUTO_PD_ANA_LDO         = BIT(7),
};

#define areg_aon_0x2b               0x2b
enum{
    FLD_AUTO_EN_CURLIMT         = BIT(0),
    //RSVD
    FLD_AUTO_PD_LC_COMP_3V      = BIT(2),
    FLD_AUTO_PD_VBAT_SW         = BIT(3),
    FLD_AUTO_PD_LDO_DCORE       = BIT(4),
    FLD_AUTO_PD_LDO_SRAM        = BIT(5),
    FLD_AUTO_PD_BG_0P8          = BIT(6),
    FLD_AUTO_PD_DIG_RET         = BIT(7),
};

#define areg_aon_0x2c               0x2c
enum{
    //RSVD
    FLD_AUTO_PD_DCDC_HIZ_0P8    = BIT(1),
};

#define areg_aon_0x2d               0x2d
enum{
    FLD_AUTO_PD_LDO_AVDD1       = BIT(0),
    FLD_AUTO_PD_LDO_AVDD2       = BIT(1),
    FLD_AUTO_PD_LDO_DVDD1       = BIT(2),
    FLD_AUTO_PD_LDO_DVDD2       = BIT(3),
    //RSVD
    //RSVD
    FLD_ISO_EN                  = BIT(6),
    FLD_PD_SEQUENCE_EN          = BIT(7),
};

#define areg_aon_0x2e               0x2e
enum{
    //RSVD
    FLD_AUTO_PD_VDD_CORE            = BIT(1),
    FLD_AUTO_PD_VDD_RAM             = BIT(2),
    FLD_AUTO_PD_VDD_RETRAM          = BIT(3),
    FLD_AUTO_PD_POWER_BBPLL_AUDIO   = BIT(4),
    FLD_AUTO_PD_POWER_BBPLL         = BIT(5),
    FLD_AUTO_EN_BYPASS_LDODIG_3V    = BIT(6),
    FLD_AUTO_EN_BYPASS_LDORAM_3V    = BIT(7),
};

#define areg_aon_0x4b               0x4b
enum {
    FLD_WAKEUP_EN_PAD           = BIT(0),
    FLD_WAKEUP_EN_CORE          = BIT(1),
    FLD_WAKEUP_EN_TIMER         = BIT(2),
    FLD_WAKEUP_EN_COMPARATOR    = BIT(3),
//  FLD_WAKEUP_EN_MDEC          = BIT(4),
    FLD_WAKEUP_EN_CTB           = BIT(5),
//  FLD_WAKEUP_EN_WT            = BIT(6),
//  FLD_WAKEUP_EN_SHUTDOWN      = BIT(7),
};

#define areg_aon_0x4e               0x4e
enum{
    FLD_XTAL_QUICK          = BIT_RNG(0, 2),
    //RSVD
    FLD_CLK32K_SEL          = BIT(7),//0: 32k rc, 1: 32k xtal
};

#define areg_aon_0x4f               0x4f
enum{
    FLD_RC_32K_CAP          = BIT_RNG(0, 5),
    FLD_RC_32K_CAP_SEL      = BIT(6),
    FLD_RC_24M_CAP_SEL      = BIT(7),
};

#define areg_aon_0x50               0x50
#define areg_aon_0x51               0x51

#define areg_aon_0x52               0x52

#define areg_aon_0x64               0x64
typedef enum {
    FLD_WAKEUP_STATUS_PAD           = BIT(0),
//  FLD_WAKEUP_STATUS_CORE          = BIT(1),
    FLD_WAKEUP_STATUS_TIMER         = BIT(2),
    FLD_WAKEUP_STATUS_COMPARATOR    = BIT(3),
//  FLD_WAKEUP_STATUS_MDEC          = BIT(4),
//  FLD_WAKEUP_STATUS_CTB           = BIT(5),
//  FLD_WAKEUP_STATUS_WT            = BIT(6),
//  FLD_WAKEUP_STATUS_VBUS          = BIT(7),
    FLD_WAKEUP_STATUS_ALL           = 0x2f,
}pm_wakeup_status_e;

#define areg_aon_0x69               0x69
enum{
    FLD_PD_SM_BUSY          = BIT(5),   /*
                                            The pd_sm_busy bit just represents the completion of power switch.
                                            During power switch it will be 1, and after the switch is completed, it will become 0.
                                        */
};

#define areg_aon_0x7d               0x7d
typedef enum{
    FLD_PD_ZB_EN            = BIT(0),   //baseband, for both RF and N22. //weather to power on the BASEBAND before suspend.
    FLD_PD_USB_EN           = BIT(1),   //weather to power on the USB before suspend.
    FLD_PD_AUDIO_EN         = BIT(2),   //weather to power on the NPE before suspend.
    FLD_PD_NPE_EN           = BIT(3),
    FLD_PD_DSP_EN           = BIT(4),
    FLD_PD_WT_EN            = BIT(5),
    FLD_PG_CLK_EN           = BIT(7),   //1:enable change power sequence clk
}pm_pd_module_e;

#endif
