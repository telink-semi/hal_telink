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

#if (INTER_TEST_MODE == TEST_ONCA)

#if(FREERTOS_ENABLE)
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include "semphr.h"
#include "stack/ble/os_sup/os_sup.h"
#include "tlk_riscv.h"
_attribute_ble_data_retention_ static TaskHandle_t hBleTask = NULL;
_attribute_ble_data_retention_ volatile BaseType_t APP_isDeepRetnFlag = pdFALSE;
static void led_task(void *pvParameters);
static void ble_task(void *pvParameters);
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
    DBG_CHN13_HIGH;
    blc_sdk_irq_handler ();
    DBG_CHN13_LOW;
}

#if (FREERTOS_ENABLE)
PLIC_ISR_REGISTER_OS(stimer_irq_handler, IRQ_SYSTIMER)
#else
PLIC_ISR_REGISTER(stimer_irq_handler, IRQ_SYSTIMER)
#endif

void ble_stx_test(void);
#if (MCU_CORE_TYPE == MCU_CORE_B92)
#define LED1                    GPIO_PD0
#define LED2                    GPIO_PD1
#elif (MCU_CORE_TYPE == MCU_CORE_TL751X)
#define LED1                    GPIO_PF1
#define LED2                    GPIO_PE7
//#define LED1                  GPIO_PA5
//#define LED2                  GPIO_PA6
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
        wd_32k_stop();
        CCLK_96M_HCLK_48M_PCLK_24M;
    #elif (MCU_CORE_TYPE == MCU_CORE_TL751X)
        sys_init(VBAT_MAX_VALUE_GREATER_THAN_3V6);
        wd_32k_stop();
        wd_stop();
        #if(ONCA_CHIP_VESION == ONCA_CHIP_A0)
        pm_set_avdd1(PM_AVDD1_VOLTAGE_1V050);
        pm_set_dvdd2(PM_DVDD2_VOLTAGE_0V750);
        pm_set_avdd2(PM_AVDD2_VOLTAGE_2V346);
        pm_set_dvdd1(PM_DVDD1_VOLTAGE_0V725);

        #endif
        CCLK_96M_HCLK_96M_PCLK_24M_MSPI_48M;
    #endif

    rf_enable_bb_debug();

    gpio_function_en(LED1);
    gpio_function_en(LED2);
    gpio_output_en(LED1);
    gpio_output_en(LED2);

    rf_drv_ble_init();
    user_init_normal();
    irq_enable();

#if (FREERTOS_ENABLE)
    mtime_clk_init(CLK_32K_RC);
    /* 3: enable machine time interrupt */
    core_mie_enable(FLD_MIE_MTIE);

    blc_setOsSupEnable(1); /* Enable OS support */
    blc_ll_registerGiveSemCb(os_give_sem_from_isr, os_give_sem); /* Register semaphore to ble module */
    blc_ll_registerMutexSemCb(os_take_mutex_sem, os_give_mutex_sem);
    xBleSendDataMutex = xSemaphoreCreateMutex();
    configASSERT( xBleSendDataMutex );

//  blc_ll_enOsPowerManagement_module();
    xTaskCreate( led_task,     "tLed",  512,   (void*)0, (tskIDLE_PRIORITY+1), 0 );
    xTaskCreate( ble_task,     "tble", 1024,   (void*)0, (tskIDLE_PRIORITY+2), &hBleTask );
    os_give_sem(); /* !!! important */
    vTaskStartScheduler();

#else

    while(1)
    {
        main_loop ();

        static u32 cpuRunTime = 0;
        if(clock_time_exceed(cpuRunTime, 500*1000))
        {
            cpuRunTime = clock_time();
            gpio_toggle(LED2);
        }
    }
#endif
    return 0;
}

#define TEST_MAC                0xEE
#define TEST_CHN                37      // 37/38/39 adv channel

#define BLE_ACCESS_CODE         0xd6be898e//0xA5CC336A//0xd6be898e//

rf_packet_adv_t debug_pkt_adv = {
        rf_tx_packet_dma_len(sizeof (rf_packet_adv_t) - 4),     // dma_len
        LL_TYPE_ADV_NONCONN_IND, 0, 0, 0, 0,                    // type
        sizeof (rf_packet_adv_t) - 6,       // rf_len
        {TEST_MAC, TEST_MAC, TEST_MAC, TEST_MAC, TEST_MAC, TEST_MAC},   // advA
        // data
        {0},
};

void ble_stx_test(void)
{
    rf_set_ble_crc_adv ();
    rf_access_code_comm(BLE_ACCESS_CODE);

    rf_set_tx_dma(2,128);

    unsigned long tx_begin_tick;


    tx_begin_tick = stimer_get_tick();

    rf_set_tx_rx_off_auto_mode();
    rf_set_ble_chn (TEST_CHN);  //2402

    debug_pkt_adv.data[0] ++;


    rf_start_stx ((void *)&debug_pkt_adv, clock_time() + 100);

    delay_us(2000);  //2mS is enough for packet sending

    if(rf_get_irq_status(FLD_RF_IRQ_TX))
    {
        rf_clr_irq_status(FLD_RF_IRQ_TX);
        gpio_toggle(LED1);
    }

    delay_ms(10);

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

    }
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
}

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

static void led_task(void *pvParameters)
{
    (void)pvParameters;
    while(1)
    {
        gpio_toggle(LED2);
        traceAPP_LED_Task_Toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
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

#endif

#endif
