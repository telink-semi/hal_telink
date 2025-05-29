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
 * This file uses some APIs that were provided by mbed TLS (https://tls.mbed.org)
 *
 ****************************************************************************/

#include "TagConfig.h"

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)

#include "TagCore.h"
#include "TagFwUpdate.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"

#include "PortBle.h"
#include "PortFwUpdate.h"
#include "PortOs.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "FwUp"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

STATIC_VARIABLE fwUpdateData_t fwUpdateData =
    {
        .state = FW_UPDATE_STATE_IDLE,
        .chunkSeqNum = 0,
        .imgComputedCrc = 0,
        .preOffset = 0,
        .preSegmentedFirmwareLength = 0,
        .currentPos = 0,
        .currentEepromPos = 0,
        .peerDeviceId = 0,
};

STATIC_VARIABLE PortTimerHandle_t fwUpdateConnectionTimer = NULL;
STATIC_VARIABLE PortTimerHandle_t fwUpdateIndTimer = NULL;

STATIC_FUNCTION void fwUpdateClearData(void)
{
    fwUpdateData.state = FW_UPDATE_STATE_IDLE;
    fwUpdateData.chunkSeqNum = 0;
    fwUpdateData.imgComputedCrc = 0;
    fwUpdateData.preOffset = 0;
    fwUpdateData.preSegmentedFirmwareLength = 0;
    fwUpdateData.currentPos = 0;
    fwUpdateData.currentEepromPos = 0;
    fwUpdateData.peerDeviceId = 0;
}

STATIC_FUNCTION void fwUpdateStopTimer(void)
{
    if (fwUpdateConnectionTimer != NULL)
    {
        PortTimerDelete(fwUpdateConnectionTimer, 0);
        fwUpdateConnectionTimer = NULL;
    }
}

STATIC_FUNCTION void fwUpdateIndStopTimer(void)
{
    if (fwUpdateIndTimer != NULL)
    {
        PortTimerDelete(fwUpdateIndTimer, 0);
        fwUpdateIndTimer = NULL;
    }
}

STATIC_FUNCTION void fwUpdateRefreshTagAdvertising(void)
{
    TagRefreshAdv();
}

STATIC_FUNCTION void fwUpdateSetState(FwUpdateState_t state)
{
    TAG_LOG_D("state=%d ", state);

    fwUpdateData.state = state;
}

STATIC_FUNCTION void fwUpdateTagConnectionTimeout(TagTaskWorkParam param)
{
    static uint32_t prevOffset = 0;

    TAG_LOG_D("Callback for periodic Timer that checks the firmware update progress.(%d)", fwUpdateData.currentPos);

    if (fwUpdateData.currentPos != prevOffset)
    {
        prevOffset = fwUpdateData.currentPos;
    }
    else
    {
        PortFwUpdateFailedCb();

        prevOffset = 0;
        fwUpdateStopTimer();
        fwUpdateClearData();
        fwUpdateRefreshTagAdvertising();
    }
}

STATIC_FUNCTION void fwUpdateConnectionTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(fwUpdateTagConnectionTimeout, NULL);
}

STATIC_FUNCTION void fwUpdateStartTimer(EndUserDevice *endUserDevice)
{
    if (fwUpdateConnectionTimer != NULL)
    {
        PortTimerDelete(fwUpdateConnectionTimer, 0);
        fwUpdateConnectionTimer = NULL;
    }

    fwUpdateConnectionTimer = PortTimerCreate("fwUpdateConnectionTimeout",                                             /* Text name. */
                                              CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(FW_UPDATE_CONNECTION_PARAM_TIMEOUT)), /* Timer period. */
                                              true,                                                                    /* Enable auto reload. */
                                              endUserDevice,                                                           /* ID as tagContext */
                                              fwUpdateConnectionTimeoutCallback);                                      /* The callback function. */

    if (fwUpdateConnectionTimer)
    {
        TAG_LOG_D("StartTimer");
        PortTimerStart(fwUpdateConnectionTimer, 0);
    }
    else
    {
        TAG_LOG_E("Failed");
    }
}

STATIC_FUNCTION void fwUpdateMemcpy(void *pDst, const void *pSrc, uint32_t len)
{
    while (len)
    {
        *((uint8_t *)pDst) = *((const uint8_t *)pSrc);
        pDst = ((uint8_t *)pDst) + 1;
        pSrc = ((const uint8_t *)pSrc) + 1;
        len--;
    }
}

STATIC_FUNCTION TagError_t fwUpdateSetFirmwareData(TagControlServiceData *data)
{
    if (!data)
    {
        TAG_LOG_E("Data is null");
        return TAG_ERROR_INVALID_ARG;
    }

    uint16_t length = data->cValueLength;
    uint8_t *pData = data->aValue;

    uint16_t dataLen = 0;

    if (length <= (FW_UPDATE_OPCODE_FIELD_LEN + FW_UPDATE_OFFSET_FIELD_LEN + FW_UPDATE_SEGMENTED_FIRMWARE_DATA_FIELD_LEN))
    {
        TAG_LOG_E("Error : Invalid len : %d, dataLen: %d", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareData.opCode, pData, FW_UPDATE_OPCODE_FIELD_LEN);
    dataLen += FW_UPDATE_OPCODE_FIELD_LEN;

    fwUpdateMemcpy(&fwUpdateData.firmwareData.offset, pData + dataLen, FW_UPDATE_OFFSET_FIELD_LEN);
    dataLen += FW_UPDATE_OFFSET_FIELD_LEN;

    fwUpdateMemcpy(&fwUpdateData.firmwareData.segmentedFirmwareDataLength, pData + dataLen, FW_UPDATE_SEGMENTED_FIRMWARE_DATA_FIELD_LEN);
    dataLen += FW_UPDATE_SEGMENTED_FIRMWARE_DATA_FIELD_LEN;

    if (length <= (dataLen + fwUpdateData.firmwareData.segmentedFirmwareDataLength))
    {
        TAG_LOG_E("Error : Invalid firmwareLen : %d, dataLen: %d", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    memset(&fwUpdateData.firmwareData.segmentedFirmwareData, 0x00, FW_UPDATE_IMAGE_CHUNK_DATA_SIZE);
    fwUpdateMemcpy(&fwUpdateData.firmwareData.segmentedFirmwareData, pData + dataLen, fwUpdateData.firmwareData.segmentedFirmwareDataLength);
    dataLen += fwUpdateData.firmwareData.segmentedFirmwareDataLength;

    if (length != (dataLen + FW_UPDATE_SEGMENTED_FIRMWARE_DATA_CRC_FIELD_LEN))
    {
        TAG_LOG_E("Error : Invalid windowlen : %d, dataLen: %d", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareData.argumentsCRC16, pData + dataLen, FW_UPDATE_SEGMENTED_FIRMWARE_DATA_CRC_FIELD_LEN);

    // TAG_LOG_D("opCode : %d, offset : %d, fwLength : %d", fwUpdateData.firmwareData.opCode, fwUpdateData.firmwareData.offset, fwUpdateData.firmwareData.segmentedFirmwareDataLength);

    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t fwUpdateSetFirmwareInformation(TagControlServiceData *data)
{
    if (!data)
    {
        TAG_LOG_E("Data is null");
        return TAG_ERROR_INVALID_ARG;
    }

    uint16_t length = data->cValueLength;
    uint8_t *pData = data->aValue;

    uint16_t dataLen = 0;

    if (length <= (FW_UPDATE_OPCODE_FIELD_LEN + FW_UPDATE_FIRMWARE_SIZE_FIELD_LEN + FW_UPDATE_FIRMWARE_CRC_FIELD_LEN + FW_UPDATE_FIRMWARE_VERSION_LENGTH_FIELD_LEN))
    {
        TAG_LOG_E("Error : Invalid len : %d, dataLen: %d", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.opCode, pData, FW_UPDATE_OPCODE_FIELD_LEN);
    dataLen += FW_UPDATE_OPCODE_FIELD_LEN;

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.totalFirmwareSize, pData + dataLen, FW_UPDATE_FIRMWARE_SIZE_FIELD_LEN);
    dataLen += FW_UPDATE_FIRMWARE_SIZE_FIELD_LEN;

    if (fwUpdateData.firmwareInfo.totalFirmwareSize == 0)
    {
        TAG_LOG_E("Error : Invalid firmware size");
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.totalFirmwareCRC16, pData + dataLen, FW_UPDATE_FIRMWARE_CRC_FIELD_LEN);
    dataLen += FW_UPDATE_FIRMWARE_CRC_FIELD_LEN;

    if (fwUpdateData.firmwareInfo.totalFirmwareCRC16 == 0)
    {
        TAG_LOG_E("Error : Invalid crc16");
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.newFirmwareVersionLength, pData + dataLen, FW_UPDATE_FIRMWARE_VERSION_LENGTH_FIELD_LEN);
    dataLen += FW_UPDATE_FIRMWARE_VERSION_LENGTH_FIELD_LEN;

    TAG_LOG_D("newFirmwareVersionLength : %u", fwUpdateData.firmwareInfo.newFirmwareVersionLength);

    if (length <= (dataLen + fwUpdateData.firmwareInfo.newFirmwareVersionLength))
    {
        TAG_LOG_E("Error : Invalid firmwareLen : %u, dataLen: %u", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.newFirmwareVersion, pData + dataLen, fwUpdateData.firmwareInfo.newFirmwareVersionLength);
    dataLen += fwUpdateData.firmwareInfo.newFirmwareVersionLength;

    if (length != (dataLen + FW_UPDATE_TRANSFER_WINDOW_FIELD_LEN))
    {
        TAG_LOG_E("Error : Invalid windowlen : %d, dataLen: %d", length, dataLen);
        return TAG_ERROR_INVALID_ARG;
    }

    unsigned int major = 0, minor = 0, patch = 0;

    int result = sscanf((void *) fwUpdateData.firmwareInfo.newFirmwareVersion, "%3u.%3u.%3u", &major, &minor, &patch);

    if (result != 3)
    {
        TAG_LOG_E("Error : Invalid version : %u.%u.%u, result:%d", major, minor, patch, result);
        return TAG_ERROR_INVALID_ARG;
    }

    fwUpdateMemcpy(&fwUpdateData.firmwareInfo.transferWindow, pData + dataLen, FW_UPDATE_TRANSFER_WINDOW_FIELD_LEN);

    if (PortFwUpdateGetMaxWriteWithoutResponse()> 0)
    {
        if (fwUpdateData.firmwareInfo.transferWindow != PortFwUpdateGetMaxWriteWithoutResponse())
        {
            TAG_LOG_E("Error : Invalid transferWindow : %d", fwUpdateData.firmwareInfo.transferWindow);
            return TAG_ERROR_INVALID_ARG;
        }
    }
    else
    {
        if (fwUpdateData.firmwareInfo.transferWindow == 0 || fwUpdateData.firmwareInfo.transferWindow > 255)
        {
            TAG_LOG_E("Invalid transferWindow : %d", fwUpdateData.firmwareInfo.transferWindow);
            return TAG_ERROR_INVALID_ARG;
        }
    }

    TAG_LOG_D("opCode : %u", fwUpdateData.firmwareInfo.opCode);
    TAG_LOG_D("totalFirmwareSize : %u", fwUpdateData.firmwareInfo.totalFirmwareSize);
    TAG_LOG_D("totalFirmwareCRC16 : 0x%x", fwUpdateData.firmwareInfo.totalFirmwareCRC16);
    TAG_LOG_D("transferWindow : %u", fwUpdateData.firmwareInfo.transferWindow);

    return TAG_ERROR_NONE;
}

STATIC_FUNCTION void fwUpdateContinueImageDownload(EndUserDevice *endUserDevice)
{
    if (!endUserDevice)
    {
        TAG_LOG_E("Invalid data");
        return;
    }

    FwUpdateState_t state = FwUpdateGetState();

    switch (state)
    {
    case FW_UPDATE_STATE_IDLE:
        TAG_LOG_D("State : idle");
        break;

    case FW_UPDATE_STATE_TRANSFER_IN_PROGRESS:
        if (fwUpdateData.chunkSeqNum == fwUpdateData.firmwareInfo.transferWindow)
        {
            fwUpdateData.chunkSeqNum = 0U;

            TagError_t result = TAG_ERROR_NONE;

            //TAG_LOG_I("Data [%d / %d]", fwUpdateData.currentPos, fwUpdateData.firmwareInfo.totalFirmwareSize);

            if ((result = FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)) != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Send command error(%d)", result);
                PortBleGapDisconnect(&endUserDevice->portConnHandle);
            }
        }
        break;

    case FW_UPDATE_STATE_TRANSFER_SUCCESS:
        TAG_LOG_D("State : Trasnsfer success");
        break;

    case FW_UPDATE_STATE_TRANSFER_FAILURE:
        TAG_LOG_D("State : Trasnsfer failure");
        break;

    case FW_UPDATE_STATE_UPDATE_NOT_ALLOWED:
        TAG_LOG_D("State : Trasnsfer not allowed");
        break;

    default:
        TAG_LOG_E("Unknown state (%d)", state);
        break;
    };
}

STATIC_FUNCTION TagError_t fwUpdateWriteToFlash(uint16_t length, uint8_t *pData, uint32_t Addr)
{
    TagError_t ret = TAG_ERROR_NONE;

    uint8_t result = 0;

    do
    {
        /* Try to write the data chunk into the external EEPROM */
        if ((result = PortFwUpdateWriteFlash(length, Addr, pData)) != 0)
        {
            TAG_LOG_E("Write failed (%d)", result);
            ret = TAG_ERROR_FW_UPDATE_STORAGE_ERROR;
            break;
        }
        /* Data chunk successfully written into EEPROM Update operation parameters */
        fwUpdateData.currentPos += length;

    } while (0);

    return ret;
}

STATIC_FUNCTION TagError_t fwUpdateHandleDataChunk(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    uint16_t receivedFirmwareCrc;
    uint16_t crcLength;
    uint16_t length;
    uint32_t predictOffset;

    TagError_t result = TAG_ERROR_NONE;

    uint8_t *pData = NULL;

    if (!endUserDevice || !data)
    {
        TAG_LOG_E("Invalid parameter");
        return TAG_ERROR_INVALID_ARG;
    }

    length = data->cValueLength;
    pData = data->aValue;

    fwUpdateData.chunkSeqNum += 1U;

    // 1. Check receviced data len
    predictOffset = fwUpdateData.preOffset + fwUpdateData.preSegmentedFirmwareLength;

    if (predictOffset != fwUpdateData.firmwareData.offset)
    {
        TAG_LOG_E("Invalid offset: %d, offset : %d)", predictOffset, fwUpdateData.firmwareData.offset);

        if (fwUpdateData.chunkSeqNum == fwUpdateData.firmwareInfo.transferWindow)
        {
            FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_TRANSFER_FAILURE);
        }

        return TAG_ERROR_FW_UPDATE_UNEXPECTED_SEQ_NUMBER;
    }

    if (fwUpdateData.currentPos % FW_UPDATE_SEGMENTED_DATA_LEN_IS_MULTIPLE_OF_16)
    {
        TAG_LOG_E("This value must be a multiple of 16 except for the last segmented firmware data.");
        return TAG_ERROR_INVALID_ARG;
    }

    // 2. Check Crc
    crcLength = (uint16_t)(length - sizeof(fwUpdateData.firmwareData.opCode) - sizeof(fwUpdateData.firmwareData.argumentsCRC16));

    receivedFirmwareCrc = TagUtilCrc16(pData + sizeof(uint8_t), (uint16_t)crcLength, 0);

    if (receivedFirmwareCrc != fwUpdateData.firmwareData.argumentsCRC16)
    {
        TAG_LOG_E("Invalid Crc : 0x%x, argumentsCRC16 : 0x%x", receivedFirmwareCrc, fwUpdateData.firmwareData.argumentsCRC16);
        return TAG_ERROR_CRC_MISMATCH;
    }

    // 3. Write image in external flash
    result = fwUpdateWriteToFlash(fwUpdateData.firmwareData.segmentedFirmwareDataLength,
                                  fwUpdateData.firmwareData.segmentedFirmwareData,
                                  fwUpdateData.currentEepromPos);
    if (result == TAG_ERROR_NONE)
    {
        fwUpdateData.preOffset = fwUpdateData.firmwareData.offset;
        fwUpdateData.preSegmentedFirmwareLength = fwUpdateData.firmwareData.segmentedFirmwareDataLength;
        // fwUpdateData.currentPos += fwUpdateData.firmwareData.segmentedFirmwareDataLength;
        fwUpdateData.currentEepromPos += fwUpdateData.firmwareData.segmentedFirmwareDataLength;
        fwUpdateData.imgComputedCrc = TagUtilCrc16(fwUpdateData.firmwareData.segmentedFirmwareData,
                                                   (uint16_t)fwUpdateData.firmwareData.segmentedFirmwareDataLength, fwUpdateData.imgComputedCrc);

        //TAG_LOG_I("Data [%d / %d]", fwUpdateData.currentPos, fwUpdateData.firmwareInfo.totalFirmwareSize);

        // 4. Checking firmware download is completed
        if (fwUpdateData.currentPos >= fwUpdateData.firmwareInfo.totalFirmwareSize)
        {
            // 5. Checking total firmware Crc
            if (fwUpdateData.currentPos != fwUpdateData.firmwareInfo.totalFirmwareSize)
            {
                fwUpdateSetState(FW_UPDATE_STATE_TRANSFER_FAILURE);

                TAG_LOG_E("currentPos: %u, totalFirmwareSize : %u", fwUpdateData.currentPos, fwUpdateData.firmwareInfo.totalFirmwareSize);
                result = TAG_ERROR_INVALID_ARG;
                goto Clean_Up;
            }

            // 6. Checking total firmware Crc
            if (fwUpdateData.imgComputedCrc != fwUpdateData.firmwareInfo.totalFirmwareCRC16)
            {
                fwUpdateSetState(FW_UPDATE_STATE_TRANSFER_FAILURE);

                TAG_LOG_E("imgComputedCrc: 0x%x, totalFirmwareCRC16 : 0x%x", fwUpdateData.imgComputedCrc, fwUpdateData.firmwareInfo.totalFirmwareCRC16);
                result = TAG_ERROR_CRC_MISMATCH;
                goto Clean_Up;
            }
            else
            {
                TAG_LOG_E("The device received all firmware data from App. Now device will reboot to update a binary in device.");
                TAG_LOG_E("If a device has some problems with memory or storage, a firmware update may fail after rebooting the device.");

                fwUpdateSetState(FW_UPDATE_STATE_TRANSFER_SUCCESS);

                PortFwUpdateEndCb();

                if ((result = FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_TRANSFER_SUCCESS)) == TAG_ERROR_NONE)
                {
                    fwUpdateStopTimer();
                    fwUpdateIndStopTimer();

                    PortTaskDelay(CONV_MS_TO_TICKS(3000));
                    PortFwUpdateSuccessCb(&endUserDevice->portConnHandle);
                }
            }
        }
        else
        {
            fwUpdateContinueImageDownload(endUserDevice);
        }
    }
    else
    {
        fwUpdateSetState(FW_UPDATE_STATE_TRANSFER_FAILURE);
        TAG_LOG_E("fwUpdateWriteToFlash failed: %d", result);
    }

Clean_Up:

    return result;
}

void FwUpdateInit(void)
{
    fwUpdateStopTimer();
    fwUpdateClearData();
    PortFwUpdateInitCb();
}

FwUpdateState_t FwUpdateGetState(void)
{
    return fwUpdateData.state;
}

TagError_t FwUpdateSendCommand(EndUserDevice *endUserDevice, FwUpdateState_t state)
{
    TagError_t result = TAG_ERROR_NONE;
    TagBleError_t bleResult = TAG_BLE_ERROR_ATT_NO_ERROR;

    TagControlServiceData *data;

    if (!endUserDevice)
    {
        TAG_LOG_E("No end user device");
        return TAG_ERROR_INVALID_ARG;
    }

    data = AllocateTagControlServiceData(1);
    if (data == NULL)
    {
        TAG_LOG_E("Error: allocate data");
        return TAG_ERROR_MEM_ALLOC;
    }

    data->aValue[0] = state;

    bleResult = TagControlSendIndication(endUserDevice, CTRL_FIRMWARE_TRANSFER, data, ENCRYPTION_REQUIRED);

    if (bleResult != TAG_BLE_ERROR_ATT_NO_ERROR)
    {
        TAG_LOG_E("Error: Send Indication (0x%x)", bleResult);
        result = TAG_ERROR_FW_UPDATE_SEND_INDICATION_ERROR;
    }

    FreeTagControlServiceData(data);

    return result;
}

STATIC_FUNCTION void fwUpdateTagSendIndication(TagTaskWorkParam param)
{
    TagError_t result = TAG_ERROR_NONE;

    EndUserDevice *endUserDevice = TagFindEndUserDevice(fwUpdateData.peerDeviceId);

    TAG_LOG_E("Send fwIndication");

    if (!endUserDevice)
    {
        TAG_LOG_E("IndParam is null");
        return;
    }

    if ((result = FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)) == TAG_ERROR_NONE)
    {
        if (TagSoundPlayItem(SOUND_ITEM_PRESS) == SOUND_NOT_PLAYED)
        {
            TAG_LOG_E("Failed to play PRESS sound");
        }

        fwUpdateSetState(FW_UPDATE_STATE_TRANSFER_IN_PROGRESS);
        fwUpdateStartTimer(endUserDevice);

        PrepareFwUpdate(fwUpdateData.peerDeviceId);
    }
    else
    {
        TAG_LOG_E("SendInd error");
    }
}

STATIC_FUNCTION void fwUpdateIndTimeoutCallback(PortTimerHandle_t timer)
{
    TagPutPostWork(fwUpdateTagSendIndication, NULL);
}

STATIC_FUNCTION TagError_t fwUpdateSendTransferInProgressInd(void)
{
    TagError_t result = TAG_ERROR_NONE;

    TAG_LOG_E("Run fwTimer");

    if (fwUpdateIndTimer != NULL)
    {
        PortTimerDelete(fwUpdateIndTimer, 0);
        fwUpdateIndTimer = NULL;
    }

    fwUpdateIndTimer = PortTimerCreate("fwUpdateIndTimeout",                                                    /* Text name. */
                                       CONV_MS_TO_TICKS(CONVERT_SEC_TO_MS(FW_UPDATE_INDICATION_DELAY_TIMEOUT)), /* Timer period. */
                                       false,                                                                   /* Disable auto reload. */
                                       NULL,                                                                    /* ID as tagContext */
                                       fwUpdateIndTimeoutCallback);                                             /* The callback function. */

    if (fwUpdateIndTimer)
    {
        PortTimerStart(fwUpdateIndTimer, 0);
    }
    else
    {
        TAG_LOG_E("indTimer Failed");
        result = TAG_ERROR_OPERATION_FAILURE;
    }

    return result;
}

TagError_t FwUpdateAttributeWrittenWithoutResponse(EndUserDevice *endUserDevice, TagControlServiceData *data)
{
    TagError_t ret = TAG_ERROR_NONE;

    uint8_t opCode;

    if (!endUserDevice)
    {
        TAG_LOG_E("No end user device");
        return TAG_ERROR_INVALID_ARG;
    }

    if (!data)
    {
        TAG_LOG_E("Data is null");
        return TAG_ERROR_INVALID_ARG;
    }

    opCode = data->aValue[0];

    switch (opCode)
    {
    case OP_CODE_FIRMWARE_INFORMATION:
    {
        fwUpdateStopTimer();
        fwUpdateClearData();

        PortFwUpdateStartCb();
        fwUpdateData.peerDeviceId = endUserDevice->deviceId;
        fwUpdateData.currentEepromPos = PortFwUpdateGetStartAddress();
        if ((ret = fwUpdateSetFirmwareInformation(data)) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("fwData error");
            break;
        }

        if ((ret = fwUpdateSendTransferInProgressInd()) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("fwInd error");
            break;
        }

        if ((ret = PortFwUpdateEraseFlash(fwUpdateData.firmwareInfo.totalFirmwareSize, fwUpdateData.currentEepromPos)) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("fwErase error");
            break;
        }
    }
    break;

    case OP_CODE_FIRMWARE_DATA:
        if (fwUpdateData.currentPos == 0)
        {
            TAG_LOG_I("Start FOTA");
        }

        if (FwUpdateGetState() != FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)
        {
            TAG_LOG_E("Invalid state (%d)", FwUpdateGetState());
            ret = TAG_ERROR_OPERATION_FAILURE;
        }
        else
        {
            if ((ret = fwUpdateSetFirmwareData(data)) == TAG_ERROR_NONE)
            {
                ret = fwUpdateHandleDataChunk(endUserDevice, data);
            }
        }
        break;

    case OP_CODE_FIRMWARE_UPDATE_CANCEL:
        TAG_LOG_E("Update Cancel");
        ret = TAG_ERROR_FW_UPDATE_UPDATE_CANCEL;
        break;

    default:
        TAG_LOG_E("Invalid command :%d", opCode);
        ret = TAG_ERROR_INVALID_ARG;
        break;
    };

    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Error: WithoutResponse (%d)", ret);

        if (FwUpdateGetState() != FW_UPDATE_STATE_TRANSFER_IN_PROGRESS)
        {
            if (FwUpdateSendCommand(endUserDevice, FW_UPDATE_STATE_TRANSFER_FAILURE) != TAG_ERROR_NONE)
            {
                PortBleGapDisconnect(&endUserDevice->portConnHandle);
            }

            PortFwUpdateFailedCb();

            fwUpdateStopTimer();
            fwUpdateIndStopTimer();
            fwUpdateClearData();
            fwUpdateRefreshTagAdvertising();
        }
    }

    return ret;
}
#endif /* TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE */
