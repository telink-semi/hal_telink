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

#ifndef TAGSDK_PORT_INC_PORTLEDBLINK_H_
#define TAGSDK_PORT_INC_PORTLEDBLINK_H_

#include "TagConfig.h"

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
#include "TagErrorType.h"


/**
 * @brief   Init the LED hardware configuration.
 *
 * @details This function initialize LED hardware configuration.
 */
TagError_t PortLedBlinkHwCtrlInit(void);


/**
 * @brief   Turn on the LED which is used for blinking.
 *
 * @details This function turn on the LED which is used for blinking.
 */
TagError_t PortLedBlinkHwCtrlOn(void);


/**
 * @brief   Turn off the LED which is used for blinking.
 *
 * @details This function turn off the LED which is used for blinking.
 */
TagError_t PortLedBlinkHwCtrlOff(void);

#endif // TAG_ACCESSORY_OPTION_LED_BLINKING

#endif  /* TAGSDK_PORT_INC_PORTLEDBLINK_H_ */
