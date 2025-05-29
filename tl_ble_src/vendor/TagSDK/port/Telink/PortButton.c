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

#if (FREERTOS_ENABLE)
    #include "tlk_riscv.h"
    #include <FreeRTOS.h>
    #include <task.h>
    #include <timers.h>
    #include <queue.h>
    #include <event_groups.h>
    #include "app_freertos.h"
  #include "stack/ble/os_sup/os_sup.h"
#endif

#include "TagConfig.h"
#include "TagBtnCallback.h"
#include "TagErrorType.h"
#include "TagDebug.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
#define BTN_LONG_PRS_TIME       2000
#define BTN_DOUBLE_PRS_TIME     300
#define SLEEP_TIME_MS   50
bool app_button_press_state(void);

/**
     * @brief       this function is used to detect if button pressed or released.
     * @param[in]   none
     * @return      none
     */
void Portble_btn_press(void)
{
    SystemButtonEventCallback(EVENT_BUTTON_PUSHED);
}

void Portble_btn_d_press(void)
{
    SystemButtonEventCallback(EVENT_BUTTON_DOUBLE_PUSHED);
}

void Portble_btn_l_press(void)
{
    SystemButtonEventCallback(EVENT_BUTTON_HOLD);
}

bool PortButtonIsPressed(void)
{
    return app_button_press_state();
}

TagError_t PortButtonInit(void)
{
    void app_buttonTaskCreate(void);
    app_buttonTaskCreate();
    return TAG_ERROR_NONE;
}
