/*
 * hal_dma.c
 *
 *  Created on: 2024
 *      Author: ADmin
 */

#include "stack/ble/hal/hal_internal.h"

_attribute_ram_code_ void blt_hal_reset_baseband(void)
{
    /* process all potential TX DMA conflict */
    rf_dma_reset();

    /*
     * Temporarily solve extended adv can not exit from while (!HAL_GET_RF_TX_IRQ)
     * need to find the root cause. todo by QiuWei. Reproducing step has been recorded.
     * situation 1: two extended adv sets. one is primary 1M and aux adv 1M, another is primary S8 and aux adv S8.
     * situation 2: connect the adv, but continue to send adv. i.e. not stop the adv. still keep two adv sets.
     * situation 3: switch connection phy to S8.
     * then wait some time and check whether occur that can not exit from the while.
     *
     * 20240531 SiHui & QiuWei & RongLu:
     * For B91 RF register loss when reset baseband. We found that more and more RF module was affected, include:
     * (1).1M/2M/Code PHY (2).rf common register (3).tx power index (4).fast settle
     * we think it's too difficult to handle them, need big change in stack code, increase some fsm prepare time(need adjust early set time,
     * which lead to RF bandwidth compromise). Most importantly, we are not sure if we neglect any other module affected, which would lead to
     * more serious problems compared to what we want solve. So we decide to abandon B91 reset baseband.
     * To improve the initial problem(stop FSM at coded PHY may cause next near task RF status error) what we want solve,
     * we need add software timeout for every RF status while check(such as "while(!(reg_rf_irq_status & FLD_RF_IRQ_TX))")
     */
    ble_rf_reset_baseband();
}
