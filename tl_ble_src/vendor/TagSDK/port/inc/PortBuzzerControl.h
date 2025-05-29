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

#ifndef TAGSDK_PORT_INC_PORTBUZZERCONTROL_H_
#define TAGSDK_PORT_INC_PORTBUZZERCONTROL_H_

#include "TagSoundPlayer.h"
#include "TagErrorType.h"

/**
 * @brief   Init the buzzer hardware.
 *
 * @details This function initialize the buzzer hardware.
 */
void PortBuzzerHwCtrlInit(void);

/**
 * @brief   Stop buzzer hardware.
 *
 * @details This function stops the buzzer hardware. The hardware should be dormant state.
 */
void PortBuzzerHwCtrlStop(void);

/**
 * @brief   Mute the buzzer hardware.
 *
 * @details This function make the buzzer hardware mute. The hardware could be active state.
 */
void PortBuzzerHwCtrlMute(void);

/**
 * @brief   Start the buzzer hardware
 *
 * @details This function make the buzzer hardware to play given frequency until PortBuzzerHwCtrlStop called.
 * @param[in]   freq    frequency of tone, in HZ. (e.g. 1000 for 1KHz)
 * @return  TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBuzzerHwCtrlStart(uint32_t freq);

/**
 * @brief   Set volume
 *
 * @details This function set volume of the buzzer hardware.
 *          The hardware should support two audibly distinguishable levels in addition to mute.
 * @param[in]   volume  volume level to set.
 * @return  TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBuzzerHwCtrlSetVolume(SoundVolume_t volume);

/**
 * @brief   device open
 *
 * @details This function open device of the buzzer hardware.
 */
void PortBuzzerOpen(void);

/**
 * @brief   device close
 *
 * @details This function close device of the buzzer hardware.
 */
void PortBuzzerClose(void);

/**
 * @brief   get open state
 *
 * @details This function return open state flag.
 */
bool PortBuzzerGetOpenstate(void);
#endif /* TAGSDK_PORT_INC_PORTBUZZERCONTROL_H_ */
