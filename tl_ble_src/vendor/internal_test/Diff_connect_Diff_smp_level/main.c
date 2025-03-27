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

#if (INTER_TEST_MODE == TEST_DIFF_CON_DIFF_SMP_LEVEL)

#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "semphr.h"
#include "stack/ble/os_sup/os_sup.h"
#include "app_ui.h"
#include "tlk_riscv.h"
_attribute_ble_data_retention_ static TaskHandle_t hBleTask = NULL;
_attribute_ble_data_retention_ volatile BaseType_t APP_isDeepRetnFlag = pdFALSE;
#if UI_LED_ENABLE
static void led_task(void *pvParameters);

#endif
static void ble_task(void *pvParameters);
#if UI_KEYBOARD_ENABLE
static void keyboard_task( void *pvParameters );
_attribute_ble_data_retention_ static TaskHandle_t hKeyTask = NULL;
#endif
static void os_give_sem_from_isr(void);
static void os_give_sem(void);
void os_take_mutex_sem(void);
void os_give_mutex_sem(void);
_attribute_ble_data_retention_ static SemaphoreHandle_t xBleSendDataMutex = NULL;
;
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
#if (FREERTOS_ENABLE)
PLIC_ISR_REGISTER_OS(rf_irq_handler, IRQ_ZB_RT)
#else
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)
#endif
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
#if (FREERTOS_ENABLE)
PLIC_ISR_REGISTER_OS(stimer_irq_handler, IRQ_SYSTIMER)
#else
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)
#endif

#if (FREERTOS_ENABLE && UI_KEYBOARD_ENABLE)
_attribute_ram_code_sec_noinline_ void gpio_irq_handler(void)
{
    gpio_clr_irq_status(FLD_GPIO_IRQ_CLR);
    plic_interrupt_disable(IRQ_GPIO);
    xTaskResumeFromISR(hKeyTask);
}
PLIC_ISR_REGISTER_OS(gpio_irq_handler, IRQ_GPIO)
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
        CCLK_32M_HCLK_32M_PCLK_16M;
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        sys_init(DCDC_1P4_LDO_2P0, VBAT_MAX_VALUE_GREATER_THAN_3V6, GPIO_VOLTAGE_3V3, INTERNAL_CAP_XTAL24M);
        wd_32k_stop();
        CCLK_32M_HCLK_32M_PCLK_16M;
    #elif (MCU_CORE_TYPE == MCU_CORE_TL751X)
        sys_init(VBAT_MAX_VALUE_GREATER_THAN_3V6);
        CCLK_96M_HCLK_96M_PCLK_24M_MSPI_48M;
    #elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
        sys_init(LDO_0P94_LDO_1P8,VBAT_MAX_VALUE_GREATER_THAN_3V6,INTERNAL_CAP_XTAL24M);
        gpio_set_up_down_res(GPIO_SWS, GPIO_PIN_PULLUP_1M);
        wd_32k_stop();
        PLL_240M_CCLK_40M_HCLK_40M_PCLK_40M_MSPI_40M;
    #endif

    /* detect if MCU is wake_up from deep retention mode */
    int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp



    rf_drv_ble_init();

    gpio_init(!deepRetWakeUp);

    if( deepRetWakeUp ){ //MCU wake_up from deepSleep retention mode
        #if (FREERTOS_ENABLE)
        extern void vPortRestoreTick(void);
        vPortRestoreTick();
        #endif
        user_init_deepRetn ();
    }
    else{ //MCU power_on or wake_up from deepSleep mode
        user_init_normal();
    }

    irq_enable();
#if (FREERTOS_ENABLE)
    extern void vPortRestoreTask(void);
    if(deepRetWakeUp){  //  Tasks do not support deep retention, due to RAM limitation
        configPOST_SLEEP_PROCESSING(0);
        vPortRestoreTask();
    }
    else{
        blc_setOsSupEnable(1); /* Enable OS support */
        blc_ll_registerGiveSemCb(os_give_sem_from_isr, os_give_sem); /* Register semaphore to ble module */
        blc_ll_registerMutexSemCb(os_take_mutex_sem, os_give_mutex_sem);
        xBleSendDataMutex = xSemaphoreCreateMutex();
        configASSERT( xBleSendDataMutex );
        #if (PM_DEEPSLEEP_RETENTION_ENABLE)
            #if (MCU_CORE_TYPE == MCU_CORE_B91)
               #error "B91 deep retention 64K SRAM not enough !!!"
            #elif (MCU_CORE_TYPE == MCU_CORE_B92)
                blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW96K);
            #endif
        #endif

        blc_ll_enOsPowerManagement_module();
        #if UI_LED_ENABLE
        xTaskCreate( led_task,     "tLed",  512,   (void*)0, (tskIDLE_PRIORITY+1), 0 );
        #endif
        xTaskCreate( ble_task,     "tble", 1024,   (void*)0, (tskIDLE_PRIORITY+2), &hBleTask );
        #if UI_KEYBOARD_ENABLE
        xTaskCreate( keyboard_task, "tkey", 512,   (void*)0, (tskIDLE_PRIORITY+1), &hKeyTask );
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
    (void)uxExpectedIdleTime;
    extern void app_process_power_management(void);
    app_process_power_management();

}

void vPostSleepProcessing( unsigned long uxExpectedIdleTime )
{
    (void)uxExpectedIdleTime;
    APP_isDeepRetnFlag = pdTRUE;
}

void vApplicationIdleHook( void )
{
    /* Doesn't do anything yet. */
    //deepretion
    if(APP_isDeepRetnFlag == pdTRUE)
    {
        APP_isDeepRetnFlag = pdFALSE;
        #if UI_KEYBOARD_ENABLE
        if( eTaskGetState( hKeyTask ) == eSuspended )
        {
            vTaskResume(hKeyTask);
        }

        #endif
    }
    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (BATT_CHECK_ENABLE)
    traceAPP_BAT_Task_BEGIN();
    /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
     500ms in the demo. Users can modify this time according to their needs.*/
    extern u32  lowBattDet_tick;
    if(battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000) ){
        lowBattDet_tick = clock_time();
        user_battery_power_check(BAT_DEEP_THRESHOLD_MV);
    }
    traceAPP_BAT_Task_END();
    #endif

    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
}

#if UI_LED_ENABLE
static void led_task(void *pvParameters)
{
    (void)pvParameters;
    while(1)
    {
        gpio_toggle(GPIO_LED_BLUE);
        traceAPP_LED_Task_Toggle();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

#endif
/**
 * @brief   BLE Advertising data
 */
const u8    tbl_advData_os[] = {
     16, DT_COMPLETE_LOCAL_NAME,                'p', 'e', 'r', 'i', 'p', 'h', 'r', '_', 'd', 'e', 'm', 'o','_','O', 'S',
     2,  DT_FLAGS,                              0x05,                   // BLE limited discoverable mode and BR/EDR not supported
     3,  DT_APPEARANCE,                         0x80, 0x01,             // 384, Generic Remote Control, Generic category
     5,  DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, 0x12, 0x18, 0x0F, 0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8    tbl_scanRsp_os [] = {
     16, DT_COMPLETE_LOCAL_NAME,                'p', 'e', 'r', 'i', 'p', 'h', 'r', '_', 'd', 'e', 'm', 'o','_','O', 'S',
};

static void ble_task( void *pvParameters )
{
    (void)pvParameters;
    blc_ll_setAdvData(tbl_advData_os, sizeof(tbl_advData_os));
    blc_ll_setScanRspData(tbl_scanRsp_os, sizeof(tbl_scanRsp_os));
    blc_ll_setAdvParam(ADV_INTERVAL_100MS, ADV_INTERVAL_200MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
    //blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);
    while(1)
    {
        ulTaskNotifyTake(pdTRUE,  portMAX_DELAY);
        traceAPP_BLE_Task_BEGIN();
        ////////////////////////////////////// BLE entry /////////////////////////////////
        blc_sdk_main_loop();
        traceAPP_BLE_Task_END();
        //debug
        //uxTaskGetStackHighWaterMark(NULL);
    }
}
#if UI_KEYBOARD_ENABLE
void proc_keyboardSupend (u8 e, u8 *p, int n)
{
    (void)e;
    (void)p;
    (void)n;
    APP_isDeepRetnFlag = pdTRUE;
    DBG_CHN6_HIGH;
    DBG_CHN6_LOW;
}
static void keyboard_task( void *pvParameters )
{
    (void)pvParameters;
    while(1)
    {
         traceAPP_KEY_Task_BEGIN();
        proc_keyboard (0, 0, 0);
         traceAPP_KEY_Task_END();
        //debug
        if(key_not_released || scan_pin_need)
        {
            vTaskDelay(pdMS_TO_TICKS(40));
        }
        else
        {
            plic_interrupt_enable(IRQ_GPIO);
            vTaskSuspend( NULL );
        }

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
static volatile BaseType_t xErrorDetected = pdFALSE;
_attribute_ram_code_
void os_take_mutex_sem(void)
{
    traceAPP_MUTEX_Task_BEGIN();
    if( xSemaphoreTake( xBleSendDataMutex, portMAX_DELAY ) != pdFAIL )
    {
        xErrorDetected = pdTRUE;
    }
}
_attribute_ram_code_
void os_give_mutex_sem(void)
{
    traceAPP_MUTEX_Task_END();
    if( xSemaphoreGive( xBleSendDataMutex ) != pdPASS )
    {
        xErrorDetected = pdTRUE;
    }
}
#endif /* End of FREERTOS_ENABLE */

#endif
