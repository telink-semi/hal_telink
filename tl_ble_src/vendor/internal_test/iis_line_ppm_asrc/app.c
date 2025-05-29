/********************************************************************************************************
 * @file    app.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app.h"
#include "app_audio.h"


#if (INTER_TEST_MODE == TEST_PPM_ASRC_WITH_IIS_LINEIN)

/**
 * @brief       This function servers to deal with the usb command.
 * @param[in]   p - data sent from usb
 * @param[in]   len - the size of p
 * @return      0
 */
int my_usb_audio_debug(unsigned char *p, int len)
{
    my_dump_str_data(1, "my_usb_audio_debug receive\r\n", p, len);

    if (p[0] != 0x11) {
        return 1;
    }

    switch (p[1]) {
    case 0:
        break;
    default:
        break;
    }
    return 0;
}

/**
 * @brief       This function servers to set the time for the next timer interrupt.
 * @param[in]   type - type of timer,
 * @param[in]   mode - mode of timer,
 * @param[in]   init_tick - init the tick,
 * @param[in]   cap_tick - the time for the next timer interrupt,
 * @return      none.
 */
_attribute_ram_code_ void my_timer_set_mode(timer_type_e type, timer_mode_e mode, unsigned int init_tick, unsigned int cap_tick)
{
    u32 r              = core_interrupt_disable();
    reg_tmr_tick(type) = init_tick;
    reg_tmr_capt(type) = cap_tick;

    switch (type) {
    case TIMER0:
        reg_tmr_sta |= FLD_TMR_STA_TMR0; //clear irq status
        reg_tmr_ctrl0 &= (~FLD_TMR0_MODE);
        reg_tmr_ctrl0 |= mode;
        break;
    case TIMER1:
        reg_tmr_sta |= FLD_TMR_STA_TMR1; //clear irq status
        reg_tmr_ctrl0 &= (~FLD_TMR1_MODE);
        reg_tmr_ctrl0 |= (mode << 4);
        break;
    default:
        break;
    }
    core_restore_interrupt(r);
}

/**
 * @brief       This function servers to start the timer.
 * @param[in]   type - type of timer,
 * @return      none.
 */
_attribute_ram_code_ void my_timer_start(timer_type_e type)
{
    u32 r = core_interrupt_disable();
    switch (type) {
    case TIMER0:
        reg_tmr_ctrl0 |= FLD_TMR0_EN;
        break;
    case TIMER1:
        reg_tmr_ctrl0 |= FLD_TMR1_EN;
        break;
    default:
        break;
    }
    core_restore_interrupt(r);
}

/**
 * @brief       This function servers to stop the timer.
 * @param[in]   type - type of timer,
 * @return      none.
 */
_attribute_ram_code_ void my_timer_stop(timer_type_e type)
{
    u32 r = core_interrupt_disable();
    switch (type) {
    case TIMER0:
        reg_tmr_ctrl0 &= (~FLD_TMR0_EN);
        timer_clr_irq_status(FLD_TMR_STA_TMR0);
        break;
    case TIMER1:
        reg_tmr_ctrl0 &= (~FLD_TMR1_EN);
        timer_clr_irq_status(FLD_TMR_STA_TMR1);
        break;
    default:
        break;
    }
    core_restore_interrupt(r);
}

/**
 * @brief       System timer interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void stimer_irq_handler(void)
{
    log_task_begin_irq(1, SL01_IRQ);
    my_timer_set_mode(TIMER0, TIMER_MODE_SYSCLK, 0, (ASRC_OFFSET0_TICK * SYSTEM_TIMER_TICK_1US * sys_clk.pclk + SYSTEM_TIMER_TICK_1US / 2) / SYSTEM_TIMER_TICK_1US);
    my_timer_start(TIMER0);


    my_timer_set_mode(TIMER1, TIMER_MODE_SYSCLK, 0, (ASRC_OFFSET1_TICK * SYSTEM_TIMER_TICK_1US * sys_clk.pclk + SYSTEM_TIMER_TICK_1US / 2) / SYSTEM_TIMER_TICK_1US);
    my_timer_start(TIMER1);

    asrc_i2s_48k_ppm();

    systimer_clr_irq_status();

    //set_next_anchor
    async.task_tick += 10 * 1000 * SYSTEM_TIMER_TICK_1US;
    stimer_set_irq_capture(async.task_tick);
    log_task_end_irq(1, SL01_IRQ);
}
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

/**
 * @brief       Timer0 interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void timer0_irq_handler(void)
{
    log_task_begin_irq(1, SL01_dbug0);
    my_timer_stop(TIMER0);
    app_audio_input_task();

    log_task_end_irq(1, SL01_dbug0);
}
PLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)

/**
 * @brief       Timer1 interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void timer1_irq_handler(void)
{
    log_task_begin_irq(1, SL01_dbug1);
    my_timer_stop(TIMER1);
    app_audio_output_task();
    log_task_end_irq(1, SL01_dbug1);
}
PLIC_ISR_REGISTER(timer1_irq_handler, IRQ_TIMER1)

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
    //////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
    blc_debug_enableStackLog(STK_LOG_NONE);
    myudb_register_hci_debug_cb(my_usb_audio_debug);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();
    //////////////////////////// basic hardware Initialization  End /////////////////////////////////


    app_audio_init();

    plic_interrupt_enable(IRQ_SYSTIMER);
    plic_set_priority(IRQ_SYSTIMER, 3);
    stimer_set_irq_mask(FLD_SYSTEM_IRQ);

    plic_interrupt_enable(IRQ_TIMER0);
    plic_set_priority(IRQ_TIMER0, 1);

    plic_interrupt_enable(IRQ_TIMER1);
    plic_set_priority(IRQ_TIMER1, 1);

    async.task_tick = clock_time() + 1000 * SYSTEM_TIMER_TICK_1US;
    stimer_set_irq_capture(async.task_tick);
    analog_write_reg8(0x1c, 0xb0); // close charge auto mode.
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
//_attribute_ram_code_
int main_idle_loop(void)
{
    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif
    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)

    #endif


    return 0; //must return 0 due to SDP flow
}

u32 ledToggleTick = 0;

_attribute_no_inline_
    //_attribute_ram_code_
    void
    main_loop(void)
{
    if (clock_time_exceed(ledToggleTick, 1000 * 1000)) { //led toggle interval: 1000mS
        ledToggleTick = clock_time();
        gpio_toggle(GPIO_LED_RED);
    }
    main_idle_loop();
}

#endif
