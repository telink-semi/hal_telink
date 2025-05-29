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

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "TagConfig.h"

#include "TagErrorType.h"
#include "battery_check.h"


#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
#define  BATTERY_FULL_MV 3000.0
extern u16 batt_vol_mv;
TagError_t PortBatteryInit(void)
{
    battery_set_detect_enable(1);
    return TAG_ERROR_NONE;
}

TagError_t PortBatteryGetLevel (u32 *batt_lvl)
{
    if (!batt_lvl)
    {
        return TAG_ERROR_INVALID_ARG;
    }
//    batt_vol_mv = 3000;
    app_battery_power_check(2000);
    *batt_lvl = batt_vol_mv * 99 / BATTERY_FULL_MV;
    return TAG_ERROR_NONE;
}
