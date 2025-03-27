/********************************************************************************************************
 * @file    hal_internal.h
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
#ifndef STACK_HAL_HAL_INTERNAL_H_
#define STACK_HAL_HAL_INTERNAL_H_

#include "driver.h"
#include "ext_driver/driver_ext.h"
#include "hal.h"





/*  This code in RF irq and system irq put in RAM by force
 * Because of the flash resource contention problem, when the
 * flash access is interrupted by a higher priority interrupt,
 * the interrupt processing function cannot operate the flash(For Eagle)
*/
#ifndef STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION
#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION               1
#endif

/*  Limit some code from sram to flash
*/
#ifndef STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2
#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2            1
#endif

/* MCU hardware TX auto FIFO 4096 bytes limitation, only B91 issue */
#ifndef MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION
#define MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION                    0
#endif



/* Modem IP Information, current only TL751X used. */
#ifndef HAL_CHIP_USE_CSEM_MODEM_IP

    #define STOP_RF_STATE_MACHINE                                   do{reg_rf_ll_cmd = 0x80; }while(0)
    #define HAL_GET_RF_TX_IRQ                                       (reg_rf_irq_status & FLD_RF_IRQ_TX)
    #define HAL_GET_RF_RX_IRQ                                       (reg_rf_irq_status & FLD_RF_IRQ_RX)
    #define HAL_CLEAR_RF_TX_IRQ                                     do {reg_rf_irq_status = FLD_RF_IRQ_TX;} while(0)
    #define HAL_CLEAR_RF_RX_IRQ                                     do {reg_rf_irq_status = FLD_RF_IRQ_RX;} while(0)
    #define HAL_CLEAR_RF_TX_RX_IRQ                                  do {reg_rf_irq_status = FLD_RF_IRQ_TX | FLD_RF_IRQ_RX;} while(0)
    #define HAL_NONE_CSEM_IP_SET_DEFAULT_TX_DMA                     do{ble_rf_set_tx_dma(0, 17);}while(0)
    #define HAL_NONE_CSEM_IP_SET_DEFAULT_RX_DMA                     do{ble_rf_set_rx_dma((u8*)glb_temp_rx_buff, 4);}while(0)
    #define HAL_WAIT_MODEM_SEQ_TIME
    #define HAL_CSEM_IP_WAIT_TX_DONE
    #define HAL_CSEM_IP_RESET_BASEBAND
    #define HAL_CSEM_IP_SET_DEFAULT_TX_DMA
    #define HAL_CSEM_IP_SET_DEFAULT_RX_DMA

    #define rf_ble_csem_set_tx_rx_settle(x,y,z)
    #define rf_ble_csem_close_rx_continue_mode()

#endif



#if 0
/* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
 * but process method maybe different for new MCU, so move this function to HAL.
 * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
 * so we disable RF mask to prevent RF IRQ status sending to PLIC */
static inline void blt_hal_prevent_rf_irq_status_to_plic_module(void)
{
    HAL_BLE_STACK_RF_IRQ_MASK_CLEAR;
}

/* SiHui found problem on B91. same situation for other RISV MCU with PLIC module.
 * but process method maybe different for new MCU, so move this function to HAL.
 * FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
 * so we disable RF mask to prevent RF IRQ status sending to PLIC */
static inline void blt_hal_restore_rf_irq_status_to_plic_module(void)
{
    //HAL_BLE_STACK_RF_IRQ_MASK_SET;
    reg_rf_irq_mask = FLD_RF_IRQ_RX | FLD_RF_IRQ_TX | FLD_RF_IRQ_CMD_DONE  | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT | FLD_RF_IRQ_RX_CRC_2;

    #if 0 //can also solve above problem, but too complex, do not use
          //attention that it's tested on B91.
        irq_disable();
        plic_set_priority(IRQ_ZB_RT, 3);
        u32 claim = plic_interrupt_claim();
        my_dump_str_data(DBG_PRDADV_LOGIC, "debug 1", &claim, 4);
        plic_interrupt_complete(IRQ_ZB_RT);//complete interrupt
        plic_set_priority(IRQ_ZB_RT, 2);
        irq_enable();
    #endif
}

/* FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
 * so we disable RF mask to prevent RF IRQ status sending to PLIC */
blt_hal_prevent_rf_irq_status_to_plic_module();


/* FSM IRQ status will send to PLIC module, clear reg_rf_irq_status can not drop RF IRQ,
 * so we disable RF mask to prevent RF IRQ status sending to PLIC */
blt_hal_restore_rf_irq_status_to_plic_module();
#endif



#endif /* STACK_HAL_HAL_INTERNAL_H_ */
