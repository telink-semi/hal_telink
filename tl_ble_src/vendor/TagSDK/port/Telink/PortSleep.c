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

//#include <pthread.h>
#include <stdint.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "TagConfig.h"

#include "TagDebug.h"
#include "TagErrorType.h"

#include "PortSleep.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
_attribute_ble_data_retention_ static uint16_t sleep_wake_lock_map;

bool PortBle_IsTagWakeLock(void)
{
    if(sleep_wake_lock_map)return true;
    return false;
}

TagError_t PortSleepWakeLock(PortWakelockType id)
{
    if ((sleep_wake_lock_map & (1 << id)))
    {
        return TAG_ERROR_NONE; //already locked
    }
    sleep_wake_lock_map |= (1 << id);
//    blc_pm_setSleepMask(PM_SLEEP_DISABLE);
    return TAG_ERROR_NONE;
}

TagError_t PortSleepWakeUnlock(PortWakelockType id)
{
    if (!(sleep_wake_lock_map & (1 << id)))
    {
        return TAG_ERROR_NONE; //already unlocked
    }

    sleep_wake_lock_map &= ~(1 << id);
//    if(sleep_wake_lock_map == 0)
//    {
//        blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_PERIPHR);
//    }
    return TAG_ERROR_NONE;
}

TagError_t PortSleepInit(void)
{
    return TAG_ERROR_NONE;
}

