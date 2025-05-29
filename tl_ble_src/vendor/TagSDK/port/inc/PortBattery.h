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

#ifndef TAGSDK_PORT_INC_PORTBATTERY_H_
#define TAGSDK_PORT_INC_PORTBATTERY_H_

#include <stdint.h>

#include "TagErrorType.h"

typedef struct {
    uint16_t soc;
    uint16_t voltage;
} percent_volt_data_t;

/** @brief Initialize battery state
 *
 * @details Initialize battery state
 *
 * @retval TAG_ERROR_NONE Successfully locked with the requested id
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 * @retval TAG_ERROR_NOT_SUPPORTED sleep lock operation is not supported
 *
 */
TagError_t PortBatteryInit(void);

/** @brief Get battery level.
 *
 * @details Get device battery level in range from 0 to 100.
 *          Battery level must be in range 0-100 value.
 *
 * @param[out] batt_lvl not-null, battery level output pointer
 *
 * @retval TAG_ERROR_NONE Successfully get battery level
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 *
 */
TagError_t PortBatteryGetLevel(uint32_t *batt_lvl);

#endif /* TAGSDK_PORT_INC_PORTBATTERY_H_ */
