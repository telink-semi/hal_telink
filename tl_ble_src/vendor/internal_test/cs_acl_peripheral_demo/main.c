/********************************************************************************************************
 * @file    main.c
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
#include "app.h"

#if (INTER_TEST_MODE == TEST_CS_ACL_PERIPHERAL)
#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "semphr.h"
#include "stack/ble/os_sup/os_sup.h"
#include "app_ui.h"

_attribute_ble_data_retention_ static TaskHandle_t hBleTask = NULL;
_attribute_ble_data_retention_ volatile BaseType_t APP_isDeepRetnFlag = pdFALSE;
#if UI_LED_ENABLE
static void led_task(void *pvParameters);
#endif
static void ble_task(void *pvParameters);
#if UI_KEYBOARD_ENABLE
static void keyboardCallback( TimerHandle_t xTimer );
_attribute_ble_data_retention_ static TimerHandle_t xKeyTimer = NULL;
#endif
static void os_give_sem_from_isr(void);
static void os_give_sem(void);

#endif



/**
 * @brief       BLE RF interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void rf_irq_handler(void)
{
    DBG_CHN14_HIGH;

    blc_sdk_irq_handler ();

    DBG_CHN14_LOW;
}
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
/**
 * @brief       System timer interrupt handler.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void stimer_irq_handler(void)
{
    DBG_CHN15_HIGH;

    blc_sdk_irq_handler ();

    DBG_CHN15_LOW;
}
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)

#if (FREERTOS_ENABLE && UI_KEYBOARD_ENABLE)
_attribute_ram_code_sec_noinline_ void gpio_irq_handler(void)
{
    gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
    plic_interrupt_disable(IRQ_GPIO);
    if(!APP_isDeepRetnFlag)
    {
        xTimerStartFromISR( xKeyTimer, 0UL );
    }
}
#endif
/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
    DBG_CHN0_LOW;

    /* this function must called before "sys_init()" when:
     * (1). For all IC: using 32K RC for power management,
       (2). For B91 only: even no power management */
    blc_pm_select_internal_32k_crystal();

    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        sys_init(DCDC_1P4_LDO_1P8, VBAT_MAX_VALUE_GREATER_THAN_3V6,INTERNAL_CAP_XTAL24M);
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
        wd_32k_stop();          //todo: Deep wakeup shall not call wd stop after A1. Jaguar A0 have problem on PM now, so call 32k watchdog stop here now. See <Skype-B91m driver: 2022-10-25>
    #endif

    /* detect if MCU is wake_up from deep retention mode */
    int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

    CCLK_32M_HCLK_32M_PCLK_16M;

    rf_drv_ble_init();

    gpio_init(!deepRetWakeUp);

    if( deepRetWakeUp ){ //MCU wake_up from deepSleep retention mode
        user_init_deepRetn ();
    }
    else{ //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
    }

    irq_enable();
#if (FREERTOS_ENABLE)
    extern void vPortRestoreTask();
    if(deepRetWakeUp){  //  Tasks do not support deep retention, due to RAM limitation
        configPOST_SLEEP_PROCESSING(0);
        vPortRestoreTask();
    }
    else{
        blc_setOsSupEnable(1); /* Enable OS support */
        blc_ll_registerGiveSemCb(os_give_sem_from_isr, os_give_sem); /* Register semaphore to ble module */

        #if (PM_DEEPSLEEP_RETENTION_ENABLE)
            #if (MCU_CORE_TYPE == MCU_CORE_B91)
               #error "B91 deep retention 64K SRAM not enough !!!"
            #elif (MCU_CORE_TYPE == MCU_CORE_B92)
                blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW96K);
            #endif
        #endif

        blc_ll_enOsPowerManagement_module();
        #if UI_LED_ENABLE
        xTaskCreate( led_task, "tLed", configMINIMAL_STACK_SIZE,   (void*)0, (tskIDLE_PRIORITY+1), 0 );
        #endif
        xTaskCreate( ble_task, "tble", configMINIMAL_STACK_SIZE*4, (void*)0, (tskIDLE_PRIORITY+2), &hBleTask );
        #if UI_KEYBOARD_ENABLE
        xKeyTimer = xTimerCreate( "KeyTimer", pdMS_TO_TICKS(40), pdTRUE, NULL, keyboardCallback );
        if( xKeyTimer != NULL )
        {
            xTimerStart( xKeyTimer, 0UL );
        }
        #endif
        os_give_sem(); /* !!! important */

        vTaskStartScheduler();
    }
#else

    while(1)
    {
        main_loop ();
    }
#endif


    return 0;
}

/**
 *******************************************************************************
 *
 * OS Start
 *
 *******************************************************************************
 */
#if FREERTOS_ENABLE
void vPreSleepProcessing( unsigned long uxExpectedIdleTime )
{
    extern void app_process_power_management(void);
    app_process_power_management();

}

void vPostSleepProcessing( unsigned long uxExpectedIdleTime )
{
    //todo
}

void vApplicationIdleHook( void )
{
    /* Doesn't do anything yet. */
    //deepretion
    if(APP_isDeepRetnFlag == pdTRUE)
    {
        APP_isDeepRetnFlag = pdFALSE;
        #if UI_KEYBOARD_ENABLE
        if(key_not_released || scan_pin_need)
        {
            xTimerStart( xKeyTimer, 0UL );
        }
        #endif
    }

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
}

#if UI_LED_ENABLE
static void led_task(void *pvParameters)
{
    while(1)
    {
        gpio_toggle(GPIO_LED_BLUE);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
#endif

static void ble_task( void *pvParameters )
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,  portMAX_DELAY);
        DBG_CHN3_HIGH;
        ////////////////////////////////////// BLE entry /////////////////////////////////
        blc_sdk_main_loop();
        DBG_CHN3_LOW;
    }
}
#if UI_KEYBOARD_ENABLE
void proc_keyboardSupend (u8 e, u8 *p, int n)
{
    APP_isDeepRetnFlag = pdTRUE;
    plic_interrupt_enable(IRQ_GPIO);
    extern void proc_keyboard (u8 e, u8 *p, int n);
    proc_keyboard(0, 0, 0);
}

static void keyboardCallback( TimerHandle_t xTimer )
{
    /* The parameter is not used in this case. */
    ( void ) xTimer;
    ////////////////////////////////////// UI entry /////////////////////////////////
    proc_keyboard (0, 0, 0);
    if(key_not_released || scan_pin_need)
    {
        //todo
    }
    else
    {
        xTimerStop( xKeyTimer, 0UL );
        plic_interrupt_enable(IRQ_GPIO);
    }
}
#endif

_attribute_ram_code_
void os_give_sem_from_isr(void)
{
    if(hBleTask == NULL)
        return;
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(hBleTask, &pxHigherPriorityTaskWoken);
}

_attribute_ram_code_
void os_give_sem(void)
{
    if(hBleTask == NULL)
        return;
    xTaskNotifyGive(hBleTask);

}
#endif
#endif /* End of FREERTOS_ENABLE */
