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

#ifndef TAGSDK_PORT_INC_PORTBUTTON_H_
#define TAGSDK_PORT_INC_PORTBUTTON_H_

#include <stdbool.h>

/** @brief Check if operation button is pressed.
 *
 * @retval true Operation button is pressed.
 * @retval false Operation button is not pressed.
 *
 */
bool PortButtonIsPressed(void);

/** @brief Initialize button configuration
 *
 * @detail Tag is using only one button to communicate with users.
 *         This function configures one physical button and
 *         calls SystemButtonEventCallback function for the button events.
 *
 * @retval      TAG_ERROR_NONE for Success
 */
TagError_t PortButtonInit(void);

#endif /* TAGSDK_PORT_INC_PORTBUTTON_H_ */
