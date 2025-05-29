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

#include <stdarg.h>
#include <stdio.h>

#include "TagConfig.h"

#include "TagCore.h"
#include "TagErrorType.h"
#include "TagFwUpdate.h"
#include "TagUtil.h"
#include "TagVersion.h"

#include "PortDebug.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "LogM"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define DUMP_TMP_BUF_SIZE 50

typedef struct
{
    char *logBuffer;
    int bufSize;
    int offset;
    int dumpIndex;
    int dumpCount;
    TagError_t err;
} TagLogDumpState_t;

static TagError_t getDumpStateLine(TagLogDumpState_t *dumpState, const char *fmt, ...)
{
    char dumpBuf[DUMP_TMP_BUF_SIZE] = "";
    int tmpLen;
    va_list args;

    if (dumpState->err != TAG_ERROR_NONE)
    {
        return dumpState->err;
    }

    if (dumpState->dumpIndex > (dumpState->dumpCount)++)
    {
        return dumpState->err = TAG_ERROR_NONE;
    }

    va_start(args, fmt);
    tmpLen = vsnprintf(dumpBuf, DUMP_TMP_BUF_SIZE, fmt, args);
    va_end(args);

    if (tmpLen > dumpState->bufSize - dumpState->offset)
    {
        return dumpState->err = TAG_ERROR_OPERATION_FAILURE;
    }

    if (dumpState->logBuffer)
    {
        strncpy(dumpState->logBuffer + dumpState->offset, dumpBuf, tmpLen);
    }

    dumpState->offset += tmpLen;
    dumpState->dumpIndex = dumpState->dumpIndex + 1;

    return dumpState->err = TAG_ERROR_NONE;
}

static int getDumpState(char *logBuffer, int logSize, int *dumpIndex)
{
    int offset = 0;

    char tmpBuf[20] = "";

    TagLogDumpState_t *dumpState = TagMalloc(sizeof(TagLogDumpState_t));
    if(!dumpState) {
        TAG_LOG_E("dumpState malloc failed");
        return offset;
    }
    memset(dumpState, 0, sizeof(TagLogDumpState_t));

    if (!gTagContext)
    {
        goto exit;
    }

    dumpState->bufSize = logSize;
    dumpState->logBuffer = logBuffer;
    dumpState->dumpIndex = *dumpIndex;

    if (dumpState->logBuffer)
    {
        memset(dumpState->logBuffer, '\0', dumpState->bufSize);
    }

    getDumpStateLine(dumpState, "SDK_VER:%.9s\n", TAGSDK_VERSION_STRING);
    getDumpStateLine(dumpState, "FW_REV:%.9s\n", DEVICE_FW_VERSION_STRING);
    getDumpStateLine(dumpState, "MNID:%.4s\n", TagUtilConvertIntToString(gTagContext->MNID, tmpBuf, 4));
    getDumpStateLine(dumpState, "setupID:%.4s\n", TagUtilConvertIntToString(gTagContext->setupID, tmpBuf, 4));
    getDumpStateLine(dumpState, "serialNumber:****%.4s\n", TagUtilConvertIntToString(gTagContext->serialNumber, tmpBuf, 4));

    if (dumpState->err != TAG_ERROR_NONE)
    {
        *dumpIndex = dumpState->dumpIndex;
    }
    else
    {
        *dumpIndex = -1;
    }

exit:
    TAG_LOG_D("DUMP size %d/%d, remain %d", dumpState->offset, dumpState->bufSize, dumpState->bufSize - dumpState->offset);

    offset = dumpState->offset;

    TagFree(dumpState);

    return offset;
}

#define DUMP_MAX_SIZE 4096
TagLogParam_t *TagLogParamInit(TagLogType tagLogType, log_from_t from)
{
    TagLogParam_t *logParam = NULL;
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    int dumpIndex = 0;
    TAG_MEM_CHECK("LogParamInit");

    if (tagLogType < 0 || tagLogType >= TAG_LOG_TYPE_MAX)
    {
        TAG_LOG_E("Wrong Tag Log Type %d", tagLogType);
        return NULL;
    }

    logParam = (TagLogParam_t *)TagMalloc(sizeof(TagLogParam_t));
    if (!logParam)
    {
        return NULL;
    }

    memset(logParam, 0, sizeof(TagLogParam_t));

    logParam->debugLogParam = PortDebugLogParamInit(tagLogType);
    logParam->debugLogSize = PortGetDebugLogSize(logParam->debugLogParam, from);
    logParam->dumpSize = getDumpState(NULL, DUMP_MAX_SIZE, &dumpIndex);
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    return logParam;
}

TagError_t TagLogParamDeinit(TagLogParam_t *logParam)
{
    TagError_t err = TAG_ERROR_NONE;
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    if (!logParam)
    {
        return TAG_ERROR_INVALID_ARG;
    }
    err = PortDebugLogParamDeinit(logParam->debugLogParam);
    TagFree(logParam);

    TAG_MEM_CHECK("LogParamDeinit");
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    return err;
}
size_t TagLogGetSize(TagLogParam_t *logParam)
{
    if (!logParam)
    {
        return 0;
    }

    return logParam->debugLogSize + logParam->dumpSize;
}

size_t TagLogGetLog(TagLogParam_t *logParam, char *logBuf, size_t bufSize, log_from_t from)
{
    size_t writtenSize = 0;
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    if (!logParam)
    {
        return 0;
    }

    if (logParam->dumpIndex >= 0)
    {
        writtenSize = getDumpState(logBuf, bufSize, &(logParam->dumpIndex));
        logParam->writtenLen += writtenSize;
        goto exit;
    }

    if (logParam->debugLogOffset < logParam->debugLogSize)
    {
        if (bufSize > logParam->debugLogSize - logParam->debugLogOffset)
        {
            bufSize = logParam->debugLogSize - logParam->debugLogOffset;
        }
        writtenSize = PortReadDebugLog(logParam->debugLogParam, logParam->debugLogOffset, logBuf, bufSize, from);
        logParam->debugLogOffset += writtenSize;
        logParam->writtenLen += writtenSize;
        goto exit;
    }

exit:
    if (writtenSize == 0)
    {
        TAG_LOG_E("Log Done : %u / %u (%u + %u)", logParam->writtenLen, logParam->debugLogSize + logParam->dumpSize, logParam->dumpSize, logParam->debugLogSize);
    }
#endif /* TAG_CONFIG_USE_DEBUG_CHARACTERISTICS */
    return writtenSize;
}
