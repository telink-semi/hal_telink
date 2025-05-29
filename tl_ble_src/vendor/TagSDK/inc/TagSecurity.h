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

#ifndef TAGSDK_INC_TAGSECURITY_H_
#define TAGSDK_INC_TAGSECURITY_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "TagErrorType.h"
#include "TagCore.h"

extern TagSecurityContext_t *gSecuContext;

#define GET_NUMBER_OF_PRIVACY_ID(v0,v1,v2,v3) ( (((uint32_t)(v0) <<  0) & (uint32_t)0x000000FF) | \
                                             (((uint32_t)(v1) <<  8) & (uint32_t)0x0000FF00) | \
                                             (((uint32_t)(v2) << 16) & (uint32_t)0x00FF0000) | \
                                             (((uint32_t)(v3) << 24) & (uint32_t)0xFF000000)  )

void TagCryptoSecurityBufferFree(TagSecurityBuffer_t *buffer);

TagSecurityContext_t *TagCryptoContextInit(void);
TagError_t TagCryptoContextReset(TagSecurityContext_t *context);

size_t TagCryptoAesGetAlignSize(TagSecurityKeyType_t key_type, size_t data_size);

TagError_t TagCryptoAesFunction(
        const unsigned char *key_p, size_t key_len,
        const unsigned char *iv_p, size_t iv_len,
        TagSecurityKeyType_t cipher_type,
        TagSecurityCipherMode_t cipher_mode,
        const unsigned char *input_p, size_t input_len,
        TagSecurityBuffer_t *output_buf);

TagError_t TagCryptoEcdhInit(TagSecurityContext_t *context);
TagError_t TagCryptoEcdhSetParams(TagSecurityContext_t *context, TagSecurityEcdhParams_t *ecdh_set_params);
TagError_t TagCryptoEcdhComputeSharedSecret(TagSecurityContext_t *context, TagSecurityBuffer_t *output_buf);
TagError_t TagCryptoEcdhDeinit(TagSecurityContext_t *context);

TagError_t TagCryptoSha256(const unsigned char *input, size_t input_len, unsigned char *output, size_t output_len);

TagError_t TagCryptoX963KdfWithSha256(
        const unsigned char *input_z_buf, size_t input_z_size,
        const unsigned char *input_info_buf, size_t input_info_size,
        unsigned char *output_buf, size_t output_size);

TagError_t TagCryptoBase64Encode(const unsigned char *src, size_t src_len,
                                 unsigned char *dst, size_t dst_len,
                                 size_t *out_len);

TagError_t TagCryptoBase64Decode(const unsigned char *src, size_t src_len,
                                 unsigned char *dst, size_t dst_len,
                                 size_t *out_len);

TagError_t TagCryptoBase64EncodeUrlsafe(const unsigned char *src, size_t src_len,
                                        unsigned char *dst, size_t dst_len,
                                        size_t *out_len);

TagError_t TagCryptoBase64DecodeUrlsafe(const unsigned char *src, size_t src_len,
                                        unsigned char *dst, size_t dst_len,
                                        size_t *out_len);

void TagCryptoInit(void);

/**
 * @brief Get Authentication Service supported cipher name
 *
 * @details This function returns supported cipher name
 *
 */
const char *TagAuthGetSupportedCipher(void);
TagSecurityCipherType_t TagAuthGetCipherType(char *inputCipherName, size_t inputLen);
TagError_t TagAuthSaveSelectedCipher(TagSecurityContext_t *context, EndUserDevice *endUserDevice, char *inputCipherName, size_t inputLen);

TagError_t TagAuthGetTagKey(TagSecurityContext_t *context, EndUserDevice *endUserDevice,
        TagSecurityTagKeyType_t keyType,
        TagSecurityBuffer_t *outputBuf);

TagError_t TagAuthCheckEncryptedDataValidation(
        TagSecurityContext_t *context, EndUserDevice *endUserDevice,
        TagSecurityBuffer_t *receivedEncryptedDataE,
        TagSecurityBuffer_t *outputEncryptedDataT);

TagError_t TagAuthCreateNonceT(TagSecurityContext_t *context, EndUserDevice *endUserDevice);

TagError_t TagAuthSaveInfo(TagSecurityContext_t *context, EndUserDevice *endUserDevice,
        TagSecurityTagInfoType_t infoType, unsigned char *src, size_t length);

TagError_t TagAuthGenerateMasterSecret(TagSecurityContext_t *context);

TagError_t TagAuthGetPrivacyId(TagSecurityContext_t *context, TagSecurityBuffer_t *outputPrivacyId);

TagError_t TagAuthConnectionInit(TagSecurityContext_t *context, EndUserDevice *endUserDevice);
TagError_t TagAuthConnectionDeinit(TagSecurityAuthParams_t *authParam);

TagError_t TagAuthRemoveOnboardingInfo(TagSecurityContext_t *context);

TagError_t TagAuthEncryptData(const uint8_t *inputValue, size_t inputLen, TagSecurityBuffer_t *outputBuf, EndUserDevice *endUserDevice, TagSecurityTagKeyType_t keyType);
TagError_t TagAuthDecryptData(const uint8_t *inputValue, size_t inputLen, TagSecurityBuffer_t *outputBuf, EndUserDevice *endUserDevice, TagSecurityTagKeyType_t keyType);

#endif /* TAGSDK_INC_TAGSECURITY_H_ */
