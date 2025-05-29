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

#include "TagConfig.h"

#include "TagErrorType.h"
#include "TagNV.h"
#include "TagSecurity.h"

#include "PortRandom.h"
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
#include "PortEncryption.h"
#endif

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "ScAth"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

// #define DEBUG_SECURITY_AUTH_PID

const char *TagAuthGetSupportedCipher(void)
{
    const char *cipher = CIPHER_NAME_AES128_CSC_PKCS7PADDING;

    return cipher;
}

static uint16_t tagAuthGetPrivacyRand(uint32_t numberOfPrivacyId)
{
    static int privacyIdListIndex = 0;
    static uint16_t privacyIdList[TAG_SECURITY_PRIVACY_ID_CYCLE] = {0};
    int i, j;
    uint16_t randVal;
    int outVal;
    int sum = 0;
    int writtenNumber;

    if (privacyIdListIndex >= TAG_SECURITY_PRIVACY_ID_CYCLE)
    {
        writtenNumber = TAG_SECURITY_PRIVACY_ID_CYCLE;
    }
    else
    {
        writtenNumber = privacyIdListIndex;
    }

    randVal = (uint16_t)(PortRandomGetData() % (numberOfPrivacyId - writtenNumber));
    outVal = randVal;
    for (i = 0; i < writtenNumber; i++)
    {
        for (j = 0; j < writtenNumber; j++)
        {
            if (privacyIdList[j] <= outVal)
            {
                sum++;
            }
        }
        if (outVal == randVal + sum)
        {
            break;
        }
        else
        {
            outVal = randVal + sum;
            sum = 0;
            continue;
        }
    }

    privacyIdList[privacyIdListIndex % TAG_SECURITY_PRIVACY_ID_CYCLE] = outVal;
    privacyIdListIndex++;

    if (privacyIdListIndex == 2 * TAG_SECURITY_PRIVACY_ID_CYCLE)
    {
        privacyIdListIndex = TAG_SECURITY_PRIVACY_ID_CYCLE;
    }
    return outVal;
}

#ifdef DEBUG_SECURITY_AUTH_PID
static int tagUtilByteToString(uint8_t *byteArray, uint16_t byteArrayLen, char *hexStr, uint16_t hexStrLen, uint16_t *outLen)
{
    int i;
    int c = 0;

    if (!byteArray || !hexStr)
    {
        TAG_LOG_E("input invalid");
        return -1;
    }

    if (hexStrLen < (byteArrayLen * 2 + 1))
    {
        TAG_LOG_E("not enough output buffer size (%d < %d)", hexStrLen, (byteArrayLen * 2 + 1));
        return -1;
    }

    for (i = 0; i < byteArrayLen; i++)
    {
        c += snprintf(hexStr + c, hexStrLen - c, "%02x", byteArray[i]);
    }

    hexStr[c] = '\0';
    if (outLen)
    {
        *outLen = c;
    }

    return 0;
}
#endif

TagError_t TagAuthGetPrivacyId(TagSecurityContext_t *context,
                               TagSecurityBuffer_t *outputPrivacyId)
{
    TagError_t err;
    TagSecurityBuffer_t keyBuf = {0};
    TagSecurityBuffer_t encryptedPId = {0};
    uint16_t randVal;
    uint8_t rawPrivacyId[TAG_SECURITY_PRIVACY_ID_SEED_LEN + 4];

#ifdef DEBUG_SECURITY_AUTH_PID
    char dbg[(TAG_SECURITY_PRIVACY_ID_LEN + 4) * 2 + 1];
#endif

    if (!context || !outputPrivacyId || !context->privacyIdSeed.p || !context->privacyIdIv.p)
    {
        TAG_LOG_E("Failed to get Pid : input is null");
        return TAG_ERROR_INVALID_ARG;
    }

    if ((context->privacyIdSeed.len != TAG_SECURITY_PRIVACY_ID_SEED_LEN) || context->privacyIdIv.len != TAG_SECURITY_IV_LEN)
    {
        TAG_LOG_E("Failed to get Pid : length is invalid : seed %zu, iv %zu",
                  context->privacyIdSeed.len, context->privacyIdIv.len);
        return TAG_ERROR_INVALID_ARG;
    }

    randVal = tagAuthGetPrivacyRand(context->numberOfPrivacyId);

    TAG_LOG_D("ps: %d", context->numberOfPrivacyId);

    rawPrivacyId[0] = rawPrivacyId[TAG_SECURITY_PRIVACY_ID_SEED_LEN + 2] = (randVal >> 8) & 0xff;
    rawPrivacyId[1] = rawPrivacyId[TAG_SECURITY_PRIVACY_ID_SEED_LEN + 3] = (randVal)&0xff;
    memcpy(rawPrivacyId + 2, context->privacyIdSeed.p, TAG_SECURITY_PRIVACY_ID_SEED_LEN);

#ifdef DEBUG_SECURITY_AUTH_PID
    tagUtilByteToString(rawPrivacyId, sizeof(rawPrivacyId), dbg, sizeof(dbg), NULL);
    TAG_LOG_I("ScGetPid:raw PID: %s", dbg);
#endif

    err = TagAuthGetTagKey(context, 0, TAG_SECURITY_TAG_KEY_TYPE_PK, &keyBuf);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to load PK : %d", err);
        return err;
    }

    err = TagCryptoAesFunction(keyBuf.p, keyBuf.len, context->privacyIdIv.p,
                               context->privacyIdIv.len, TAG_SECURITY_KEY_TYPE_AES128,
                               TAG_SECURITY_CIPHER_ENCRYPT, rawPrivacyId, sizeof(rawPrivacyId),
                               &encryptedPId);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to generate PID : %d", err);
        goto exit;
    }

    outputPrivacyId->len = TAG_SECURITY_PRIVACY_ID_LEN;
    outputPrivacyId->p = (uint8_t *)TagMalloc(outputPrivacyId->len);
    if (!outputPrivacyId->p)
    {
        TAG_LOG_E("Failed to malloc for outputPrivacyId->p");
        err = TAG_ERROR_MEM_ALLOC;
        goto exit;
    }
    memcpy(outputPrivacyId->p, encryptedPId.p, TAG_SECURITY_PRIVACY_ID_LEN);

#ifdef DEBUG_SECURITY_AUTH_PID
    tagUtilByteToString(outputPrivacyId->p, outputPrivacyId->len, dbg, sizeof(dbg), NULL);
    TAG_LOG_I("PID: %s", dbg);
#endif

exit:
    TagCryptoSecurityBufferFree(&keyBuf);
    TagCryptoSecurityBufferFree(&encryptedPId);
    return err;
}

STATIC_FUNCTION TagError_t tagAuthSaveNvData(TagSecurityContext_t *context,
                                    TagNVItem_t nvType, uint8_t *src, size_t length)
{
    TagError_t err = TAG_ERROR_NONE;
    TagNVData_t nvData;

#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    TagSecurityBuffer_t encryptedDataBuf = {0};
#endif

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    if (nvType == TAG_NV_E2E_ENCRYPTION_KEY_AES || nvType == TAG_NV_MASTER_SECRET_AES)
    {
        err = PortKeyEncrypt(src, length, &encryptedDataBuf.p, &encryptedDataBuf.len);
        if (err != TAG_ERROR_NONE)
        {
            return TAG_ERROR_SECURITY_KEY_ENCRYPTION;
        }
    }
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (nvType)
    {
    case TAG_NV_MASTER_SECRET_AES:
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
        nvData.data.masterSecretAES = encryptedDataBuf.p;
        nvData.dataLength = encryptedDataBuf.len;
#else
        nvData.data.masterSecretAES = src;
        nvData.dataLength = length;
#endif
        if (nvData.dataLength < TAG_SECURITY_MASTER_SECRET_LEN)
        {
            err = TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
            goto exit;
        }
        break;
    case TAG_NV_E2E_ENCRYPTION_KEY_AES:
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
        nvData.data.e2eEncryptionKeyAES = encryptedDataBuf.p;
        nvData.dataLength = encryptedDataBuf.len;
#else
        nvData.data.e2eEncryptionKeyAES = src;
        nvData.dataLength = length;
#endif
        break;
    case TAG_NV_PRIVACY_ID_SEED:
        nvData.data.privacyIdSeed = src;
        nvData.dataLength = length;
        break;
    case TAG_NV_PRIVACY_ID_IV:
        nvData.data.privacyIdIv = src;
        nvData.dataLength = length;
        break;
    default:
        err = TAG_ERROR_SECURITY_UNEXPECTED_TYPE;
        goto exit;
    }
#pragma GCC diagnostic pop

    err = TagNVStore(nvType, &nvData);
    if (err)
    {
        goto exit;
    }
exit:
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    TagCryptoSecurityBufferFree(&encryptedDataBuf);
#endif
    return err;
}

STATIC_FUNCTION TagError_t tagAuthLoadNvData(TagSecurityContext_t *context,
                                    TagNVItem_t nvType,
                                    TagSecurityBuffer_t *outputBuf)
{
    TagError_t err = TAG_ERROR_NONE;
    TagSecurityBuffer_t loadedDataBuf = {0};
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    TagSecurityBuffer_t decryptedDataBuf = {0};
#endif
    TagNVData_t nvData;

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }
    if (!outputBuf)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_OUTPUT;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (nvType)
    {
    case TAG_NV_MASTER_SECRET_AES:
        loadedDataBuf.len = TAG_NV_MASTER_SECRET_AES_MAX_SZ;
        loadedDataBuf.p = (uint8_t *)TagMalloc(loadedDataBuf.len);
        if (!loadedDataBuf.p)
        {
            return TAG_ERROR_SECURITY_BUFFER_MEM_ALLOC;
        }
        nvData.data.masterSecretAES = loadedDataBuf.p;
        break;
    case TAG_NV_E2E_ENCRYPTION_KEY_AES:
        loadedDataBuf.len = TAG_NV_E2E_ENCRYPTION_KEY_AES_MAX_SZ;
        loadedDataBuf.p = (uint8_t *)TagMalloc(loadedDataBuf.len);
        if (!loadedDataBuf.p)
        {
            return TAG_ERROR_SECURITY_BUFFER_MEM_ALLOC;
        }
        nvData.data.e2eEncryptionKeyAES = loadedDataBuf.p;
        break;
    default:
        return TAG_ERROR_SECURITY_UNEXPECTED_TYPE;
    }
#pragma GCC diagnostic pop

    err = TagNVLoad(nvType, &nvData);
    if (err == TAG_ERROR_NV_NOT_EXIST)
    {
        TAG_LOG_D("nvData is not saved yet", nvType);
        goto exit;
    }
    else if (err)
    {
        goto exit;
    }
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    err = PortKeyDecrypt(loadedDataBuf.p, loadedDataBuf.len, &decryptedDataBuf.p, &decryptedDataBuf.len);
    if (err != TAG_ERROR_NONE)
    {
        err = TAG_ERROR_SECURITY_KEY_ENCRYPTION;
        goto exit;
    }
    outputBuf->len = decryptedDataBuf.len;
#else
    outputBuf->len = loadedDataBuf.len;
#endif

    outputBuf->p = (uint8_t *)TagMalloc(outputBuf->len);
    if (!outputBuf->p)
    {
        err = TAG_ERROR_SECURITY_OUTPUT_MEM_ALLOC;
        goto exit;
    }

#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    memcpy(outputBuf->p, decryptedDataBuf.p, decryptedDataBuf.len);
#else
    memcpy(outputBuf->p, loadedDataBuf.p, loadedDataBuf.len);
#endif

exit:
    TagCryptoSecurityBufferFree(&loadedDataBuf);
#ifdef CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
    TagCryptoSecurityBufferFree(&decryptedDataBuf);
#endif
    return err;
}

TagError_t TagAuthGetTagKey(TagSecurityContext_t *context, EndUserDevice *endUserDevice,
                            TagSecurityTagKeyType_t keyType,
                            TagSecurityBuffer_t *outputBuf)
{
    TagError_t err = TAG_ERROR_NONE;
    TagSecurityBuffer_t kdfZBuf = {0};
    if (!context || !outputBuf)
    {
        TAG_LOG_E("Failed to get tag key : input is NULL\n");
        return TAG_ERROR_INVALID_ARG;
    }

    if ((keyType == TAG_SECURITY_TAG_KEY_TYPE_NCK) || (keyType == TAG_SECURITY_TAG_KEY_TYPE_CK))
    {
        if (endUserDevice == NULL)
        {
            TAG_LOG_E("No end user device");
            return TAG_ERROR_INVALID_ARG;
        }
    }

    switch (keyType)
    {
    case TAG_SECURITY_TAG_KEY_TYPE_MS:
        err = tagAuthLoadNvData(context, TAG_NV_MASTER_SECRET_AES, outputBuf);
        if (err)
        {
            TAG_LOG_E("Failed to load MS : %d", err);
        }
        return err;
    case TAG_SECURITY_TAG_KEY_TYPE_PK:
        /* falling through */
    case TAG_SECURITY_TAG_KEY_TYPE_BAK:
        /* falling through */
    case TAG_SECURITY_TAG_KEY_TYPE_CK:
        /* falling through */
    case TAG_SECURITY_TAG_KEY_TYPE_NBAK:
        /* falling through */
    case TAG_SECURITY_TAG_KEY_TYPE_SK:
        err = tagAuthLoadNvData(context, TAG_NV_MASTER_SECRET_AES, &kdfZBuf);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to load MS : %d", err);
            return err;
        }
        kdfZBuf.len = TAG_SECURITY_MS_KDF_LEN;
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_NCK:

        err = TagAuthGetTagKey(context, endUserDevice, TAG_SECURITY_TAG_KEY_TYPE_NBAK, &kdfZBuf);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to load NBAK : %d", err);
            return err;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_MAX:
    default:
        TAG_LOG_E("Failed to get tag key : Unexpected Key type : %d", keyType);
        return TAG_ERROR_SECURITY_UNEXPECTED_TYPE;
    }
    outputBuf->len = TAG_SECURITY_SECRET_LEN_AES128;
    outputBuf->p = (uint8_t *)TagMalloc(outputBuf->len);
    if (!outputBuf->p)
    {
        TAG_LOG_E("Failed to alloc output buf for tag key");
        return TAG_ERROR_SECURITY_OUTPUT_MEM_ALLOC;
    }
    switch (keyType)
    {
    case TAG_SECURITY_TAG_KEY_TYPE_PK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         (const uint8_t *)TAG_SECURITY_TAG_KEY_KDF_INFO_PK,
                                         strlen(TAG_SECURITY_TAG_KEY_KDF_INFO_PK), outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for PK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_BAK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         (const uint8_t *)TAG_SECURITY_TAG_KEY_KDF_INFO_BAK,
                                         strlen(TAG_SECURITY_TAG_KEY_KDF_INFO_BAK), outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for  BAK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_CK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len, outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for CK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_NBAK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         (const uint8_t *)TAG_SECURITY_TAG_KEY_KDF_INFO_NBAK,
                                         strlen(TAG_SECURITY_TAG_KEY_KDF_INFO_NBAK), outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for NBAK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_NCK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len, outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for NCK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_SK:
        err = TagCryptoX963KdfWithSha256(kdfZBuf.p, kdfZBuf.len,
                                         (const uint8_t *)TAG_SECURITY_TAG_KEY_KDF_INFO_SK,
                                         strlen(TAG_SECURITY_TAG_KEY_KDF_INFO_SK), outputBuf->p,
                                         outputBuf->len);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to KDF for SK : %d", err);
            goto exit;
        }
        break;
    case TAG_SECURITY_TAG_KEY_TYPE_MS:
    case TAG_SECURITY_TAG_KEY_TYPE_MAX:
    default:
        TAG_LOG_E("Unexpected Tag Key type : %d", keyType);
        err = TAG_ERROR_INVALID_ARG;
        goto exit;
    }

exit:
    if (err != TAG_ERROR_NONE)
    {
        TagCryptoSecurityBufferFree(outputBuf);
    }
    TagCryptoSecurityBufferFree(&kdfZBuf);
    return err;
}

TagSecurityCipherType_t TagAuthGetCipherType(char *inputCipherName, size_t inputLen)
{
    TagSecurityCipherType_t ret = TAG_SECURITY_CIPHER_TYPE_NOT_SUPPORTED;
    const char *cipherName;
    size_t cipherLen;

    if (!inputCipherName || (inputLen <= 0))
    {
        return TAG_SECURITY_CIPHER_TYPE_NOT_SUPPORTED;
    }

    cipherName = TagAuthGetSupportedCipher();
    cipherLen = strlen(cipherName);
    if ((cipherLen == inputLen) && (strncmp(cipherName, inputCipherName, inputLen) == 0))
    {
        ret = TAG_SECURITY_CIPHER_TYPE_AES128_CBC_PKCS7PADDING;
        goto exit;
    }

exit:
    return ret;
}

TagError_t TagAuthSaveSelectedCipher(TagSecurityContext_t *context, EndUserDevice *endUserDevice, char *inputCipherName, size_t inputLen)
{
    TagSecurityCipherType_t cipherType;

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }
    if (!endUserDevice)
    {
        return TAG_ERROR_INVALID_ARG;
    }
    if (!inputCipherName || (inputLen <= 0))
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }
    if (!endUserDevice->authParam)
    {
        return TAG_ERROR_SECURITY_CIPHER_PARAMS_NULL;
    }

    cipherType = TagAuthGetCipherType(inputCipherName, inputLen);
    if (cipherType == TAG_SECURITY_CIPHER_TYPE_NOT_SUPPORTED)
    {
        return TAG_ERROR_SECURITY_CIPHER_INVALID_ALGO;
    }

    endUserDevice->authParam->cipherType = cipherType;
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t tagAuthCheckEncryptedData(
    TagSecurityContext_t *context, EndUserDevice *endUserDevice,
    TagSecurityBuffer_t *receivedEncryptedDataE,
    TagSecurityTagKeyType_t keyType,
    int *isEncryptedData)
{
    TagError_t err;
    TagSecurityBuffer_t keyBuf = {0};
    TagSecurityBuffer_t encryptedDataE = {0};

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }
    if (!isEncryptedData || !receivedEncryptedDataE)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }

    if (endUserDevice == NULL)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    if (!endUserDevice->authParam->nonceT.p || !receivedEncryptedDataE->p)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }

    err = TagAuthGetTagKey(context, endUserDevice, keyType, &keyBuf);
    if (err != TAG_ERROR_NONE)
    {
        goto exit;
    }

    err = TagCryptoAesFunction(keyBuf.p, keyBuf.len,
                               endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len,
                               TAG_SECURITY_KEY_TYPE_AES128, TAG_SECURITY_CIPHER_ENCRYPT,
                               (const uint8_t *)TAG_SECURITY_TAG_ENCRYPT_STRING, strlen(TAG_SECURITY_TAG_ENCRYPT_STRING),
                               &encryptedDataE);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("failed to make encryptDataE %d", err);
        goto exit;
    }

    if ((receivedEncryptedDataE->len != encryptedDataE.len) || (memcmp((char *)receivedEncryptedDataE->p,
                                                                       (char *)encryptedDataE.p, encryptedDataE.len) != 0))
    {
        TAG_LOG_D("Encrypted value is not same as received value. This case may have occurred because the key was not registered properly.");
        *isEncryptedData = 0;
    }
    else
    {
        *isEncryptedData = 1;
    }

exit:
    TagCryptoSecurityBufferFree(&keyBuf);
    TagCryptoSecurityBufferFree(&encryptedDataE);
    return err;
}

TagError_t TagAuthCheckEncryptedDataValidation(
    TagSecurityContext_t *context, EndUserDevice *endUserDevice,
    TagSecurityBuffer_t *receivedEncryptedDataE,
    TagSecurityBuffer_t *outputEncryptedDataT)
{
    TagError_t err;
    int isEncryptedData;
    TagSecurityBuffer_t keyBuf = {0};

    if (!context || !outputEncryptedDataT || !receivedEncryptedDataE || !endUserDevice)
    {
        TAG_LOG_E("input is null");
        return TAG_ERROR_INVALID_ARG;
    }

    if (!endUserDevice->authParam->nonceE.p || !receivedEncryptedDataE->p)
    {
        TAG_LOG_E("input buffer is null");
        return TAG_ERROR_INVALID_ARG;
    }

    err = tagAuthCheckEncryptedData(context, endUserDevice, receivedEncryptedDataE,
                                    TAG_SECURITY_TAG_KEY_TYPE_BAK, &isEncryptedData);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to check encryptedData for BAK, err:%d", err);
        return err;
    }

    if (isEncryptedData)
    {
        err = TagAuthGetTagKey(context, endUserDevice, TAG_SECURITY_TAG_KEY_TYPE_BAK,
                               &keyBuf);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to get tag key. keyType:%d, err:%d",
                      TAG_SECURITY_TAG_KEY_TYPE_BAK, err);
            goto exit;
        }

        err = TagCryptoAesFunction(keyBuf.p, keyBuf.len, endUserDevice->authParam->nonceE.p,
                                   endUserDevice->authParam->nonceE.len, TAG_SECURITY_KEY_TYPE_AES128,
                                   TAG_SECURITY_CIPHER_ENCRYPT,
                                   (const uint8_t *)TAG_SECURITY_TAG_ENCRYPT_STRING,
                                   strlen(TAG_SECURITY_TAG_ENCRYPT_STRING), outputEncryptedDataT);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to make encryptDataE, err %d", err);
            goto exit;
        }
        endUserDevice->authParam->validationType = TAG_SECURITY_TAG_VALIDATION_TYPE_BAK;
        goto exit;
    }
    else
    {
        err = tagAuthCheckEncryptedData(context, endUserDevice, receivedEncryptedDataE,
                                        TAG_SECURITY_TAG_KEY_TYPE_NBAK, &isEncryptedData);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to check encryptedData for NBAK, err %d", err);
            goto exit;
        }
        if (isEncryptedData)
        {
            endUserDevice->authParam->validationType = TAG_SECURITY_TAG_VALIDATION_TYPE_NBAK;
        }
    }
exit:
    TagCryptoSecurityBufferFree(&keyBuf);
    TAG_LOG_I("Auth Validation Type %d", endUserDevice->authParam->validationType);
    return err;
}

TagError_t TagAuthGenerateMasterSecret(TagSecurityContext_t *context)
{
    TagError_t err;

    TagSecurityEcdhParams_t ecdh_params = {0};
    TagSecurityBuffer_t secret_buf = {0};

    if (!context)
    {
        TAG_LOG_E("Failed to generate MS : context is not initialized");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (!context->cloudPublicKey.p || !context->randomValue.p)
    {
        TAG_LOG_E("security info for MasterSecret is not saved");
        return TAG_ERROR_INVALID_ARG;
    }

    TAG_MEM_CHECK("GenMS Start");
    err = TagCryptoEcdhInit(context);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to ecdh init, err = %d", err);
        return err;
    }

    ecdh_params.c_pubkey.p = (uint8_t *)context->cloudPublicKey.p; // receive from mobile
    ecdh_params.c_pubkey.len = context->cloudPublicKey.len;
    ecdh_params.salt.p = (uint8_t *)context->randomValue.p; // receive from mobile
    ecdh_params.salt.len = context->randomValue.len;

    err = TagCryptoEcdhSetParams(context, &ecdh_params);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to set ecdh params = %d", err);
        goto exit;
    }

    err = TagCryptoEcdhComputeSharedSecret(context, &secret_buf);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to compute shared secret %d", err);
        goto exit;
    }

    err = tagAuthSaveNvData(context, TAG_NV_MASTER_SECRET_AES, secret_buf.p, secret_buf.len);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to save MS key, %d", err);
        goto exit;
    }
    TAG_LOG_I("MasterSecret Generated");
exit:
    TagCryptoEcdhDeinit(context);
    TagCryptoSecurityBufferFree(&secret_buf);

    TAG_MEM_CHECK("GenMS End");
    return err;
}

TagError_t TagAuthCreateNonceT(TagSecurityContext_t *context, EndUserDevice *endUserDevice)
{
    int i;

    if (!context || !endUserDevice)
    {
        TAG_LOG_E("input is null");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (!endUserDevice->authParam)
    {
        TAG_LOG_E("authParam is not initialized");
        return TAG_ERROR_INVALID_ARG;
    }

    if (endUserDevice->authParam->nonceT.p)
    {
        TAG_LOG_E("nonceT is already created!, deviceId %d", endUserDevice->deviceId);
        TagCryptoSecurityBufferFree(&endUserDevice->authParam->nonceT);
    }

    endUserDevice->authParam->nonceT.len = TAG_SECURITY_IV_LEN;
    endUserDevice->authParam->nonceT.p = (uint8_t *)TagMalloc(endUserDevice->authParam->nonceT.len);
    if (!endUserDevice->authParam->nonceT.p)
    {
        TAG_LOG_E("Failed to malloc for nonce buffer");
        return TAG_ERROR_MEM_ALLOC;
    }

    for (i = 0; i < endUserDevice->authParam->nonceT.len; i++)
    {
        endUserDevice->authParam->nonceT.p[i] = (uint8_t)PortRandomGetData();
    }

    TAG_LOG_I("nonce for DId %d is created", endUserDevice->deviceId);

    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t tagAuthSaveBuffer(TagSecurityBuffer_t *dst, uint8_t *src, size_t length)
{
    if (!dst || !src || length == 0)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    if (dst->p)
    {
        /* overwrite saved buffer for test*/
        TagFree(dst->p);
    }

    dst->len = length;
    dst->p = (uint8_t *)TagMalloc(length);
    if (!dst->p)
    {
        TAG_LOG_E("Failed to alloc buffer for auth info");
        return TAG_ERROR_MEM_ALLOC;
    }
    memcpy(dst->p, src, length);

    return TAG_ERROR_NONE;
}

TagError_t TagAuthConnectionInit(TagSecurityContext_t *context, EndUserDevice *endUserDevice)
{
    TagSecurityAuthParams_t *auth_param;

    if (!context || !endUserDevice)
    {
        TAG_LOG_E("input is null");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    auth_param = (TagSecurityAuthParams_t *)TagMalloc(sizeof(TagSecurityAuthParams_t));
    if (!auth_param)
    {
        TAG_LOG_E("Failed to malloc for auth param");
        return TAG_ERROR_MEM_ALLOC;
    }

    memset((void *)auth_param, 0, sizeof(TagSecurityAuthParams_t));

    endUserDevice->authParam = auth_param;

    return TAG_ERROR_NONE;
}

TagError_t TagAuthConnectionDeinit(TagSecurityAuthParams_t *authParam)
{
    if (!authParam)
    {
        TAG_LOG_I("AuthParam is null");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    TagCryptoSecurityBufferFree(&authParam->nonceT);
    TagCryptoSecurityBufferFree(&authParam->nonceE);
    TagCryptoSecurityBufferFree(&authParam->commandKey);

    memset((void *)authParam, 0, sizeof(TagSecurityAuthParams_t));
    TagFree((void *)authParam);

    return TAG_ERROR_NONE;
}

TagError_t TagAuthRemoveOnboardingInfo(TagSecurityContext_t *context)
{
    if (!context)
    {
        TAG_LOG_E("Failed to remove onboarding info : Ctx null");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (context->cloudPublicKey.p)
    {
        TagCryptoSecurityBufferFree(&context->cloudPublicKey);
    }
    if (context->randomValue.p)
    {
        TagCryptoSecurityBufferFree(&context->randomValue);
    }

    return TAG_ERROR_NONE;
}

TagError_t TagAuthSaveInfo(TagSecurityContext_t *context, EndUserDevice *endUserDevice,
                           TagSecurityTagInfoType_t infoType, uint8_t *src, size_t length)
{
    TagError_t err = TAG_ERROR_NONE;

    if (!context || !endUserDevice)
    {
        TAG_LOG_E("input is null");
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (!src || length == 0 || !endUserDevice->authParam)
    {
        TAG_LOG_E("Failed to save auth info : argument is invalid");
        return TAG_ERROR_INVALID_ARG;
    }

    switch (infoType)
    {
    case TAG_SECURITY_TAG_INFO_TYPE_CLOUD_PUBLIC_KEY:
        err = tagAuthSaveBuffer(&context->cloudPublicKey, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_RANDOM_VALUE:
        err = tagAuthSaveBuffer(&context->randomValue, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_NONCE_T:
        err = tagAuthSaveBuffer(&endUserDevice->authParam->nonceT, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_NONCE_E:
        err = tagAuthSaveBuffer(&endUserDevice->authParam->nonceE, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_SELECTED_CIPHER:
        /* ignore selected cipher */
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_SEED:
        err = tagAuthSaveNvData(context, TAG_NV_PRIVACY_ID_SEED, src, length);
        if (err != TAG_ERROR_NONE)
        {
            break;
        }
        err = tagAuthSaveBuffer(&context->privacyIdSeed, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_IV:
        err = tagAuthSaveNvData(context, TAG_NV_PRIVACY_ID_IV, src, length);
        if (err != TAG_ERROR_NONE)
        {
            break;
        }
        err = tagAuthSaveBuffer(&context->privacyIdIv, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_E2E_ENCRYPTION_KEY:
        err = tagAuthSaveNvData(context, TAG_NV_E2E_ENCRYPTION_KEY_AES, src, length);
        break;
    case TAG_SECURITY_TAG_INFO_TYPE_E2E_ENCRYPTION:
    case TAG_SECURITY_TAG_INFO_TYPE_NUMBER_OF_PRIVACY_ID:
    case TAG_SECURITY_TAG_INFO_TYPE_MAX:
    default:
        TAG_LOG_E("Failed to save auth info : unexpected bufferType");
        return TAG_ERROR_INVALID_ARG;
    }

    if (err == TAG_ERROR_NONE)
    {
        TAG_LOG_I("Auth info[%d] is saved", infoType);
    }

    return err;
}

TagError_t TagAuthEncryptData(const uint8_t *inputValue, size_t inputLen, TagSecurityBuffer_t *outputBuf, EndUserDevice *endUserDevice, TagSecurityTagKeyType_t keyType)
{
    TagError_t err = TAG_ERROR_NONE;

    if (endUserDevice == NULL)
    {
        TAG_LOG_E("invalid device");
        return TAG_ERROR_INVALID_ARG;
    }

    if ((keyType != TAG_SECURITY_TAG_KEY_TYPE_CK) && (keyType != TAG_SECURITY_TAG_KEY_TYPE_NCK))
    {
        TAG_LOG_E("invalid keytype %d", keyType);
        return TAG_ERROR_INVALID_ARG;
    }

    if (!endUserDevice->authParam->commandKey.p)
    {
        err = TagAuthGetTagKey(gSecuContext, endUserDevice, keyType, &endUserDevice->authParam->commandKey);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to get Tag Key. deviceId %d, keyType %d", endUserDevice->deviceId, keyType);
            return err;
        }
    }

    err = TagCryptoAesFunction(endUserDevice->authParam->commandKey.p, endUserDevice->authParam->commandKey.len,
                               endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len,
                               TAG_SECURITY_KEY_TYPE_AES128, TAG_SECURITY_CIPHER_ENCRYPT,
                               inputValue, inputLen, outputBuf);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to encrypt data. deviceId %d, keyType %d", endUserDevice->deviceId, keyType);
        return err;
    }

    return err;
}

TagError_t TagAuthDecryptData(const uint8_t *inputValue, size_t inputLen, TagSecurityBuffer_t *outputBuf, EndUserDevice *endUserDevice, TagSecurityTagKeyType_t keyType)
{
    TagError_t err = TAG_ERROR_NONE;

    if (endUserDevice == NULL)
    {
        TAG_LOG_E("invalid device");
        return TAG_ERROR_INVALID_ARG;
    }

    if ((keyType != TAG_SECURITY_TAG_KEY_TYPE_CK) && (keyType != TAG_SECURITY_TAG_KEY_TYPE_NCK))
    {
        TAG_LOG_E("invalid keytype %d", keyType);
        return TAG_ERROR_INVALID_ARG;
    }

    if (!endUserDevice->authParam->commandKey.p)
    {
        err = TagAuthGetTagKey(gSecuContext, endUserDevice, keyType, &endUserDevice->authParam->commandKey);
        if (err != TAG_ERROR_NONE)
        {
            TAG_LOG_E("Failed to get Tag Key. deviceId %d, keyType %d", endUserDevice->deviceId, keyType);
            return err;
        }
    }

    err = TagCryptoAesFunction(endUserDevice->authParam->commandKey.p, endUserDevice->authParam->commandKey.len,
                               endUserDevice->authParam->nonceT.p, endUserDevice->authParam->nonceT.len,
                               TAG_SECURITY_KEY_TYPE_AES128, TAG_SECURITY_CIPHER_DECRYPT,
                               inputValue, inputLen, outputBuf);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("Failed to decrypt data. deviceId %d, keyType %d", endUserDevice->deviceId, keyType);
        return err;
    }

    return err;
}
