/********************************************************************************************************
 * @file    efuse.c
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
#include "reg_include/soc.h"

#define   EFUSE_BASE_ADDR           SC_BASE_ADDR

#define   reg_efuse_wdat_word       REG_ADDR32(EFUSE_BASE_ADDR + 0x48)
#define   efuse_wdat(i)             REG_ADDR8(EFUSE_BASE_ADDR + 0x48+((i)&0x03)) /* i[0-3] */

#define   reg_efuse_rdat_word       REG_ADDR32(EFUSE_BASE_ADDR + 0x4c)
#define   efuse_rdat(i)             REG_ADDR8(EFUSE_BASE_ADDR + 0x4c+((i)&0x03)) /* i[0-3] */

#define   reg_efuse_addr            REG_ADDR8(EFUSE_BASE_ADDR + 0x50) /* bit[0:4],word unit */

/**
 * This shows the status control register of efuse.
 * BIT[0] Write enable.
 * BIT[1] Read enable.
 * BIT[2] Efuse enable.
 * BIT[3] Trigger the efuse state machine to read or write data. No need to manually set 0.
 * BIT[4] In redundancy mode, this bit needs to be pulled down.
 * BIT[5] In redundancy mode, this bit needs to be pulled high.
 * BIT[6] margin read enable.
 * BIT[7] When the efuse is busy, this bit is 1.
 */
#define   reg_efuse_ctrl            REG_ADDR8(EFUSE_BASE_ADDR + 0x51)
enum{
    FLD_EFUSE_WREN      =   BIT(0),
    FLD_EFUSE_RDEN      =   BIT(1),
    FLD_EFUSE_EN        =   BIT(2),
    FLD_EFUSE_WR_TRIG   =   BIT(3),
    FLD_EFUSE_RSB       =   BIT(4),
    FLD_EFUSE_RWL       =   BIT(5),
    FLD_EFUSE_MR        =   BIT(6),
    FLD_EFUSE_BUSY      =   BIT(7),
};

/**
 * This shows the status control register of efuse.
 * BITRNG[0,1]  Configuration of the redundancy bit function.
 * BIT[2]       When the efuse is ready, this bit is 1.
 * BIT[3]       key_lock function, write 1 enable.
 * BIT[4]       The pd signal of efuse, read-only.
 */
#define   reg_efuse_ctrl1               REG_ADDR8(EFUSE_BASE_ADDR + 0x52)
enum{
    FLD_EFUSE_RB_SEL              = BIT_RNG(0,1),
    FLD_EFUSE_READY               = BIT(2),
    FLD_KEY_LOCK                  = BIT(3),
    FLD_EFUSE_PD                  = BIT(4),
};

#define   reg_efuse_b0              REG_ADDR32(EFUSE_BASE_ADDR + 0x0c)

#define   reg_efuse_id_read_en      REG_ADDR8(EFUSE_BASE_ADDR+0x34)   //read_en: 0x65

/**
 * This register is the clock configuration for the efuse.
 * BITRNG[0,1]  Timing configuration for non program mode. For example, startup, read mode.
 * BITRNG[2,3]  Timing Configuration for program mode.
 */
#define   reg_efuse_timimng_cfg     REG_ADDR8(EFUSE_BASE_ADDR + 0x9a)
enum{
    FLD_EFUSE_TIMING_CONFIG       = BIT_RNG(0,1),
    FLD_EFUSE_PGM_TIMING_CONFIG   = BIT_RNG(2,3),
};

typedef enum
{
    EFUSE_TIME_PCLK_24M           = 0x00,
    EFUSE_TIME_PCLK_48M           = 0x05,
    EFUSE_TIME_PCLK_96M           = 0x0a,
    EFUSE_TIME_PCLK_192M          = 0x0f,
}efuse_time_t;

/**
 * @brief   This function serve to set efuse time config.
 * @return  none.
 * @note    PCLK: 24MHz:0x00; 48MHz:0x05; 96MHz:0x0a; 192MHz:0x0f.
 */
void efuse_timing_config(efuse_time_t time)
{
    reg_efuse_timimng_cfg = time;
}

/**
 * @brief   This function gets the hardware auto-load data.
 * @return  The value is the first 32bit of efuse, the die_func value.
 */
unsigned long efuse_b0(void)
{
    return reg_efuse_b0;
}

/**
 * @brief       This function servers to write efuse.
 * @param[in]   addr    - the read address must align word (4bytes): 0,4,8...
 * @param[out]  buff    - the pointer to the data for read. It's best to align 4 bytes.
 * @param[in]   len     - write length unit byte.
 * @return      none
 */
void efuse_read(unsigned char addr,unsigned char* buff, unsigned short len)
{
    unsigned char word_len = len >> 2;
    unsigned char single_len = len & 3;
    reg_efuse_ctrl |= (FLD_EFUSE_EN | FLD_EFUSE_RDEN);
    reg_efuse_addr = addr>>2;//(align word)
    for (unsigned int i = 0; i < word_len; i++)
    {
        while(FLD_EFUSE_READY != (reg_efuse_ctrl1 & FLD_EFUSE_READY));
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        while(reg_efuse_ctrl & FLD_EFUSE_BUSY);
        ((unsigned int *)buff)[i] = reg_efuse_rdat_word ;
    }
    if(single_len)//must read word
    {
        while(FLD_EFUSE_READY != (reg_efuse_ctrl1 & FLD_EFUSE_READY));
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        while(reg_efuse_ctrl & FLD_EFUSE_BUSY);
        unsigned int temp = reg_efuse_rdat_word ;
        for (unsigned int i = 0; i < single_len; i++)
        {
            buff[(word_len*4) + i]= (temp>>(i*8))&0xff;
        }
    }
    reg_efuse_ctrl &= ~(FLD_EFUSE_EN | FLD_EFUSE_RDEN);
}

/**
 * @brief       This function servers to read efuse.
 * @param[in]   addr    - the write address must align word (4bytes): 0,4,8...
 * @param[in]   buff    - the pointer to the data for write. It's best to align 4 bytes.
 * @param[in]   len     - write length unit byte.
 * @return      none
 */
void efuse_write(unsigned char addr, unsigned char *buff, unsigned short len)
{
    unsigned char word_len = len >> 2;
    unsigned char single_len = len & 3;
    reg_efuse_ctrl |= (FLD_EFUSE_EN | FLD_EFUSE_WREN);
    for (unsigned int i = 0; i < word_len; i++)
    {
        reg_efuse_addr = (addr>>2)+i;
        reg_efuse_wdat_word=((unsigned int *)buff)[i];
        while(FLD_EFUSE_READY != (reg_efuse_ctrl1 & FLD_EFUSE_READY));
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        while(reg_efuse_ctrl & FLD_EFUSE_BUSY);
    }
    if(single_len)//must write word
    {
        reg_efuse_addr = (addr>>2)+word_len;
        unsigned int temp=0;
        for (unsigned int i = 0; i < single_len; i++)
        {
            temp |=((buff[(word_len*4)+i])&0xff)<<(i*8);
        }
        reg_efuse_wdat_word=temp;
        while(FLD_EFUSE_READY != (reg_efuse_ctrl1 & FLD_EFUSE_READY));
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;
        while(reg_efuse_ctrl & FLD_EFUSE_BUSY);
    }
    reg_efuse_ctrl &= ~(FLD_EFUSE_EN | FLD_EFUSE_WREN);
}
