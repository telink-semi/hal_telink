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

#ifndef STACK_HAL_HAL_TL751X_HAL_H_
#define STACK_HAL_HAL_TL751X_HAL_H_

/* MCU market positioning suport LE Audio*/
#ifndef MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN
#define MARKET_POSITIONING_LE_AUDIO_SUPPORT_EN                  0
#endif

#define AES_CCM_DEC_US                                          200//TODO:need test

/* MCU hardware support Channel Sounding */
#define HARDWARE_CHANNEL_SOUNDING_SUPPORT_EN                    0

#define TNOP                                                    __asm__("nop")



#if 0   //Original value before B92 SDK Release
    #define PPM_IDX_LONG_SLEEP_MIN                              3   //300 ppm todo:A3 Verification is OK, but still observe for a period of time.(202300605)
#else       //For B92 SDK Release, give more early window for stable. TODO: calibrate this later.
    #define PPM_IDX_LONG_SLEEP_MIN                              4
#endif

#define PPM_IDX_SHORT_SLEEP_MIN                                 5   //500 ppm todo:A3 Verification is OK, but still observe for a period of time.(202300605)
#define PPM_IDX_MAX                                             10  //1000 ppm

//for aes module in ceva IP ,such as B91 and B92,aes module must switch address when use in BLE,other IC no need care.
#define HAL_CEVA_AES_ADDRESS_SWITCH

//different IC ,its rf dma tx/rx wptr/rptr maybe different,use macro instead of register.
#define HAL_REG_RF_DMA_FIFO_TX_RPTR                             (reg_rf_dma_tx_rptr(0))

#define HAL_REG_RF_DMA_FIFO_TX_WPTR                             (reg_rf_dma_tx_wptr(0))

#define HAL_BLE_STACK_RF_IRQ_MASK_SET                               do{reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX | BLMS_FLG_RF_CONN_DONE;}while(0)

#define HAL_BLE_STACK_RF_IRQ_MASK_CLEAR                             do{reg_rf_irq_mask = 0;}while(0)









#define HAL_CHIP_USE_CSEM_MODEM_IP                              1

#define STOP_RF_STATE_MACHINE

#define HAL_GET_RF_TX_IRQ                                       (rf_get_irq_status(FLD_RF_IRQ_MDM_TX_END))

#define HAL_GET_RF_RX_IRQ                                       (reg_rf_irq_status & FLD_RF_IRQ_RX)

#define HAL_CLEAR_RF_TX_IRQ                                     do {rf_clr_irq_status(FLD_RF_IRQ_MDM_TX_END);} while(0)

#define HAL_CLEAR_RF_RX_IRQ                                     do {rf_clr_irq_status(FLD_RF_IRQ_RX);} while(0)

#define HAL_CLEAR_RF_TX_RX_IRQ                                  do {rf_clr_irq_status(FLD_RF_IRQ_MDM_TX_END); rf_clr_irq_status(FLD_RF_IRQ_RX);} while(0)

#define HAL_WAIT_MODEM_SEQ_TIME                                 do {delay_us(5); }while(0)
                                                                /* The Tx IRQ signal appears before the last bit of the TX data packet TX_PATH_DLY us (10us), add margin 5us */
#define HAL_CSEM_IP_WAIT_TX_DONE                                do {unsigned int curr_time = clock_time(); while(!clock_time_exceed(curr_time, ONCA_CHIP_TX_PATH_DELAY + 5));} while(0)

#define HAL_CSEM_IP_RESET_BASEBAND                              do{rf_dma_reset();rf_clr_dig_logic_state();}while(0)

#define HAL_CSEM_IP_SET_DEFAULT_TX_DMA                          do{ble_rf_set_tx_dma(0, 17);}while(0)

#define HAL_CSEM_IP_SET_DEFAULT_RX_DMA                          do{ble_rf_set_rx_dma((u8*)glb_temp_rx_buff, 4);}while(0)

#define HAL_NONE_CSEM_IP_SET_DEFAULT_TX_DMA

#define HAL_NONE_CSEM_IP_SET_DEFAULT_RX_DMA

#define HAL_GET_RF_NESN                                         ((reg_rf_ll_pid_h & FLD_RF_NESN)>>4)







/**
 * @brief       This function serves to get the timestamp in 24M system timer.
 * @param[in]   none.
 * @return      none.
 */
static inline unsigned int hal_rf_get_rx_timestamp(void)
{
    return reg_rf_timestamp;
}

#define HAL_SKE_ENABLE                                              do{}while(0)
#endif /* STACK_HAL_HAL_TL751X_HAL_H_ */
