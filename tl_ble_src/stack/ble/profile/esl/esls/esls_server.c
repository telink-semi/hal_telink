/********************************************************************************************************
 * @file    esls_server.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

static int blt_eslss_init(u8 initType, const void *param);

_attribute_ble_data_retention_ /* retention TODO: */
    blc_eslp_server_ctrl_t eslp_server_ctrl = {
        .process =
            {
                      .pNext       = NULL,
                      .id          = ESL_ESLS_SERVER,
                      .usedAclRole = 0,
                      .init        = blt_eslss_init,
                      .connect     = NULL,
                      .discov      = NULL,
                      .loop        = NULL,
                      },
};

void blc_esl_registerESLSControlServer(const blc_eslss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&eslp_server_ctrl, param);
}

blc_esls_server_t *blt_eslp_getServerInst(u16 connHandle)
{
#if (0) //TODO
    int ret = blc_audio_getAclRole(connHandle);
    if (ret < 0) {
        BLT_ESLS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            blt_audio_sendSvrGapRoleErrEvt(connHandle, ESL_ESLS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &eslp_server_ctrl.eslpServer;
}

static void blt_eslss_sendEvt(u16 connHandle, int evtID, u8 *data, u16 dataLen)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);

    if (eslss->cb) {
        eslss->cb(connHandle, evtID, data, dataLen);
    }
}

static int blt_eslss_eslAddressWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_eslss_eslAddressEvt_t pEvt;

    if (valueLen != sizeof(blc_esls_eslAddress_t)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (writeValue[0] == 0xFF) {
        // ESL spec 3.1.1.2 ESL_ID: "The value 0xFF is reserved for the Broadcast Address"
        return ATT_ERR_VALUE_NOT_ALLOWED;
    }

    pEvt.eslAddress.eslId   = writeValue[0];
    pEvt.eslAddress.groupId = writeValue[1] & 0x7F;

    blt_eslss_sendEvt(connHandle, ESL_EVT_ESLSS_ESL_ADDRESS, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_eslss_apSyncKetMaterialWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_eslss_apSyncKeyMaterialEvt_t pEvt;

    if (valueLen != sizeof(pEvt.apSyncKeyMaterial)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    memcpy(pEvt.apSyncKeyMaterial.sessionKey, writeValue, sizeof(pEvt.apSyncKeyMaterial.sessionKey));
    memcpy(pEvt.apSyncKeyMaterial.IV, &writeValue[16], sizeof(pEvt.apSyncKeyMaterial.IV));

    blt_eslss_sendEvt(connHandle, ESL_EVT_ESLSS_AP_SYNC_KEY_MATERIAL, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_eslss_eslResponseKeyMaterialWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_eslss_eslResponseKeyMaterialEvt_t pEvt;

    if (valueLen != sizeof(pEvt.eslResponseKeyMaterial)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    memcpy(pEvt.eslResponseKeyMaterial.sessionKey, writeValue, sizeof(pEvt.eslResponseKeyMaterial.sessionKey));
    memcpy(pEvt.eslResponseKeyMaterial.IV, &writeValue[16], sizeof(pEvt.eslResponseKeyMaterial.IV));

    blt_eslss_sendEvt(connHandle, ESL_EVT_ESLSS_ESL_RESPONSE_KEY_MATERIAL, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static int blt_eslss_eslCurrentAbsoluteTimeWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_eslss_eslCurrentAbsoluteTimeEvt_t pEvt;

    if (valueLen != sizeof(pEvt.eslCurrentAbsoluteTime)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    BYTE_TO_UINT32(pEvt.eslCurrentAbsoluteTime, writeValue);

    blt_eslss_sendEvt(connHandle, ESL_EVT_ESLSS_ESL_CURRENT_ABSOLUTE_TIME, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static ble_sts_t blt_esl_fillResponse(blc_eslss_controlPointResponseHdr_t *rsp, u8 *value, u16 *len)
{
    u8  opcode;
    u8 *ptr;

    ptr    = value + 1;
    opcode = rsp->opcode;

    switch (rsp->opcode) {
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR:
    {
        blc_eslss_controlPointResponseError_t *errorRsp = (blc_eslss_controlPointResponseError_t *)rsp;

        if (*len < sizeof(*errorRsp)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.1 Error
        U8_TO_STREAM(ptr, errorRsp->error);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_LED_STATE:
    {
        blc_eslss_controlPointResponseLedState_t *ledStateRsp = (blc_eslss_controlPointResponseLedState_t *)rsp;

        if (*len < sizeof(*ledStateRsp)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.2 LED State
        U8_TO_STREAM(ptr, ledStateRsp->ledId);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_BASIC_STATE:
    {
        blc_eslss_controlPointResponseBasicState_t *basicRsp = (blc_eslss_controlPointResponseBasicState_t *)rsp;

        if (*len < sizeof(*basicRsp)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.3 Basic State
        U16_TO_STREAM(ptr,
                      basicRsp->serviceNeeded | basicRsp->synchronized << 1 | basicRsp->activeLed << 2 | basicRsp->pendingLedUpdate << 3 | basicRsp->pendingDisplayUpdate << 4);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_DISPLAY_STATE:
    {
        blc_eslss_controlPointResponseDisplayState_t *displayState = (blc_eslss_controlPointResponseDisplayState_t *)rsp;

        if (*len < sizeof(*displayState)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.4 Display State
        U8_TO_STREAM(ptr, displayState->displayId);
        U8_TO_STREAM(ptr, displayState->imageId);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_0:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_1:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_2:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_3:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_4:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_5:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_6:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_7:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_8:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_9:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_A:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_B:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_C:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_D:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_E:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_F:
    {
        blc_eslss_controlPointResponseSensorValue_t *sensorValue = (blc_eslss_controlPointResponseSensorValue_t *)rsp;

        if (*len < (sizeof(*sensorValue) + ((opcode & 0xF0) >> 4))) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.5 Sensor Value
        U8_TO_STREAM(ptr, sensorValue->sensorId);
        memcpy(ptr, sensorValue->sensorData, (opcode & 0xF0) >> 4);
        ptr += (opcode & 0xF0) >> 4;
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_0:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_1:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_2:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_3:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_4:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_5:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_6:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_7:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_8:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_9:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_A:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_B:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_C:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_D:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_E:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_F:
    {
        blc_eslss_controlPointResponseVendorSpecific_t *vendor = (blc_eslss_controlPointResponseVendorSpecific_t *)rsp;

        if (*len < (sizeof(*vendor) + ((opcode & 0xF0) >> 4) + 1)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        // ESLS 3.9.3.6 Vendor-specific response
        memcpy(ptr, vendor->parameters, ((opcode & 0xF0) >> 4) + 1);
        ptr += ((opcode & 0xF0) >> 4) + 1;
        break;
    }
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    value[0] = opcode;
    *len     = ptr - value;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslss_updateEslControlPointResponse(u16 connHandle, blc_eslss_controlPointResponseHdr_t *rsp)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    ble_sts_t          status;
    u8                *value = NULL;
    u16               *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslControlPointHdl, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // Set space available in buffer
    *len = BLC_ESLS_CMD_RSP_MAX_LENGTH;

    status = blt_esl_fillResponse(rsp, value, len);
    if (status == BLE_SUCCESS) {
        return blc_gatts_notifyAttr(connHandle, eslss->eslControlPointHdl);
    }

    *len = 0;

    return status;
}

static bool blt_eslss_sensorCharSupported(blc_esls_server_t *eslss)
{
    return !!eslss->eslSensorInformationHdl;
}

static bool blt_eslss_validSensorId(blc_esls_server_t *eslss, u16 connHandle, u8 sensorId)
{
    u8  *value  = NULL;
    u16 *len    = NULL;
    u16  offset = 0;
    u8   id     = 0;

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslSensorInformationHdl, &value, &len);
    if (!value) {
        return false;
    }

    while (offset < *len) {
        if (id == sensorId) {
            return true;
        }

        if (value[offset] == BLC_ESLS_SENSOR_INFORMATION_SIZE_0) {
            offset += 3;
        } else {
            offset += 5;
        }

        id++;
    }

    return false;
}

static bool blt_eslss_displayCharSupported(blc_esls_server_t *eslss)
{
    return !!eslss->eslDisplayInformationHdl;
}

static bool blt_eslss_validDisplayId(blc_esls_server_t *eslss, u16 connHandle, u8 displayId)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslDisplayInformationHdl, &value, &len);
    if (!value) {
        return false;
    }

    return displayId < (*len / 5);
}

static bool blt_eslss_validImageId(blc_esls_server_t *eslss, u16 connHandle, u8 imageId)
{
    u8 *value;

    value = blc_gatts_getAttributeValueByHandle(connHandle, eslss->eslImageInformationHdl);
    if (!value) {
        return false;
    }

    return imageId <= *value;
}

static bool blt_eslss_ledCharSupported(blc_esls_server_t *eslss)
{
    return !!eslss->eslLedInformationHdl;
}

static bool blt_eslss_validLedId(blc_esls_server_t *eslss, u16 connHandle, u8 ledId)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslLedInformationHdl, &value, &len);
    if (!value) {
        return false;
    }

    return ledId < *len;
}

static void blt_eslss_setEslControlPointErrorResponse(u8 errorCode, blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen)
{
    blc_eslss_controlPointResponseError_t *errorRsp = (blc_eslss_controlPointResponseError_t *)rsp;

    errorRsp->hdr.opcode = BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR;
    errorRsp->error      = errorCode;
    *rspLen              = sizeof(*errorRsp);
}

static int blt_eslss_eslProcessControlPointWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen, blc_eslss_controlPointCommandHdr_t *cmd, u16 *cmdLen,
                                                      blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    u8                 opcode, len, eslId, errorCode = 0;

    *rspLen = *cmdLen = 0;

    // ESLS spec 3.9.1.1 Command opcodes: "The shortest possible TLV is 2 octets in size, and the longest is 17 octets."
    if (valueLen < 2 || valueLen > 17) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    STREAM_TO_U8(opcode, writeValue);

    /*
     * ESLS spec 3.9.1.1.1 Additional requirements for commands: "In each command, the first parameter, consisting of the octet
     * immediately following the opcode, is the ESL_ID parameter."
     */
    STREAM_TO_U8(eslId, writeValue);
    if (connHandle == 0xFFFF) {
        // Should be Broadcast Address or ESL ID, otherwise ignore command
        if ((eslId != eslss->eslId) && (eslId != BLC_ESLS_ESL_ID_BROADCAST)) {
            goto done;
        }
    } else {
        if (eslId != eslss->eslId || eslss->eslId == BLC_ESLS_ESL_ID_BROADCAST) {
            /*
             * ESLS spec 3.9.2 Command behavior "If an opcode is written to the ESL Control Point
             * characteristic and the ESL_ID value specified within the opcode (as described in
             * Section 3.9.1.1.1) does not match the ESL_ID of the ESL or matches the Broadcast Address,
             * then the ESL shall reject the command by responding with the Error response"
             */
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;

            goto done;
        }
    }

    len = (opcode & 0xF0) >> 4;
    if (len + 2 != valueLen) {
        errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;

        goto done;
    }

    switch (opcode) {
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_PING:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UNASSOCIATE_FROM_AP:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_SERVICE_RESET:
        *cmdLen = sizeof(blc_eslss_controlPointCommandHdr_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_FACTORY_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UPDATE_COMPLETE:
        if (connHandle == 0xFFFF) {
            /*
             * ESLP specification 3.1.3 ESL behavior in the Synchronized state:
             * The Factory Reset command described in the ESL Service [5] is not
             * valid in the Synchronized state (see Section 5.3.1.3.1).
             * The Update Complete command described in the ESL Service [5]
             * is not valid in the Synchronized state (see Section 5.3.1.3.1).
             */
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_STATE;
            break;
        }
        *cmdLen = sizeof(blc_eslss_controlPointCommandHdr_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_READ_SENSOR_DATA:
    {
        blc_eslss_controlPointCommandReadSensorData_t *cmdReadSensorData = (blc_eslss_controlPointCommandReadSensorData_t *)cmd;
        u8                                             sensorId;

        if (!blt_eslss_sensorCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }

        STREAM_TO_U8(sensorId, writeValue);
        if (!blt_eslss_validSensorId(eslss, connHandle, sensorId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else {
            cmdReadSensorData->sensorId = sensorId;
            *cmdLen                     = sizeof(*cmdReadSensorData);
        }

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_REFRESH_DISPLAY:
    {
        blc_eslss_controlPointCommandRefreshDisplay_t *cmdRefreshDisplay = (blc_eslss_controlPointCommandRefreshDisplay_t *)cmd;
        u8                                             displayId;

        if (!blt_eslss_displayCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }

        STREAM_TO_U8(displayId, writeValue);
        if (!blt_eslss_validDisplayId(eslss, connHandle, displayId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else {
            cmdRefreshDisplay->displayId = displayId;
            *cmdLen                      = sizeof(*cmdRefreshDisplay);
        }

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_IMAGE:
    {
        blc_eslss_controlPointCommandDisplayImage_t *cmdDisplayImage = (blc_eslss_controlPointCommandDisplayImage_t *)cmd;
        u8                                           displayId, imageId;

        if (!blt_eslss_displayCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }
        STREAM_TO_U8(displayId, writeValue);
        STREAM_TO_U8(imageId, writeValue);
        if (!blt_eslss_validDisplayId(eslss, connHandle, displayId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else if (!blt_eslss_validImageId(eslss, connHandle, imageId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_IMAGE_INDEX;
        } else {
            cmdDisplayImage->displayId = displayId;
            cmdDisplayImage->imageId   = imageId;
            *cmdLen                    = sizeof(*cmdDisplayImage);
        }
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_TIMED_IMAGE:
    {
        blc_eslss_controlPointCommandDisplayTimedImage_t *cmdDisplayTimedImage = (blc_eslss_controlPointCommandDisplayTimedImage_t *)cmd;
        u8                                                displayId, imageId;
        u32                                               absoluteTime;

        if (!blt_eslss_displayCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }

        STREAM_TO_U8(displayId, writeValue);
        STREAM_TO_U8(imageId, writeValue);
        STREAM_TO_U32(absoluteTime, writeValue);
        if (!blt_eslss_validDisplayId(eslss, connHandle, displayId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else if (!blt_eslss_validImageId(eslss, connHandle, imageId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_IMAGE_INDEX;
        } else {
            cmdDisplayTimedImage->absoluteTime = absoluteTime;
            cmdDisplayTimedImage->displayId    = displayId;
            cmdDisplayTimedImage->imageId      = imageId;
            *cmdLen                            = sizeof(*cmdDisplayTimedImage);
        }
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_CONTROL:
    {
        blc_eslss_controlPointCommandLedControl_t *cmdLedControl = (blc_eslss_controlPointCommandLedControl_t *)cmd;
        u8                                         ledId;

        if (!blt_eslss_ledCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }

        STREAM_TO_U8(ledId, writeValue);
        if (!blt_eslss_validLedId(eslss, connHandle, ledId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else {
            u16 repeatVal;
            u8  colorSheme;

            STREAM_TO_U8(colorSheme, writeValue);

            cmdLedControl->ledId      = ledId;
            cmdLedControl->colorRed   = colorSheme & 0x03;
            cmdLedControl->colorGreen = (colorSheme & 0x0C) >> 2;
            cmdLedControl->colorBlue  = (colorSheme & 0x30) >> 4;
            cmdLedControl->brightness = (colorSheme & 0xC0) >> 6;
            memcpy(cmdLedControl->flashingPattern, writeValue, sizeof(cmdLedControl->flashingPattern));
            writeValue += sizeof(cmdLedControl->flashingPattern);

            STREAM_TO_U16(repeatVal, writeValue);
            cmdLedControl->repeatType     = repeatVal & 0x0001;
            cmdLedControl->repeatDuration = repeatVal >> 1;
            *cmdLen                       = sizeof(*cmdLedControl);
        }
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_TIMED_CONTROL:
    {
        blc_eslss_controlPointCommandLedTimedControl_t *cmdLedTimedControl = (blc_eslss_controlPointCommandLedTimedControl_t *)cmd;
        u8                                              ledId;

        if (!blt_eslss_ledCharSupported(eslss)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
            break;
        }

        STREAM_TO_U8(ledId, writeValue);
        if (!blt_eslss_validLedId(eslss, connHandle, ledId)) {
            errorCode = BLC_ESLSS_ERROR_CODE_INVALID_PARAMETERS;
        } else {
            u16 repeatVal;
            u8  colorSheme;

            STREAM_TO_U8(colorSheme, writeValue);

            cmdLedTimedControl->ledId      = ledId;
            cmdLedTimedControl->colorRed   = colorSheme & 0x03;
            cmdLedTimedControl->colorGreen = (colorSheme & 0x0C) >> 2;
            cmdLedTimedControl->colorBlue  = (colorSheme & 0x30) >> 4;
            cmdLedTimedControl->brightness = (colorSheme & 0xC0) >> 6;
            memcpy(cmdLedTimedControl->flashingPattern, writeValue, sizeof(cmdLedTimedControl->flashingPattern));
            writeValue += sizeof(cmdLedTimedControl->flashingPattern);
            STREAM_TO_U16(repeatVal, writeValue);

            cmdLedTimedControl->repeatType     = repeatVal & 0x0001;
            cmdLedTimedControl->repeatDuration = repeatVal >> 1;

            STREAM_TO_U32(cmdLedTimedControl->absoluteTime, writeValue);

            *cmdLen = sizeof(*cmdLedTimedControl);
        }

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_0:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_1:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_2:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_3:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_4:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_5:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_6:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_7:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_8:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_9:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_A:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_B:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_C:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_D:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_E:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_F:
    {
        blc_eslss_controlPointCommandVendorSpecific_t *cmdVendor = (blc_eslss_controlPointCommandVendorSpecific_t *)cmd;

        memcpy(cmdVendor->parameters, writeValue, len);
        *cmdLen = sizeof(*cmdVendor) + len;
        break;
    }
    default:
        errorCode = BLC_ESLSS_ERROR_CODE_INVALID_OPCODE;
        break;
    }

done:
    if (errorCode && (eslId != BLC_ESLS_ESL_ID_BROADCAST)) {
        blt_eslss_setEslControlPointErrorResponse(errorCode, rsp, rspLen);
    }

    if (*cmdLen) {
        // Add opcode and ESL ID fields
        cmd->eslId  = eslId;
        cmd->opcode = opcode;
    }

    return ATT_SUCCESS;
}

static int blt_eslss_eslControlPointWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    u8                                   buf[BLC_ESLS_CMD_RSP_MAX_LENGTH];
    blc_eslss_controlPointResponseHdr_t *rsp = (blc_eslss_controlPointResponseHdr_t *)buf;
    blc_eslss_controlPointCommandHdr_t  *cmd = (blc_eslss_controlPointCommandHdr_t *)buf;
    u16                                  cmdLen, rspLen;

    int status = blt_eslss_eslProcessControlPointWriteCback(connHandle, writeValue, valueLen, cmd, &cmdLen, rsp, &rspLen);
    if (status == ATT_SUCCESS) {
        if (rspLen) {
            blc_eslss_updateEslControlPointResponse(connHandle, rsp);
        } else {
            blt_eslss_sendEvt(connHandle, ESL_EVT_ESLSS_ESL_CONTROL_POINT_COMMAND, (u8 *)buf, cmdLen);
        }
    }

    return status;
}

static int blt_eslss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);

    (void)opcode;

    if (attrHandle == eslss->eslAddressHdl) {
        return blt_eslss_eslAddressWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == eslss->apSyncKetMaterialHdl) {
        return blt_eslss_apSyncKetMaterialWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == eslss->eslResponseKeyMaterialHdl) {
        return blt_eslss_eslResponseKeyMaterialWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == eslss->eslCurrentAbsoluteTimeHdl) {
        return blt_eslss_eslCurrentAbsoluteTimeWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == eslss->eslControlPointHdl) {
        return blt_eslss_eslControlPointWriteCback(connHandle, writeValue, valueLen);
    }

    return ATT_SUCCESS;
}

static void blt_eslss_initEslAddress(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Address char too many, max num is %d", p->num);
    } else {
        eslss->eslAddressHdl = p->charHandle;
    }
}

static void blt_eslss_initApSyncKeyMaterial(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: AP Sync Material char too many, max num is %d", p->num);
    } else {
        eslss->apSyncKetMaterialHdl = p->charHandle;
    }
}

static void blt_eslss_initEslResponseKeyMaterial(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL REsponse Key Material char too many, max num is %d", p->num);
    } else {
        eslss->eslResponseKeyMaterialHdl = p->charHandle;
    }
}

static void blt_eslss_initEslCurrentAbsoluteTime(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Current Absolute Time char too many, max num is %d", p->num);
    } else {
        eslss->eslCurrentAbsoluteTimeHdl = p->charHandle;
    }
}

static void blt_eslss_initEslDisplayInformation(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Display Information char too many, max num is %d", p->num);
    } else {
        eslss->eslDisplayInformationHdl = p->charHandle;
    }
}

static void blt_eslss_initEslImageInformation(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Image Information char too many, max num is %d", p->num);
    } else {
        eslss->eslImageInformationHdl = p->charHandle;
    }
}

static void blt_eslss_initEslSensorInformation(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Sensor Information char too many, max num is %d", p->num);
    } else {
        eslss->eslSensorInformationHdl = p->charHandle;
    }
}

static void blt_eslss_initEslLedInformation(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL LED Information char too many, max num is %d", p->num);
    } else {
        eslss->eslLedInformationHdl = p->charHandle;
    }
}

static void blt_eslss_initEslControlPoint(atts_foundCharParam_t *p, void *input)
{
    blc_esls_server_t *eslss = (blc_esls_server_t *)input;
    if (p->num) {
        BLT_ESLS_LOG("ERR: ESL Control Point char too many, max num is %d", p->num);
    } else {
        eslss->eslControlPointHdl = p->charHandle;
    }
}

static const atts_findCharList_t eslssChar[] = {
    {
        .charUuid    = characteristicEslAddressUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslAddress,
    },
    {
        .charUuid    = characteristicApSyncKeyMaterialUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initApSyncKeyMaterial,
    },
    {
        .charUuid    = characteristicEslResponseKeyMaterialUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslResponseKeyMaterial,
    },
    {
        .charUuid    = characteristicEslCurrentAbsoluteTimeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslCurrentAbsoluteTime,
    },
    {
        .charUuid    = characteristicEslDisplayInformationUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslDisplayInformation,
    },
    {
        .charUuid    = characteristicEslImageInformationUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslImageInformation,
    },
    {
        .charUuid    = characteristicEslSensorInformationUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslSensorInformation,
    },
    {
        .charUuid    = characteristicEslLedInformationUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslLedInformation,
    },
    {
        .charUuid    = characteristicEslControlPointUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_eslss_initEslControlPoint,
    },
};

static void blt_eslss_serviceInit(const void *param)
{
    blc_esls_server_t          *server     = blt_eslp_getServerInst(0xFFFF);
    const blc_eslss_regParam_t *eslssParam = param ? (const blc_eslss_regParam_t *)param : &defaultEslpsParam;
    memset((u8 *)server, 0, sizeof(server));
    blc_atts_findCharacteristicByServiceUuid((const u8 *)serviceElectronicShelfLabelUuid, ATT_16_UUID_LEN, eslssChar, ARRAY_SIZE(eslssChar), (void *)server);
    blc_eslss_clearEslId(0xFFFF);
    blc_eslss_updateEslDisplayInformation(0xFFFF, eslssParam->displayDataNum, eslssParam->displayData);
    blc_eslss_updateEslImageInformation(0xFFFF, eslssParam->maxImageIndex);
    blc_eslss_updateEslLedInformation(0xFFFF, eslssParam->ledInformationsNum, eslssParam->ledInfo);
    blc_eslss_updateEslSensorInformation(0xFFFF, eslssParam->sensorInformationsNum, eslssParam->sensorInfo);
}

static int blt_eslss_init(u8 initType, const void *param)
{
#if (0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_esls_server_t)), blc_esls_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        blc_svc_addEslsGroup();
        blc_svc_eslsCbackRegister(NULL, blt_eslss_writeCback);
        BLT_ESLS_LOG("Server init");
        blt_eslss_serviceInit(param);
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      blc_svc_removeEslsGroup();
    //      BLT_ESLS_LOG("Server Deinit");
    //  }
    return 0;
}

ble_sts_t blc_eslss_updateEslDisplayInformation(u16 connHandle, u8 displayDataNum, const blc_esls_displayData_t *data)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    extern const u16   eslsEslDisplayInformationMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;
    u8                *ptr;

    if (displayDataNum * 5 > eslsEslDisplayInformationMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // Check if displayData is correct
    for (u8 i = 0; i < displayDataNum; i++) {
        if (data[i].displayType > BLC_ESLS_DISPLAY_TYPE_FULL_RGB || data[i].displayType < BLC_ESLS_DISPLAY_TYPE_BLACK_WHITE) {
            return GATT_ERR_INVALID_PARAMETER;
        }
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslDisplayInformationHdl, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    ptr = value;
    for (u8 i = 0; i < displayDataNum; i++) {
        U16_TO_STREAM(ptr, data[i].width);
        U16_TO_STREAM(ptr, data[i].height);
        U8_TO_STREAM(ptr, data[i].displayType);
    }

    *len = ptr - value;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslss_updateEslImageInformation(u16 connHandle, u8 maxImageIndex)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    u8                *value;

    value = blc_gatts_getAttributeValueByHandle(connHandle, eslss->eslImageInformationHdl);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = maxImageIndex;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslss_updateEslSensorInformation(u16 connHandle, u8 sensorInformationsNum, const blc_esls_sensorInformation_t *info)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    extern const u16   eslsEslSensorInformationMaxSize;
    u8                *value        = NULL;
    u16               *len          = NULL;
    u16                requiredSize = 0;

    for (u8 i = 0; i < sensorInformationsNum; i++) {
        if (info[i].size == BLC_ESLS_SENSOR_INFORMATION_SIZE_0) {
            requiredSize += 3;
        } else if (info[i].size == BLC_ESLS_SENSOR_INFORMATION_SIZE_1) {
            requiredSize += 5;
        } else {
            return GATT_ERR_INVALID_PARAMETER;
        }
    }

    if (requiredSize > eslsEslSensorInformationMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslSensorInformationHdl, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    for (u8 i = 0; i < sensorInformationsNum; i++) {
        U8_TO_STREAM(value, info[i].size);
        if (info[i].size == BLC_ESLS_SENSOR_INFORMATION_SIZE_0) {
            U16_TO_STREAM(value, info[i].sensorType0);
        } else {
            U32_TO_STREAM(value, info[i].sensorType1);
        }
    }

    *len = requiredSize;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslss_updateEslLedInformation(u16 connHandle, u8 ledInformationsNum, const blc_esls_ledInformation_t *info)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);
    extern const u16   eslsEslLedInformationMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;

    if (ledInformationsNum > eslsEslLedInformationMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    // Check validity
    for (u8 i = 0; i < ledInformationsNum; i++) {
        if ((info[i].type != BLC_ESLS_LED_INFORMATION_SRGB) && (info[i].type != BLC_ESLS_LED_INFORMATION_MONOCHROME)) {
            return GATT_ERR_INVALID_PARAMETER;
        }
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, eslss->eslLedInformationHdl, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    for (u8 i = 0; i < ledInformationsNum; i++) {
        U8_TO_STREAM(value, BLC_ESLS_LED_FIELD_SET(BLC_ESLS_LED_TYPE, info[i].type) | BLC_ESLS_LED_FIELD_SET(BLC_ESLS_LED_BLUE, info[i].blue) |
                                BLC_ESLS_LED_FIELD_SET(BLC_ESLS_LED_GREEN, info[i].green) | BLC_ESLS_LED_FIELD_SET(BLC_ESLS_LED_RED, info[i].red));
    }

    *len = ledInformationsNum;

    return BLE_SUCCESS;
}

void blc_eslss_setEslId(u16 connHandle, u8 eslId)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);

    eslss->eslId = eslId;
}

void blc_eslss_clearEslId(u16 connHandle)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(connHandle);

    eslss->eslId = BLC_ESLS_ESL_ID_BROADCAST;
}

void blc_eslss_setElectronicShelfLabelCback(eslsCallback_t cb)
{
    blc_esls_server_t *eslss = blt_eslp_getServerInst(0xFFFF);

    eslss->cb = cb;
}

u16 blc_eslss_eslCommandPayloadParse(u8 *val, u16 len, blc_eslss_controlPointCommandHdr_t *cmd, u16 *cmdLen, blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen)
{
    u16 reqLen = 0;

    if (len < 2) {
        return 0;
    }

    reqLen = ((val[0] & 0xF0) >> 4) + 2;
    if (reqLen > len) {
        return 0;
    }

    blt_eslss_eslProcessControlPointWriteCback(0xFFFF, val, reqLen, cmd, cmdLen, rsp, rspLen);

    return reqLen;
}

bool blc_eslss_eslCommandPayloadGetRspSlot(u8 *val, u16 len, u8 *rspSlot)
{
    blc_esls_server_t *eslss     = blt_eslp_getServerInst(0xFFFF);
    bool               slotFound = false;
    u8                 slot      = 0;

    for (u8 offset = 0; offset < len;) {
        u8 param_len = (val[offset] & 0xF0) >> 4;

        if (param_len + 2 > (len - offset)) {
            break;
        }

        // check ESL id
        if (val[offset + 1] == eslss->eslId) {
            *rspSlot  = slot;
            slotFound = true;
        }

        offset += param_len + 2;
        slot++;
    }

    return slotFound;
}

u16 blc_eslss_eslResponsePayloadWrite(u8 *buffer, u16 length, blc_eslss_controlPointResponseHdr_t *rsp)
{
    return blt_esl_fillResponse(rsp, buffer, &length) == BLE_SUCCESS ? length : 0;
}

u16 blc_esl_getCommandSize(blc_eslss_controlPointCommandHdr_t *cmd)
{
    u16 len;

    switch (cmd->opcode) {
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_PING:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UNASSOCIATE_FROM_AP:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_SERVICE_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_FACTORY_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UPDATE_COMPLETE:
        len = sizeof(blc_eslss_controlPointCommandHdr_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_READ_SENSOR_DATA:
        len = sizeof(blc_eslss_controlPointCommandReadSensorData_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_REFRESH_DISPLAY:
        len = sizeof(blc_eslss_controlPointCommandRefreshDisplay_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_IMAGE:
        len = sizeof(blc_eslss_controlPointCommandDisplayImage_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_TIMED_IMAGE:
        len = sizeof(blc_eslss_controlPointCommandDisplayTimedImage_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_CONTROL:
        len = sizeof(blc_eslss_controlPointCommandLedControl_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_TIMED_CONTROL:
        len = sizeof(blc_eslss_controlPointCommandLedTimedControl_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_0:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_1:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_2:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_3:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_4:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_5:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_6:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_7:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_8:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_9:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_A:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_B:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_C:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_D:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_E:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_F:
        len = sizeof(blc_eslss_controlPointCommandVendorSpecific_t) + ((cmd->opcode & 0xF0) >> 4);
        break;
    default:
        len = 0;
        break;
    }

    return len;
}

u16 blc_esl_getResponseSize(blc_eslss_controlPointResponseHdr_t *rsp)
{
    u16 len;

    switch (rsp->opcode) {
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR:
        len = sizeof(blc_eslss_controlPointResponseError_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_LED_STATE:
        len = sizeof(blc_eslss_controlPointResponseLedState_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_BASIC_STATE:
        len = sizeof(blc_eslss_controlPointResponseBasicState_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_DISPLAY_STATE:
        len = sizeof(blc_eslss_controlPointResponseDisplayState_t);
        break;
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_0:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_1:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_2:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_3:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_4:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_5:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_6:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_7:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_8:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_9:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_A:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_B:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_C:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_D:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_E:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_F:
        len = sizeof(blc_eslss_controlPointResponseSensorValue_t) + ((rsp->opcode & 0xF0) >> 4);
        break;
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_0:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_1:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_2:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_3:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_4:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_5:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_6:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_7:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_8:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_9:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_A:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_B:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_C:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_D:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_E:
    case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_F:
        len = sizeof(blc_eslss_controlPointResponseVendorSpecific_t) + ((rsp->opcode & 0xF0) >> 4) + 1;
        break;
    default:
        len = 0;
        break;
    }

    return len;
}
