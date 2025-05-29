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

#ifndef TAGSDK_INC_TAGDEBUG_H_
#define TAGSDK_INC_TAGDEBUG_H_

#include <stddef.h>

#include "TagConfig.h"
#include "TagErrorType.h"
#include "PortDebug.h"

#define LOG_PREFIX NULL

#define FILE_LOG_LEVEL_ERROR    0
#define FILE_LOG_LEVEL_INFO     1
#define FILE_LOG_LEVEL_DEBUG    2

#define FILE_LOG_LEVEL FILE_LOG_LEVEL_INFO

#define CHECK_RESULT_EQ(x, y, fmt, args...) \
    do                                      \
    {                                       \
        if ((x) == (y))                     \
        {                                   \
            TAG_LOG_E(fmt, ##args);         \
        }                                   \
    } while (0)

#define CHECK_RESULT_NOT_EQ(x, y, fmt, args...) \
    do                                          \
    {                                           \
        if ((x) != (y))                         \
        {                                       \
            TAG_LOG_E(fmt, ##args);             \
        }                                       \
    } while (0)

#if defined(TAG_CONFIG_LOG_LEVEL_ERROR)
#define TAG_LOG_E(fmt, args...)                             \
    do                                                      \
    {                                                       \
        if (FILE_LOG_LEVEL >= FILE_LOG_LEVEL_ERROR)         \
        {                                                   \
            PortDebugLog(LOG_PREFIX, "" fmt "", ##args);    \
        }                                                   \
    } while (0)
#else
#define TAG_LOG_E(fmt, args...)
#endif

#if defined(TAG_CONFIG_LOG_LEVEL_INFO)
#define TAG_LOG_I(fmt, args...)                             \
    do                                                      \
    {                                                       \
        if (FILE_LOG_LEVEL >= FILE_LOG_LEVEL_INFO)          \
        {                                                   \
            PortDebugLog(LOG_PREFIX, "" fmt "", ##args);    \
        }                                                   \
    } while (0)
#else
#define TAG_LOG_I(fmt, args...)
#endif

#if defined(TAG_CONFIG_LOG_LEVEL_DEBUG)
#define TAG_LOG_D(fmt, args...)                             \
    do                                                      \
    {                                                       \
        if (FILE_LOG_LEVEL >= FILE_LOG_LEVEL_DEBUG)         \
        {                                                   \
            PortDebugLog(LOG_PREFIX, "" fmt "", ##args);    \
        }                                                   \
    } while (0)
#else
#define TAG_LOG_D(fmt, args...)
#endif

#ifdef TAG_CONFIG_ENABLE_MEM_CHECK
#define TAG_MEM_CHECK(fmt, args...) PortDebugCheckMem(LOG_PREFIX, "" fmt "", ##args)
#else
#define TAG_MEM_CHECK(fmt, args...)
#endif

#ifdef TAG_CONFIG_DEBUG_EXPORT_STATIC_FUNC
#define STATIC_FUNCTION
#define STATIC_VARIABLE
#else
#define STATIC_FUNCTION static
#define STATIC_VARIABLE static
#endif

typedef enum
{
    TAG_LOG_TYPE_DEBUG,
    TAG_LOG_TYPE_LOGGING,
    TAG_LOG_TYPE_MAX,
} TagLogType;

typedef struct
{
    int dumpIndex;
    int dumpSize;
    int debugLogOffset;
    int debugLogSize;
    int writtenLen;
    PORT_DEBUG_LOG_PARAM *debugLogParam;
} TagLogParam_t;

TagLogParam_t *TagLogParamInit(TagLogType tagLogType, log_from_t from);
TagError_t TagLogParamDeinit(TagLogParam_t *logParam);
size_t TagLogGetLog(TagLogParam_t *logParam, char *logBuf, size_t bufSize, log_from_t from);
size_t TagLogGetSize(TagLogParam_t *logParam);

#endif /* TAGSDK_INC_TAGDEBUG_H_ */
