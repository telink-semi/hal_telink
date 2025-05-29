/********************************************************************************************************
 * @file    app_port_freertos.c
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
#include "app_port_freertos.h"

#if (INTER_TEST_MODE == TEST_RAS_SERVER)

#if (FREERTOS_ENABLE)
    #include <FreeRTOS.h>
    #include <task.h>
    #include <timers.h>
    #include "semphr.h"
    #include "stack/ble/os_sup/os_sup.h"


_attribute_ble_data_retention_ static TaskHandle_t hBleTask = NULL;


_attribute_ble_data_retention_ volatile BaseType_t APP_isDeepRetnFlag = pdFALSE;

void vPreSleepProcessing(unsigned long uxExpectedIdleTime)
{
    (void)uxExpectedIdleTime;
    extern void app_process_power_management(u8 e, u8 * p, int n);
    app_process_power_management(0, 0, 0);
}

void vPostSleepProcessing(unsigned long uxExpectedIdleTime)
{
    (void)uxExpectedIdleTime;
    APP_isDeepRetnFlag = pdTRUE;
}

void vApplicationIdleHook(void)
{
    /* Doesn't do anything yet. */
    //deepretion
    if (APP_isDeepRetnFlag == pdTRUE) {
        APP_isDeepRetnFlag = pdFALSE;
    }
    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (BATT_CHECK_ENABLE)
    traceAPP_BAT_Task_BEGIN();
    /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
     500ms in the demo. Users can modify this time according to their needs.*/
    extern u32 lowBattDet_tick;
    if (battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000)) {
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
_attribute_ble_data_retention_ static TaskHandle_t hLedTask = NULL;

static void led_task(void *pvParameters)
{
    (void)pvParameters;
    while (1) {
        gpio_toggle(GPIO_LED_GREEN);
        traceAPP_LED_Task_Toggle();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_ledTaskCreate(void)
{
    BaseType_t ret;
    ret = xTaskCreate(led_task, "tLed", configMINIMAL_STACK_SIZE * 6, (void *)0, (tskIDLE_PRIORITY + 1), &hLedTask);
    configASSERT(ret == pdPASS);
}
    #endif //#if UI_LED_ENABLE


void                                                    os_take_mutex_sem(void);
void                                                    os_give_mutex_sem(void);
_attribute_ble_data_retention_ static SemaphoreHandle_t xBleSendDataMutex = NULL;

_attribute_ram_code_ void os_give_sem_from_isr(void)
{
    if (hBleTask == NULL) {
        return;
    }
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(hBleTask, &pxHigherPriorityTaskWoken);
}

_attribute_ram_code_ void os_give_sem(void)
{
    if (hBleTask == NULL) {
        return;
    }
    xTaskNotifyGive(hBleTask);
}

static volatile BaseType_t xErrorDetected = pdFALSE;

_attribute_ram_code_ void os_take_mutex_sem(void)
{
    traceAPP_MUTEX_Task_BEGIN();
    if (xSemaphoreTake(xBleSendDataMutex, portMAX_DELAY) != pdFAIL) {
        xErrorDetected = pdTRUE;
    }
}

_attribute_ram_code_ void os_give_mutex_sem(void)
{
    traceAPP_MUTEX_Task_END();
    if (xSemaphoreGive(xBleSendDataMutex) != pdPASS) {
        xErrorDetected = pdTRUE;
    }
}

/**
 * @brief   BLE Advertising data
 */
const u8 tbl_advData_os[] = {
    13,
    DT_COMPLETE_LOCAL_NAME,
    'c',
    's',
    '_',
    't',
    'e',
    'l',
    'i',
    'n',
    'k',
    '_',
    'o',
    's',
    2,
    DT_FLAGS,
    0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,
    DT_APPEARANCE,
    0x80,
    0x01, // 384, Generic Remote Control, Generic category
    5,
    DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x12,
    0x18,
    0x0F,
    0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp_os[] = {
    13,
    DT_COMPLETE_LOCAL_NAME,
    'c',
    's',
    '_',
    't',
    'e',
    'l',
    'i',
    'n',
    'k',
    '_',
    'o',
    's',
};

static void ble_task(void *pvParameters)
{
    (void)pvParameters;
    blc_ll_setAdvData(tbl_advData_os, sizeof(tbl_advData_os));
    blc_ll_setScanRspData(tbl_scanRsp_os, sizeof(tbl_scanRsp_os));
    blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        traceAPP_BLE_Task_BEGIN();
        ////////////////////////////////////// BLE entry /////////////////////////////////
        blc_sdk_main_loop();

        blc_prf_main_loop();

        extern void cs_procedure_ctrl(void);
        cs_procedure_ctrl();

        traceAPP_BLE_Task_END();
        //debug
        //uxTaskGetStackHighWaterMark(NULL);
    }
}

void app_BleTaskCreate(void)
{
    BaseType_t ret;
    blc_ll_registerGiveSemCb(os_give_sem_from_isr, os_give_sem); /* Register semaphore to ble module */
    blc_ll_registerMutexSemCb(os_take_mutex_sem, os_give_mutex_sem);

    xBleSendDataMutex = xSemaphoreCreateMutex();
    configASSERT(xBleSendDataMutex);

    ret = xTaskCreate(ble_task, "tble", configMINIMAL_STACK_SIZE * 24, (void *)0, (tskIDLE_PRIORITY + 2), &hBleTask);

    configASSERT(ret == pdPASS);
}


    #if ((configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS > 0) && (configSUPPORT_DYNAMIC_ALLOCATION == 1))
_attribute_ble_data_retention_ static TaskHandle_t hCpuTask = NULL;

static void cpu_task(void *pvParameters)
{
    char pWriteBuffer[512];
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        vTaskList((char *)&pWriteBuffer);
        printf("task_name   task_state priority stack tasK_num\n");
        printf("%s", pWriteBuffer);
    }
    vTaskDelete(NULL);
    return;
}
    #endif

void app_TaskCreate(void)
{
    blc_setOsSupEnable(1); /* Enable OS support */
    blc_ll_enOsPowerManagement_module();
    #if UI_LED_ENABLE
    app_ledTaskCreate();
    #endif
    app_BleTaskCreate();
    os_give_sem(); /* !!! important */

    blc_ll_enOsPowerManagement_module();
    #if ((configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS > 0) && (configSUPPORT_DYNAMIC_ALLOCATION == 1))
    xTaskCreate(cpu_task, "cpu_task", configMINIMAL_STACK_SIZE * 4, (void *)0, (tskIDLE_PRIORITY), &hCpuTask);
    #endif
    printf("freeRtos TaskCreate complete\r\n");
}

#endif //#if(FREERTOS_ENABLE)
#endif //#if (INTER_TEST_MODE == TEST_RAS_SERVER)