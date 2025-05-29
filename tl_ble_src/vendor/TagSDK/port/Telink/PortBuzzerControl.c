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

#include "tlkapi_debug.h"
#include "TagConfig.h"

#include "PortBuzzerControl.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
static bool open_state = false;
#define SIMU_BUZZ_GPIO GPIO_LED_BLUE
void PortBuzzerHwCtrlInit()
{
    //


}

void PortBuzzerHwCtrlMute()
{
    gpio_write(SIMU_BUZZ_GPIO, 0);
}

void PortBuzzerHwCtrlStop()
{
    PortBuzzerHwCtrlMute();
}

TagError_t PortBuzzerHwCtrlSetVolume(SoundVolume_t volume)
{
    switch (volume)
    {
        case SOUND_VOLUME_MUTE:
            break;
        case SOUND_VOLUME_NORMAL:
            break;
        case SOUND_VOLUME_LOUD:
            break;
        default:
            return TAG_ERROR_INVALID_ARG;
    }
    return TAG_ERROR_NONE;
}

TagError_t PortBuzzerHwCtrlStart(uint32_t freq)
{
    UNUSEDARG(freq)
    gpio_write(SIMU_BUZZ_GPIO, 1);
    return TAG_ERROR_NONE;
}

void PortBuzzerOpen(void)
{
    open_state = true;
}

void PortBuzzerClose(void)
{
    open_state = false;
}

bool PortBuzzerGetOpenstate(void)
{
    return open_state;
}
