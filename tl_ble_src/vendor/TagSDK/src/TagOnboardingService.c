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

#include "PortOs.h"
#include "TagConfig.h"

#include "TagCore.h"
#include "TagDebug.h"
#include "TagErrorType.h"
#include "TagNV.h"
#include "TagNVItem.h"
#include "TagOnboardingService.h"
#include "TagSecurity.h"
#include "TagSoundPlayer.h"
#include "TagUtil.h"
#include "TagVersion.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "OBD"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

#define CLOUDKEY_MAX_LENGTH 32
#define CONFIRM_STATUS_LENGTH 1
#define CONVERTED_RANDUM_VALUE_LENGTH 32
#define HASHED_SERIAL_LENGTH 32
#define ONBOARDING_CANCEL 0
#define ONBOARDING_FINISH 1
#define SC_CAPABILITY_LEN 1
#define SEED_MAX_LENGTH 32
#define TAG_INIT_SIZE 1
#define TRANSFER_LOG_DATA_OPCODE 0x01
#define TRANSFER_LOG_INFORMATION_OPCODE 0x00
#define TRANSFER_LOG_INFORMATION_LENGTH 6
#define TRANSFER_LOGGING_OFF 0
#define TRANSFER_LOGGING_ON 1
#define ONBOARDING_STATUS(x) ((x)->onBoardingStatus)

STATIC_VARIABLE size_t sDumpOffset = 0;
STATIC_VARIABLE size_t sHeaderOffset = 0;
STATIC_VARIABLE unsigned char sButtonPress = BUTTON_RELEASE;

STATIC_FUNCTION TagError_t tagOnboardingGetTagKey(void);

STATIC_FUNCTION TagError_t tagOnboardingGetTagKey(void)
{
    TagError_t tagError;
    TagSecurityBuffer_t easysetupKeyBuf = {0};

    tagError = TagAuthGetTagKey(gSecuContext, gTagContext->endUserDevices, TAG_SECURITY_TAG_KEY_TYPE_CK, &easysetupKeyBuf);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get tag sercurity key (%d)", tagError);
    }

    TagCryptoSecurityBufferFree(&easysetupKeyBuf);
    return tagError;
}

STATIC_FUNCTION TagError_t tagOnboardingDecryptData(BleEvent *event)
{
    TagError_t tagError = TAG_ERROR_NONE;
    TagSecurityBuffer_t decryptedMsg = {0};

    if ((TagCharIsEncrypted(event->eventData.gattData.charIndex) == true) && (event->eventType == BleAttributeWritten))
    {
        tagError = TagAuthDecryptData(event->eventData.gattData.value, event->eventData.gattData.valueLength,
                                      &decryptedMsg, gTagContext->endUserDevices, TAG_SECURITY_TAG_KEY_TYPE_CK);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to decrypt Data (%d)", tagError);
            goto exit;
        }

        memset(event->eventData.gattData.value, 0, event->eventData.gattData.valueLength);
        event->eventData.gattData.valueLength = decryptedMsg.len;
        memcpy(event->eventData.gattData.value, (unsigned char *)decryptedMsg.p, event->eventData.gattData.valueLength);
    }

exit:
    TagCryptoSecurityBufferFree(&decryptedMsg);
    return tagError;
}

STATIC_FUNCTION TagError_t tagOnboardingEncryptData(BleEvent *event)
{
    TagError_t tagError = TAG_ERROR_NONE;
    TagSecurityBuffer_t encryptedMsg = {0};

    if (TagCharIsEncrypted(event->eventData.gattData.charIndex) == true)
    {
        tagError = TagAuthEncryptData(event->eventData.gattData.value, event->eventData.gattData.valueLength,
                                      &encryptedMsg, gTagContext->endUserDevices, TAG_SECURITY_TAG_KEY_TYPE_CK);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to encrypt Data (%d)", tagError);
            goto exit;
        }

        memset(event->eventData.gattData.value, 0, event->eventData.gattData.valueLength);
        TagFree(event->eventData.gattData.value);
        event->eventData.gattData.valueLength = encryptedMsg.len;
        event->eventData.gattData.value = TagMalloc(event->eventData.gattData.valueLength);
        memset(event->eventData.gattData.value, 0, event->eventData.gattData.valueLength);
        memcpy(event->eventData.gattData.value, (unsigned char *)encryptedMsg.p, event->eventData.gattData.valueLength);
    }

exit:
    TagCryptoSecurityBufferFree(&encryptedMsg);
    return tagError;
}

STATIC_FUNCTION TagError_t tagOnboardingSetReadValue(BleEvent *event, const unsigned char *attrValue, size_t attrValueLen)
{
    TagError_t tagError = TAG_ERROR_NONE;

    event->eventData.gattData.valueLength = attrValueLen;
    event->eventData.gattData.value = TagMalloc(event->eventData.gattData.valueLength);
    memcpy(event->eventData.gattData.value, attrValue, event->eventData.gattData.valueLength);
    tagError = tagOnboardingEncryptData(event);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to encrypt message (%d)", tagError);
    }
    return tagError;
}

STATIC_FUNCTION TagError_t tagOnboardingSendIndication(PortBleConnInfo *connInfo, OnboardingCharacteristic characteristic,
                                              const unsigned char *attrValue, size_t attrValueLen)
{
    PortBleAttrInfo attrInfo;
    TagError_t tagError = TAG_ERROR_NONE;
    TagSecurityBuffer_t encryptedMsg = {0};
    unsigned char *indicationMsg = NULL;

    if (TagCharIsEncrypted(characteristic) == true)
    {
        tagError = TagAuthEncryptData(attrValue, attrValueLen, &encryptedMsg, gTagContext->endUserDevices, TAG_SECURITY_TAG_KEY_TYPE_CK);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to encrypt Data (%d)", tagError);
            goto exit;
        }

        indicationMsg = TagMalloc(encryptedMsg.len);
        memset(indicationMsg, 0, encryptedMsg.len);
        attrValueLen = encryptedMsg.len;
        memcpy(indicationMsg, (unsigned char *)encryptedMsg.p, attrValueLen);
        TagCryptoSecurityBufferFree(&encryptedMsg);
    }
    else
    {
        indicationMsg = TagMalloc(attrValueLen);
        memset(indicationMsg, 0, attrValueLen);
        memcpy(indicationMsg, attrValue, attrValueLen);
    }

    tagError = PortBleChangeAttrInfoByIndex(&attrInfo, characteristic);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Indication handle isn't found");
        goto exit;
    }

    tagError = PortBleGattSendIndication(connInfo, &attrInfo, indicationMsg, attrValueLen);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to send indication (%d)", tagError);
        goto exit;
    }

exit:
    if (indicationMsg)
        TagFree(indicationMsg);
    return tagError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingFirmwareVersion(BleEvent *event)
{
    const char *fwVer = DEVICE_FW_VERSION_STRING;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = tagOnboardingSetReadValue(event, (const unsigned char *)fwVer, strlen(fwVer));
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingBleScCapability(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    unsigned char capability[SC_CAPABILITY_LEN + 1] = {0, 0};

    TAG_LOG_I("Onboarding start!!!");

    gTagContext->onBoardingStatus = ONBOARDING_START;

    capability[0] = SC_TYPE_ED25519;

    tagError = tagOnboardingSetReadValue(event, (unsigned char *)capability, SC_CAPABILITY_LEN);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSerialHashed(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagDeviceInfoData_t *serialInfo = NULL;
    TagError_t tagError = TAG_ERROR_NONE;
    unsigned char hashedSn[HASHED_SERIAL_LENGTH] = {
        0,
    };

    serialInfo = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
    if (!serialInfo)
    {
        TAG_LOG_E("Failed to alloc device info - serial");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    tagError = TagCryptoSha256((const unsigned char *)serialInfo->data, serialInfo->dataLength, hashedSn, HASHED_SERIAL_LENGTH);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to make hashed Serial number");
        bleError = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
        goto exit;
    }

    tagError = tagOnboardingSetReadValue(event, (unsigned char *)hashedSn, HASHED_SERIAL_LENGTH);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    if (serialInfo)
    {
        TagFreeDeviceInfo(serialInfo);
    }
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingOwnershipConfirm(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    unsigned char confirmStatus[CONFIRM_STATUS_LENGTH + 1] = {0, 0};

    confirmStatus[0] = sButtonPress;

    tagError = tagOnboardingGetTagKey();
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to get tag key (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    tagError = tagOnboardingSetReadValue(event, (unsigned char *)confirmStatus, CONFIRM_STATUS_LENGTH);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingManufacturerName(BleEvent *event)
{
    const char *mnmnStr;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagNVData_t nvData;
    uint8_t len;

    nvData.data.manufacturerName = TagMalloc(TAG_NV_MANUFACTURER_NAME_MAX_SZ);
    if (nvData.data.manufacturerName == NULL)
    {
        TAG_LOG_E("Failed to alloc for manufacturerName");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset(nvData.data.manufacturerName, 0, TAG_NV_MANUFACTURER_NAME_MAX_SZ);

    tagError = TagNVLoad(TAG_NV_MANUFACTURER_NAME, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
#if !defined(TAG_CONFIG_USE_ONBOARD_CONF_HEADER)
        TAG_LOG_I("Failed to get TAG_NV_MANUFACTURER_NAME from NV. Try to get from header");
#endif
        TagFree(nvData.data.manufacturerName);

        /* Try to get mnmn from TagOnboardingService again */
        mnmnStr = TagGetOnboardConfStrPtr(TAG_ONBOARD_CONF_MANUFACTURER_NAME);
        if (!mnmnStr)
        {
            TAG_LOG_E("Failed to load mnmn from TagOnboardingConfig");
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)mnmnStr, strlen(mnmnStr));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
    }
    else
    {
        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)nvData.data.manufacturerName, strlen(nvData.data.manufacturerName));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            TagFree(nvData.data.manufacturerName);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
        TagFree(nvData.data.manufacturerName);
    }

    len = event->eventData.gattData.valueLength;

    if (len > MNMN_SIZE)
    {
        TAG_LOG_E("You needs to increase MNMN GattDB size. You must change MNMN_SIZE to %d on TagBleGattUUID.h", len);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingVendorID(BleEvent *event)
{
    const char *vidStr;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagNVData_t nvData;

    nvData.data.vendorId = TagMalloc(TAG_NV_VENDOR_ID_MAX_SZ);
    if (nvData.data.vendorId == NULL)
    {
        TAG_LOG_E("Failed to alloc for vendorId");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset(nvData.data.vendorId, 0, TAG_NV_VENDOR_ID_MAX_SZ);

    tagError = TagNVLoad(TAG_NV_VENDOR_ID, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
#if !defined(TAG_CONFIG_USE_ONBOARD_CONF_HEADER)
        TAG_LOG_I("Failed to load TAG_NV_VENDOR_ID from NV. Try to get from header");
#endif
        TagFree(nvData.data.vendorId);

        /* Try to get vendorId from TagOnboardingConfig again */
        vidStr = TagGetOnboardConfStrPtr(TAG_ONBOARD_CONF_VENDOR_ID);
        if (!vidStr)
        {
            TAG_LOG_E("Failed to load vid from TagOnboardingConfig");
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)vidStr, strlen(vidStr));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
    }
    else
    {
        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)nvData.data.vendorId, strlen(nvData.data.vendorId));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            TagFree(nvData.data.vendorId);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
        TagFree(nvData.data.vendorId);
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingIdentifier(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagDeviceInfoData_t *serialInfo = NULL;
    TagError_t tagError = TAG_ERROR_NONE;

    serialInfo = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
    if (!serialInfo)
    {
        TAG_LOG_E("Failed to alloc TagDeviceInfo for serial");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    tagError = tagOnboardingSetReadValue(event, (unsigned char *)serialInfo->data, serialInfo->dataLength);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    if (serialInfo)
    {
        TagFreeDeviceInfo(serialInfo);
    }
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingConfigurationVersion(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = tagOnboardingSetReadValue(event, (const unsigned char *)CONFIGURATION_VERSION_STRING, strlen((const char *)CONFIGURATION_VERSION_STRING));
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSupportedCipher(BleEvent *event)
{
    const char *supportedCipher;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    supportedCipher = TagAuthGetSupportedCipher();

    tagError = tagOnboardingSetReadValue(event, (const unsigned char *)supportedCipher, strlen(supportedCipher));
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSelectedCipher(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = TagAuthSaveSelectedCipher(gSecuContext, gTagContext->endUserDevices, (void *)event->eventData.gattData.value,
                                         event->eventData.gattData.valueLength);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save Selected Cipher (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSeed(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = TagAuthSaveInfo(gSecuContext, gTagContext->endUserDevices, TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_SEED,
                               (unsigned char *)event->eventData.gattData.value, event->eventData.gattData.valueLength);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save Privacy Id Seed (%d)", tagError);
        bleError = TAG_BLE_ERROR_INVALID_PDU;
        goto exit;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingNumberOfPrivacyID(BleEvent *event)
{
    TagNVData_t nvData;

    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    uint32_t poolSize = 0;

    const char *src = (const char *)event->eventData.gattData.value;

    if (event->eventData.gattData.valueLength != 4)
    {
        TAG_LOG_E("Failed to save Number Of PrivacyID");
        return TAG_BLE_ERROR_INVALID_PDU;
    }

    poolSize = GET_NUMBER_OF_PRIVACY_ID(src[3], src[2], src[1], src[0]);
    nvData.data.numberOfPrivacyId = poolSize;
    nvData.dataLength = sizeof(poolSize);

    tagError = TagNVStore(TAG_NV_NUMBER_OF_PRIVACY_ID, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to store NV regionID (%d)", tagError);
        return TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    gSecuContext->numberOfPrivacyId = poolSize;

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSetupComplete(BleEvent *event)
{
    const char *finish = "FINISH";
    int handle = event->eventData.gattData.portAttrInfo.handle;
    int onboarding_result = ONBOARDING_CANCEL;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagNVData_t nvData;

    TAG_LOG_I("Setup complete messge : %s", (const char *)event->eventData.gattData.value);

    if (!strncmp((const char *)event->eventData.gattData.value, (const char *)finish, event->eventData.gattData.valueLength))
    {
        nvData.data.region = gTagContext->regionID;
        nvData.dataLength = sizeof(nvData.data.region);
        tagError = TagNVStore(TAG_NV_REGION, &nvData);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to store NV regionID (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        nvData.dataLength = 1;
        nvData.data.onboarded = TAG_ONBOARDED_VALUE_ONBOARDED;

        tagError = TagNVStore(TAG_NV_ONBOARDED, &nvData);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to save TAG_NV_ONBOARDED (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
        onboarding_result = ONBOARDING_FINISH;
    }
    else
    {
        TAG_LOG_E("Onboarding fail");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    if (onboarding_result == ONBOARDING_FINISH)
    {
        TagTransferState(TAG_STATE_CONNECTED);
        PortTimerStart(gTagContext->OOBCompletedTimer, 0);
        gTagContext->OOBCompletedActiveFlag = 1;
        if (gTagContext->OOBAdvFastIntervalTimer)
        {
            PortTimerDelete(gTagContext->OOBAdvFastIntervalTimer, 0);
            gTagContext->OOBAdvFastIntervalTimer = NULL;
        }
        TagRefreshAdv();

        gTagContext->onBoardingStatus = ONBOARDING_COMPLETE;

        CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONNECTED), SOUND_NOT_PLAYED, "Failed to play CONNECTED sound");

        if (TagBleGattUnregisterCharacteristic(ONBOARDING_SERVICE) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to unregister Onboarding service GattCharacteristic ");
        }

        if (TagBleGattUnregisterService(ONBOARDING_SERVICE) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to unregister Onboarding service GattService");
        }
    }
    else
    {
        nvData.dataLength = 1;
        nvData.data.onboarded = TAG_ONBOARDED_VALUE_NOT_ONBOARDED;

        tagError = TagNVStore(TAG_NV_ONBOARDED, &nvData);
        if (tagError)
        {
            TAG_LOG_E("Failed to save TAG_NV_NOT_ONBOARDED (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        }
    }

    sButtonPress = BUTTON_RELEASE;

exit:
    event->eventData.gattData.portAttrInfo.handle = handle;
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSupportedConfirmMethod(BleEvent *event)
{
    const char *supportedConfirmMethod = "BUTTON, SERIAL";
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = tagOnboardingSetReadValue(event, (const unsigned char *)supportedConfirmMethod, strlen((const char *)supportedConfirmMethod));
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set read value (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSelectedConfirmMethod(BleEvent *event)
{
    const char *button = "BUTTON";
    const char *serial = "SERIAL";
    const char *buttonConfirm = "OK";
    int handle = event->eventData.gattData.portAttrInfo.handle;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagSecurityBuffer_t encryptedData = {0};

    if (!strncmp((const char *)event->eventData.gattData.value, (const char *)button, event->eventData.gattData.valueLength))
    {
        if (sButtonPress == BUTTON_PRESS)
        {
            TAG_LOG_I("Button is already confirmed");

            tagError = tagOnboardingSendIndication(&event->eventData.gattData.portConnHandle,
                                                   ONBD_CONFIRM_RESULT, (const unsigned char *)buttonConfirm, strlen((const char *)buttonConfirm));
            if (tagError != TAG_ERROR_NONE)
            {
                TAG_LOG_E("Logging handle isn't found (%d)", tagError);
                bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
                goto exit;
            }

            gTagContext->onBoardingStatus = CLOUD_REGISTRATION;
            goto exit;
        }
        gTagContext->onBoardingStatus = BUTTON_CONFIRM;
    }
    else if (!strncmp((const char *)event->eventData.gattData.value, (const char *)serial, event->eventData.gattData.valueLength))
    {
        TAG_LOG_I("Serial confirm");
        gTagContext->onBoardingStatus = QR_CONFIRM;
    }
    else
    {
        TAG_LOG_E("No matched confirm method (%s)", (char *)event->eventData.gattData.value);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    event->eventData.gattData.portAttrInfo.handle = handle;

    TagCryptoSecurityBufferFree(&encryptedData);
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingSerialConfirm(BleEvent *event)
{
    const char *serialNumberMatch = "OK";
    const char *serialNumberUnMatch = "DENIED";
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    int handle = event->eventData.gattData.portAttrInfo.handle;
    TagSecurityBuffer_t encryptedData = {0};
    TagDeviceInfoData_t *serialInfo = NULL;

    serialInfo = TagAllocDeviceInfo(TAG_DEVICE_INFO_SERIAL);
    if (!serialInfo)
    {
        TAG_LOG_E("Failed to alloc TagDeviceInfo for serial");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    if ((!strncmp((const char *)event->eventData.gattData.value, (const char *)serialInfo->data, event->eventData.gattData.valueLength)) && (event->eventData.gattData.valueLength == serialInfo->dataLength))
    {
        tagError = tagOnboardingSendIndication(&event->eventData.gattData.portConnHandle,
                                               ONBD_CONFIRM_RESULT, (const unsigned char *)serialNumberMatch, strlen((const char *)serialNumberMatch));
    }
    else
    {
        tagError = tagOnboardingSendIndication(&event->eventData.gattData.portConnHandle,
                                               ONBD_CONFIRM_RESULT, (const unsigned char *)serialNumberUnMatch, strlen((const char *)serialNumberUnMatch));
    }

    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Logging handle isn't found (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

exit:
    event->eventData.gattData.portAttrInfo.handle = handle;

    if (serialInfo)
    {
        TagFreeDeviceInfo(serialInfo);
    }
    TagCryptoSecurityBufferFree(&encryptedData);
    gTagContext->onBoardingStatus = CLOUD_REGISTRATION;
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingCloudPublicKey(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = TagAuthSaveInfo(gSecuContext, gTagContext->endUserDevices, TAG_SECURITY_TAG_INFO_TYPE_CLOUD_PUBLIC_KEY,
                               event->eventData.gattData.value, event->eventData.gattData.valueLength);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save cloud public key (%d)", tagError);
        bleError = TAG_BLE_ERROR_INVALID_PDU;
    }

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingRandomValue(BleEvent *event)
{
    int length;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    unsigned char randomValue[CONVERTED_RANDUM_VALUE_LENGTH] = {
        0,
    };

    tagError = TagUtilConvertHexToString(randomValue, CONVERTED_RANDUM_VALUE_LENGTH, (unsigned char *)event->eventData.gattData.value,
                                         event->eventData.gattData.valueLength, (size_t *)&length);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to convert hexstrinig (%d)", tagError);
        bleError = TAG_BLE_ERROR_INVALID_PDU;
        goto exit;
    }

    tagError = TagAuthSaveInfo(gSecuContext, gTagContext->endUserDevices, TAG_SECURITY_TAG_INFO_TYPE_RANDOM_VALUE,
                               (unsigned char *)randomValue, length);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save random value (%d)", tagError);
        bleError = TAG_BLE_ERROR_INVALID_PDU;
        goto exit;
    }

    tagError = TagAuthGenerateMasterSecret(gSecuContext);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to make master secret (%d)", tagError);
        bleError = TAG_BLE_ERROR_AUTHENTICATION_FAILURE;
        goto exit;
    }
exit:
    if (TagAuthRemoveOnboardingInfo(gSecuContext) != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to reset onboarding security info ");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingRegion(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;

    memcpy(&gTagContext->regionID, event->eventData.gattData.value, 1);

    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingModelName(BleEvent *event)
{
    const char *modelNameStr;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    TagNVData_t nvData;
    uint8_t len;

    nvData.data.modelName = TagMalloc(TAG_NV_MODEL_NAME_MAX_SZ);
    if (nvData.data.modelName == NULL)
    {
        TAG_LOG_E("Failed to alloc for modelName");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset(nvData.data.modelName, 0, TAG_NV_MODEL_NAME_MAX_SZ);

    tagError = TagNVLoad(TAG_NV_MODEL_NAME, &nvData);
    if (tagError != TAG_ERROR_NONE)
    {
#if !defined(TAG_CONFIG_USE_ONBOARD_CONF_HEADER)
        TAG_LOG_I("Failed to load TAG_NV_MODEL_NAME from NV. Try to get from header");
#endif
        TagFree(nvData.data.modelName);

        /* Try to get modelName from TagOnboardingConfig again */
        modelNameStr = TagGetOnboardConfStrPtr(TAG_ONBOARD_CONF_MODEL_NAME);
        if (!modelNameStr)
        {
            TAG_LOG_E("Failed to load modelName from TagOnboardingConfig");
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)modelNameStr, strlen(modelNameStr));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
    }
    else
    {
        tagError = tagOnboardingSetReadValue(event, (const unsigned char *)nvData.data.modelName, strlen(nvData.data.modelName));
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to set read value (%d)", tagError);
            TagFree(nvData.data.modelName);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }
        TagFree(nvData.data.modelName);
    }

    len = event->eventData.gattData.valueLength;

    if (len > MODEL_NAME_SIZE)
    {
        TAG_LOG_E("You needs to increase MODEL_NAME GattDB size. You must change MODEL_NAME_SIZE to %d on TagBleGattUUID.h", len);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
    }

exit:
    return bleError;
}

TagBleError_t TagOnboardingTransferLogData(BleEvent *event)
{
    uint16_t mtuSize;
    size_t log_size = 0;
    size_t offset = 0;
    size_t maxLogSegmentSize = 0;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;
    unsigned char *segmentedLogDataTransfer = NULL;

    if (gTagContext->endUserDevices->logParam)
    {
        tagError = PortBleGetMtu(&gTagContext->endUserDevices->portConnHandle, &mtuSize);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to get MTU %d", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        segmentedLogDataTransfer = TagMalloc(mtuSize);
        if (segmentedLogDataTransfer == NULL)
        {
            TAG_LOG_E("Failed to allocate TransferLogData");
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

        memset(segmentedLogDataTransfer, 0, mtuSize);

        segmentedLogDataTransfer[offset++] = TRANSFER_LOG_DATA_OPCODE;

        offset += writeLittleEndian(segmentedLogDataTransfer, offset, gTagContext->endUserDevices->logParam->writtenLen, sizeof(uint32_t));
        maxLogSegmentSize = (((mtuSize - 3) - (mtuSize - 3) % 16) - 16);
        log_size = TagLogGetLog(gTagContext->endUserDevices->logParam, (char *)segmentedLogDataTransfer + offset + sizeof(unsigned short), maxLogSegmentSize, LOG_FROM_RAM);
        offset += writeLittleEndian(segmentedLogDataTransfer, offset, log_size, sizeof(unsigned short));

        tagError = tagOnboardingSendIndication(&event->eventData.gattData.portConnHandle,
                                               ONBD_LOGGING, (unsigned char *)segmentedLogDataTransfer, log_size + offset);
        if (tagError != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Logging handle isn't found (%d)", tagError);
            bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
            goto exit;
        }

    exit:
        if (segmentedLogDataTransfer)
        {
            TagFree(segmentedLogDataTransfer);
        }
        if (log_size == 0)
        {
            TagLogParamDeinit(gTagContext->endUserDevices->logParam);
            gTagContext->endUserDevices->logParam = NULL;
        }
    }
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingTransferLogInfo(BleEvent *event)
{
    size_t logSize = 0;
    size_t offset = 0;
    unsigned char *startLogTransfer;
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    startLogTransfer = TagMalloc(TRANSFER_LOG_INFORMATION_LENGTH);
    if (startLogTransfer == NULL)
    {
        TAG_LOG_E("Failed to allocate TransferLogInfo");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    memset(startLogTransfer, 0, TRANSFER_LOG_INFORMATION_LENGTH);

    startLogTransfer[offset++] = TRANSFER_LOG_INFORMATION_OPCODE;

    if (gTagContext->endUserDevices->logParam)
    {
        TagLogParamDeinit(gTagContext->endUserDevices->logParam);
    }

    gTagContext->endUserDevices->logParam = TagLogParamInit(TAG_LOG_TYPE_LOGGING, LOG_FROM_RAM);

    logSize = TagLogGetSize(gTagContext->endUserDevices->logParam);

    offset += writeLittleEndian(startLogTransfer, offset, (uint64_t)logSize, sizeof(uint32_t));

    tagError = PortBleGattSendIndication(&event->eventData.gattData.portConnHandle, &event->eventData.gattData.portAttrInfo, (unsigned char *)startLogTransfer, offset);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to send indication TransferLogInfo (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

exit:
    if (startLogTransfer)
    {
        TagFree(startLogTransfer);
    }
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingLogging(BleEvent *event)
{
    TagBleError_t bleError;

    sHeaderOffset = 0;
    sDumpOffset = 0;

    if ((gTagContext->onBoardingStatus > PRE_ONBOARDING) && (gTagContext->onBoardingStatus < ONBOARDING_COMPLETE))
    {
        TAG_LOG_E("Failed to complete onboarding (%d).", gTagContext->onBoardingStatus );
    }

    TAG_LOG_I("Logging start");

    bleError = tagOnboardingTransferLogInfo(event);
    if (bleError)
    {
        TAG_LOG_E("Failed to transfer Loginfo (%d)", bleError);
        goto exit;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingPrivacyIdIv(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    tagError = TagAuthSaveInfo(gSecuContext, gTagContext->endUserDevices, TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_IV,
                               (unsigned char *)event->eventData.gattData.value, event->eventData.gattData.valueLength);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save Privacy Id Iv (%d)", tagError);
        bleError = TAG_BLE_ERROR_INVALID_PDU;
        goto exit;
    }

exit:
    return bleError;
}

STATIC_FUNCTION TagBleError_t tagOnboardingReportButtonConfirm(void)
{
    const char *buttonConfirm = "OK";
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError = TAG_ERROR_NONE;

    gTagContext->onBoardingStatus = CLOUD_REGISTRATION;

    tagError = tagOnboardingSendIndication(&gTagContext->endUserDevices->portConnHandle, ONBD_CONFIRM_RESULT,
                                           (const unsigned char *)buttonConfirm, strlen((const char *)buttonConfirm));
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Logging handle isn't found (%d)", tagError);
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

exit:
    return bleError;
}

void TagOnboardingReportButtonEvent(ButtonStatus buttonPress)
{
    TagError_t tagError;
    bool saveButtonStatus = true;

    switch (ONBOARDING_STATUS(gTagContext))
    {
    case BUTTON_CONFIRM:
        if (buttonPress == BUTTON_PRESS)
        {
            tagError = tagOnboardingReportButtonConfirm();
            if (tagError)
            {
                TAG_LOG_E("Failed to report button confirmation (%d)", tagError);
                break;
            }
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_CONFIRM), SOUND_NOT_PLAYED, "Failed to play CONFIRM sound");
        }
        break;
    case PRE_ONBOARDING:
    case ONBOARDING_COMPLETE:
    case ONBOARDED:
        /* Onboarding button event is valid only onboarding is in progress */
        TAG_LOG_D("Onboarding is not in progress(%u)", gTagContext->onBoardingStatus);
        saveButtonStatus = false;
        /* fall through */
    case ONBOARDING_START:
    case QR_CONFIRM:
    case CLOUD_REGISTRATION:
    default:
        if (buttonPress == BUTTON_PRESS)
        {
            CHECK_RESULT_EQ(TagSoundPlayItem(SOUND_ITEM_PRESS), SOUND_NOT_PLAYED, "Failed to play PRESS sound");
        }
        break;
    }

    if (saveButtonStatus)
    {
        sButtonPress = buttonPress;
    }
}

void TagOnboardingCheckOnboardingFailRecovery(BleEvent *event)
{
    if ((gTagContext->onBoardingStatus > PRE_ONBOARDING) && (gTagContext->onBoardingStatus < ONBOARDING_COMPLETE))
    {
        TAG_LOG_I("Failed to complete onboarding. Clear data");

        if (TagNVFactoryReset() != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to reset NV Factory");
        }

        if (TagCryptoContextReset(gSecuContext) != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to reset Crypto context");
        }

        gTagContext->onBoardingStatus = PRE_ONBOARDING;
    }
}

TagBleError_t TagOnboardingCallback(BleEvent *event)
{
    TagBleError_t bleError = TAG_BLE_ERROR_ATT_NO_ERROR;
    TagError_t tagError;

    TAG_LOG_D("TagOnboardingCallback (handle:%d,index:%d)", event->eventData.gattData.portAttrInfo.handle,
              event->eventData.gattData.charIndex);

    if (!gTagContext->endUserDevices)
    {
        TAG_LOG_E("No ble connection");
        bleError = TAG_BLE_ERROR_UNDEFINED_ERROR;
        goto exit;
    }

    tagError = tagOnboardingDecryptData(event);
    if (tagError != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to decrypt the message (%d)", tagError);
        bleError = TAG_BLE_ERROR_INSUFFICIENT_ENCRYPTION;
        goto exit;
    }

    switch (event->eventData.gattData.charIndex)
    {
    case ONBD_DEVICE_FIRMWARE_VERSION:
        bleError = tagOnboardingFirmwareVersion(event);
        break;
    case ONBD_BLE_SC_CAPABILITY:
        bleError = tagOnboardingBleScCapability(event);
        break;
    case ONBD_HASHED_SERIAL_NUMBER:
        bleError = tagOnboardingSerialHashed(event);
        break;
    case ONBD_CONFIRM_STATUS:
        bleError = tagOnboardingOwnershipConfirm(event);
        break;
    case ONBD_MNMN:
        bleError = tagOnboardingManufacturerName(event);
        break;
    case ONBD_VID:
        bleError = tagOnboardingVendorID(event);
        break;
    case ONBD_IDENTIFIER:
        bleError = tagOnboardingIdentifier(event);
        break;
    case ONBD_CONFIGURATION_VERSION:
        bleError = tagOnboardingConfigurationVersion(event);
        break;
    case ONBD_SUPPORTED_CIPHER:
        bleError = tagOnboardingSupportedCipher(event);
        break;
    case ONBD_SELECTED_CIPHER:
        bleError = tagOnboardingSelectedCipher(event);
        break;
    case ONBD_SEED:
        bleError = tagOnboardingSeed(event);
        break;
    case ONBD_NUMBER_OF_PRIVACY_ID:
        bleError = tagOnboardingNumberOfPrivacyID(event);
        break;
    case ONBD_SETUP_COMPLETE:
        bleError = tagOnboardingSetupComplete(event);
        break;
    case ONBD_SUPPORTED_CONFIRM_METHOD_LIST:
        bleError = tagOnboardingSupportedConfirmMethod(event);
        break;
    case ONBD_SELECTED_CONFIRM_METHOD:
        bleError = tagOnboardingSelectedConfirmMethod(event);
        break;
    case ONBD_SERIAL_CONFIRM:
        bleError = tagOnboardingSerialConfirm(event);
        break;
    case ONBD_CLOUD_PUBLIC_KEY:
        bleError = tagOnboardingCloudPublicKey(event);
        break;
    case ONBD_RANDOM_VALUE:
        bleError = tagOnboardingRandomValue(event);
        break;
    case ONBD_REGION:
        bleError = tagOnboardingRegion(event);
        break;
    case ONBD_MODEL_NAME:
        bleError = tagOnboardingModelName(event);
        break;
    case ONBD_LOGGING:
        bleError = tagOnboardingLogging(event);
        break;
    case ONBD_PRIVACY_ID_VECTOR:
        bleError = tagOnboardingPrivacyIdIv(event);
        break;
    default:
        TAG_LOG_E("Nothing to handle (index:%d)", event->eventData.gattData.charIndex);
        bleError = TAG_BLE_ERROR_REQUEST_NOT_SUPPORTED;
        break;
    }

exit:
    return bleError;
}
