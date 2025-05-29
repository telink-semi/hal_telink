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

#ifndef TAGSDK_INC_TAGSECURITYTYPES_H_
#define TAGSDK_INC_TAGSECURITYTYPES_H_

#define TAG_SECURITY_B64_ALIGN_LEN(x)   (((x) + 3) & ~3u)
#define TAG_SECURITY_B64_ENCODE_LEN(x)  ((((x) + 2) / 3) * 4 + 1)
#define TAG_SECURITY_B64_DECODE_LEN(x)  (TAG_SECURITY_B64_ALIGN_LEN(x) / 4 * 3 + 1)

#define TAG_SECURITY_CIPHER_ALIGN_LEN(x)    ((x) + (16 - ((x) % 16))) // AES_128 (block size is 16 bytes)

#define TAG_SECURITY_NOUSE_DEVICE_ID       0
#define TAG_SECURITY_MAX_PRIVACY_ID_SIZE        65535
#define TAG_SECURITY_PRIVACY_ID_SIZE            1000
#define TAG_SECURITY_PRIVACY_ID_CYCLE           10
#define TAG_SECURITY_PRIVACY_ID_SEED_LEN        8
#define TAG_SECURITY_PRIVACY_ID_LEN             8

#define TAG_SECURITY_ED25519_LEN                32
#define TAG_SECURITY_MASTER_SECRET_LEN          32
#define TAG_SECURITY_SECRET_LEN_AES256          32
#define TAG_SECURITY_SECRET_LEN_AES128          16
#define TAG_SECURITY_IV_LEN                     16
#define TAG_SECURITY_SHA256_LEN                 32
#define TAG_SECURITY_MS_KDF_LEN                 16
#define TAG_SECURITY_AES128_BLOCK_LEN           16

#define TAG_SECURITY_SIGNATURE_ED25519_LEN      64
#define TAG_SECURITY_SIGNATURE_UNKNOWN_LEN      0

#define CIPHER_NAME_AES128_CSC_PKCS7PADDING "AES_128-CBC-PKCS7Padding"

typedef enum {
    TAG_SECURITY_CIPHER_TYPE_NOT_SUPPORTED = -1,
    TAG_SECURITY_CIPHER_TYPE_AES128_CBC_PKCS7PADDING = 0,
    TAG_SECURITY_CIPHER_TYPE_MAX
} TagSecurityCipherType_t;

typedef enum {
    TAG_SECURITY_TAG_KEY_TYPE_MS,
    TAG_SECURITY_TAG_KEY_TYPE_PK,
    TAG_SECURITY_TAG_KEY_TYPE_BAK,
    TAG_SECURITY_TAG_KEY_TYPE_CK,
    TAG_SECURITY_TAG_KEY_TYPE_NBAK,
    TAG_SECURITY_TAG_KEY_TYPE_NCK,
    TAG_SECURITY_TAG_KEY_TYPE_SK,
    TAG_SECURITY_TAG_KEY_TYPE_MAX,
} TagSecurityTagKeyType_t;

typedef enum {
    TAG_SECURITY_TAG_INFO_TYPE_CLOUD_PUBLIC_KEY,
    TAG_SECURITY_TAG_INFO_TYPE_RANDOM_VALUE,
    TAG_SECURITY_TAG_INFO_TYPE_NONCE_T,
    TAG_SECURITY_TAG_INFO_TYPE_NONCE_E,
    TAG_SECURITY_TAG_INFO_TYPE_SELECTED_CIPHER,
    TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_SEED,
    TAG_SECURITY_TAG_INFO_TYPE_PRIVACY_ID_IV,
    TAG_SECURITY_TAG_INFO_TYPE_E2E_ENCRYPTION,
    TAG_SECURITY_TAG_INFO_TYPE_E2E_ENCRYPTION_KEY,
    TAG_SECURITY_TAG_INFO_TYPE_NUMBER_OF_PRIVACY_ID,
    TAG_SECURITY_TAG_INFO_TYPE_MAX,
} TagSecurityTagInfoType_t;

typedef enum {
    TAG_SECURITY_TAG_VALIDATION_TYPE_NOT_FOUND,
    TAG_SECURITY_TAG_VALIDATION_TYPE_BAK,
    TAG_SECURITY_TAG_VALIDATION_TYPE_NBAK,
    TAG_SECURITY_TAG_VALIDATION_TYPE_MAX,
} TagSecurityTagValidationType_t;

#define TAG_SECURITY_TAG_KEY_KDF_INFO_PK "privacy"
#define TAG_SECURITY_TAG_KEY_KDF_INFO_BAK "bleAuthentication"
#define TAG_SECURITY_TAG_KEY_KDF_INFO_NBAK "nonOwner"
#define TAG_SECURITY_TAG_KEY_KDF_INFO_SK "signing"

#define TAG_SECURITY_TAG_ENCRYPT_STRING "smartthings"

/**
 * @brief Algorithm types of key
 */
typedef enum {
    TAG_SECURITY_KEY_TYPE_UNKNOWN = 0,
    TAG_SECURITY_KEY_TYPE_ED25519,
    TAG_SECURITY_KEY_TYPE_RSA2048,
    TAG_SECURITY_KEY_TYPE_AES256 = 3,               /* must be 3 because referenced in ss.lib */
    TAG_SECURITY_KEY_TYPE_AES128 = 5,               /* must be 5 */
    TAG_SECURITY_KEY_TYPE_ECCP256,
    TAG_SECURITY_KEY_TYPE_MAX,
} TagSecurityKeyType_t;

/**
 * @brief A handle of security context
 */
typedef unsigned int SecurityHandle;

/**
 * @brief Indicate a sub system is initialized
 */
typedef enum {
    TAG_SECURITY_SUB_NONE    = 0,
    TAG_SECURITY_SUB_PK      = (1 << 0),
    TAG_SECURITY_SUB_CIPHER  = (1 << 1),
    TAG_SECURITY_SUB_ECDH    = (1 << 2),
    TAG_SECURITY_SUB_MANAGER = (1 << 3),
    TAG_SECURITY_SUB_STORAGE = (1 << 4),
} TagSecuritySubSystem_t;

/**
 * @brief Types of cipher operation
 */
typedef enum {
    TAG_SECURITY_CIPHER_DECRYPT = 1,
    TAG_SECURITY_CIPHER_ENCRYPT,
} TagSecurityCipherMode_t;

/**
* @brief Contains a buffer information
*/
typedef struct {
    size_t len;                                     /**< @brief length of buffer */
    unsigned char *p;                               /**< @brief pointer of buffer */
} TagSecurityBuffer_t;

/**
* @brief Contains ecdh information
*/
typedef struct {
    TagSecurityBuffer_t t_seckey;                 /** @brief a pointer to a things secret key based on curve25519 (software backend only) */
    TagSecurityBuffer_t c_pubkey;                 /** @brief a pointer to a server public key based on curve25519 */
    TagSecurityBuffer_t salt;                     /** @brief a pointer to a random token as a salt */
} TagSecurityEcdhParams_t;

/**
* @brief Contains Authentication Service information
*/
typedef struct {
    TagSecurityBuffer_t nonceT;                 /** @brief a pointer to nonceT */
    TagSecurityBuffer_t nonceE;                 /** @brief a pointer to nonceE */
    TagSecurityTagValidationType_t validationType;
    TagSecurityCipherType_t cipherType;
    TagSecurityBuffer_t commandKey;
} TagSecurityAuthParams_t;

/**
 * @brief Contains a security context data
 */
typedef struct {
    SecurityHandle handle;                         /**< @brief handle of context */
    TagSecuritySubSystem_t subSystem;           /**< @brief flag to know whether the sub system has been initialized */

    TagSecurityEcdhParams_t *ecdhParams;        /**< @brief contains parameter for ecdh system */

    TagSecurityBuffer_t cloudPublicKey;
    TagSecurityBuffer_t randomValue;
    TagSecurityBuffer_t privacyIdSeed;
    TagSecurityBuffer_t privacyIdIv;

    uint32_t numberOfPrivacyId;

} TagSecurityContext_t;

#endif /* TAGSDK_INC_TAGSECURITYTYPES_H_ */
