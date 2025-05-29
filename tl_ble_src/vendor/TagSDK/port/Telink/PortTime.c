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

#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
//#include <sys/sysinfo.h>
#if (FREERTOS_ENABLE)
    #include "tlk_riscv.h"
    #include <FreeRTOS.h>
    #include <task.h>
    #include "app_freertos.h"
#endif

#include "TagConfig.h"

#include "TagDebug.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

_attribute_ble_data_retention_ uint64_t secondOffset = 1609459200; // 2021/01/01
_attribute_ble_data_retention_ uint32_t rtcMsOld = 0;


void PortTimeSetRtcTime(uint64_t seconds)
{
#if FREERTOS_ENABLE
    TickType_t currentTickCount = xTaskGetTickCount();
    rtcMsOld = currentTickCount;
#else
    uint32_t c_us = clock()/SYSTEM_TIMER_TICK_1US;
    rtcMsOld = c_us/1000;
#endif
    secondOffset = seconds;
}

uint64_t PortTimeGetRtcTime(void)
{
#if FREERTOS_ENABLE
    TickType_t currentTickCount = xTaskGetTickCount();
    return (secondOffset + (currentTickCount -rtcMsOld)  * portTICK_PERIOD_MS /1000);
#else
    uint32_t c_us = clock()/SYSTEM_TIMER_TICK_1US;
     return (secondOffset + (c_us/1000 - rtcMsOld)/1000 );
#endif
}

uint32_t PortTimeGetBootTimeMs(void)
{
#if FREERTOS_ENABLE
    TickType_t currentTickCount = xTaskGetTickCount();
    return currentTickCount * portTICK_PERIOD_MS;
#else
    uint32_t c_us = clock()/SYSTEM_TIMER_TICK_1US;
    return( c_us/1000);
#endif
}

uint32_t PortTimeGetBootTime(void)
{
    return PortTimeGetBootTimeMs()/1000;
}

TagError_t PortTimeInit(void)
{
    return TAG_ERROR_NONE;
}

void PortTimeDelayBusy(uint32_t ms)
{
}
