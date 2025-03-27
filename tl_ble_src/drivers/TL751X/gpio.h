/********************************************************************************************************
 * @file    gpio.h
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
/** @page GPIO
 *
 *  Introduction
 *  ===============
 *  B92 contain two six group gpio(A~F), total 44 gpio pin.
 *
 *  API Reference
 *  ===============
 *  Header File: gpio.h
 */
#ifndef DRIVERS_GPIO_H_
#define DRIVERS_GPIO_H_


#include "lib/include/plic.h"
#include "analog.h"
#include "reg_include/gpio_reg.h"
/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**
 *  @brief  Define GPIO group types
 */
typedef enum{
    GPIO_GROUP_A    = 0,
    GPIO_GROUP_B    = 1,
    GPIO_GROUP_C    = 2,
    GPIO_GROUP_D    = 3,
    GPIO_GROUP_E    = 4,
    GPIO_GROUP_F    = 5,
    GPIO_GROUP_G    = 6,
    GPIO_GROUP_H    = 7,
    GPIO_GROUP_I    = 8,
    GPIO_GROUP_J    = 9,
    GPIO_GROUP_ANA  = 10,
}gpio_group_e;
/**
 *  @brief  Define GPIO types
 */
typedef enum{
    GPIO_GROUPA    = 0x000,
    GPIO_GROUPB    = 0x100,
    GPIO_GROUPC    = 0x200,
    GPIO_GROUPD    = 0x300,
    GPIO_GROUPE    = 0x400,
    GPIO_GROUPF    = 0x500,
    GPIO_GROUPG    = 0X600,
    GPIO_GROUPH    = 0X700,
    GPIO_GROUPI    = 0X800,
    GPIO_GROUPJ    = 0X900,
    GPIO_GPOUPANA  = 0xa00,
    GPIO_ALL       = 0xb00,

    GPIO_PA0 = GPIO_GROUPA | BIT(0),
    GPIO_PA1 = GPIO_GROUPA | BIT(1),
    GPIO_PA2 = GPIO_GROUPA | BIT(2),
    GPIO_PA3 = GPIO_GROUPA | BIT(3),GPIO_USB_DM=GPIO_PA3,  // default: SSPI_SI
    GPIO_PA4 = GPIO_GROUPA | BIT(4),GPIO_USB_DP=GPIO_PA4,  // default: SSPI_CN
    GPIO_PA5 = GPIO_GROUPA | BIT(5),GPIO_DM=GPIO_PA5,      // default: SSPI_CK
    GPIO_PA6 = GPIO_GROUPA | BIT(6),GPIO_DP=GPIO_PA6,      // default: SSPI_SO
    GPIO_PA7 = GPIO_GROUPA | BIT(7),GPIO_SWS=GPIO_PA7,     // only support SWS_IO(default)
    GPIOA_ALL = GPIO_GROUPA | 0x00ff,

    GPIO_PB0 = GPIO_GROUPB | BIT(0),
    GPIO_PB1 = GPIO_GROUPB | BIT(1), // default: TCK
    GPIO_PB2 = GPIO_GROUPB | BIT(2), // default: TMS
    GPIO_PB3 = GPIO_GROUPB | BIT(3), // default: TDO
    GPIO_PB4 = GPIO_GROUPB | BIT(4), // default: TDI
    GPIO_PB5 = GPIO_GROUPB | BIT(5),
    GPIO_PB6 = GPIO_GROUPB | BIT(6),
    GPIO_PB7 = GPIO_GROUPB | BIT(7),
    GPIOB_ALL = GPIO_GROUPB | 0x00ff,

    GPIO_PC0 = GPIO_GROUPC | BIT(0), // only support EMMC_CK
    GPIO_PC1 = GPIO_GROUPC | BIT(1), // only support EMMC_CMD
    GPIO_PC2 = GPIO_GROUPC | BIT(2), // only support EMMC_DAT0
    GPIO_PC3 = GPIO_GROUPC | BIT(3), // only support EMMC_DAT1
    GPIO_PC4 = GPIO_GROUPC | BIT(4), // only support EMMC_DAT2
    GPIO_PC5 = GPIO_GROUPC | BIT(5), // only support EMMC_DAT3
    GPIO_PC6 = GPIO_GROUPC | BIT(6), // only support EMMC_DAT4
    GPIO_PC7 = GPIO_GROUPC | BIT(7), // only support EMMC_DAT5
    GPIOC_ALL = GPIO_GROUPC | 0x00ff,

    GPIO_PD0 = GPIO_GROUPD | BIT(0), // only support EMMC_DAT6
    GPIO_PD1 = GPIO_GROUPD | BIT(1), // only support EMMC_DAT7
    GPIO_PD2 = GPIO_GROUPD | BIT(2),
    GPIO_PD3 = GPIO_GROUPD | BIT(3),
    GPIO_PD4 = GPIO_GROUPD | BIT(4),
    GPIO_PD5 = GPIO_GROUPD | BIT(5),
    GPIO_PD6 = GPIO_GROUPD | BIT(6),
    GPIO_PD7 = GPIO_GROUPD | BIT(7),
    GPIOD_ALL = GPIO_GROUPD | 0x00ff,

    GPIO_PE0 = GPIO_GROUPE | BIT(0), // only support LSPI_CK
    GPIO_PE1 = GPIO_GROUPE | BIT(1), // only support LSPI_MOSI
    GPIO_PE2 = GPIO_GROUPE | BIT(2), // only support LSPI_MISO
    GPIO_PE3 = GPIO_GROUPE | BIT(3), // only support LSPI_IO2
    GPIO_PE4 = GPIO_GROUPE | BIT(4), // only support LSPI_IO3
    GPIO_PE5 = GPIO_GROUPE | BIT(5), // only support LSPI_IO4
    GPIO_PE6 = GPIO_GROUPE | BIT(6), // only support LSPI_IO5
    GPIO_PE7 = GPIO_GROUPE | BIT(7), // only support LSPI_IO6
    GPIOE_ALL = GPIO_GROUPE | 0x00ff,

    GPIO_PF0 = GPIO_GROUPF | BIT(0), // only support LSPI_IO7
    GPIO_PF1 = GPIO_GROUPF | BIT(1), // only support LSPI_DM
    GPIO_PF2 = GPIO_GROUPF | BIT(2),
    GPIO_PF3 = GPIO_GROUPF | BIT(3),
    GPIO_PF4 = GPIO_GROUPF | BIT(4),
    GPIO_PF5 = GPIO_GROUPF | BIT(5),
    GPIO_PF6 = GPIO_GROUPF | BIT(6),
    GPIO_PF7 = GPIO_GROUPF | BIT(7),
    GPIOF_ALL = GPIO_GROUPF | 0x00ff,

    GPIO_PG0 = GPIO_GROUPG | BIT(0),
    GPIO_PG1 = GPIO_GROUPG | BIT(1),
    GPIO_PG2 = GPIO_GROUPG | BIT(2),
    GPIO_PG3 = GPIO_GROUPG | BIT(3),
    GPIO_PG4 = GPIO_GROUPG | BIT(4),
    GPIO_PG5 = GPIO_GROUPG | BIT(5),
    GPIO_PG6 = GPIO_GROUPG | BIT(6),
    GPIO_PG7 = GPIO_GROUPG | BIT(7),
    GPIOG_ALL = GPIO_GROUPG | 0x00ff,

    GPIO_PH0 = GPIO_GROUPH | BIT(0), // only support USB1_DM
    GPIO_PH1 = GPIO_GROUPH | BIT(1), // only support USB1_DP
    GPIO_PH2 = GPIO_GROUPH | BIT(2), // only support DM
    GPIO_PH3 = GPIO_GROUPH | BIT(3), // only support DP
    GPIO_PH4 = GPIO_GROUPH | BIT(4),
    GPIO_PH5 = GPIO_GROUPH | BIT(5),
    GPIO_PH6 = GPIO_GROUPH | BIT(6),
    GPIOH_ALL = GPIO_GROUPH | 0x007f,

    GPIO_PI0 = GPIO_GROUPI | BIT(0), // only support MSPI_MOSI(default)
    GPIO_PI1 = GPIO_GROUPI | BIT(1), // only support MSPI_CK(default)
    GPIO_PI2 = GPIO_GROUPI | BIT(2), // only support MSPI_IO3(default)
    GPIO_PI3 = GPIO_GROUPI | BIT(3), // only support MSPI_CN(default)
    GPIO_PI4 = GPIO_GROUPI | BIT(4), // only support MSPI_MISO(default)
    GPIO_PI5 = GPIO_GROUPI | BIT(5), // only support MSPI_IO2(default)
    GPIO_PI6 = GPIO_GROUPI | BIT(6), // only support MSPI_IO4(default)
    GPIO_PI7 = GPIO_GROUPI | BIT(7), // only support MSPI_IO5(default)
    GPIOI_ALL = GPIO_GROUPI | 0x00ff,

    GPIO_PJ0 = GPIO_GROUPJ | BIT(0), // only support MSPI_IO6(default)
    GPIO_PJ1 = GPIO_GROUPJ | BIT(1), // only support MSPI_IO7(default)
    GPIO_PJ2 = GPIO_GROUPJ | BIT(2), // default: MSPI_CN1
    GPIO_PJ3 = GPIO_GROUPJ | BIT(3), // default: MSPI_CN2
    GPIO_PJ4 = GPIO_GROUPJ | BIT(4), // default: MSPI_CN3
    GPIO_PJ5 = GPIO_GROUPJ | BIT(5), // default: MSPI_DM
    GPIOJ_ALL = GPIO_GROUPJ | 0x003f,

    GPIO_ANA0 = GPIO_GPOUPANA | BIT(0), // GPIO(default) no other functions
    GPIO_ANA1 = GPIO_GPOUPANA | BIT(1), // GPIO(default) no other functions
    GPIO_ANA_ALL = GPIO_GPOUPANA | 0x0003,
}gpio_pin_e;
/**
 *  @brief  Define GPIO function pin types.
 */
typedef enum{
    GPIO_FC_PA0 = GPIO_PA0 ,
    GPIO_FC_PA1 = GPIO_PA1,
    GPIO_FC_PA2 = GPIO_PA2,
    GPIO_FC_PA3 = GPIO_PA3,
    GPIO_FC_PA4 = GPIO_PA4,
    GPIO_FC_PA5 = GPIO_PA5,
    GPIO_FC_PA6 = GPIO_PA6,

    GPIO_FC_PB0 = GPIO_PB0,
    GPIO_FC_PB1 = GPIO_PB1,
    GPIO_FC_PB2 = GPIO_PB2,
    GPIO_FC_PB3 = GPIO_PB3,
    GPIO_FC_PB4 = GPIO_PB4,
    GPIO_FC_PB5 = GPIO_PB5,
    GPIO_FC_PB6 = GPIO_PB6,
    GPIO_FC_PB7 = GPIO_PB7,

    GPIO_FC_PD2 = GPIO_PD2,
    GPIO_FC_PD3 = GPIO_PD3,
    GPIO_FC_PD4 = GPIO_PD4,
    GPIO_FC_PD5 = GPIO_PD5,
    GPIO_FC_PD6 = GPIO_PD6,
    GPIO_FC_PD7 = GPIO_PD7,

    GPIO_FC_PF2 = GPIO_PF2,
    GPIO_FC_PF3 = GPIO_PF3,
    GPIO_FC_PF4 = GPIO_PF4,
    GPIO_FC_PF5 = GPIO_PF5,
    GPIO_FC_PF6 = GPIO_PF6,
    GPIO_FC_PF7 = GPIO_PF7,

    GPIO_FG_PG0 = GPIO_PG0,
    GPIO_FG_PG1 = GPIO_PG1,
    GPIO_FG_PG2 = GPIO_PG2,
    GPIO_FG_PG3 = GPIO_PG3,
    GPIO_FG_PG4 = GPIO_PG4,
    GPIO_FG_PG5 = GPIO_PG5,
    GPIO_FG_PG6 = GPIO_PG6,
    GPIO_FG_PG7 = GPIO_PG7,

    GPIO_FC_PH4 = GPIO_PH4,
    GPIO_FC_PH5 = GPIO_PH5,
    GPIO_FC_PH6 = GPIO_PH6,

    GPIO_FC_PJ2 = GPIO_PJ2,
    GPIO_FC_PJ3 = GPIO_PJ3,
    GPIO_FC_PJ4 = GPIO_PJ4,
    GPIO_FC_PJ5 = GPIO_PJ5,

    GPIO_NONE_PIN = 0x000,
}gpio_func_pin_e;

/**
 *  @brief  select pin as TCK of DSP_JTAG
 */
typedef enum{
    DSP_TCK_A0  = GPIO_PA0,
    DSP_TCK_A4  = GPIO_PA4,
    DSP_TCK_B1  = GPIO_PB1,
    DSP_TCK_B5  = GPIO_PB5,
    DSP_TCK_D3  = GPIO_PD3,
    DSP_TCK_D7  = GPIO_PD7,
    DSP_TCK_F5  = GPIO_PF5,
    DSP_TCK_G1  = GPIO_PG1,
    DSP_TCK_G5  = GPIO_PG5,
    DSP_TCK_H5  = GPIO_PH5,
    DSP_TCK_J2  = GPIO_PJ2,
}dsp_tck_pin_e;

/**
 *  @brief  select pin as TMS of DSP_JTAG
 */
typedef enum{
    DSP_TMS_A1  = GPIO_PA1,
    DSP_TMS_A5  = GPIO_PA5,
    DSP_TMS_B2  = GPIO_PB2,
    DSP_TMS_B6  = GPIO_PB6,
    DSP_TMS_D4  = GPIO_PD4,
    DSP_TMS_F2  = GPIO_PF2,
    DSP_TMS_F6  = GPIO_PF6,
    DSP_TMS_G2  = GPIO_PG2,
    DSP_TMS_G6  = GPIO_PG6,
    DSP_TMS_H6  = GPIO_PH6,
    DSP_TMS_J3  = GPIO_PJ3,
}dsp_tms_pin_e;

/**
 *  @brief  select pin as TDO of DSP_JTAG
 */
typedef enum{
    DSP_TDO_A2  = GPIO_PA2,
    DSP_TDO_A6  = GPIO_PA6,
    DSP_TDO_B3  = GPIO_PB3,
    DSP_TDO_B7  = GPIO_PB7,
    DSP_TDO_D5  = GPIO_PD5,
    DSP_TDO_F3  = GPIO_PF3,
    DSP_TDO_F7  = GPIO_PF7,
    DSP_TDO_G3  = GPIO_PG3,
    DSP_TDO_G7  = GPIO_PG7,
    DSP_TDO_J4  = GPIO_PJ4,
}dsp_tdo_pin_e;

/**
 *  @brief  select pin as TDI of DSP_JTAG
 */
typedef enum{
    DSP_TDI_A3  = GPIO_PA3,
    DSP_TDI_B0  = GPIO_PB0,
    DSP_TDI_B4  = GPIO_PB4,
    DSP_TDI_D2  = GPIO_PD2,
    DSP_TDI_D6  = GPIO_PD6,
    DSP_TDI_F4  = GPIO_PF4,
    DSP_TDI_G0  = GPIO_PG0,
    DSP_TDI_G4  = GPIO_PG4,
    DSP_TDI_H4  = GPIO_PH4,
    DSP_TDI_J5  = GPIO_PJ5,
}dsp_tdi_pin_e;

/**
 *  @brief  select pin as DSP_JTAG
 */
typedef struct{
    dsp_tck_pin_e tck;
    dsp_tms_pin_e tms;
    dsp_tdo_pin_e tdo;
    dsp_tdi_pin_e tdi;
}dsp_jtag_pin_st;

/**
 *  @brief  Define GPIO function mux types
 */
typedef enum{
    PWM0           = 1,
    PWM1           = 1,
    PWM2           = 1,
    PWM3           = 1,
    PWM4           = 1,
    PWM5           = 1,
    PWM0_N         = 2,
    PWM1_N         = 2,
    PWM2_N         = 2,
    PWM3_N         = 2,
    PWM4_N         = 2,
    PWM5_N         = 2,
    GSPI_CSN0_IO   = 3,
    GSPI_CLK_IO    = 3,
    GSPI_MOSI_IO   = 3,
    GSPI_MISO_IO   = 3,
    GSPI_IO2_IO    = 3,
    GSPI_IO3_IO    = 3,
    GSPI_IO4_IO    = 3,
    GSPI_IO5_IO    = 3,
    GSPI_IO6_IO    = 3,
    GSPI_IO7_IO    = 3,
    GSPI_CN1       = 3,
    GSPI_CN2       = 3,
    GSPI_DM_IO     = 3,
    GSPI_CN3       = 3,
    GSPI_CN0_IO    = 3,
    I2C_SCL_IO     = 4,
    I2C_SDA_IO     = 4,
    I2C1_SCL_IO    = 5,
    I2C1_SDA_IO    = 5,
    UART0_CTS_I    = 6,
    UART0_RTS      = 6,
    UART0_TX       = 6,
    UART0_RTX_IO   = 6,
    UART1_CTS_I    = 7,
    UART1_RTS      = 7,
    UART1_TX       = 7,
    UART1_RTX      = 7,
    UART2_CTS_I    = 8,
    UART2_RTS      = 8,
    UART2_TX       = 8,
    UART2_RTX_IO   = 8,
    UART3_CTS_I    = 9,
    UART3_RTS      = 9,
    UART3_TX       = 9,
    UART3_RTX_IO   = 9,
    DMIC0_CLK      = 10, // non-existent
    DMIC0_CLK0     = 10,
    DMIC0_CLK1     = 10,
    DMIC0_DAT_I    = 10,
    DMIC1_CLK      = 11, // non-existent
    DMIC1_CLK0     = 11,
    DMIC1_CLK1     = 11,
    DMIC1_DAT_I    = 11,
    DMIC2_CLK0     = 12,
    DMIC2_CLK1     = 12,
    DMIC2_DAT_I    = 12,
    SPDIF_TX       = 13,
    SPDIF_RX_I     = 13,
    SSPI_CK_I      = 14,
    SSPI_SO_IO     = 14,
    SSPI_CN_I      = 14,
    SSPI_SI_IO     = 14,
    DSP_TCK_I      = 15,
    DSP_TMS_I      = 15,
    DSP_TDI_I      = 15,
    DSP_TDO_IO     = 15,
    SWM_IO         = 16,
    CLK_7816       = 16,
    LSPI_CN_IO     = 16,
    DBG_PROBE_CLK  = 16,
    ATSEL_0        = 17,
    ATSEL_1        = 17,
    ATSEL_2        = 17,
    ATSEL_3        = 17,
    RX_CYC2LNA     = 18,
    TX_CYC2PA      = 18,
    BT_INBAND      = 19,
    BT_STATUS      = 19,
    BT_ACTIVITY    = 19,
    WIFI_DENY_I    = 19,
    EMMC_RSTN      = 20,
    EMMC_WP_I      = 20,
    EMMC_CDN_I     = 20,
    EMMC_DS_I      = 20,
    I2S0_BCK_IO    = 21,
    I2S0_LR0_IO    = 21,
    I2S0_DAT0_IO   = 21,
    I2S0_LR1_IO    = 21,
    I2S0_DAT1_IO   = 21,
    I2S1_BCK_IO    = 22,
    I2S1_LR0_IO    = 22,
    I2S1_DAT0_IO   = 22,
    I2S1_LR1_IO    = 22,
    I2S1_DAT1_IO   = 22,
    I2S2_BCK_IO    = 23,
    I2S2_LR0_IO    = 23,
    I2S2_DAT0_IO   = 23,
    I2S2_LR1_IO    = 23,
    I2S2_DAT1_IO   = 23,
    I2S0_CLK       = 24,
    I2S1_CLK       = 24,
    I2S2_CLK       = 24,

/*************** non-existent **********************/
    I2S0_LR_OUT_IO = 21,
    I2S0_DAT_OUT   = 21,
    I2S0_LR_IN_IO  = 21,
    I2S0_DAT_IN_I  = 21,
    GSPI_CSN1      = 3,
    GSPI_CSN2      = 3,
    GSPI_CSN3      = 3,
}gpio_func_e;

/**
 *  @brief  Define GPIO mux func
 */
typedef enum{
    AS_GPIO,
    AS_SSPI_SI,
    AS_SSPI_CN,
    AS_SSPI_CK,
    AS_SSPI_SO,
    AS_SWS,
    AS_TCK,
    AS_TMS,
    AS_TDO,
    AS_TDI,
    AS_MSPI_MOSI,
    AS_MSPI_CK,
    AS_MSPI_IO3,
    AS_MSPI_CN,
    AS_MSPI_MISO,
    AS_MSPI_IO2,
    AS_MSPI_IO4,
    AS_MSPI_IO5,
    AS_MSPI_IO6,
    AS_MSPI_IO7,
    AS_MSPI_CN1,
    AS_MSPI_CN2,
    AS_MSPI_CN3,
    AS_MSPI_DM,
}gpio_fuc_e;

/*
 * @brief define gpio irq status types
 */
typedef enum{
    FLD_GPIO_IRQ_CLR            = BIT(0),
    FLD_GPIO_IRQ_GPIO2RISC0_CLR = BIT(1),
    FLD_GPIO_IRQ_GPIO2RISC1_CLR = BIT(2),
}gpio_irq_status_e;


/*
 *  @brief define gpio irq mask types
 */
typedef enum{
    GPIO_IRQ_MASK_GPIO       = BIT(2),
    GPIO_IRQ_MASK_GPIO2RISC0 = BIT(3),
    GPIO_IRQ_MASK_GPIO2RISC1 = BIT(4),
}gpio_irq_mask_e;

/*
 * @brief define gpio group irq types
 */
typedef enum{
    FLD_GPIO_GROUP_IRQ0 = BIT(0),
    FLD_GPIO_GROUP_IRQ1 = BIT(1),
    FLD_GPIO_GROUP_IRQ2 = BIT(2),
    FLD_GPIO_GROUP_IRQ3 = BIT(3),
    FLD_GPIO_GROUP_IRQ4 = BIT(4),
    FLD_GPIO_GROUP_IRQ5 = BIT(5),
    FLD_GPIO_GROUP_IRQ6 = BIT(6),
    FLD_GPIO_GROUP_IRQ7 = BIT(7),
}gpio_group_irq_e;

/**
 *  @brief  Define rising/falling types
 */
typedef enum{
    POL_RISING   = 0,
    POL_FALLING  = 1,
}gpio_pol_e;

/**
 *  @brief  Define interrupt types
 */
typedef enum{
    INTR_RISING_EDGE=0,
    INTR_FALLING_EDGE ,
    INTR_HIGH_LEVEL,
    INTR_LOW_LEVEL,
} gpio_irq_trigger_type_e;

/**
 *  @brief  Define pull up or down types
 */
typedef enum {
    GPIO_PIN_UP_DOWN_FLOAT = 0,
    GPIO_PIN_PULLUP_1M     = 1,
    GPIO_PIN_PULLDOWN_100K = 2,
    GPIO_PIN_PULLUP_10K    = 3,
}gpio_pull_type_e;

typedef enum{
    PROBE_CLK32K            = 0,
    PROBE_RC24M             = 1,
    PROBE_PLL0              = 2,
    PROBE_PLL1              = 3,
    PROBE_XTL48M            = 4,
    PROBE_HCLK              = 5,
    PROBE_PCLK              = 6,
    PROBE_CLK_DSP           = 7,
    PROBE_CLK_MSPI          = 8,
    PROBE_CLK_NPE           = 9,
    PROBE_CLK_N22           = 10,
    PROBE_CLK_LSPI          = 11,
    PROBE_CLK_GSPI          = 12,
    PROBE_CLK_SDIO          = 13,
    PROBE_CLK_STIMER        = 14,
    PROBE_CLK_USBPHY        = 15,
    PROBE_CLK_USBPHY1       = 16,
    PROBE_CLK_7816          = 17,
    PROBE_CLK_ZB_MST        = 18,
    PROBE_DBG_CLK           = 19,
    PROBE_CLK_ACLK_DBG      = 20,
    PROBE_CLK_WT            = 21,
    PROBE_CLK_XO_EXIT       = 22,
}probe_clk_sel_e;
/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/**
 * @brief      This function servers to enable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
static inline void gpio_function_en(gpio_pin_e pin)
{
    BM_SET(reg_gpio_func(pin), pin & 0xff);
}

/**
 * @brief      This function servers to disable gpio function.
 * @param[in]  pin - the selected pin.
 * @return     none.
 */
static inline void gpio_function_dis(gpio_pin_e pin)
{
    BM_CLR(reg_gpio_func(pin), pin & 0xff);
}

/**
 * @brief     This function set the pin's output high level.
 * @param[in] pin - the pin needs to set its output level.
 * @return    none.
 */
static inline void gpio_set_high_level(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13f,(analog_read_reg8(0x13f))|bit);
    }
    else{
        reg_gpio_out_set(pin) = bit;
    }
}

/**
 * @brief     This function set the pin's output low level.
 * @param[in] pin - the pin needs to set its output level.
 * @return    none.
 */
static inline void gpio_set_low_level(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x140,(analog_read_reg8(0x140))|bit);
    }
    else{
        reg_gpio_out_clear(pin) = bit;
    }
}

/**
 * @brief     This function set the pin's output level.
 * @param[in] pin - the pin needs to set its output level
 * @param[in] value - value of the output level(1: high 0: low)
 * @return    none
 */
static inline void gpio_set_level(gpio_pin_e pin, unsigned char value)
{
    if(value)
    {
        gpio_set_high_level(pin);
    }
    else
    {
        gpio_set_low_level(pin);
    }
}

/**
 * @brief     This function read the pin's input level.
 * @param[in] pin - the pin needs to read its input level.
 * @return    1: the pin's input level is high.
 *            0: the pin's input level is low.
 */
static inline _Bool gpio_get_level(gpio_pin_e pin)
{
    return BM_IS_SET(reg_gpio_in(pin), pin & 0xff);
}

/**
 * @brief      This function read all the pins' input level.
 * @param[out] p - the buffer used to store all the pins' input level
 * @return     none
 */
static inline void gpio_get_level_all(unsigned char *p)
{
    p[0] = reg_gpio_pa_in;
    p[1] = reg_gpio_pb_in;
    p[2] = reg_gpio_pc_in;
    p[3] = reg_gpio_pd_in;
    p[4] = reg_gpio_pe_in;
    p[5] = reg_gpio_pf_in;
    p[6] = reg_gpio_pg_in;
    p[7] = reg_gpio_ph_in;
    p[8] = reg_gpio_pi_in;
    p[9] = reg_gpio_pj_in;
    p[10] = reg_gpio_pana_in;
}

/**
 * @brief     This function set the pin toggle.
 * @param[in] pin - the pin needs to toggle.
 * @return    none.
 */
static inline void gpio_toggle(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x141,(analog_read_reg8(0x141))|bit);
    }
    else{
        reg_gpio_out_toggle(pin) = bit;
    }
}
/**
 * @brief      This function enable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function.
 * @return     none.
 */
static inline void gpio_output_en(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))&(~(bit<<2)));
    }
    else{
        BM_CLR(reg_gpio_oen(pin), bit);
    }
}

/**
 * @brief      This function disable the output function of a pin.
 * @param[in]  pin - the pin needs to set the output function.
 * @return     none.
 */
static inline void gpio_output_dis(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        analog_write_reg8(0x13d,(analog_read_reg8(0x13d))|(bit<<2));
    }
    else{
        BM_SET(reg_gpio_oen(pin), bit);
    }
}

/**
 * @brief      This function enable set output function of a pin.
 * @param[in]  pin - the pin needs to set the output function (1: enable,0: disable)
 * @return     none
 */
static inline void gpio_set_output(gpio_pin_e pin, unsigned char value)
{
    if(value)
    {
        gpio_output_en(pin);
    }
    else
    {
        gpio_output_dis(pin);
    }
}

/**
 * @brief      This function determines whether the output function of a pin is enabled.
 * @param[in]  pin - the pin needs to determine whether its output function is enabled.
 * @return     1: the pin's output function is enabled.
 *             0: the pin's output function is disabled.
 */
static inline _Bool  gpio_is_output_en(gpio_pin_e pin)
{
    unsigned char bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        return !BM_IS_SET(analog_read_reg8(0x13d),(bit<<2));
    }
    else{
        return !BM_IS_SET(reg_gpio_oen(pin), bit);
    }
}

/**
 * @brief     This function determines whether the input function of a pin is enabled.
 * @param[in] pin - the pin needs to determine whether its input function is enabled(not include group_pc).
 * @return    1: the pin's input function is enabled.
 *            0: the pin's input function is disabled.
 */
static inline _Bool gpio_is_input_en(gpio_pin_e pin)
{
    unsigned char   bit = pin & 0xff;
    if(((pin>>8)&0xff) == GPIO_GROUP_ANA){
        return BM_IS_SET(analog_read_reg8(0x13d),bit);
    }
    else{
        return BM_IS_SET(reg_gpio_ie(pin), bit);
    }
}

/**
 * @brief      This function serves to enable gpio irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_irq_en(gpio_pin_e pin)
{
    BM_SET(reg_gpio_irq_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to disable gpio irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_irq_dis(gpio_pin_e pin)
{
    BM_CLR(reg_gpio_irq_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to enable gpio risc0 irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc0_irq_en(gpio_pin_e pin)
{
    BM_SET(reg_gpio_irq_risc0_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to disable gpio risc0 irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc0_irq_dis(gpio_pin_e pin)
{
    BM_CLR(reg_gpio_irq_risc0_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to enable gpio risc1 irq function.
 * @param[in]  pin  - the pin needs to enable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc1_irq_en(gpio_pin_e pin)
{
    BM_SET(reg_gpio_irq_risc1_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to disable gpio risc1 irq function.
 * @param[in]  pin  - the pin needs to disable its IRQ.
 * @return     none.
 */
static inline void gpio_gpio2risc1_irq_dis(gpio_pin_e pin)
{
    BM_CLR(reg_gpio_irq_risc1_en(pin), pin & 0xff);
}

/**
 * @brief      This function serves to clr gpio irq status.
 * @param[in]  status  - the irq need to clear.
 * @return     none.
 */
static inline void gpio_clr_irq_status(gpio_irq_status_e status)
{
    reg_gpio_irq_clr = status;
}

/**
 * @brief      This function serves to clr gpio group irq status.
 * @param[in]  status  - the irq need to clear.
 * @return     none.
 */
static inline void gpio_clr_group_irq_status(gpio_group_irq_e status)
{
    reg_gpio_irq_src = status;
}

/**
 * @brief      This function serves to enable gpio irq mask function.
 * @param[in]  mask  - to select interrupt type.
 * @return     none.
 */
static inline void gpio_set_irq_mask(gpio_irq_mask_e mask)
{
    BM_SET(reg_gpio_irq_ctrl, mask);
}

/**
 * @brief      This function serves to enable gpio irq group mask function.
 * @param[in]  mask  - to select interrupt type.
 * @return     none.
 */
static inline void gpio_set_group_irq_mask(gpio_group_irq_e mask)
{
   BM_SET(reg_gpio_irq_src_mask, mask);
}

/**
 * @brief      This function serves to disable gpio irq mask function.
 *             if disable gpio interrupt,choose disable gpio mask , use interface gpio_clr_irq_mask instead of gpio_irq_dis/gpio_gpio2risc0_irq_dis/gpio_gpio2risc1_irq_dis.
 * @return     none.
 */
static inline void gpio_clr_irq_mask(gpio_irq_mask_e mask)
{
     BM_CLR(reg_gpio_irq_ctrl, mask);
}

/**
 * @brief      This function serves to disable gpio irq group mask function.
 *             if disable gpio src irq interrupt,can choose this function disable gpio group irq mask.
 * @return     none.
 */
static inline void gpio_clr_group_irq_mask(gpio_group_irq_e mask)
{
    BM_CLR(reg_gpio_irq_src_mask, mask);
}

/**
 * @brief      This function set the pin's driving strength at strong.
 * @param[in]  pin - the pin needs to set the driving strength.
 * @return     none.
 */
void gpio_ds_en(gpio_pin_e pin);

/**
 * @brief      This function set the pin's driving strength.
 * @param[in]  pin - the pin needs to set the driving strength at poor.
 * @return     none.
 */
void gpio_ds_dis(gpio_pin_e pin);

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type.
 *                            0: rising edge.
 *                            1: falling edge.
 *                            2: high level.
 *                            3: low level.
 * @return    none.
 */
void gpio_set_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC0.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none.
 */
void gpio_set_gpio2risc0_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ_RISC1.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type 0  rising edge 1 falling edge 2 high level 3 low level
 * @return    none.
 */
void gpio_set_gpio2risc1_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);

/**
 * @brief     This function set a pin's IRQ.
 * @param[in] pin           - the pin needs to enable its IRQ.
 * @param[in] trigger_type  - gpio interrupt type.
 *                            0: rising edge.
 *                            1: falling edge.
 *                            2: high level.
 *                            3: low level
 * @note      if you want to use this irq,you should select irq_group first,which correspond to the function "gpio_set_src_irq_group()".
 * @return    none.
 */
void gpio_set_src_irq(gpio_pin_e pin, gpio_irq_trigger_type_e trigger_type);
/**
 * @brief      This function serves to set the gpio-mux function.
 * @param[in]  pin      - the pin needs to set.
 * @param[in]  function - the function need to set.
 * @return     none.
 */
void gpio_set_mux_function(gpio_func_pin_e pin,gpio_func_e function);

/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_en(gpio_pin_e pin);

/**
 * @brief      This function disable the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function.
 * @return     none.
 */
void gpio_input_dis(gpio_pin_e pin);

/**
 * @brief      This function set the input function of a pin.
 * @param[in]  pin - the pin needs to set the input function
 * @param[in]  value - enable or disable the pin's input function(1: enable,0: disable )
 * @return     none
 */
void gpio_set_input(gpio_pin_e pin, unsigned char value);

/**
 * @brief      This function servers to set the specified GPIO as high resistor.
 * @param[in]  pin  - select the specified GPIO, GPIOI GPIOJ group is not included in GPIO_ALL.
 * @return     none.
 * @note       -# gpio_shutdown(GPIO_ALL) is a debugging method only and is not recommended for use in applications.
 *             -# gpio_shutdown(GPIO_ALL) set all GPIOs to high impedance except SWS and MSPI.
 *             -# If you want to use JTAG/USB in active state, or wake up the MCU with a specific pin,
 *                you can enable the corresponding pin after calling gpio_shutdown(GPIO_ALL).
 */
void gpio_shutdown(gpio_pin_e pin);

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] pin - the pin needs to set its pull-up/down resistor.
 * @param[in] up_down_res - the type of the pull-up/down resistor.
 * @return    none.
 */
void gpio_set_up_down_res(gpio_pin_e pin, gpio_pull_type_e up_down_res);

/**
 * @brief     This function set pin's  pull-down register.
 * @param[in] pin - the pin needs to set its pull-down register.
 * @return    none.
 * @attention  This function sets the digital pull-down, it will not work after entering low power consumption.
 */
void gpio_set_digital_pulldown(gpio_pin_e pin);

/**
 * @brief     This function set pin's  pull-up register.
 * @param[in] pin - the pin needs to set its pull-up register.
 * @return    none.
 * @attention  This function sets the digital pull-up, it will not work after entering low power consumption.
 */
void gpio_set_digital_pullup(gpio_pin_e pin);

/**
 * @brief     This function disable pin's  pull-down register.
 * @param[in] pin - the pin needs to disable its pull-down register.
 * @return    none.
 */
void gpio_digital_pulldown_dis(gpio_pin_e pin);

/**
 * @brief     This function disable pin's  pull-up register.
 * @param[in] pin - the pin needs to disable its pull-up register.
 * @return    none.
 */
void gpio_digital_pullup_dis(gpio_pin_e pin);

/**
 * @brief     This function set the pin as JTAG function for DSP.
 * @param[in] pin - the pin needs to set to JTAG function.
 * @return    none.
 */
void dsp_jtag_set_pin(gpio_pin_e pin);

/**
 * @brief     This function is used to enable the JTAG function of the DSP.
 * @param[in] dsp_jtag_pin - DSP signal line structure.
 * @return    none.
 */
void dsp_jtag_set_pin_en(dsp_jtag_pin_st *dsp_jtag_pin);

/**
 * @brief     This function is used to set JTAG or SDP function for d25f and n22 core.
 * @param[in] pin
 * @return    none.
 */
void jtag_sdp_set_pin(gpio_pin_e pin);

/**
 * @brief     This function serves to set JTAG(4 wires) pin for d25f and n22 core. Where, PB[4]; PB[3]; PB[2]; PB[1] correspond to TDI; TDO; TMS; TCK functions mux respectively.
 * @param[in] none
 * @return    none.
 * @note      Power-on or hardware reset will detect the level of PB6 (reboot will not detect it), detecting a low level is configured as JTAG,
               detecting a high level is configured as SDP.  the level of PB6 can not be configured internally by the software, and can only be input externally.
 */
void jtag_set_pin_en(void);
/**
 * @brief     This function serves to set SDP(2 wires) pin for d25f and n22 core. where, PB[2]; PB[1] correspond to TMS and TCK functions mux respectively.
 * @param[in] none
 * @return    none.
 * @note      Power-on or hardware reset will detect the level of PB6 (reboot will not detect it), detecting a low level is configured as JTAG,
               detecting a high level is configured as SDP.  the level of PB6 can not be configured internally by the software, and can only be input externally.
 */
void sdp_set_pin_en(void);

/**
 * @brief     This function set probe clk output.
 * @param[in] pin
 * @param[in] sel_clk
 * @return    none.
 */
void  gpio_set_probe_clk_function(gpio_func_pin_e pin,probe_clk_sel_e sel_clk);

/**
 * @brief     This function select the irq group source.
 * @param[in] group - gpio irq group,include group A,B,C,D,E,F.
 * @note      after you choose the gpio_group,you should set the pin's irq one by one,which correspond to the function "gpio_set_src_irq()".
 * @return    none.
 */
static inline void gpio_set_src_irq_group(gpio_group_e group)
{
    reg_gpio_irq_sel &= (~group);
    reg_gpio_irq_sel |= group;
}

#endif

