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

 #include <stdint.h>

#ifndef TAGSDK_PORT_INC_PORTTIME_H_
#define TAGSDK_PORT_INC_PORTTIME_H_

#define PRI_UINT64_FMT(value) ((uint32_t) (value>>32)), ((uint32_t)value)

/** @brief Set RTC time
 *
 * @param[in] seconds rtc time to set
 */
void PortTimeSetRtcTime(uint64_t seconds);

/** @brief Get RTC time in seconds
 *
 * @return rtc times in seconds
 */
uint64_t PortTimeGetRtcTime(void);

/** @brief Get time since boot in milliseconds
 *
 * @return boot time in milliseconds
 */
uint32_t PortTimeGetBootTimeMs(void);

/** @brief Get time since boot in seconds
 *
 * @return boot time in seconds
 */
uint32_t PortTimeGetBootTime(void);

/** @brief Get RTC time in milliseconds
 *
 * @return rtc times in milliseconds
 */
uint64_t PortTimeGetMs(void);

/** @brief Initialization function for time management
 *
 * @retval TAG_ERROR_NONE Successfully locked with the requested id
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 * @retval TAG_ERROR_NOT_SUPPORTED sleep lock operation is not supported
 */
TagError_t PortTimeInit(void);

/** @brief Delay busy time for certain milliseconds (ms)
 *
 * @param[in] ms delay time in milliseconds
 */
void PortTimeDelayBusy(uint32_t ms);

#endif /* TAGSDK_PORT_INC_PORTTIME_H_ */
