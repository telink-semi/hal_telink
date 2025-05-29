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

#ifndef TAGSDK_INC_TAGFWUPDATE_H_
#define TAGSDK_INC_TAGFWUPDATE_H_

#include "TagConfig.h"
#include "TagErrorType.h"
#include "TagControlService.h"

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)

#define FW_UPDATE_REBOOT_DELAY_TIME (3)         /* sec*/
#define FW_UPDATE_CONNECTION_PARAM_TIMEOUT (10) /* sec*/
#define FW_UPDATE_INDICATION_DELAY_TIMEOUT (8)  /* sec*/

#define FW_UPDATE_OPCODE_FIELD_LEN 1
#define FW_UPDATE_OFFSET_FIELD_LEN 4
#define FW_UPDATE_SEGMENTED_FIRMWARE_DATA_FIELD_LEN 2
#define FW_UPDATE_SEGMENTED_FIRMWARE_DATA_CRC_FIELD_LEN 2
#define FW_UPDATE_SEGMENTED_DATA_LEN_IS_MULTIPLE_OF_16 16

#define FW_UPDATE_FIRMWARE_SIZE_FIELD_LEN 4
#define FW_UPDATE_FIRMWARE_CRC_FIELD_LEN 2
#define FW_UPDATE_FIRMWARE_VERSION_LENGTH_FIELD_LEN 1
#define FW_UPDATE_TRANSFER_WINDOW_FIELD_LEN 1

#define FW_UPDATE_FIRMWARE_VERSION_SIZE (12U)
#define FW_UPDATE_IMAGE_CHUNK_DATA_SIZE (255U)
#define FW_UPDATE_UPDATE_INFO_SIZE 60

/**
 * @brief Contains a enumeration values for managing firmware update state.
 */
typedef enum
{
    FW_UPDATE_STATE_IDLE                  = 0x00,
    FW_UPDATE_STATE_TRANSFER_IN_PROGRESS  = 0x01,
    FW_UPDATE_STATE_TRANSFER_SUCCESS      = 0x02,
    FW_UPDATE_STATE_TRANSFER_FAILURE      = 0x03,
    FW_UPDATE_STATE_UPDATE_NOT_ALLOWED    = 0x04

} FwUpdateState_t;

typedef struct
{
    uint8_t opCode;
    uint32_t totalFirmwareSize;
    uint16_t totalFirmwareCRC16;
    uint8_t newFirmwareVersionLength;
    uint8_t newFirmwareVersion[FW_UPDATE_FIRMWARE_VERSION_SIZE];
    uint8_t transferWindow;
} transferFwInformation_t;

typedef struct
{
    uint8_t opCode;
    uint32_t offset;
    uint16_t segmentedFirmwareDataLength;
    uint8_t segmentedFirmwareData[FW_UPDATE_IMAGE_CHUNK_DATA_SIZE];
    uint16_t argumentsCRC16;
} transferFwData_t;

typedef struct
{
    FwUpdateState_t state;
    transferFwInformation_t firmwareInfo;
    transferFwData_t firmwareData;

    uint16_t chunkSeqNum;
    uint16_t imgComputedCrc;
    uint32_t preOffset;
    uint32_t preSegmentedFirmwareLength;
    uint32_t currentPos;
    uint32_t currentEepromPos;
    TagBleDeviceId peerDeviceId;
} fwUpdateData_t;

typedef enum
{
    OP_CODE_FIRMWARE_INFORMATION = 0x00,
    OP_CODE_FIRMWARE_DATA = 0x01,
    OP_CODE_FIRMWARE_UPDATE_CANCEL = 0x02
} fwUpdateOpCode_t;

/**
 * @brief Init ota information
 *
 * @details This function will Initialize firmware update information before updating device .
 *
 */
void FwUpdateInit(void);

/**
 * @brief Get OTA client state
 *
 * @details This function will get firmware update state.
 *
 * @return return firmware update state
 *
 */
FwUpdateState_t FwUpdateGetState (void);

/**
 * @brief Send indication to server (Plugins)
 *
 * @details This function will send message to server (Plugins)
 *
 * @param[in]  endUserDevice  The device info of the connected peer
 * @param[in]  state  firmware update state
 *
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 */
TagError_t FwUpdateSendCommand (EndUserDevice *endUserDevice, FwUpdateState_t state);

/**
 * @brief Handles the Attribute Written Without Response GATT event
 *
 * @details This function will get firmware data from server (Plugins).
 *
 * @param[in]  endUserDevice  The device info of the connected peer
 * @param[in]  state  firmware update state
 *
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 */
TagError_t FwUpdateAttributeWrittenWithoutResponse (EndUserDevice *endUserDevice, TagControlServiceData *data);

#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
#endif /* TAGSDK_INC_TAGFWUPDATE_H_ */

/*! *********************************************************************************
 * @}
 ********************************************************************************** */
