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

#ifndef TAGSDK_PORT_INC_PORTDEBUG_H_
#define TAGSDK_PORT_INC_PORTDEBUG_H_

#include <stdbool.h>

#include "TagConfig.h"
#include "TagErrorType.h"

typedef void PORT_DEBUG_LOG_PARAM;

/** @brief Contains a enumeration values for types of reading of writing logs
 *
 */
typedef enum
{
    LOG_FROM_RAM,
    LOG_FROM_FLASH,
} log_from_t;

/** @brief Print Tag debug logs
 *
 * @param[in] prefix    Log prefix at the head of log string with brackets
 * @param[in] fmt       Print string format
 * @param[in] ...       Print format parameters
 *
 */
void PortDebugLog(const char* prefix, const char* fmt, ...);

/** @brief Initialize the log parameter to use to read the log
 *
 * @param[in] tagLogType Type of the log
 *
 * @return Log parameter
 *
 */
PORT_DEBUG_LOG_PARAM *PortDebugLogParamInit(int tagLogType);

/** @brief Deinitialize the log parameter
 *
 * @param[in] debugLogParam Log parameter to deinitialize
 *
 * @retval 0 for success, return other values for failure
 *
 */
int PortDebugLogParamDeinit(PORT_DEBUG_LOG_PARAM *debugLogParam);

/** @brief Get size of the logs
 *
 * @param[in] debugLogParam Log parameter
 * @param[in] from Where the log is saved
 *
 * @return Log size
 *
 */
int PortGetDebugLogSize(PORT_DEBUG_LOG_PARAM *debugLogParam, log_from_t from);

/** @brief Print memory status logs
 *
 * @param[in] prefix    Log prefix at the head of log string with brackets
 * @param[in] fmt       Print string format
 * @param[in] ...       Print format parameters
 *
 */
void PortDebugCheckMem(const char *prefix, const char* fmt, ...);

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
/** @brief Read the saved logs
 *
 * @param[in] debugLogParam Log parameter
 * @param[in] offset Offset of the log to read
 * @param[out] buf Buffer to read
 * @param[in] readLen Length of the log to read
 * @param[in] from Where the log is saved
 *
 * @return The size of the read log
 *
 */
int PortReadDebugLog(PORT_DEBUG_LOG_PARAM *debugLogParam, int offset, char *buf, int readLen, log_from_t from);

/** @brief Write the logs
 *
 * @param[in] buf Buffer of the log to write
 * @param[in] logSize Size of the buffer
 *
 */
void PortLogWriteDebugLog(const char *buf, int logSize);

/** @brief Set whether to print prefix or not
 *
 * @param[in] skip Whether to print prefix or not
 *
 */
void PortDebugSkipPrefix(bool skip);
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

#endif /* TAGSDK_PORT_INC_PORTDEBUG_H_ */
