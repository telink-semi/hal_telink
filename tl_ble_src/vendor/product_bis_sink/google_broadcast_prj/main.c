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
#include "../bis_sink_config.h"
#if (PRODUCT_BIS_SINK_SELECT == PRODUCT_GOOGLE_BROADCAST_SINK)

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app.h"

#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#include "semphr.h"
#include "stack/ble/os_sup/os_sup.h"

_attribute_ble_data_retention_ static TaskHandle_t hBleTask = NULL;

static void led_task(void *pvParameters);
static void led1_task(void *pvParameters);
static void ble_task(void *pvParameters);

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

/**
 * @brief       This is main function
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ int main(void)
{
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

    CCLK_96M_HCLK_48M_PCLK_24M;

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
        vPortRestoreTask();
    }
    else{
        blc_setOsSupEnable(1); /* Enable OS support */
        blc_ll_registerGiveSemCb(os_give_sem_from_isr, os_give_sem); /* Register semaphore to ble module */

        xTaskCreate( led_task, "tLed", configMINIMAL_STACK_SIZE,   (void*)0, (tskIDLE_PRIORITY+1), 0 );
        xTaskCreate( led1_task,"tLed1",configMINIMAL_STACK_SIZE,   (void*)0, (tskIDLE_PRIORITY+1), 0 );
        xTaskCreate( ble_task, "tble", configMINIMAL_STACK_SIZE*4, (void*)0, (tskIDLE_PRIORITY+2), &hBleTask );

        os_give_sem(); /* !!! important */

        vTaskStartScheduler();
    }
#else

    while(1)
    {
        main_loop ();
    }

#endif
}

#if FREERTOS_ENABLE
static void led_task(void *pvParameters)
{
    while(1)
    {
        gpio_toggle(GPIO_LED_RED);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void led1_task(void *pvParameters)
{
    while(1)
    {
        gpio_toggle(GPIO_LED_BLUE);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void ble_task( void *pvParameters )
{
    while(1)
    {
        main_loop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

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

#endif /* End of FREERTOS_ENABLE */


#endif
