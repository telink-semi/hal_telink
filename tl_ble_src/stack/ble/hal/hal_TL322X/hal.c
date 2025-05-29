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
#if HARDWARE_CHANNEL_SOUNDING_SUPPORT_EN
    //when cs proceudre is ranging, modem can't be reset, it will affect cs phase continue between cs subevents.
    reg_n22_rst &= ~((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB)<<8));
    reg_n22_rst |= ((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB)<<8));
#else
    rf_clr_dig_logic_state();
#endif
}

_attribute_ram_code_ void debug_gpio_init(void)
{
#if (BOARD_SELECT == BOARD_322X_FPGA_KU115_SOLO)
    gpio_function_en(LED2);
    gpio_output_en(LED2);
    gpio_input_dis(LED2);

    gpio_function_en(LED3);
    gpio_output_en(LED3);
    gpio_input_dis(LED3);

    gpio_function_en(LED4);
    gpio_output_en(LED4);
    gpio_input_dis(LED4);

    gpio_set_level(LED2, 0);
    gpio_set_level(LED3, 0);
    gpio_set_level(LED4, 0);
#elif (BOARD_SELECT == BOARD_322X_EVK_C1T371A20)
    gpio_function_en(GPIO_LED_RED);
    gpio_output_en(GPIO_LED_RED);
    gpio_input_dis(GPIO_LED_RED);

    gpio_function_en(GPIO_LED_GREEN);
    gpio_output_en(GPIO_LED_GREEN);
    gpio_input_dis(GPIO_LED_GREEN);

    gpio_function_en(GPIO_LED_BLUE);
    gpio_output_en(GPIO_LED_BLUE);
    gpio_input_dis(GPIO_LED_BLUE);

    gpio_function_en(GPIO_LED_WHITE);
    gpio_output_en(GPIO_LED_WHITE);
    gpio_input_dis(GPIO_LED_WHITE);

    gpio_set_level(GPIO_LED_RED,   0);
    gpio_set_level(GPIO_LED_GREEN, 0);
    gpio_set_level(GPIO_LED_BLUE,  0);
    gpio_set_level(GPIO_LED_WHITE, 0);
#endif
    gpio_function_en(TLKAPI_DEBUG_GPIO_PIN);
    gpio_set_up_down_res(TLKAPI_DEBUG_GPIO_PIN, GPIO_PIN_PULLUP_1M);
    gpio_output_en(TLKAPI_DEBUG_GPIO_PIN);
    gpio_set_high_level(TLKAPI_DEBUG_GPIO_PIN);

#if DEBUG_GPIO_CHAN_ENABLE
    gpio_function_en(GPIO_CHN0);
    gpio_function_en(GPIO_CHN1);
    gpio_function_en(GPIO_CHN2);
    gpio_function_en(GPIO_CHN3);
    gpio_function_en(GPIO_CHN4);
    gpio_function_en(GPIO_CHN5);
    gpio_function_en(GPIO_CHN6);
    gpio_function_en(GPIO_CHN7);
    gpio_function_en(GPIO_CHN8);
    gpio_function_en(GPIO_CHN9);
    gpio_function_en(GPIO_CHN10);
    gpio_function_en(GPIO_CHN11);
    gpio_function_en(GPIO_CHN12);
    gpio_function_en(GPIO_CHN13);
    gpio_function_en(GPIO_CHN14);
    gpio_function_en(GPIO_CHN15);

    gpio_output_en(GPIO_CHN0);
    gpio_output_en(GPIO_CHN1);
    gpio_output_en(GPIO_CHN2);
    gpio_output_en(GPIO_CHN3);
    gpio_output_en(GPIO_CHN4);
    gpio_output_en(GPIO_CHN5);
    gpio_output_en(GPIO_CHN6);
    gpio_output_en(GPIO_CHN7);
    gpio_output_en(GPIO_CHN8);
    gpio_output_en(GPIO_CHN9);
    gpio_output_en(GPIO_CHN10);
    gpio_output_en(GPIO_CHN11);
    gpio_output_en(GPIO_CHN12);
    gpio_output_en(GPIO_CHN13);
    gpio_output_en(GPIO_CHN14);
    gpio_output_en(GPIO_CHN15);

    gpio_set_level(GPIO_CHN0, 0);
    gpio_set_level(GPIO_CHN1, 0);
    gpio_set_level(GPIO_CHN2, 0);
    gpio_set_level(GPIO_CHN3, 0);
    gpio_set_level(GPIO_CHN4, 0);
    gpio_set_level(GPIO_CHN5, 0);
    gpio_set_level(GPIO_CHN6, 0);
    gpio_set_level(GPIO_CHN7, 0);
    gpio_set_level(GPIO_CHN8, 0);
    gpio_set_level(GPIO_CHN9, 0);
    gpio_set_level(GPIO_CHN10, 0);
    gpio_set_level(GPIO_CHN11, 0);
    gpio_set_level(GPIO_CHN12, 0);
    gpio_set_level(GPIO_CHN13, 0);
    gpio_set_level(GPIO_CHN14, 0);
    gpio_set_level(GPIO_CHN15, 0);
#endif

    rf_enable_bb_debug();
}
