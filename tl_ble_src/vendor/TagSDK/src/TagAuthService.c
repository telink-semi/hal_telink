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

#include "TagConfig.h"

#include "TagAuthService.h"
#include "TagBleCallback.h"
#include "TagCore.h"
#include "TagErrorType.h"
#include "TagNV.h"
#include "TagSecurity.h"
#include "TagUtil.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

static TagError_t loadPrivacyIdInformation(void)
{
    TagNVData_t nvData;
    uint8_t *tmp;
    TagError_t ret;

    if (!gSecuContext)
    {
        TAG_LOG_E("security context is null");
        return TAG_ERROR_INVALID_ARG;
    }

    /*
     * Privacy ID Seed
     */
    tmp = (uint8_t *)TagMalloc(TAG_NV_PRIVACY_ID_SEED_MAX_SZ);
    if (!tmp)
    {
        TAG_LOG_E("Failed to alloc for Privacy ID Seed");
        return TAG_ERROR_MEM_ALLOC;
    }

    nvData.data.privacyIdSeed = tmp;
    ret = TagNVLoad(TAG_NV_PRIVACY_ID_SEED, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load Privacy ID Seed");
        TagFree(tmp);
        return ret;
    }

    gSecuContext->privacyIdSeed.p = tmp;
    gSecuContext->privacyIdSeed.len = nvData.dataLength;

    /*
     * Privacy ID IV
     */
    tmp = (uint8_t *)TagMalloc(TAG_NV_PRIVACY_ID_IV_MAX_SZ);
    if (!tmp)
    {
        TAG_LOG_E("Failed to alloc for Privacy ID IV");
        return TAG_ERROR_MEM_ALLOC;
    }

    nvData.data.privacyIdIv = tmp;
    ret = TagNVLoad(TAG_NV_PRIVACY_ID_IV, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load Privacy ID Iv");
        TagFree(tmp);
        return ret;
    }

    gSecuContext->privacyIdIv.p = tmp;
    gSecuContext->privacyIdIv.len = nvData.dataLength;

    /*
     * Number of Privacy ID
     */
    ret = TagNVLoad(TAG_NV_NUMBER_OF_PRIVACY_ID, &nvData);
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load Number of Privacy ID");
        return -1;
    }
    gSecuContext->numberOfPrivacyId = nvData.data.numberOfPrivacyId;

    return TAG_ERROR_NONE;
}

static TagError_t loadNvDataForOffline(void)
{
    if (gTagContext->state != TAG_STATE_OFFLINE)
    {
        return TAG_ERROR_NONE;
    }

    return loadPrivacyIdInformation();
}

static TagBleError_t tagValidationCheck(BleEvent *event, TagSecurityBuffer_t *encryptedDataT)
{
    TagError_t err = TAG_BLE_ERROR_ATT_NO_ERROR;
    EndUserDevice *endUserDevice;

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
    if (endUserDevice == NULL)
    {
        TAG_LOG_E("Failed to get endUserDevice");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    switch (endUserDevice->authParam->validationType)
    {
    case TAG_SECURITY_TAG_VALIDATION_TYPE_BAK:
        TAG_LOG_D("authenticated (Owner)");

        err = PortBleGattSendIndication(&event->eventData.gattData.portConnHandle, &event->eventData.gattData.portAttrInfo,
                                        encryptedDataT->p, encryptedDataT->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("failed to indicate %d", err);
            goto exit;
        }

        endUserDevice->commandKeyType = TAG_SECURITY_TAG_KEY_TYPE_CK;
        endUserDevice->deviceType = END_USER_DEVICE_OWNER;
        if (endUserDevice->unknownBleConnectionTimer)
        {
            PortTimerDelete(endUserDevice->unknownBleConnectionTimer, 0);
            endUserDevice->unknownBleConnectionTimer = NULL;
        }
        gTagContext->currentConnections++;
        /* Disconnect non owner end user devices when owner connected */
        if (gTagContext->nonOwnerConnections >= 1)
        {
            EndUserDevice *nonOwnerDevice = gTagContext->endUserDevices;
            while (nonOwnerDevice)
            {
                if (nonOwnerDevice->deviceType == END_USER_DEVICE_NON_OWNER)
                {
                    err = PortBleGapDisconnect(&nonOwnerDevice->portConnHandle);
                    if (err != TAG_ERROR_NONE)
                    {
                        TAG_LOG_E("Failed to excute PortBleGapDisconnect %d", err);
                    }
                }
                nonOwnerDevice = nonOwnerDevice->next;
            }
        }
        if (!IsTagStateOOB(gTagContext))
        {
            PortTimerStart(endUserDevice->initConnectionParmRecoveryTimer, 0);
            TagTransferState(TAG_STATE_CONNECTED);
        }
        TagRefreshAdv();
        break;
    case TAG_SECURITY_TAG_VALIDATION_TYPE_NBAK:
        if (gTagContext->state == TAG_STATE_OVERMATURE_OFFLINE)
        {
            TAG_LOG_D("authenticated (Non owner)");
            /* Only permit one non owner device connection */
            if (gTagContext->nonOwnerConnections >= 1)
            {
                TAG_LOG_I("Reject non owner connections (nonOwnerConnects:%d)", gTagContext->nonOwnerConnections);
                err = PortBleGapDisconnect(&event->eventData.gattData.portConnHandle);
                if (err != TAG_ERROR_NONE)
                {
                    TAG_LOG_E("Failed to excute PortBleGapDisconnect %d", err);
                }
                goto exit;
            }
            endUserDevice->commandKeyType = TAG_SECURITY_TAG_KEY_TYPE_NCK;
            endUserDevice->deviceType = END_USER_DEVICE_NON_OWNER;
            gTagContext->nonOwnerConnections++;
            if (endUserDevice->unknownBleConnectionTimer)
            {
                PortTimerDelete(endUserDevice->unknownBleConnectionTimer, 0);
                endUserDevice->unknownBleConnectionTimer = NULL;
            }
        }
        else
        {
            TAG_LOG_E("not authenticated by not overmature offline");
            err = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
        }
        break;
    case TAG_SECURITY_TAG_VALIDATION_TYPE_NOT_FOUND:
    case TAG_SECURITY_TAG_VALIDATION_TYPE_MAX:
    default:
        TAG_LOG_E("not authenticated");
        err = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
    }
exit:
    return err;
}

TagError_t AuthServiceInit(void)
{
    TagError_t ret;

    gSecuContext = TagCryptoContextInit();
    if (!gSecuContext)
    {
        TAG_LOG_E("Failed to init security context handle");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    ret = loadNvDataForOffline();
    if (ret != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load NV data for Auth Service");
        return ret;
    }

    return TAG_ERROR_NONE;
}

static TagBleError_t tagGattCipherReadProcess(BleEvent *event)
{
    const char *cipher;

    if (event == NULL)
    {
        TAG_LOG_E("Failed to get ble event");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    cipher = TagAuthGetSupportedCipher();

    event->eventData.gattData.valueLength = strlen(cipher);
    event->eventData.gattData.value = TagMalloc(event->eventData.gattData.valueLength);
    if (!event->eventData.gattData.value)
    {
        TAG_LOG_E("Failed to alloc gattData value for cipher");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    memcpy(event->eventData.gattData.value, cipher, event->eventData.gattData.valueLength);

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

static TagBleError_t tagGattCipherWriteProcess(BleEvent *event)
{
    if (event == NULL || event->eventData.gattData.value == NULL || event->eventData.gattData.valueLength == 0)
    {
        TAG_LOG_E("Failed to get client cipher");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
    TagUtilDumpMem("AUTH_CIPHER", event->eventData.gattData.value, event->eventData.gattData.valueLength);
#endif
    if (memcmp(event->eventData.gattData.value, TagAuthGetSupportedCipher(), event->eventData.gattData.valueLength))
    {
        TAG_LOG_E("not supported cipher (%s)", (char *)event->eventData.gattData.value);
        return TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

static TagBleError_t tagGattNonceWriteProcess(BleEvent *event)
{
    TagError_t err;
    EndUserDevice *endUserDevice = NULL;

    if (event == NULL || event->eventData.gattData.value == NULL || event->eventData.gattData.valueLength != TAG_SECURITY_IV_LEN)
    {
        TAG_LOG_E("Failed to get nonce info");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);

    err = TagAuthSaveInfo(gSecuContext, endUserDevice, TAG_SECURITY_TAG_INFO_TYPE_NONCE_E, event->eventData.gattData.value, event->eventData.gattData.valueLength);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save nonceE :%d", err);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
    TagUtilDumpMem("AUTH_NONCE E", event->eventData.gattData.value, event->eventData.gattData.valueLength);
#endif
    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

static TagBleError_t tagGattNonceWritePostProcess(BleEvent *event)
{
    TagError_t err;
    EndUserDevice *endUserDevice;

    if (event == NULL)
    {
        TAG_LOG_E("Failed to get attrInfo");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
    if (endUserDevice == NULL)
    {
        TAG_LOG_E("Failed to get endUserDevice");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    TagAuthCreateNonceT(gSecuContext, endUserDevice);

#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
    TagUtilDumpMem("AUTH_NONCE T", endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len);
#endif

    err = PortBleGattSendIndication(&event->eventData.gattData.portConnHandle, &event->eventData.gattData.portAttrInfo,
                                    endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to malloc");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

static TagBleError_t tagGattEncryptedDataWriteProcess(BleEvent *event)
{
    uint8_t *encryptedDataEBuf;

    if (event == NULL || event->eventData.gattData.value == NULL || event->eventData.gattData.valueLength == 0)
    {
        TAG_LOG_E("Failed to get attrInfo");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    encryptedDataEBuf = (uint8_t *)TagMalloc(event->eventData.gattData.valueLength);
    if (encryptedDataEBuf == NULL)
    {
        TAG_LOG_E("Failed to alloc, size = %zu", event->eventData.gattData.valueLength);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    memcpy(encryptedDataEBuf, event->eventData.gattData.value, event->eventData.gattData.valueLength);
    TagFree(event->eventData.gattData.value);
    event->eventData.gattData.value = encryptedDataEBuf;
#ifdef TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
    TagUtilDumpMem("AUTH_ENCRYPTED_DATA", event->eventData.gattData.value, event->eventData.gattData.valueLength);
#endif
    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

static TagBleError_t tagGattEncryptedDataWritePostProcess(BleEvent *event)
{
    TagError_t err;
    TagSecurityBuffer_t encryptedDataE = {0};
    TagSecurityBuffer_t encryptedDataT = {0};
    EndUserDevice *endUserDevice;

    if (event == NULL || event->eventData.gattData.value == NULL || event->eventData.gattData.valueLength == 0)
    {
        TAG_LOG_E("Failed to get attrInfo");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    endUserDevice = TagFindEndUserDeviceFromPortHandle(&event->eventData.gattData.portConnHandle);
    if (endUserDevice == NULL)
    {
        TAG_LOG_E("Failed to get endUserDevice");
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    encryptedDataE.p = event->eventData.gattData.value;
    encryptedDataE.len = event->eventData.gattData.valueLength;

    err = TagAuthCheckEncryptedDataValidation(gSecuContext, endUserDevice, &encryptedDataE, &encryptedDataT);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to check encrypted data %d", err);
        goto exit;
    }

    err = tagValidationCheck(event, &encryptedDataT);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to validation check %d", err);
        goto exit;
    }

exit:
    TagCryptoSecurityBufferFree(&encryptedDataT);
    return TAG_BLE_ERROR_ATT_NO_ERROR;
}

TagBleError_t AuthServiceReadCallback(BleEvent *event)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    TAG_LOG_D("AuthSvcReadCb enter");

    if (event == NULL)
    {
        TAG_LOG_E("auth event is null");
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    switch (event->eventData.gattData.charIndex)
    {
    case AUTH_CIPHER:
        status = tagGattCipherReadProcess(event);
        break;
    default:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    TAG_LOG_D("AuthSvcCb done = %d", status);

    return status;
}

TagBleError_t AuthServiceWrittenCallback(BleEvent *event)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;

    TAG_LOG_D("AuthSvcWrittenCb enter");

    if (event == NULL)
    {
        TAG_LOG_E("auth event is null");
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    switch (event->eventData.gattData.charIndex)
    {
    case AUTH_CIPHER:
        status = tagGattCipherWriteProcess(event);
        break;
    case AUTH_NONCE:
        status = tagGattNonceWriteProcess(event);
        break;
    case AUTH_ENCRYPTED_DATA:
        status = tagGattEncryptedDataWriteProcess(event);
        break;
    default:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    TAG_LOG_D("AuthSvcCb done = %d", status);

    return status;
}

void AuthServiceWrittenPostCallback(TagTaskWorkParam bleEvent)
{
    TagBleError_t status = TAG_BLE_ERROR_ATT_NO_ERROR;
    BleEvent *event = (BleEvent *)bleEvent;

    TAG_LOG_D("AuthSvcPostWrittenCb enter");

    if (event == NULL)
    {
        TAG_LOG_E("AuthSvcPostWrittenCb event is null");
        return;
    }

    switch (event->eventData.gattData.charIndex)
    {
    case AUTH_CIPHER:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        break;
    case AUTH_NONCE:
        status = tagGattNonceWritePostProcess(event);
        break;
    case AUTH_ENCRYPTED_DATA:
        status = tagGattEncryptedDataWritePostProcess(event);
        break;
    default:
        status = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
    }

    TAG_LOG_D("AuthSvcPostCb done = %d", status);

    if (status != TAG_BLE_ERROR_ATT_NO_ERROR)
    {
        TAG_LOG_E("AuthSvcPostCb error : %d", status);
    }

    if (event->eventData.gattData.value)
    {
        TagFree(event->eventData.gattData.value);
    }
    TagFree(event);
}
