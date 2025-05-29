/********************************************************************************************************
 * @file    soc.h
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
#pragma once

#include "bit.h"

/*Attention: for all the 2-byte address operation (like REG_ADDR16,write_reg16,read_reg16) ,
 *           the address parameter should  be multiple of 2. For example:REG_ADDR16(0x2),REG_ADDR16(0x4).
 *
 *           for all the 4-byte address operation (like REG_ADDR32,write_reg32,read_reg32) ,
 *           the address parameter should  be multiple of 4. For example:write_reg32(0x4),write_reg32(0x8).
*/
#define FLASH_R_BASE_ADDR           0x20000000
#define REG_RW_BASE_ADDR            0x80000000
#define REG_ADDR8(a)                (*(volatile unsigned char*)(REG_RW_BASE_ADDR | (a)))
#define REG_ADDR16(a)               (*(volatile unsigned short*)(REG_RW_BASE_ADDR | (a)))
#define REG_ADDR32(a)               (*(volatile unsigned long*)(REG_RW_BASE_ADDR | (a)))

#define write_reg8(addr,v)          (*(volatile unsigned char*)(REG_RW_BASE_ADDR | (addr)) = (unsigned char)(v))
#define write_reg16(addr,v)         (*(volatile unsigned short*)(REG_RW_BASE_ADDR | (addr)) = (unsigned short)(v))
#define write_reg32(addr,v)         (*(volatile unsigned long*)(REG_RW_BASE_ADDR | (addr)) = (unsigned long)(v))

#define read_reg8(addr)             (*(volatile unsigned char*)(REG_RW_BASE_ADDR | (addr)))
#define read_reg16(addr)            (*(volatile unsigned short*)(REG_RW_BASE_ADDR | (addr)))
#define read_reg32(addr)            (*(volatile unsigned long*)(REG_RW_BASE_ADDR | (addr)))

#define write_sram8(addr,v)         (*(volatile unsigned char*)( (addr)) = (unsigned char)(v))
#define write_sram16(addr,v)        (*(volatile unsigned short*)( (addr)) = (unsigned short)(v))
#define write_sram32(addr,v)        (*(volatile unsigned long*)( (addr)) = (unsigned long)(v))

#define read_sram8(addr)            (*(volatile unsigned char*)((addr)))
#define read_sram16(addr)           (*(volatile unsigned short*)((addr)))
#define read_sram32(addr)           (*(volatile unsigned long*)((addr)))
#define TCMD_UNDER_BOTH             0xc0
#define TCMD_UNDER_RD               0x80
#define TCMD_UNDER_WR               0x40

#define TCMD_MASK                   0x3f

#define TCMD_WRITE                  0x3
#define TCMD_WAIT                   0x7
#define TCMD_WAREG                  0x8

#define D25_ILM_BASE                (0x20000)
#define D25_DLM_BASE                (0x80000)

#define SC_BASE_ADDR                0x80140800

#define reg_mspi_clk_set            REG_ADDR8(SC_BASE_ADDR + 0x00)
enum{
    FLD_MSPI_CLK_MOD        =   BIT_RNG(0, 3),
    FLD_MSPI_DIV_IN_SEL     =   BIT_RNG(4, 5),//0:rc24m 1:xtl48m 2:pll0 3:pll1
};

#define reg_lspi_clk_set            REG_ADDR8(SC_BASE_ADDR + 0x01)
enum{
    FLD_LSPI_CLK_MOD        =   BIT_RNG(0,3),
    FLD_LSPI_DIV_IN_SEL     =   BIT_RNG(4,5),
};

#define reg_gspi_clk_set            REG_ADDR16(SC_BASE_ADDR + 0x02)
enum{
    FLD_GSPI_CLK_MOD        =   BIT_RNG(0,7),
    FLD_GSPI_DIV_IN_SEL     =   BIT_RNG(8,9),
};

#define reg_sdio_clk_set            REG_ADDR8(SC_BASE_ADDR + 0x04)
enum{
    FLD_SDIO_CLK_MOD        =   BIT_RNG(0, 3),
    FLD_SDIO_DIV_IN_SEL     =   BIT_RNG(4, 5),//0:rc24m 1:xtl48m 2:pll0 3:pll1
};

#define reg_wt_clk_set          REG_ADDR8(SC_BASE_ADDR + 0x05)
enum{
    FLD_WT_CLK_MOD          =   BIT_RNG(0, 3),
};

#define reg_efuse_b             REG_ADDR32(SC_BASE_ADDR + 0x0c)

#define reg_cache_init          REG_ADDR8(SC_BASE_ADDR + 0x10)
enum{
    FLD_ICACHE_DISABLE_INIT =   BIT(6),
    FLD_DCACHE_DISABLE_INIT =   BIT(7),
};

#define reg_eoc_ts_value        REG_ADDR8(SC_BASE_ADDR + 0x11)

#define reg_mcu_ctrl            REG_ADDR8(SC_BASE_ADDR + 0x14)
enum{
    FLD_MCU_REBOOT          = BIT(7),
};

#define reg_busclk_ratio        REG_ADDR8(SC_BASE_ADDR + 0x18)//[1:0] 0: hclk is the same as pclk, 1:hclk is 2 times of pclk, 2: hclk is 4 times of pclk 

#define reg_probe_clk_sel       REG_ADDR8(SC_BASE_ADDR + 0x1a)

#define reg_rst                 REG_ADDR32(SC_BASE_ADDR + 0x20)
#define reg_rst0                REG_ADDR8(SC_BASE_ADDR + 0x20)
enum{
    FLD_RST0_LSPI           =   BIT(0),
    FLD_RST0_I2C            =   BIT(1),
    FLD_RST0_UART0          =   BIT(2),
    FLD_RST0_USB            =   BIT(3),
    FLD_RST0_PWM            =   BIT(4),
    //RSVD
    FLD_RST0_UART1          =   BIT(6),
    FLD_RST0_SWIRE          =   BIT(7),
};


#define reg_rst1                    REG_ADDR8(SC_BASE_ADDR + 0x21)
enum{
    FLD_RST1_CHG            =   BIT(0),
    FLD_RST1_SYS_STIMER     =   BIT(1),
    FLD_RST1_DMA            =   BIT(2),
    FLD_RST1_ALGM           =   BIT(3),//ALGM stands for writing the analog register on the master side of a module,alg is the slave side.
    FLD_RST1_PKE            =   BIT(4),
    //RSVD
    FLD_RST1_GSPI           =   BIT(6),
    FLD_RST1_SPISLV         =   BIT(7),
};

/**
 * (rst2[3] | rst_n) & rst2[4] = d25f reset signal (reset mcu if 0).
 * If rst2[4] is 0, mcu is reset. rst2[3] is whether d25f is reset when suspend is set. In fact, this bit is always 1, that is, the mcu is not reset when suspend is set.
 * rst_n here is a hardware control, which is 1 when active and 0 when suspend.
 */
#define reg_rst2                    REG_ADDR8(SC_BASE_ADDR + 0x22)
enum{
    FLD_RST2_TIMER           =  BIT(0),
    FLD_RST2_AUD             =  BIT(1),
    FLD_RST2_I2C1            =  BIT(2),
    FLD_RST2_REST_MCU_DIS    =  BIT(3),
    FLD_RST2_MCU_REST_ENABLE =  BIT(4),
    FLD_RST2_LM              =  BIT(5),//LM stands for local memory, DSRAM, IRAM
    FLD_RST2_TRNG            =  BIT(6),
    FLD_RST2_DPR             =  BIT(7),//DPR is a module that can automatically adjust power consumption and is not currently in use.
};

#define reg_rst3                 REG_ADDR8(SC_BASE_ADDR + 0x23)
enum{
    FLD_RST3_DMA1           =   BIT(0),
    FLD_RST3_TRACE          =   BIT(1),
    FLD_RST3_BROM           =   BIT(2),
    //RSVD
    FLD_RST3_MSPI           =   BIT(4),
    FLD_RST3_QDEC           =   BIT(5),
    FLD_RST3_SARADC         =   BIT(6),
    FLD_RST3_ALG            =   BIT(7),
};

#define reg_clk_en                  REG_ADDR32(SC_BASE_ADDR + 0x24)

#define reg_clk_en0                 REG_ADDR8(SC_BASE_ADDR + 0x24)
enum{
    FLD_CLK0_LSPI_EN        =   BIT(0),
    FLD_CLK0_I2C_EN         =   BIT(1),
    FLD_CLK0_UART0_EN       =   BIT(2),
    FLD_CLK0_USB_EN         =   BIT(3),
    FLD_CLK0_PWM_EN         =   BIT(4),
    FLD_CLK0_DBGEN          =   BIT(5),//The dbgen module, if the communication port swire is disabled in efuse, can be activated by a transaction
    FLD_CLK0_UART1_EN       =   BIT(6),
    FLD_CLK0_SWIRE_EN       =   BIT(7),
};

#define reg_clk_en1                 REG_ADDR8(SC_BASE_ADDR + 0x25)
enum{
    FLD_CLK1_CHG_EN         =   BIT(0),
    FLD_CLK1_SYS_STIMER_EN  =   BIT(1),
    FLD_CLK1_DMA_EN         =   BIT(2),
    FLD_CLK1_ALGM_EN        =   BIT(3),
    FLD_CLK1_PKE_EN         =   BIT(4),
    FLD_CLK1_MACHINETIME    =   BIT(5),
    FLD_CLK1_GSPI_EN        =   BIT(6),
    FLD_CLK1_SPISLV_EN      =   BIT(7),

};

#define reg_clk_en2                 REG_ADDR8(SC_BASE_ADDR + 0x26)
enum{
    FLD_CLK2_TIMER_EN       =   BIT(0),
    FLD_CLK2_AUD_EN         =   BIT(1),
    FLD_CLK2_I2C1_EN        =   BIT(2),
    //RSVD
    FLD_CLK2_MCU_EN         =   BIT(4),
    FLD_CLK2_LM_EN          =   BIT(5),
    FLD_CLK2_TRNG_EN        =   BIT(6),
    FLD_CLK2_DPR_EN         =   BIT(7),
};

#define reg_clk_en3                 REG_ADDR8(SC_BASE_ADDR + 0x27)
enum{
    FLD_CLK3_DMA1_EN        =   BIT(0),
    FLD_CLK3_TRACE_EN       =   BIT(1),//TRACE: Previously, you could only read registers in core, such as PCS, through jtag. Later, we made this module, which can use swire to read some core registers.
    FLD_CLK3_BROM_EN        =   BIT(2),
    FLD_CLK3_WT_EN          =   BIT(3),
    FLD_CLK3_MSPI_EN        =   BIT(4),
    FLD_CLK3_QDEC_EN        =   BIT(5),
    FLD_CLK3_SARADC_EN      =   BIT(6),
    //RSVD
};

#define reg_hclk_sel                REG_ADDR8(SC_BASE_ADDR + 0x28)
enum{
    FLD_CLK_SCLK_DIV        =   BIT_RNG(0, 3),
    FLD_CLK_SCLK_SEL        =   BIT_RNG(4, 5),//0:rc24m 1:xtl48m 2:pll0 3:pll1
};

#define reg_wakeup_en               REG_ADDR8(SC_BASE_ADDR + 0x2e)
enum{
    FLD_USB_PWDN_I          =   BIT(0),
    FLD_GPIO_WAKEUP_I       =   BIT(1),
    FLD_QDEC_WAKEUP_I       =   BIT(2),
    FLD_USB1_PWDN_I         =   BIT(3),
    FLD_USB_RESUME          =   BIT(4),
    FLD_STANDBY_EX          =   BIT(5),
    FLD_USB1_RESUME         =   BIT(6),
};

#define reg_pwdn_en                 REG_ADDR8(SC_BASE_ADDR + 0x2f)
enum{
    FLD_SUSPEND_EN_O        =   BIT(0),
    FLD_RAMCRC_CLREN_TGL    =   BIT(4),
    FLD_RST_ALL             =   BIT(5),
    FLD_STALL_EN_TRG        =   BIT(7),
};

#define reg_rst_1                REG_ADDR32(SC_BASE_ADDR + 0x40)
#define reg_rst4                 REG_ADDR8(SC_BASE_ADDR + 0x40)
enum{
    FLD_RST4_DSP            =   BIT(0),
    FLD_RST4_SDIO           =   BIT(1),
    
    FLD_RST4_NPE            =   BIT(3),
    FLD_RST4_SKE            =   BIT(4),
    FLD_RST4_HASH           =   BIT(5),

    FLD_RST4_AHB1           =   BIT(7),//AHB1 is the bus for the subsystem of N22
};

#define reg_rst5                 REG_ADDR8(SC_BASE_ADDR + 0x41)
enum{
    FLD_RST5_USB1           =   BIT(0),
    FLD_RST5_UART2          =   BIT(1),
    FLD_RST5_UART3          =   BIT(2),
    FLD_RST5_MAILBOX        =   BIT(3),
};

#define reg_clk_en_1            REG_ADDR32(SC_BASE_ADDR + 0x44)
#define reg_clk_en4             REG_ADDR8(SC_BASE_ADDR + 0x44)
enum{
    FLD_CLK4_DSP_EN         =   BIT(0),
    FLD_CLK4_SDIO_EN        =   BIT(1),
    
    FLD_CLK4_NPE_EN         =   BIT(3),
    FLD_CLK4_SKE_EN         =   BIT(4),
    FLD_CLK4_HASH_EN        =   BIT(5),
    FLD_CLK4_HCLK_EN        =   BIT(6),
    FLD_CLK4_HCLK1_EN       =   BIT(7),//HCLK1 is the clk of the N22 subsystem
};

#define reg_clk_en5             REG_ADDR8(SC_BASE_ADDR + 0x45)
enum{
    FLD_CLK5_USB1_EN         =  BIT(0),
    FLD_CLK5_UART2_EN        =  BIT(1),
    FLD_CLK5_UART3_EN        =  BIT(2),
    FLD_CLK5_MAILBOX_EN      =  BIT(3),
};

#define reg_clk_dsp_set         REG_ADDR8(SC_BASE_ADDR + 0x55)
enum{
    FLD_CLK_DSP_MOD             =   BIT_RNG(0, 3),
    FLD_CLK_DSP_DIV_IN_SEL  =   BIT_RNG(4, 5),//0:rc24m 1:xtl48m 2:pll0 3:pll1
};

#define reg_clk_n22_set         REG_ADDR8(SC_BASE_ADDR + 0x56)
enum{
    FLD_CLK_N22_MOD             =   BIT_RNG(0, 3),
};

#define reg_clk_npe_set         REG_ADDR8(SC_BASE_ADDR + 0x57)
enum{
    FLD_CLK_NPE_MOD             =   BIT_RNG(0, 3),
    FLD_CLK_NPE_DIV_IN_SEL      =   BIT_RNG(4, 5),//0:rc24m 1:xtl48m 2:pll0 3:pll1
};

#define reg_dma_set             REG_ADDR32(SC_BASE_ADDR + 0x78)

#define D25F_SRAM_EMA_ADDR     (SC_BASE_ADDR + 0x70)//D25F_PKE_SDIO_NPE_SRAM
#define D25F_SRAM_EMA_DATA_LEN                   8
#define reg_soc_dp_sram_cfg0   REG_ADDR8(D25F_SRAM_EMA_ADDR)
enum{
    FLD_DP_SRAM_EMAA           = BIT_RNG(0,2),
    FLD_DP_SRAM_EMAWA          = BIT_RNG(3,4),
    FLD_DP_SRAM_EMASA          = BIT(5),
    FLD_DP_SRAM_WABL           = BIT(6),
};
#define reg_soc_dp_sram_cfg1  REG_ADDR8(SC_BASE_ADDR + 0x71)
enum{
    FLD_DP_SRAM_EMAB           = BIT_RNG(0,2),
    FLD_DP_SRAM_EMAWB          = BIT_RNG(3,4),
    FLD_DP_SRAM_EMASB          = BIT(5),
    FLD_DP_SRAM_WABLM          = BIT_RNG(6,7),
};

#define reg_soc_2p_reg_cfg0     REG_ADDR8(SC_BASE_ADDR + 0x72)
enum{
    FLD_2P_REG_EMAA            = BIT_RNG(0,2),
    FLD_2P_REG_EMAB            = BIT_RNG(3,5),
    FLD_2P_REG_EMASA           = BIT(6),
};

#define reg_soc_2p_reg_cfg1    REG_ADDR8(SC_BASE_ADDR + 0x73)
enum{
    FLD_2P_REG_WABL           = BIT(0),
    FLD_2P_REG_WABLM          = BIT_RNG(1,2),
};

#define reg_soc_sram_cfg0       REG_ADDR8(SC_BASE_ADDR + 0x74)
enum{
    FLD_SRAM_EMA              = BIT_RNG(0,2),
    FLD_SRAM_EMAW             = BIT_RNG(3,4),
    FLD_SRAM_EMAS             = BIT(5),
};

#define reg_soc_sram_cfg1       REG_ADDR8(SC_BASE_ADDR + 0x75)
enum{
    FLD_SRAM_RAML             = BIT(0),
    FLD_SRAM_RAWLM            = BIT_RNG(1,2),
    FLD_SRAM_WABL             = BIT(3),
    FLD_SRAM_WABLM            = BIT_RNG(4,6),
};

#define reg_soc_reg_cfg0        REG_ADDR8(SC_BASE_ADDR + 0x76)
enum{
    FLD_REG_EMA              = BIT_RNG(0,2),
    FLD_REG_EMAW             = BIT_RNG(3,4),
    FLD_REG_EMAS             = BIT(5),
};
#define reg_soc_reg_cfg1        REG_ADDR8(SC_BASE_ADDR + 0x77)
enum{
    FLD_REG_RAWL             = BIT(0),
    FLD_REG_RAWLM            = BIT_RNG(1,2),
    FLD_REG_WABL             = BIT(3),
    FLD_REG_WABLM            = BIT_RNG(4,5),
};
