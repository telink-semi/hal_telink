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

#ifndef TAGSDK_INC_TAGAUTHSERVICE_H_

#define TAGSDK_INC_TAGAUTHSERVICE_H_

#define ERROR_AUTH_SERVICE 0x81

#include "PortBle.h"

/**
 * @brief Initialize authentication service
 *
 * @details This function initialize data for authentication service
 * @return return 0 for success. negative values for failure.
 *
 */
TagError_t AuthServiceInit(void);

/**
 * @brief Authentication Service callback
 *
 * @details read callback for authentication service
 * @param[in] event Context of ble event
 * @return return 0 for success. other values for failure.
 *
 */
TagBleError_t AuthServiceReadCallback(BleEvent *event);

/**
 * @brief Authentication Service callback
 *
 * @details This function initialize data for authentication service
 * @param[in] event Context of ble event
 * @return return 0 for success. other values for failure.
 *
 */
TagBleError_t AuthServiceWrittenCallback(BleEvent *event);

/**
 * @brief Authentication Service Post callback
 *
 * @details This function do post process for authentication service
 * @param[in] event Context of ble event
 *
 */
void AuthServiceWrittenPostCallback(TagTaskWorkParam bleEvent);

#endif /* TAGSDK_INC_TAGAUTHSERVICE_H_ */
