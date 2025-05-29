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

#ifndef TAGSDK_PORT_INC_PORTSLEEP_H_
#define TAGSDK_PORT_INC_PORTSLEEP_H_

#include "TagErrorType.h"

/** @enum PortWakelockType
 * @brief Wakelock holder type
 */
typedef enum {
    PORT_WAKELOCK_PERFORMANCE,  /**< Wakelock holder id for performance */
    PORT_WAKELOCK_SOUND,        /**< Wakelock holder id for sound */
    PORT_WAKELOCK_CRYPTO,       /**< Wakelock holder id for cryption */
    PORT_WAKELOCK_UWB,          /**< Wakelock holder id for uwb */
    PORT_WAKELOCK_MAX
} PortWakelockType;

/** @brief Initialize internal sleep state
 *
 * @details Initialize internal sleep state related locks
 *
 * @retval TAG_ERROR_NONE Successfully locked with the requested id
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 * @retval TAG_ERROR_NOT_SUPPORTED sleep lock operation is not supported
 *
 */
TagError_t PortSleepInit(void);

/** @brief Lock wake sleep with type id
 *
 * @details Block a device to sleep mode. If the device is locked by same id,
 *          it ignores and return sucess.
 *
 * @note it doesn't count wake lock
 *
 * @param[in] id wakelock holder type id
 *
 * @retval TAG_ERROR_NONE Successfully locked with the requested id
 * @retval TAG_ERROR_INVALID_ARG Wrong id type
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 * @retval TAG_ERROR_NOT_SUPPORTED sleep lock operation is not supported
 *
 */
TagError_t PortSleepWakeLock(PortWakelockType id);

/** @brief Unlock wake sleep with type id
 *
 * @details Release device wake lock held by the id. If the device is not locked by the id,
 *          it ignores and return sucess.
 *
 * @note it doesn't count wake lock
 *
 * @param[in] id wakelock holder type id
 *
 * @retval TAG_ERROR_NONE Successfully unlocked with the requested id
 * @retval TAG_ERROR_INVALID_ARG Wrong id type
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 * @retval TAG_ERROR_NOT_SUPPORTED sleep lock operation is not supported
 *
 */
TagError_t PortSleepWakeUnlock(PortWakelockType id);

#endif /* TAGSDK_PORT_INC_PORTSLEEP_H_ */
