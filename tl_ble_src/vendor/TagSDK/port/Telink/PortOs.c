/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#if (FREERTOS_ENABLE)
    #include "tlk_riscv.h"
    #include <FreeRTOS.h>
    #include <task.h>
    #include <timers.h>
    #include <queue.h>
    #include <event_groups.h>
    #include "app_freertos.h"
#endif
#include "mbedtls/platform.h"
#include "TagConfig.h"

#include "PortOs.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG


void* TagMalloc(size_t size)
{
#if (FREERTOS_ENABLE)
    return pvPortMalloc(size);
#else

#endif
}

void TagFree(void *ptr)
{
#if (FREERTOS_ENABLE)
    vPortFree(ptr);
#else

#endif
}

PortTimerHandle_t PortTimerCreate(const char* timerName, uint32_t period, bool autoReload,
                                  void* const timerId, PortTimerCallbackFunction_t callbackFunc)
{

    return (PortTimerHandle_t) xTimerCreate(timerName,period,autoReload,timerId,callbackFunc);
}

void *PortTimerGetTimerId(const PortTimerHandle_t timer)
{
    return pvTimerGetTimerID(timer);
}

void PortTimerSetTimerId(PortTimerHandle_t timer, void *newId)
{
    vTimerSetTimerID(timer,newId);
}

TagError_t PortTimerChangePeriod(PortTimerHandle_t timer, uint32_t newPeriod, uint32_t blockTime)
{
    if(pdPASS == xTimerChangePeriod(timer,newPeriod,blockTime))
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;
}

TagError_t PortTimerStart(PortTimerHandle_t timer, uint32_t blockTime)
{
    if(pdPASS == xTimerStart(timer,blockTime))
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;}

TagError_t PortTimerStop(PortTimerHandle_t timer, uint32_t blockTime)
{
    if(pdPASS == xTimerStop(timer,blockTime))
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;}

TagError_t PortTimerDelete(PortTimerHandle_t timer, uint32_t blockTime)
{
    if(pdPASS == xTimerDelete(timer,blockTime))
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;
}

TagError_t PortTimerReset(PortTimerHandle_t timer, uint32_t blockTime)
{
    if(pdPASS == xTimerReset(timer,blockTime))
    return TAG_ERROR_NONE;
    else
    return TAG_ERROR_COMMON_BASE;
}

bool PortTimerIsTimerActive(PortTimerHandle_t timer)
{
    return xTimerIsTimerActive(timer);
}

PortQueueHandle_t PortQueueCreate(const unsigned long queueLength, const size_t itemSize)
{

    return (PortQueueHandle_t) xQueueCreate(queueLength,itemSize);
}

TagError_t PortQueueReset(PortQueueHandle_t queue)
{
        if(pdPASS == xQueueReset(queue))
        return TAG_ERROR_NONE;
        else
        return TAG_ERROR_COMMON_BASE;
}

TagError_t PortQueueSend(PortQueueHandle_t queue, const void *itemToQueue, uint32_t ticksToWait)
{
    if(pdPASS == xQueueSend(queue,itemToQueue,ticksToWait))
        return TAG_ERROR_NONE;
        else
        return TAG_ERROR_COMMON_BASE;
}

unsigned long PortQueueMessagesWaiting(PortQueueHandle_t queue)
{
    return uxQueueMessagesWaiting(queue);
}

TagError_t PortQueueReceive(PortQueueHandle_t queue, void *buffer, uint32_t ticksToWait)
{
    if(pdPASS ==  xQueueReceive(queue,buffer,ticksToWait))
        return TAG_ERROR_NONE;
        else
        return TAG_ERROR_COMMON_BASE;
}

unsigned long PortQueueSpacesAvailable(PortQueueHandle_t queue)
{
    return uxQueueSpacesAvailable(queue);
}

void PortQueueDelete(PortQueueHandle_t queue)
{
    vQueueDelete(queue);
}

PortEventGroupHandle_t PortEventGroupCreate(void)
{
    return (PortEventGroupHandle_t) xEventGroupCreate();
}


uint32_t PortEventGroupSetBits(PortEventGroupHandle_t eventGroup, const uint32_t bitsToSet)
{
    return (uint32_t) xEventGroupSetBits(eventGroup,bitsToSet);
}

uint32_t PortEventGroupWaitBits(PortEventGroupHandle_t eventGroup, uint32_t bitsToWaitFor,
                                bool clearOnExit, bool waitForAllBits, uint32_t ticksToWait)
{
    return (uint32_t) xEventGroupWaitBits(eventGroup,bitsToWaitFor,clearOnExit,waitForAllBits,ticksToWait);
}

void PortEventGroupDelete(PortEventGroupHandle_t eventGroup)
{
    vEventGroupDelete(eventGroup);
}

TagError_t PortTaskCreate(PortTaskFunction_t taskCode, const char * name, uint16_t stackDepth,
                          void *parameters, unsigned long priority, PortTaskHandle_t* handle)
{

       if(pdPASS ==  xTaskCreate(taskCode,name,stackDepth,parameters,priority,handle))
        return TAG_ERROR_NONE;
        else
        return TAG_ERROR_COMMON_BASE;
}

void PortTaskDelete(PortTaskHandle_t task)
{
    vTaskDelete(task);
}

void PortTaskDelay(const uint32_t ticksToDelay)
{
    vTaskDelay(ticksToDelay);
}

void PortPrepareMainTask()
{

}
