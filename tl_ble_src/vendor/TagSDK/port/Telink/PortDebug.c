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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "tlkapi_debug.h"

#include "TagConfig.h"

#include "PortDebug.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
static bool skipPrefix = true;

#define MAX_PREFIX_SIZE (5)
#define RAM_LOG_BUF_SIZE (256)

#define LOG_BUFFER_LEN 110
#define LOG_TIME_STAMP_LEN 20
#define MAX_LOG_BUF_LEN (LOG_BUFFER_LEN + LOG_TIME_STAMP_LEN)
#define LOG_TIMESTAMP_FORMAT    "%d%H%M%S "

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
uint32_t PortTimeGetBootTime(void);
void* TagMalloc(size_t size);
void TagFree(void *ptr);

#define DEBUG_RAM_SIZE (1 * 1024)

#define GET_MIN_VALUE(a,b) (((a) < (b)) ? (a) : (b))

typedef struct
{
    int endPoint;
    int startPoint;
} PortLogParam_t;

static char debugLogBuffer[DEBUG_RAM_SIZE] = {0, };
_attribute_ble_data_retention_ static int logWriteOffset = 0;
_attribute_ble_data_retention_ static bool isLogLooped = false;
_attribute_ble_data_retention_ static bool isLogLocked = false;

void PortDebugLog(const char* prefix, const char* fmt, ...)
{
    va_list ap;
    size_t prefixLen = 0;
    unsigned int  currTime = 0;

    int length = 0;

    char logData[MAX_LOG_BUF_LEN+1] = {0,};
    char *bufferPtr = logData;

    currTime = PortTimeGetBootTime();
    length = sprintf(bufferPtr, "%s%08u%s", "[", currTime, "]");
    bufferPtr += length;

    if (skipPrefix != true)
    {
        length += snprintf(bufferPtr, MAX_PREFIX_SIZE + 3, "[%.5s]", prefix);
        prefixLen = strlen(prefix) + 2;
        bufferPtr += prefixLen;
    }

    va_start(ap, fmt);
    int remainLen = RAM_LOG_BUF_SIZE - length;
    int vaLen = vsnprintf(bufferPtr, remainLen, fmt, ap);
    length += (vaLen >= remainLen)? remainLen - 1: vaLen;
    va_end(ap);

#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    PortLogWriteDebugLog(logData, length);
    PortLogWriteDebugLog("\n", 1);
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */

//    tlk_printf("%s\r\n",logData);
}

PORT_DEBUG_LOG_PARAM *PortDebugLogParamInit(int tagLogType)
{
    UNUSEDARG(tagLogType);
    PortLogParam_t *logParam;

    logParam = (PortLogParam_t *)TagMalloc(sizeof(PortLogParam_t));
    if (!logParam)
    {
        return NULL;
    }

    logParam->endPoint = isLogLooped ? DEBUG_RAM_SIZE : logWriteOffset;
    logParam->startPoint = logWriteOffset;

    isLogLocked = true;

    return (void *)logParam;
}

int PortDebugLogParamDeinit(PORT_DEBUG_LOG_PARAM *debugLogParam)
{
    isLogLocked = false;
    TagFree(debugLogParam);

    return 0;
}

int PortGetDebugLogSize(PORT_DEBUG_LOG_PARAM *debugLogParam, log_from_t from)
{
    UNUSEDARG(from);
    PortLogParam_t *logParam = (PortLogParam_t *)debugLogParam;
    return logParam->endPoint;
}

int PortReadDebugLog(PORT_DEBUG_LOG_PARAM *debugLogParam, int offset, char *buf, int readLen, log_from_t from)
{
    UNUSEDARG(from);
    PortLogParam_t *logParam = (PortLogParam_t *)debugLogParam;
    int readPoint;
    int len;

    if (!logParam)
    {
        return 0;
    }

    readPoint = (logParam->startPoint + offset) % logParam->endPoint;

    len = GET_MIN_VALUE(readLen, (logParam->endPoint - readPoint));

    memcpy(buf, debugLogBuffer + offset, len);
    return len;
}

void PortLogWriteDebugLog(const char *buf, int logSize)
{
    int writeLen;

    if (isLogLocked)
    {
        return;
    }

    writeLen = GET_MIN_VALUE(logSize, DEBUG_RAM_SIZE - logWriteOffset);

    memcpy(debugLogBuffer + logWriteOffset, buf, writeLen);
    logWriteOffset += writeLen;

    if (writeLen < logSize) {
        isLogLooped = true;
        memcpy(debugLogBuffer, buf + writeLen, logSize - writeLen);
        logWriteOffset = logSize - writeLen;
    }
}
#else
void PortDebugSkipPrefix(bool skip)
{
    skipPrefix = skip;
}

void PortDebugLog(const char* prefix, const char* fmt, ...)
{
    va_list ap;
    size_t prefixLen = 0;

    int length = 0;

    char logData[MAX_LOG_BUF_LEN+1] = {0,};
    char *bufferPtr = logData;

    if (skipPrefix != true)
    {
        length = snprintf(bufferPtr, MAX_PREFIX_SIZE + 3, "[%.5s]", prefix);
        prefixLen = strlen(prefix) + 2;
        bufferPtr += prefixLen;
    }

    va_start(ap, fmt);
    int remainLen = RAM_LOG_BUF_SIZE - length;
    int vaLen = vsnprintf(bufferPtr, RAM_LOG_BUF_SIZE - prefixLen - 1 - strlen("\r\n"), fmt, ap);
    length += (vaLen >= remainLen)? remainLen - 1: vaLen;
    va_end(ap);

    tlk_printf("%s\r\n",logData);
}

void PortDebugCheckMem(const char *prefix, const char *fmt, ...)
{
    UNUSEDARG(prefix)
    UNUSEDARG(fmt)
}

PORT_DEBUG_LOG_PARAM *PortDebugLogParamInit(int tagLogType)
{
    UNUSEDARG(tagLogType)
    return NULL;
}

int PortDebugLogParamDeinit(PORT_DEBUG_LOG_PARAM *debugLogParam)
{
    return 0;
}

int PortGetDebugLogSize(PORT_DEBUG_LOG_PARAM *debugLogParam, log_from_t from)
{

    return 0;
}

int PortReadDebugLog(PORT_DEBUG_LOG_PARAM *debugLogParam, int offset, char *buf, int readLen, log_from_t from)
{
    return 0;
}
#endif
