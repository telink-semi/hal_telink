/********************************************************************************************************
 * @file    dma_reg.h
 *
 * @brief   This is the header file for TL321X
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
#ifndef DMA_REG_H
#define DMA_REG_H
#include "soc.h"
/*******************************    dma registers:  0x100400     ******************************/
#define DMA_BASE_ADDR                  0x100400
#define reg_dma_id                  REG_ADDR32(0x100400)
#define reg_dma_cfg                 REG_ADDR32(0x100410)
//enum{
//  FLD_DMA_CHANNEL_NUM     = BIT_RNG(0,3),
//  FLD_DMA_FIFO_DEPTH      = BIT_RNG(4,9),
//  FLD_DMA_REQ_NUM         = BIT_RNG(10,14),
//  FLD_DMA_REQ_SYNC        = BIT(30),
//  FLD_DMA_CHANINXFR       = BIT(31),
//};
//in C99, FLD_DMA_CHANINXFR = BIT(31) is error
#define FLD_DMA_CHANNEL_NUM     = BIT_RNG(0,3),
#define FLD_DMA_FIFO_DEPTH      = BIT_RNG(4,9),
#define FLD_DMA_REQ_NUM         = BIT_RNG(10,14),
#define FLD_DMA_REQ_SYNC        = BIT(30),
#define FLD_DMA_CHANINXFR       = BIT(31),

#define reg_dma_ctrl_llp            REG_ADDR8(DMA_BASE_ADDR+0x25)

#define reg_dma_err_mask                REG_ADDR8(DMA_BASE_ADDR+0x29)
enum{
    FLD_DMA_CHANNEL0_ERR_MASK   = BIT(0),
    FLD_DMA_CHANNEL1_ERR_MASK   = BIT(1),
    FLD_DMA_CHANNEL2_ERR_MASK   = BIT(2),
    FLD_DMA_CHANNEL3_ERR_MASK   = BIT(3),
    FLD_DMA_CHANNEL4_ERR_MASK   = BIT(4),
    FLD_DMA_CHANNEL5_ERR_MASK   = BIT(5),
    FLD_DMA_CHANNEL6_ERR_MASK   = BIT(6),
    FLD_DMA_CHANNEL7_ERR_MASK   = BIT(7),
};
#define reg_dma_abt_mask                REG_ADDR8(DMA_BASE_ADDR+0x2a)
enum{
    FLD_DMA_CHANNEL0_ABT_MASK   = BIT(0),
    FLD_DMA_CHANNEL1_ABT_MASK   = BIT(1),
    FLD_DMA_CHANNEL2_ABT_MASK   = BIT(2),
    FLD_DMA_CHANNEL3_ABT_MASK   = BIT(3),
    FLD_DMA_CHANNEL4_ABT_MASK   = BIT(4),
    FLD_DMA_CHANNEL5_ABT_MASK   = BIT(5),
    FLD_DMA_CHANNEL6_ABT_MASK   = BIT(6),
    FLD_DMA_CHANNEL7_ABT_MASK   = BIT(7),
};

#define reg_dma_err_isr                 REG_ADDR8(DMA_BASE_ADDR+0x30)
#define reg_dma_abt_isr                 REG_ADDR8(DMA_BASE_ADDR+0x31)
#define reg_dma_tc_isr                  REG_ADDR8(DMA_BASE_ADDR+0x32)
enum{
    FLD_DMA_CHANNEL0_IRQ        = BIT(0),
    FLD_DMA_CHANNEL1_IRQ        = BIT(1),
    FLD_DMA_CHANNEL2_IRQ        = BIT(2),
    FLD_DMA_CHANNEL3_IRQ        = BIT(3),
    FLD_DMA_CHANNEL4_IRQ        = BIT(4),
    FLD_DMA_CHANNEL5_IRQ        = BIT(5),
    FLD_DMA_CHANNEL6_IRQ        = BIT(6),
    FLD_DMA_CHANNEL7_IRQ        = BIT(7),
};
#define reg_dma_ctr0(i)             REG_ADDR8(( DMA_BASE_ADDR+0x44 +(i)*0x14))
#define reg_dma_ctrl(i)             REG_ADDR32(DMA_BASE_ADDR+0x44 +(i)*0x14)
enum{
    FLD_DMA_CHANNEL_ENABLE          = BIT(0),
    FLD_DMA_CHANNEL_TC_MASK         = BIT(1),
    FLD_DMA_CHANNEL_DST_REQ_SEL     = BIT_RNG(2,7),
    FLD_DMA_CHANNEL_SRC_REQ_SEL     = BIT_RNG(8,13),
    FLD_DMA_CHANNEL_DST_ADDR_CTRL   = BIT_RNG(14,15),
    FLD_DMA_CHANNEL_SRC_ADDR_CTRL   = BIT_RNG(16,17),
    FLD_DMA_CHANNEL_DST_MODE        = BIT(18),
    FLD_DMA_CHANNEL_SRC_MODE        = BIT(19),
    FLD_DMA_CHANNEL_DST_WIDTH       = BIT_RNG(20,21),
    FLD_DMA_CHANNEL_SRC_WIDTH       = BIT_RNG(22,23),
};

#define reg_dma_ctr3(i)             REG_ADDR8(DMA_BASE_ADDR+0x47+(i)*0x14)
enum{
    FLD_DMA_SRC_BURST_SIZE          =   BIT_RNG(0,2),
    FLD_DMA_R_NUM_EN                =   BIT(4),
    FLD_DMA_PRIORITY                =   BIT(5),
    FLD_DMA_W_NUM_EN                =   BIT(6),
    FLD_DMA_AUTO_ENABLE_EN           =  BIT(7),
};

#define reg_dma_src_addr(i)         REG_ADDR32(DMA_BASE_ADDR+0x48+(i)*0x14)
#define reg_dma_dst_addr(i)         REG_ADDR32(DMA_BASE_ADDR+0x4c+(i)*0x14)

#define reg_dma_size0(i)            REG_ADDR16(DMA_BASE_ADDR+0x50+(i)*0x14)
enum{
    FLD_DMA_TANS_SIZE0              = BIT_RNG(0,15),
};
#define reg_dma_size1(i)            REG_ADDR8(DMA_BASE_ADDR+0x52+(i)*0x14)
enum{
    FLD_DMA_TANS_SIZE1              = BIT_RNG(0,5),
    FLD_DMA_TANS_IDX                = BIT_RNG(6,7),
};

#define reg_dma_size(i)             REG_ADDR32 (DMA_BASE_ADDR+0x50+(i)*0x14)

enum{
    FLD_DMA_TX_SIZE         =   BIT_RNG(0,21),
    FLD_DMA_TX_SIZE_IDX     =   BIT_RNG(22,23),
};

#define reg_dma_llp(i)              REG_ADDR32 (DMA_BASE_ADDR+0x54+(i)*0x14)

#define reg_dma_llp_int_mode(i) REG_ADDR8(DMA_BASE_ADDR+0x113+(((i)>3) ? 1 : 0))

#define reg_dma_rx_wptr             REG_ADDR8(DMA_BASE_ADDR+0xf4)
#define reg_dma_tx_wptr             REG_ADDR8(DMA_BASE_ADDR+0x100)

enum{
    FLD_DMA_WPTR_MASK =         BIT_RNG(0,4),
};


#define reg_dma_rx_rptr             REG_ADDR8(DMA_BASE_ADDR+0xf5)
#define reg_dma_tx_rptr             REG_ADDR8(DMA_BASE_ADDR+0x101)
enum{
    FLD_DMA_RPTR_MASK =         BIT_RNG(0,4),
    FLD_DMA_RPTR_SET =          BIT(5),
    FLD_DMA_RPTR_NEXT =         BIT(6),
    FLD_DMA_RPTR_CLR =          BIT(7),
};

#define reg_dma_int_wbufov_mask(i)      REG_ADDR8(DMA_BASE_ADDR + 0x28 + (((i) > 0x80) ? 1 : 0) * 0x1000)
#define reg_dmac_wbufov_isr(i)          REG_ADDR8(DMA_BASE_ADDR + 0x2c + (((i) > 0x80) ? 1 : 0) * 0x1000)
#define reg_dmac_wbufov_ctrl_isr(i)     REG_ADDR8(DMA_BASE_ADDR + 0x2d + (((i) > 0x80) ? 1 : 0) * 0x1000)

#endif
