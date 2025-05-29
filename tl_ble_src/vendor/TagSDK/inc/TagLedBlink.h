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

#ifndef TAGSDK_INC_TAG_LED_BLINK_H_
#define TAGSDK_INC_TAG_LED_BLINK_H_

/*
 * APIs
 */

#if defined(TAG_ACCESSORY_OPTION_LED_BLINKING) && (TAG_ACCESSORY_OPTION_LED_BLINKING == 1)
/**
 * @brief   Query LED status
 *
 * @details This function queries LED operational status
 * @return  returns true if LED is blinking.
 *
 */
bool TagLedBlinkIsBlinking(void);

/**
 * @brief       Initialize LED control
 *
 * @details     This function initializes LED control
 * @return      TAG_ERROR_NONE for success. otherwise failure.
 *
 */
TagError_t TagLedBlinkCtrlInit(void);

/**
 * @brief       Reset LED control
 *
 * @details     This function reset LED control.
 *
 */
void TagLedBlinkCtrlReset(void);

/**
 * @brief       Start LED blinking
 *
 * @details     This function start LED blinking
 *
 * @return      returns TAG_ERROR_NONE for success. otherwise failure.
 *
 */
TagError_t TagLedBlinkCtrlStart(void);


/**
 * @brief       Stop LED blinking
 *
 * @details     This function stop LED blinking
 *
 * @return      returns TAG_ERROR_NONE for success. otherwise failure.
 *
 */
TagError_t TagLedBlinkCtrlStop(void);

#endif /* TAG_ACCESSORY_OPTION_LED_BLINKING */

#endif /* TAGSDK_INC_TAG_LED_BLINK_H_ */
