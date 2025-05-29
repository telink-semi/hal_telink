/********************************************************************************************************
 * @file    otp.c
 *
 * @brief   This is the source file for TL721X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "reg_include/register.h"
#include "compiler.h"
#include "lib/include/stimer.h"
#include "lib/include/clock.h"
#include "lib/include/plic.h"
#include "lib/include/otp.h"
#include "lib/include/pm/pm_internal.h"

#define ADC_CALIB_EN 0

#ifndef OTP_OPERATE_MODE
    #define OTP_OPERATE_MODE 1 /* 1: auto mode; 0: manual mode(for internal test). */
#endif

/**
 * @brief IP enable time, pwe=1 ---> enable, Tcs > 10(us).
 * @note  The usage of OTP will include RC as the clock source. 
 *        Considering the accuracy of RC, so this value is set to the twice of spec min.
 */
#define Tcs (20)

/**
 * @brief Program pulse width time, pwe high time, 20 < Tpw < 25(us).
 */
#define Tpw (22)

/**
 * @brief Program pulse interval time, pwe low time, 1 < Tpwi < 5(us).
 */
#define Tpwi (4)

/**
 * @brief Program mode recovery time, pporg=0 ---> pce=0, 5 < Tppr < 100(us).
 */
#define Tppr (10)

/**
 * @brief Program mode setup time, pprog=1 ---> pwe=1, 5 < Tpps < 20(us).
 */
#define Tpps (5)

/**
 * @brief Program mode hold time, pwe=0 ---> pprog=0, 5 < Tpph < 20(us).
 */
#define Tpph (10)

/**
 * @brief Deep standby to active mode setup time, deep standby to active mode setup time, Tsas >2(us).
 */
#define Tsas (5)

/**
 * @brief PTM mode setup time, ptm  -->  pce = 1, Tms>1(ns).
 */
#define Tms (1)

/**
 * @brief PTM mode hold time, ptm  -->  pce = 0, Tmh>1(ns).
 */
#define Tmh (1)

/**
 * @brief IP enable time in program, pce = 1 ---> pprog=1, 10 < Tcsp < 100(us).
 */
#define Tcsp (20)

/**
 * @brief LDO setup time, ldo setup time, Tpls > 10(us).
 */
#define Tpls (15)

/**
 * @brief PTM type.
 */
typedef enum
{
    OTP_PTM_READ                = 0x00, /**< PTM mode read */
    OTP_PTM_PROG                = 0x02, /**< PTM mode write */
    OTP_PTM_INIT_MARGIN_READ    = 0x01, /**< PTM mode init margin read */
    OTP_PTM_PGM_MARGIN_READ     = 0x04, /**< PTM mode pgm(program) margin read */
    OTP_PTM_HT_INIT_MARGIN_READ = 0x09, /**< PTM mode ht(high temp) init margin read (internal test)*/
    OTP_PTM_HT_PGM_MARGIN_READ  = 0x0c, /**< PTM mode ht pgm margin read (internal test)*/
} otp_ptm_type_e;

/**
 * @brief Auto power up time config.
 */
typedef enum
{
    OTP_TIM_CONFIG_24M  = 0x00, /**< PCLK (0-24M] */
    OTP_TIM_CONFIG_48M  = 0x01, /**< PCLK (24-48M] */
    OTP_TIM_CONFIG_96M  = 0x02, /**< PCLK (48-96M] */
    OTP_TIM_CONFIG_192M = 0x03, /**< PCLK (96-192M] */
    OTP_TIM_CONFIG_384M = 0x04, /**< PCLK (192-384M] */
} otp_tim_config_e;
#if ADC_CALIB_EN
typedef struct
{
    unsigned char ft_vbat_gain;
    unsigned char ft_vbat_offset;
    unsigned char ft_gpio_gain;
    unsigned char ft_gpio_offset;
    unsigned char cp_vbat_gain;
    unsigned char cp_vbat_offset;
    unsigned char cp_gpio_gain;
    unsigned char cp_gpio_offset;
} adc_ft_cp_calib_t;
#endif
/**********************************************************************************************************************
 *                                                External interface                                                  *
 *********************************************************************************************************************/
/*!
 * @name External functions
 * @{
 */

/**
 * @brief      This function serves to init otp clk. This interface must be called to initialize the otp clock before using otp.
 * @return     none
 */
void otp_clk_init(void)
{
    otp_tim_config_e tim_config    = 0;
    unsigned char    capt_edge_cnt = 0;

    if (sys_clk.pclk <= 24) {
        tim_config    = OTP_TIM_CONFIG_24M;
        capt_edge_cnt = 3;  /* 3 * (1000 / 24) > 110ns(Tcd) */
    } else if (sys_clk.pclk <= 48) {
        tim_config    = OTP_TIM_CONFIG_48M;
        capt_edge_cnt = 6;  /* 6 * (1000 / 48) */
    } else if (sys_clk.pclk <= 96) {
        tim_config    = OTP_TIM_CONFIG_96M;
        capt_edge_cnt = 12; /* 12 * (1000 / 96) */
    } else if (sys_clk.pclk <= 192) {
        tim_config    = OTP_TIM_CONFIG_192M;
        capt_edge_cnt = 24; /* 24 * (1000 / 192) */
    } else {
        tim_config    = OTP_TIM_CONFIG_384M;
        capt_edge_cnt = 60; /* 40 * (1000 / 384) */
    }

    /*
     * When auto power is used, these delays are generated by internal counters, and when the system clock pclk is switched to another frequency, \n
     * these delays will also be changed, so in order to ensure that these timings are unchanged for OTP, you need to configure reg_tim_cfg accordingly.
     */
    reg_otp_ctrl5 = (reg_otp_ctrl5 & ~(FLD_OTP_TIM_CFG)) | tim_config;

    /* Config pclk cnt to latch data when OTP PCLK rising, cnt * Tpclk > Tcd(max 110ns)*/
    reg_otp_ctrl4 = ((reg_otp_ctrl4 & ~(FLD_OTP_CAP_EDGE)) | (capt_edge_cnt & FLD_OTP_CAP_EDGE));
}

/**
 * @brief      This function serves to wait until the operation of OTP is done.
 * @param[in]  none
 * @return     none
 */
static inline void otp_wait_done(void)
{
    while (reg_otp_status & FLD_OTP_BUSY);
}

/**
 * @brief      This function serves to set otp active mode(auto/manual), if otp is in deep mode, need to operate on the otp, set active mode.
 * @return     none
 */
void otp_set_active_mode(void)
{
#if (OTP_OPERATE_MODE)
    reg_otp_status |= FLD_OTP_AUTO_PWUP_TRIG;
    otp_wait_done();
#else
    reg_otp_ctrl1 |= FLD_OTP_PLDO;  /* pldo = 1 */
    delay_us(Tpls);
    reg_otp_ctrl1 |= FLD_OTP_PDSTD; /* pdstd = 1 */
    delay_us(Tsas);

    otp_read_manual_mode();
#endif
}

/**
 * @brief      This function serves to otp set deep standby mode, can enter deep to save current.
 * @return     none
 */
void otp_set_deep_standby_mode(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE); /* pce=0 */
    /* Tash >= 0(ns) */
    reg_otp_ctrl1 &= ~(FLD_OTP_PDSTD); /* pdstb=0 */
    /* Tplh >= 0(ns) */
    reg_otp_ctrl1 &= ~(FLD_OTP_PLDO); /* pldo=0 */
}

/**
 * @brief     This function is a common sequence used by these interfaces:otp_write32/otp_read_cycle_auto/otp_read_cycle_manual.
 * @param[in] ptm_mode - ptm type.
 * @return    none
 */
static void otp_start(otp_ptm_type_e ptm_mode)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);
    reg_otp_ctrl1 = ((reg_otp_ctrl1 & ~(FLD_OTP_PTM)) | ptm_mode); /* ptm mode */
    /* Tms >= 1(ns) */
    reg_otp_ctrl0 |= (FLD_OTP_PCE); /* pce = 1*/
}

/**
 * @brief      This function serves to auto read data from OTP memory.
 * @param[in]  ptm_mode - read mode.
 * @param[in]  addr - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out]  buff - data buff.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_read_cycle_auto(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    unsigned int r = core_interrupt_disable();
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    core_cclk_delay_tick((unsigned long long)(Tcs * sys_clk.cclk));

    reg_otp_pa = addr;
    buff[0]    = reg_otp_rd_dat; /* trigger read */
    otp_wait_done();

    /* pa auto inc */
    for (unsigned int i = 0; i < word_len; i++) {
        buff[i] = reg_otp_rd_dat;
        otp_wait_done();
    }

    reg_otp_ctrl0 &= ~(FLD_OTP_PCE); /* pce = 0*/
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to write data to OTP memory,4 bytes one time.
 * @param[in]  ptm_mode - write mode.
 * @param[in]  addr  - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  data  - the data need to be write,4 bytes.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_write32(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int data)
{
    unsigned int r = core_interrupt_disable();
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    /* Tcsp */
    delay_us(Tcsp);

    /* prog pas addr data */
    reg_otp_ctrl0 |= FLD_OTP_PPROG;
    reg_otp_ctrl0 |= FLD_OTP_PAS;
    reg_otp_pa     = addr;
    reg_otp_wr_dat = data;
    reg_otp_paio   = 0;
    /* Tpps */
    delay_us(Tpps);

    /* redundancy programming  38*2 */
    for (unsigned char i = 1; i <= 76; i++) {
        reg_otp_ctrl0 |= FLD_OTP_PWE;
        delay_us(Tpw);
        reg_otp_ctrl0 &= ~(FLD_OTP_PWE);
        if (i < 38) {
            reg_otp_paio = i;
        } else if (i == 38) {
            reg_otp_ctrl0 &= (~FLD_OTP_PAS);
            reg_otp_pa     = addr;
            reg_otp_wr_dat = data;
            reg_otp_paio   = 0;
        } else if ((i > 38) && (i < 76)) {
            reg_otp_paio = i - 38;
        } else if (i == 76) {
            break;
        }
        // because the for loop and the if judge the time,choose to use Tpwi/2.
        delay_us(Tpwi / 2);
    }
    delay_us(Tpph);
    reg_otp_ctrl0 &= ~(FLD_OTP_PPROG); /* pporg = 0 */
    delay_us(Tppr);
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);   /* pce = 0 */
    /* Tmh >= 1(ns) */
    reg_otp_ctrl1 = ~(FLD_OTP_PTM);
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to read data from OTP memory, belong to otp normal read.
 *             otp has three kinds of read mode,in general,just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out]  buff - data buff.
 * @return     none
 */
void otp_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
#if (OTP_OPERATE_MODE)
    otp_read_cycle_auto(OTP_PTM_READ, addr, word_len, buff);
#else
    otp_read_cycle_manual(OTP_PTM_READ, addr, word_len, buff);
#endif
}

/**
 * @brief      This function serves to write data to OTP memory.
 *             the minimum unit of otp read-write operation is 4 bytes, that is a word. meanwhile, the otp cannot be burned repeatedly,
 *             this function is limited to writing only once,this function will determine if the otp is 0xffffffff, and if it is 0xffffffff,
 *             it will write the otp.
 * @param[in]  addr - the address of the data,it has to be a multiple of 4,the OTP memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return
 *             - 0 it means that the otp operation area is 0xffffffff or the write data,
 *                return 0 not mean that the burning was successful,need to use three kinds of read mode to check whether the writing was successful.
 *             - 1 it means that there is an operation value in the operation area,it is not 0xffffffff or the write data,no burning action is performed.
 */
unsigned char otp_write(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    for (unsigned int i = 0; i < word_len; i++) {
        unsigned int temp = 0;
        otp_read(addr + i * 4, 1, (unsigned int *)&temp);
        if (temp == 0xffffffff) {
            otp_write32(OTP_PTM_PROG, addr + i * 4, buff[i]);
        } else if (temp != buff[i]) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp pgm margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return     none
 */
void otp_pgm_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
#if (OTP_OPERATE_MODE)
    otp_read_cycle_auto(OTP_PTM_PGM_MARGIN_READ, addr, word_len, buff);
#else
    otp_read_cycle_manual(OTP_PTM_PGM_MARGIN_READ, addr, word_len, buff);
#endif
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp initial margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return     none
 *
 */
void otp_initial_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
#if (OTP_OPERATE_MODE)
    otp_read_cycle_auto(OTP_PTM_INIT_MARGIN_READ, addr, word_len, buff);
#else
    otp_read_cycle_manual(OTP_PTM_INIT_MARGIN_READ, addr, word_len, buff);
#endif
}

/**
 * @brief        This function serves to check protection code according SDK version.
 * @param[in]    sdk_version, 0:driver sdk  0xff:sdk_version_ignore
 * @return       none.
 */
void otp_check_protection_code(unsigned char sdk_version)
{
    unsigned int pCode = 0;
    /* set otp active */
    otp_set_active_mode();
    otp_read(104, 1, (unsigned int *)&pCode);
    /* shutdown otp */
    otp_set_deep_standby_mode();
    pCode = pCode & 0x1f; //Bit0-4 is market protection code.

    switch (sdk_version) {
    case 0:
        //Different SDKs have different restrictions. Please modify the code according to your own situation.
        //The driver here is only for example reference.
        if (0xE0 > pCode) {
            sys_reset_all();
            while (1);
        }
        break;
    case 0xff:
        break;
    default:
        if (1) // Prevent macro setting exceptions from invalidating the ProtectionCode function
        {
            sys_reset_all();
            while (1);
        }
        break;
    }
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Internal interface                                                  *
 *********************************************************************************************************************/
/*!
 * @name Internal functions
 * @{
 */

/**
 * @brief      This function serves to get otp auto load data. As soon as the system is powered on, \n
 *             OTP will automatically load the 3-word data from OTP address 0 to the auto_load register.
 * @param[in]  index - auto load data index. Range 0-2.
 * @return     Auto load data(word).
 * @note       The auto load is triggered when the device is powered on for the first time, reset pin, or deep wake up.
 */
unsigned int otp_get_auto_load_data(unsigned char index)
{
    return reg_otp_auto_load_data(index);
}

/**
 * @brief      This function serves to enable key lock. After key lock enable, the data in OTP word address 3~10 cannot be read.
 * @return     none
 * @note       key_lock with a reset value of 0 and can only be written from 0 to 1, not from 1 to 0.
 */
void otp_key_lock(void)
{
    reg_otp_status |= FLD_OTP_KEYLOCK;
}

/**
 * @brief      This function serves to enable row test.
 * @return     none
 */
void otp_test_row_en(void)
{
    reg_otp_ctrl0 |= FLD_OTP_PTR;
    reg_otp_ctrl0 &= ~(FLD_OTP_PTC);
}

/**
 * @brief      This function serves to enable column test.
 * @return     none
 */
void otp_test_column_en(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PTR);
    reg_otp_ctrl0 |= FLD_OTP_PTC;
}

/**
 * @brief      This function serves to disable test mode.
 * @return     none
 * @note       PTR and PTC are both in normal read and write mode when they are at the same low level,
 *             and should be selected for both when they are at the same low level,
 *             but it is not mentioned in the IP manual.
 */
void otp_test_dis(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PTR | FLD_OTP_PTC);
}

/**
 * @brief      This function serves to select read mode as auto.
 * @return     none
 */
void otp_read_auto_mode(void)
{
    reg_otp_ctrl3 &= ~(FLD_OTP_MAN_MODE);
}

/**
 * @brief      This function serves to select read mode as manual.
 * @return     none
 */
void otp_read_manual_mode(void)
{
    reg_otp_ctrl3 |= FLD_OTP_MAN_MODE;
}

/**
 * @brief      This function serves to enable ECC verification function.
 * @return     none
 * @note       For each address read, the IP will deliver the 38 bits at PDOUT[37:0],
 *             PDOUT[37:32] is for 6 ECC parity bits, PDOUT[31:0 ] is for user storage data.
 *             After enabling, PDOUT[31:0] will be automatically corrected.
 */
void otp_ecc_en(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_ECC_RDB);
}

/**
 * @brief      This function serves to disable ecc.
 * @return     none
 */
void otp_ecc_dis(void)
{
    reg_otp_ctrl0 |= FLD_OTP_ECC_RDB;
}

/**
 * @brief      This function serves to manual read data from OTP memory(for internal test).
 * @param[in]  ptm_mode - read mode.
 * @param[in]  addr - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void otp_read_cycle_manual(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    unsigned int r = core_interrupt_disable();
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    delay_us(Tcs);

    for (unsigned int i = 0; i < word_len; i++) {
        reg_otp_pa = addr + i * 4;
        /* Tas >= 1(ns) */
        reg_otp_ctrl3 |= FLD_OTP_MAN_PCLK;
        /* Tkh >= 20(ns) && Tcd > 110ns*/
        core_cclk_delay_tick(120);
        reg_otp_ctrl3 &= ~(FLD_OTP_MAN_PCLK);
        buff[i] = reg_otp_rd_dat;
        /* Tkl >= 20(ns) */
        core_cclk_delay_tick(25);
    }

    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp high temp pgm margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return     none
 */
void otp_ht_pgm_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
#if (OTP_OPERATE_MODE)
    otp_read_cycle_auto(OTP_PTM_HT_PGM_MARGIN_READ, addr, word_len, buff);
#else
    otp_read_cycle_manual(OTP_PTM_HT_PGM_MARGIN_READ, addr, word_len, buff);
#endif
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp high temp initial margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return     none
 *
 */
void otp_ht_initial_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
#if (OTP_OPERATE_MODE)
    otp_read_cycle_auto(OTP_PTM_HT_INIT_MARGIN_READ, addr, word_len, buff);
#else
    otp_read_cycle_manual(OTP_PTM_HT_INIT_MARGIN_READ, addr, word_len, buff);
#endif
}

/**
 * @brief      This function serves to write data to OTP column memory.
 * @param[in]  ptm_mode - write mode.
 * @param[in]  addr  - the address of the data,the otp memory that can access is from 0x0000-0x3C0,can't access other address.
 * @param[in]  data  - the data need to be write,4 bytes.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_write32_column(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int data)
{
    unsigned int r = core_interrupt_disable();
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    /* Tcsp */
    delay_us(Tcsp);

    /* prog pas addr data */
    reg_otp_ctrl0 |= FLD_OTP_PPROG;
    reg_otp_ctrl0 |= FLD_OTP_PAS;
    reg_otp_pa     = addr;
    reg_otp_wr_dat = data;
    reg_otp_paio   = 0;
    /* Tpps */
    delay_us(Tpps);

    /* redundancy programming  38*2 */
    for (unsigned char i = 1; i <= 76; i++) {
        reg_otp_ctrl0 |= FLD_OTP_PWE;
        delay_us(Tpw);
        reg_otp_ctrl0 &= ~(FLD_OTP_PWE);
        if (i < 38) {
            reg_otp_paio = 0;
        } else if (i == 38) {
            reg_otp_ctrl0 &= (~FLD_OTP_PAS);
            reg_otp_pa     = addr;
            reg_otp_wr_dat = data;
            reg_otp_paio   = 0;
        } else if ((i > 38) && (i < 76)) {
            reg_otp_paio = 0;
        } else if (i == 76) {
            break;
        }
        // because the for loop and the if judge the time,choose to use Tpwi/2.
        delay_us(Tpwi / 2);
    }
    delay_us(Tpph);
    reg_otp_ctrl0 &= ~(FLD_OTP_PPROG); /* pporg = 0 */
    delay_us(Tppr);
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);   /* pce = 0 */
    /* Tmh >= 1(ns) */
    reg_otp_ctrl1 = ~(FLD_OTP_PTM);
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to write data to OTP row test memory.
 * @param[in]  addr  - the address of the data,the otp row test memory that can access is from 0x00-0x3F,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return
 *             - 0 it means that the otp operation area is 0xffffffff or the write data,
 *                return 0 not mean that the burning was successful,need to use three kinds of read mode to check whether the writing was successful.
 *             - 1 it means that there is an operation value in the operation area,it is not 0xffffffff or the write data,no burning action is performed.
 */
unsigned char otp_write_row(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    addr &= ~(BIT_RNG(6, 7) | (BIT_RNG(0, 1) << 8));
    return otp_write(addr, word_len, buff);
}

/**
 * @brief      This function serves to write data to OTP column test memory, 64 bytes one time, can only write bit0.
 * @param[in]  addr  - the address of the data,the otp column test memory that can access is from 0x000-0x3C0,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out] buff - data buff.
 * @return
 *             - 0 it means that the otp operation area is 0xffffffff or the write data,
 *                return 0 not mean that the burning was successful,need to use three kinds of read mode to check whether the writing was successful.
 *             - 1 it means that there is an operation value in the operation area,it is not 0xffffffff or the write data,no burning action is performed.
 */
unsigned char otp_write_column(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    BM_CLR(addr, BIT_RNG(2, 5));
    for (unsigned int i = 0; i < word_len; i++) {
        unsigned int temp = 0;
        otp_read(addr + i * 4, 1, (unsigned int *)&temp);
        if (temp == 0xffffffff) {
            otp_write32_column(OTP_PTM_PROG, addr + i * 4, buff[i]);
        } else if (temp != buff[i]) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief      This function serves to read data from OTP row test memory, belong to otp normal read.
 * @param[in]  addr  - the address of the data,the otp row test memory that can access is from 0x00-0x3F,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out]  buff - data buff.
 * @return     none
 */
void otp_row_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    addr &= ~(BIT_RNG(6, 7) | (BIT_RNG(0, 1) << 8));
    otp_read(addr, word_len, buff);
}

/**
 * @brief      This function serves to read data from OTP column test memory, belong to otp normal read.
 * @param[in]  addr  - the address of the data,the otp column test memory that can access is from 0x00-0x3C0,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[out]  buff - data buff.
 * @return     none
 */
void otp_column_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    addr &= ~(BIT_RNG(6, 7) | (BIT_RNG(0, 1) << 8));
    otp_read(addr, word_len, buff);
}

/**
 * @brief      This function serves to read IEEE address from OTP.
 * @param[out] buf  - Pointer to IEEE address buffer(IEEE address is 8bytes)
 * @return     none
 */
void otp_get_ieee_addr(unsigned char *buf)
{
    otp_set_active_mode();
    otp_read(0x6c, 2, (unsigned int *)buf);
    otp_set_deep_standby_mode();
}

#if ADC_CALIB_EN
/**
 * @brief       This function is used to Tighten the judgment of illegal values for gpio calibration and vbat calibration in the otp.
 *              The ADC vref gain calibtation should range from 1100mV to 1350mV, the ADC vref offset calibration should range from -100mV to 100mV.
 * @param[in]   gain - the value of gpio_calib_vref_gain or vbat_calib_vref_gain
 *              offset - the value of gpio_calib_vref_offset or vbat_calib_vref_offset
 *              calib_func - Function pointer to gpio_calibration or vbat_calibration.
 * @return      DRV_API_FAILURE:the calibration function is invalid; DRV_API_SUCCESS:the calibration function is valid.
 */
drv_api_status_e otp_set_adc_calib_value(unsigned char gain, signed char offset, void (*calib_func)(unsigned short, signed char))
{
    if ((gain <= 250) && (offset >= -100) && (offset <= 100)) {
        (*calib_func)(gain + 1100, offset);
        return true;
    } else {
        return false;
    }
}

/**
 * @brief      This function is used to calib ADC 1.2V vref.
 * @param[in]  none
 * @return     DRV_API_SUCCESS - the calibration value update, DRV_API_FAILURE - the calibration value is not update.
 */
drv_api_status_e otp_calib_adc_vref(void)
{
    adc_ft_cp_calib_t calib_value;
    /********************************************************************************************
        The ADC calibration value priority of TL721X is FT > CP.
        The GPIO calibration value and the VBAT calibration value do not necessarily exist at the same time.
    ********************************************************************************************/
    otp_set_active_mode();
    otp_read(0x94, 2, (unsigned int *)&calib_value);
    otp_set_deep_standby_mode();

    if (otp_set_adc_calib_value(calib_value.ft_vbat_gain, (signed char)calib_value.ft_vbat_offset, adc_set_vbat_calib_vref) || otp_set_adc_calib_value(calib_value.ft_gpio_gain, (signed char)calib_value.ft_gpio_offset, adc_set_gpio_calib_vref))     //vbat_ft and gpio_ft
    {
        if (otp_set_adc_calib_value(calib_value.cp_vbat_gain, (signed char)calib_value.cp_vbat_offset, adc_set_vbat_calib_vref) || otp_set_adc_calib_value(calib_value.cp_gpio_gain, (signed char)calib_value.cp_gpio_offset, adc_set_gpio_calib_vref)) //vbat_cp and gpio_cp
        {
            return DRV_API_FAILURE;
        }
    }
    return DRV_API_SUCCESS;
}
#endif
/**
 * @brief       This function serves to read vdd0p94 and vddo1p8 calibration data from OTP.
 * @return      res 0: ok, 1: vdd0p94 invalid, 2: vddo1p8 invalid, 3: vdd0p94 and vddo1p8 all invalid
 */
_attribute_ram_code_sec_noinline_ unsigned char otp_get_vdd0p94_vddo1p8_calib_value(void)
{
    unsigned char res_cal_vdd0p94 = 0, res_cal_vddo1p8 = 0;

    unsigned int otp_value[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

    otp_set_active_mode();

    /* 
     * 0x84 vdd0P94 FT 4bytes - otp_value[0]
     * 0x88 vdd0P94 CP 4bytes - otp_value[1]
     * 0x8c vdd1P8  FT 1byte  - otp_value[2]
     * 0x90 vdd1P8  CP 1byte  - otp_value[3]
     */
    otp_read(0x84, 4, otp_value); //readout vdd0p94 and vdd1p8 at once
    otp_set_deep_standby_mode();

    /*
     * vdd0P94 0-3 bytes FT, 4-7 bytes CP.
     * The correct data format should be 0x0_0_0_0_.(_:0~f).
     * The standard for determining the validity of data is every byte data is between 0-15 corresponding to pm_trim_vdd0p94_e.
     * Calibration priority FT > CP.
     * (added by jilong.liu, confirmed by zhengting.hu and hao.wang at 20241219)
     */
    for (int i = 0; i < 2; i++) {
        if (!(otp_value[i] & 0xf0f0f0f0)) {
            g_pm_cal_vdd0p94_info.dcdc_0p95v = otp_value[i] & 0xff;
            g_pm_cal_vdd0p94_info.ldo_0p95v  = (otp_value[i] >> 8) & 0xff;
            g_pm_cal_vdd0p94_info.dcdc_1p05v = (otp_value[i] >> 16) & 0xff;
            g_pm_cal_vdd0p94_info.ldo_1p05v  = (otp_value[i] >> 24) & 0xff;

            res_cal_vdd0p94 = 0;
            break;
        } else {
            res_cal_vdd0p94 = 1;
        }
    }

    /*
     * vddo1P8 1bytes FT, 1bytes CP. 
     * Since OTP must be written in 4 bytes, the remaining 3 bytes will written as 0xff.
     * The correct data format should be 0xffffff0_.(_:0~7).
     * The standard for determining the validity of data is every byte data is between 0-7 corresponding to pm_trim_vddo1p8_e.
     * Calibration priority FT > CP.
     * (added by jilong.liu, confirmed by zhengting.hu and hao.wang at 20241219)
     */
    for (int i = 2; i < 4; i++) {
        if (((otp_value[i] >> 8) == 0xffffff) && (!(otp_value[i] & 0xf8))) {
            g_pm_cal_vddo1p8_info = otp_value[i] & 0xff;

            res_cal_vddo1p8 = 0;
            break;
        } else {
            res_cal_vddo1p8 = 2;
        }
    }

    return (res_cal_vdd0p94 + res_cal_vddo1p8);
}

/**
 * @}
 */
