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
#include "TagConfig.h"

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS

#include "TagCore.h"

#include "PortUwb.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

UwbPowerMode_t UwbMode = UWB_POWER_MODE_DEACTIVATE;

TagError_t PortUwbSetPower(UwbPowerMode_t mode, uint32_t uwbSessionId)
{
    // TODO
    // This is simple message handling routine to show how the TAG_CONFIG_USE_UWB_CHARACTERISTICS option works.
    // This function should be implemented for each devices if UWB supported.
    TAG_LOG_I("PortUwbSetPower called. mode=%d", mode);
    UwbMode = mode;

    return TAG_ERROR_NONE;
}

TagError_t PortUwbGetPower(UwbPowerMode_t* mode)
{
    // TODO
    // This is simple message handling routine to show how the TAG_CONFIG_USE_UWB_CHARACTERISTICS option works.
    // This function should be implemented for each devices if UWB supported.

    TAG_LOG_I("PortUwbGetPower called. mode=%d", UwbMode);
    *mode = UwbMode;
    return TAG_ERROR_NONE;
}

TagError_t PortUwbSetParam(UwbParam_t* param)
{
    // TODO
    // This is simple message handling routine to show how the TAG_CONFIG_USE_UWB_CHARACTERISTICS option works.
    // This function should be implemented for each devices if UWB supported.

    TAG_LOG_I("PortUwbSetParam called. ChannelId=%d", param->channelId);
    return TAG_ERROR_NONE;
}

TagError_t PortUwbInit(void)
{
    return TAG_ERROR_NONE;
}

TagError_t PortUwbSetPowerOffDueToRing(void)
{
    return TAG_ERROR_NONE;
}

void PortUwbSwup(void)
{
    return;
}

TagError_t PortUwbStartMgr(void)
{
    return TAG_ERROR_NONE;
}

#endif /* TAG_CONFIG_USE_UWB_CHARACTERISTICS */