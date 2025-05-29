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
#ifndef STACK_HAL_HAL_B91_HAL_H_
#define STACK_HAL_HAL_B91_HAL_H_

/* MCU market positioning suport LE Audio*/
#ifndef MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN
    #define MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN 1
#endif


#define AES_CCM_DEC_US 200

#define TNOP           __asm__("nop")


/* MCU hardware TX auto FIFO 4096 bytes limitation */
#define MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION 1


/* MCU hardware support Channel Sounding */
#define HARDWARE_CHANNEL_SOUNDING_SUPPORT_EN 0


#define PPM_IDX_LONG_SLEEP_MIN               3  //300 ppm
#define PPM_IDX_SHORT_SLEEP_MIN              5  //500 ppm
#define PPM_IDX_MAX                          10 //1000 ppm

//for aes module in ceva IP ,such as B91 and B92,aes module must switch address when use in BLE,other IC no need care.
#define HAL_CEVA_AES_ADDRESS_SWITCH   \
    do {                              \
        reg_embase_addr = 0xc0000000; \
    } while (0)

//different IC ,its rf dma tx/rx wptr/rptr maybe different,use macro instead of register.
#define HAL_REG_RF_DMA_FIFO_TX_RPTR (reg_dma_tx_rptr)

#define HAL_REG_RF_DMA_FIFO_TX_WPTR (reg_dma_tx_wptr)

//different IC,its rf irq mask number and irq mask register maybe different,use macro instead of register set rf irq mask in ble stack.
#define HAL_BLE_STACK_RF_IRQ_MASK_SET                                            \
    do {                                                                         \
        reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX | BLMS_FLG_RF_CONN_DONE; \
    } while (0)

#define HAL_BLE_STACK_RF_IRQ_MASK_CLEAR \
    do {                                \
        reg_rf_irq_mask = 0;            \
    } while (0)

#define HAL_GET_RF_NESN ((reg_rf_ll_pid_h & FLD_RF_NESN) >> 4)

/**
 * @brief       This function serves to get the timestamp in 24M system timer.
 * @param[in]   none.
 * @return      none.
 */
static inline unsigned int hal_rf_get_rx_timestamp(void)
{
    return reg_rf_timestamp;
}

#define HAL_SKE_ENABLE \
    do {               \
    } while (0)
#endif /* STACK_HAL_HAL_B91_HAL_H_ */
