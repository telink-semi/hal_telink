/********************************************************************************************************
 * @file    hal.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef STACK_HAL_HAL_TL322X_HAL_H_
#define STACK_HAL_HAL_TL322X_HAL_H_


/* MCU market positioning suport LE Audio*/
#ifndef MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN
    #define MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN 0
#endif

#define SCHEDULE_USE_BB_TIMER 0

/* MCU hardware support Channel Sounding */
#define HARDWARE_CHANNEL_SOUNDING_SUPPORT_EN 1

#define AES_CCM_DEC_US                       200 //TODO:need test

#define TNOP                                 __asm__("nop")


#define PPM_IDX_LONG_SLEEP_MIN               4


#define PPM_IDX_SHORT_SLEEP_MIN              5  //500 ppm
#define PPM_IDX_MAX                          10 //1000 ppm


//for aes module in ceva IP ,such as B91 and B92,aes module must switch address when use in BLE,other IC no need care.
#define HAL_CEVA_AES_ADDRESS_SWITCH \
    do {                            \
    } while (0)

//different IC ,its rf dma tx/rx wptr/rptr maybe different,use macro instead of register.
#define HAL_REG_RF_DMA_FIFO_TX_RPTR (reg_rf_dma_tx_rptr(0))

#define HAL_REG_RF_DMA_FIFO_TX_WPTR (reg_rf_dma_tx_wptr(0))

#define FLD_DMA_RPTR_MASK           FLD_BB_DMA_RPTR_MASK
#define FLD_DMA_RPTR_SET            FLD_BB_DMA_RPTR_SET
#define FLD_DMA_RPTR_NEXT           FLD_BB_DMA_RPTR_NEXT
#define FLD_DMA_RPTR_CLR            FLD_BB_DMA_RPTR_CLR

#define FLD_DMA_WPTR_MASK           FLD_BB_DMA_WPTR_MASK

//different IC,its rf irq mask number and irq mask register maybe different,use macro instead of register set rf irq mask in ble stack.
#define HAL_BLE_STACK_RF_IRQ_MASK_SET                                            \
    do {                                                                         \
        RF_CLEAR_ALL_IRQ_MASK;                                                   \
        reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX | BLMS_FLG_RF_CONN_DONE; \
    } while (0)

#define HAL_BLE_STACK_RF_IRQ_MASK_CLEAR \
    do {                                \
        RF_CLEAR_ALL_IRQ_MASK;          \
    } while (0)

#define HAL_GET_RF_TX_FINISH_IRQ (reg_rf_irq_status & FLD_RF_IRQ_TX)

#define HAL_CLEAR_RF_TX_FINISH_IRQ         \
    do {                                   \
        reg_rf_irq_status = FLD_RF_IRQ_TX; \
    } while (0)

#define HAL_GET_RF_NESN ((reg_rf_ll_2d_sclk & FLD_RF_NESN) >> 7)
/**
 * @brief       This function serves to get the timestamp in 24M system timer.
 * @param[in]   none.
 * @return      none.
 */
#if (SCHEDULE_USE_BB_TIMER)
//system timer is 24Mhz. bb timer is 8Mhz.
static inline unsigned int hal_rf_get_rx_timestamp(void)
{
    unsigned int curStimerTick  = reg_system_tick;
    unsigned int curBBTimerTick = reg_bb_timer_tick;
    return (curStimerTick - (curBBTimerTick - reg_rf_timestamp) * 3);
}
#else
static inline unsigned int hal_rf_get_rx_timestamp(void)
{
    return reg_rf_timestamp;
}
#endif


#define HAL_SKE_ENABLE \
    do {               \
        ske_dig_en();  \
    } while (0)

void debug_gpio_init(void);

#endif /* STACK_HAL_HAL_TL721X_HAL_H_ */
