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

#include <string.h>

#include "TagConfig.h"

#include "TagCore.h"
#include "TagNV.h"
#include "TagSecurity.h"

#include "PortEncryption.h"

#include "mbedtls/ecp.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/cipher.h"
#include "mbedtls/base64.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform.h"

#ifdef LOG_PREFIX
#undef LOG_PREFIX
#endif
#define LOG_PREFIX "ScCrp"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG

TagSecurityContext_t *gSecuContext;

#define MAX_SECURITY_BUFFER_SIZE (2147483647)
void TagCryptoSecurityBufferFree(TagSecurityBuffer_t *buffer)
{
    if (buffer)
    {
        if (buffer->p)
        {
            if (buffer->len < MAX_SECURITY_BUFFER_SIZE)
            {
                memset(buffer->p, 0, buffer->len);
            }
            TagFree(buffer->p);
        }
        memset(buffer, 0, sizeof(TagSecurityBuffer_t));
    }
}

STATIC_FUNCTION TagError_t tagCryptoCopySecurityBuffer(TagSecurityBuffer_t *src, TagSecurityBuffer_t *dst)
{
    if (src->p)
    {
        if (src->len == 0)
        {
            return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
        }

        if (dst->p)
        {
            TagCryptoSecurityBufferFree(dst);
        }

        dst->p = (uint8_t *)TagMalloc(src->len);
        if (!dst->p)
        {
            return TAG_ERROR_SECURITY_BUFFER_MEM_ALLOC;
        }

        memcpy(dst->p, src->p, src->len);
        dst->len = src->len;
    }

    return TAG_ERROR_NONE;
}

size_t TagCryptoAesGetAlignSize(TagSecurityKeyType_t keyType, size_t dataSize)
{
    const mbedtls_cipher_info_t *cipher_info;
    mbedtls_cipher_context_t cipher_ctx;
    mbedtls_cipher_type_t cipher_alg;
    unsigned int block_size;
    int ret;

    if (keyType == TAG_SECURITY_KEY_TYPE_AES256)
    {
        cipher_alg = MBEDTLS_CIPHER_AES_256_CBC;
    }
    else if (keyType == TAG_SECURITY_KEY_TYPE_AES128)
    {
        cipher_alg = MBEDTLS_CIPHER_AES_128_CBC;
    }
    else
    {
        TAG_LOG_E("'%d' is not supported cipher algorithm", keyType);
        return 0;
    }

    if (!dataSize)
    {
        TAG_LOG_E("input size is zero");
        return 0;
    }

    cipher_info = mbedtls_cipher_info_from_type(cipher_alg);
    if (!cipher_info)
    {
        TAG_LOG_E("mbedtls_cipher_info_from_type returned null");
        return 0;
    }

    mbedtls_cipher_init(&cipher_ctx);

    ret = mbedtls_cipher_setup(&cipher_ctx, cipher_info);
    if (ret)
    {
        TAG_LOG_E("mbedtls_cipher_setup = -0x%04X", -ret);
        mbedtls_cipher_free(&cipher_ctx);
        return 0;
    }

    block_size = mbedtls_cipher_get_block_size(&cipher_ctx);
    if (block_size == 0)
    {
        TAG_LOG_E("mbedtls_cipher_get_block_size returned zero");
        mbedtls_cipher_free(&cipher_ctx);
        return 0;
    }

    dataSize = dataSize + (block_size - (dataSize % block_size));

    mbedtls_cipher_free(&cipher_ctx);

    return dataSize;
}

static inline void tagCryptoSecurityBufferWipe(const TagSecurityBuffer_t *input_buf, size_t wiped_len)
{
    if (input_buf && (input_buf->len < wiped_len))
    {
        int i;
        for (i = input_buf->len; i < wiped_len; i++)
        {
            input_buf->p[i] = 0;
        }
    }
}

STATIC_FUNCTION TagError_t tagCryptoCheckContextAndParamsIsValid(TagSecurityContext_t *context, TagSecuritySubSystem_t sub_system)
{
    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (sub_system & TAG_SECURITY_SUB_ECDH)
    {
        if (!context->ecdhParams)
        {
            return TAG_ERROR_SECURITY_ECDH_PARAMS_NULL;
        }
    }

    return TAG_ERROR_NONE;
}

static TagError_t tagCryptoSwapSecret(TagSecurityBuffer_t *src, TagSecurityBuffer_t *dst)
{
    uint8_t *p;
    size_t len;
    int i;

    if (!src || !src->p || (src->len == 0) || !dst)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }

    len = src->len;
    p = (uint8_t *)TagMalloc(len);

    if (!p)
    {
        return TAG_ERROR_SECURITY_BUFFER_MEM_ALLOC;
    }

    for (i = 0; i < len; i++)
    {
        p[(len - 1) - i] = src->p[i];
    }

    dst->p = p;
    dst->len = len;

    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t tagCryptoEcdhComputePremasterSecret(
    TagSecurityBuffer_t *t_seckey_buf,
    TagSecurityBuffer_t *c_pubkey_buf,
    TagSecurityBuffer_t *output_buf)
{
    TagError_t err;
    mbedtls_ecdh_context mbed_ecdh;
    mbedtls_ctr_drbg_context mbed_ctr_drbg;
    mbedtls_entropy_context mbed_entropy;
    mbedtls_ecp_group_id mbed_ecp_grp_id = MBEDTLS_ECP_DP_CURVE25519;
    const char *pers = "iot_security_ecdh";
    TagSecurityBuffer_t pmsecret_buf = {0};
    TagSecurityBuffer_t swap_buf = {0};
    size_t key_len;
    size_t secret_len;
    int ret;

    if (!t_seckey_buf || !c_pubkey_buf || !output_buf)
    {
        return TAG_ERROR_INVALID_ARG;
    }

    key_len = TAG_SECURITY_ED25519_LEN;
    secret_len = TAG_SECURITY_MASTER_SECRET_LEN;

    if (t_seckey_buf->len > key_len)
    {
        return TAG_ERROR_SECURITY_ECDH_INVALID_SECKEY;
    }

    if (c_pubkey_buf->len > key_len)
    {
        return TAG_ERROR_SECURITY_ECDH_INVALID_PUBKEY;
    }

    pmsecret_buf.len = secret_len;
    pmsecret_buf.p = (uint8_t *)TagMalloc(pmsecret_buf.len);
    if (!pmsecret_buf.p)
    {
        return TAG_ERROR_SECURITY_ECDH_PMSECRET_KEY_MEM_ALLOC;
    }

    mbedtls_ecdh_init(&mbed_ecdh);
    mbedtls_ctr_drbg_init(&mbed_ctr_drbg);
    mbedtls_entropy_init(&mbed_entropy);

    ret = mbedtls_ctr_drbg_seed(&mbed_ctr_drbg, mbedtls_entropy_func, &mbed_entropy,
                                (const uint8_t *)pers, strlen(pers));
    if (ret)
    {
        TAG_LOG_E("mbedtls_ctr_drbg_seed = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        goto exit;
    }

    ret = mbedtls_ecp_group_load(&mbed_ecdh.grp, mbed_ecp_grp_id);
    if (ret)
    {
        TAG_LOG_E("mbedtls_ecp_group_load = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        goto exit;
    }

    err = tagCryptoSwapSecret(t_seckey_buf, &swap_buf);
    if (err != TAG_ERROR_NONE)
    {
        goto exit;
    }

    ret = mbedtls_mpi_read_binary(&mbed_ecdh.d, swap_buf.p, swap_buf.len);
    if (ret)
    {
        TAG_LOG_E("mbedtls_mpi_read_binary = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        TagCryptoSecurityBufferFree(&swap_buf);
        goto exit;
    }

    TagCryptoSecurityBufferFree(&swap_buf);

    err = tagCryptoSwapSecret(c_pubkey_buf, &swap_buf);
    if (err != TAG_ERROR_NONE)
    {
        goto exit;
    }

    ret = mbedtls_mpi_read_binary(&mbed_ecdh.Qp.X, swap_buf.p, swap_buf.len);
    if (ret)
    {
        TAG_LOG_E("mbedtls_mpi_read_binary = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        TagCryptoSecurityBufferFree(&swap_buf);
        goto exit;
    }

    TagCryptoSecurityBufferFree(&swap_buf);

    ret = mbedtls_mpi_lset(&mbed_ecdh.Qp.Z, 1);
    if (ret)
    {
        TAG_LOG_E("mbedtls_mpi_lset = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        goto exit;
    }

    ret = mbedtls_ecdh_compute_shared(&mbed_ecdh.grp, &mbed_ecdh.z, &mbed_ecdh.Qp, &mbed_ecdh.d, mbedtls_ctr_drbg_random, &mbed_ctr_drbg);
    if (ret)
    {
        TAG_LOG_E("mbedtls_ecdh_compute_shared = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        goto exit;
    }

    ret = mbedtls_mpi_write_binary(&mbed_ecdh.z, pmsecret_buf.p, pmsecret_buf.len);
    if (ret)
    {
        TAG_LOG_E("mbedtls_mpi_write_binary = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_ECDH_LIBRARY;
        goto exit;
    }

    err = tagCryptoSwapSecret(&pmsecret_buf, &swap_buf);
    if (err != TAG_ERROR_NONE)
    {
        goto exit;
    }

    output_buf->p = swap_buf.p;
    output_buf->len = swap_buf.len;
    err = TAG_ERROR_NONE;

exit:
    TagCryptoSecurityBufferFree(&pmsecret_buf);
    mbedtls_ecdh_free(&mbed_ecdh);
    mbedtls_ctr_drbg_free(&mbed_ctr_drbg);
    mbedtls_entropy_free(&mbed_entropy);

    return err;
}

TagError_t TagCryptoAesFunction(
    const uint8_t *key_p,
    size_t key_len,
    const uint8_t *iv_p,
    size_t iv_len,
    TagSecurityKeyType_t cipher_type,
    TagSecurityCipherMode_t cipher_mode,
    const uint8_t *input_p,
    size_t input_len,
    TagSecurityBuffer_t *output_buf)
{
    TagError_t err;
    const mbedtls_cipher_info_t *mbed_cipher_info;
    mbedtls_cipher_type_t mbed_cipher_alg;
    mbedtls_cipher_context_t mbed_cipher_ctx;
    mbedtls_operation_t mbed_op_mode;
    size_t required_len;
    size_t expected_key_len;
    size_t key_bitlen;
    size_t iv_size;
    int ret;

    if (!input_p || (input_len == 0))
    {
        TAG_LOG_E("AES:input buffer is invalid");
        return TAG_ERROR_INVALID_ARG;
    }

    if (!output_buf)
    {
        TAG_LOG_E("AES:output buffer is null");
        return TAG_ERROR_INVALID_ARG;
    }

    if (cipher_mode == TAG_SECURITY_CIPHER_ENCRYPT)
    {
        mbed_op_mode = MBEDTLS_ENCRYPT;
    }
    else if (cipher_mode == TAG_SECURITY_CIPHER_DECRYPT)
    {
        mbed_op_mode = MBEDTLS_DECRYPT;
    }
    else
    {
        TAG_LOG_E("AES:'%d' is not a supported cipher mode", cipher_mode);
        return TAG_ERROR_SECURITY_CIPHER_INVALID_CIPHER_MODE;
    }

    if (cipher_type == TAG_SECURITY_KEY_TYPE_AES256)
    {
        mbed_cipher_alg = MBEDTLS_CIPHER_AES_256_CBC;
        expected_key_len = TAG_SECURITY_SECRET_LEN_AES256;
    }
    else if (cipher_type == TAG_SECURITY_KEY_TYPE_AES128)
    {
        mbed_cipher_alg = MBEDTLS_CIPHER_AES_128_CBC;
        expected_key_len = TAG_SECURITY_SECRET_LEN_AES128;
    }
    else
    {
        TAG_LOG_E("AES:'%d' is not a supported cipher algorithm", cipher_type);
        return TAG_ERROR_SECURITY_CIPHER_INVALID_ALGO;
    }

    if (!key_p || (key_len != expected_key_len))
    {
        TAG_LOG_E("AES:key is invalid %d@%p", (int)key_len, key_p);
        return TAG_ERROR_SECURITY_CIPHER_INVALID_KEY;
    }

    if (!iv_p || (iv_len != TAG_SECURITY_IV_LEN))
    {
        TAG_LOG_E("AES:iv is invalid %d@%p", (int)iv_len, iv_p);
        return TAG_ERROR_SECURITY_CIPHER_INVALID_IV;
    }

    mbed_cipher_info = mbedtls_cipher_info_from_type(mbed_cipher_alg);
    if (!mbed_cipher_info)
    {
        TAG_LOG_E("AES:mbedtls_cipher_info_from_type returned null");
        return TAG_ERROR_SECURITY_CIPHER_INVALID_ALGO;
    }
#ifdef TAG_CONFIG_USE_MBEDTLS_IN_SDK
    key_bitlen = mbed_cipher_info->key_bitlen;
#else
    key_bitlen = mbedtls_cipher_info_get_key_bitlen(mbed_cipher_info);
#endif

    if (key_len != (key_bitlen / 8))
    {
        TAG_LOG_E("AES:key len mismatch, %zu != %d", key_len, (key_bitlen / 8));
        return TAG_ERROR_SECURITY_CIPHER_KEY_LEN;
    }
#ifdef TAG_CONFIG_USE_MBEDTLS_IN_SDK
    iv_size = mbed_cipher_info->iv_size;
#else
    iv_size = mbedtls_cipher_info_get_iv_size(mbed_cipher_info);
#endif
    if (iv_len != iv_size)
    {
        TAG_LOG_E("AES:iv len mismatch, %zu != %d", iv_len, iv_size);
        return TAG_ERROR_SECURITY_CIPHER_IV_LEN;
    }

    mbedtls_cipher_init(&mbed_cipher_ctx);

    if (cipher_mode == TAG_SECURITY_CIPHER_ENCRYPT)
    {
        required_len = TagCryptoAesGetAlignSize(cipher_type, input_len);
    }
    else
    {
        required_len = input_len;
    }

    output_buf->p = (uint8_t *)TagMalloc(required_len);
    if (!output_buf->p)
    {
        TAG_LOG_E("AES:failed to malloc for output buffer");
        err = TAG_ERROR_MEM_ALLOC;
        goto exit;
    }

    memset(output_buf->p, 0, required_len);
    output_buf->len = required_len;

    ret = mbedtls_cipher_setup(&mbed_cipher_ctx, mbed_cipher_info);
    if (ret)
    {
        TAG_LOG_E("mbedtls_cipher_setup = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_CIPHER_LIBRARY;
        goto exit_free_output_buf;
    }

    ret = mbedtls_cipher_setkey(&mbed_cipher_ctx, key_p, key_bitlen, mbed_op_mode);
    if (ret)
    {
        TAG_LOG_E("AES:mbedtls_cipher_setkey = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_CIPHER_LIBRARY;
        goto exit_free_output_buf;
    }

    ret = mbedtls_cipher_set_padding_mode(&mbed_cipher_ctx, MBEDTLS_PADDING_PKCS7);
    if (ret)
    {
        TAG_LOG_E("AES:mbedtls_cipher_set_padding_mode = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_CIPHER_LIBRARY;
        goto exit_free_output_buf;
    }

    ret = mbedtls_cipher_crypt(&mbed_cipher_ctx, iv_p, iv_len,
                               (const uint8_t *)input_p, input_len, output_buf->p, &output_buf->len);
    if (ret)
    {
        TAG_LOG_E("AES:mbedtls_cipher_crypt = -0x%04X", -ret);
        err = TAG_ERROR_SECURITY_CIPHER_LIBRARY;
        goto exit_free_output_buf;
    }

    if (output_buf->len > required_len)
    {
        TAG_LOG_E("AES:buffer overflow in cipher '%d' (%d > %d)", cipher_mode, (int)output_buf->len, (int)required_len);
        err = TAG_ERROR_SECURITY_CIPHER_BUF_OVERFLOW;
        goto exit_free_output_buf;
    }

    tagCryptoSecurityBufferWipe(output_buf, required_len);

    err = TAG_ERROR_NONE;
    goto exit;

exit_free_output_buf:
    TagCryptoSecurityBufferFree(output_buf);
exit:
    mbedtls_cipher_free(&mbed_cipher_ctx);
    return err;
}

TagError_t TagCryptoEcdhSetParams(TagSecurityContext_t *context, TagSecurityEcdhParams_t *ecdh_set_params)
{
    TagError_t err;

    err = tagCryptoCheckContextAndParamsIsValid(context, TAG_SECURITY_SUB_ECDH);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    if (!ecdh_set_params)
    {
        return TAG_ERROR_SECURITY_ECDH_PARAMS_INPUT_NULL;
    }

    err = tagCryptoCopySecurityBuffer(&ecdh_set_params->t_seckey, &context->ecdhParams->t_seckey);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    err = tagCryptoCopySecurityBuffer(&ecdh_set_params->c_pubkey, &context->ecdhParams->c_pubkey);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    err = tagCryptoCopySecurityBuffer(&ecdh_set_params->salt, &context->ecdhParams->salt);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    return err;
}

static TagError_t tagCryptoEcdhLoadEd25519(TagSecurityContext_t *context)
{
    TagSecurityEcdhParams_t *ecdh_params;
    TagDeviceInfoData_t *keyInfo = NULL;

    ecdh_params = context->ecdhParams;

    keyInfo = TagAllocDeviceInfo(TAG_DEVICE_INFO_PRIVATE_KEY_CURVED);
    if (!keyInfo)
    {
        return TAG_ERROR_SECURITY_ECDH_CURVE_KEY_MEM_ALLOC;
    }

    ecdh_params->t_seckey.p = (uint8_t *)TagMalloc(keyInfo->dataLength);
    if (!ecdh_params->t_seckey.p)
    {
        TagFreeDeviceInfo(keyInfo);
        return TAG_ERROR_SECURITY_ECDH_SECKEY_MEM_ALLOC;
    }
    ecdh_params->t_seckey.len = keyInfo->dataLength;

    memcpy(ecdh_params->t_seckey.p, keyInfo->data, keyInfo->dataLength);

    TagFreeDeviceInfo(keyInfo);

    return TAG_ERROR_NONE;
}

TagError_t TagCryptoEcdhInit(TagSecurityContext_t *context)
{
    TagError_t err;
    TagSecurityEcdhParams_t *ecdh_params;

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    ecdh_params = (TagSecurityEcdhParams_t *)TagMalloc(sizeof(TagSecurityEcdhParams_t));
    if (!ecdh_params)
    {
        return TAG_ERROR_SECURITY_ECDH_PARAMS_MEM_ALLOC;
    }

    memset((void *)ecdh_params, 0, sizeof(TagSecurityEcdhParams_t));

    context->ecdhParams = ecdh_params;

    err = tagCryptoEcdhLoadEd25519(context);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    context->subSystem |= TAG_SECURITY_SUB_ECDH;

    return TAG_ERROR_NONE;
}

TagError_t TagCryptoEcdhDeinit(TagSecurityContext_t *context)
{
    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    if (context->ecdhParams)
    {
        if (context->ecdhParams->t_seckey.p)
        {
            TagCryptoSecurityBufferFree(&context->ecdhParams->t_seckey);
        }
        if (context->ecdhParams->c_pubkey.p)
        {
            TagCryptoSecurityBufferFree(&context->ecdhParams->c_pubkey);
        }
        if (context->ecdhParams->salt.p)
        {
            TagCryptoSecurityBufferFree(&context->ecdhParams->salt);
        }

        memset((void *)context->ecdhParams, 0, sizeof(TagSecurityEcdhParams_t));
        TagFree((void *)context->ecdhParams);
        context->ecdhParams = NULL;
    }

    context->subSystem &= ~TAG_SECURITY_SUB_ECDH;

    return TAG_ERROR_NONE;
}

TagError_t TagCryptoEcdhComputeSharedSecret(TagSecurityContext_t *context, TagSecurityBuffer_t *output_buf)
{
    TagError_t err;
    TagSecurityEcdhParams_t *ecdh_params;
    TagSecurityBuffer_t pmsecret_buf = {0};
    TagSecurityBuffer_t secret_buf = {0};
    TagSecurityBuffer_t shared_secret_buf = {0};

    err = tagCryptoCheckContextAndParamsIsValid(context, TAG_SECURITY_SUB_ECDH);
    if (err != TAG_ERROR_NONE)
    {
        return err;
    }

    ecdh_params = context->ecdhParams;

    err = tagCryptoEcdhComputePremasterSecret(&ecdh_params->t_seckey, &ecdh_params->c_pubkey,
                                              &pmsecret_buf);
    if (err != TAG_ERROR_NONE)
    {
        goto exit;
    }

    secret_buf.len = pmsecret_buf.len + ecdh_params->salt.len;
    secret_buf.p = (uint8_t *)TagMalloc(secret_buf.len);
    if (!secret_buf.p)
    {
        err = TAG_ERROR_SECURITY_ECDH_SECRET_KEY_WITH_SALT_MEM_ALLOC;
        goto exit_free_pmsecret;
    }

    memcpy(secret_buf.p, pmsecret_buf.p, pmsecret_buf.len);
    memcpy(secret_buf.p + pmsecret_buf.len, ecdh_params->salt.p, ecdh_params->salt.len);

    shared_secret_buf.len = TAG_SECURITY_SHA256_LEN;
    shared_secret_buf.p = (uint8_t *)TagMalloc(shared_secret_buf.len);
    if (!shared_secret_buf.p)
    {
        err = TAG_ERROR_SECURITY_ECDH_SHARED_SECRET_KEY_MEM_ALLOC;
        goto exit_free_secret;
    }

    err = TagCryptoSha256(secret_buf.p, secret_buf.len, shared_secret_buf.p, shared_secret_buf.len);
    if (err != TAG_ERROR_NONE)
    {
        err = TAG_ERROR_SECURITY_ECDH_SHARED_SECRET_KEY_SHA;
        goto exit_free_shared_secret;
    }

    if (output_buf)
    {
        *output_buf = shared_secret_buf;
    }

    err = TAG_ERROR_NONE;
    goto exit_free_secret;

exit_free_shared_secret:
    TagCryptoSecurityBufferFree(&shared_secret_buf);
exit_free_secret:
    TagCryptoSecurityBufferFree(&secret_buf);
exit_free_pmsecret:
    TagCryptoSecurityBufferFree(&pmsecret_buf);
exit:
    return err;
}

TagError_t TagCryptoSha256(const uint8_t *input, size_t input_len, uint8_t *output, size_t output_len)
{
    int ret;

    if (!input || (input_len == 0))
    {
        TAG_LOG_E("SHA:invalid input with %d@%p", (int)input_len, input);
        return TAG_ERROR_INVALID_ARG;
    }

    if (!output || (output_len < TAG_SECURITY_SHA256_LEN))
    {
        TAG_LOG_E("SHA:invalid output with %d@%p", (int)output_len, output);
        return TAG_ERROR_INVALID_ARG;
    }

    ret = mbedtls_sha256_ret(input, input_len, output, 0);
    if (ret)
    {
        TAG_LOG_E("SHA:mbedtls_sha256_ret = -0x%04X", -ret);
        return TAG_ERROR_SECURITY_SHA256;
    }

    return TAG_ERROR_NONE;
}

void TagCryptoInit(void)
{
    PortEncryptionInit();
}

TagSecurityContext_t *TagCryptoContextInit(void)
{
    TagSecurityContext_t *context;

    context = (TagSecurityContext_t *)TagMalloc(sizeof(TagSecurityContext_t));
    if (!context)
    {
        TAG_LOG_E("Failed to malloc for context init");
        return NULL;
    }

    memset(context, 0, sizeof(TagSecurityContext_t));

    return context;
}

TagError_t TagCryptoContextReset(TagSecurityContext_t *context)
{
    EndUserDevice *endUserDevice = gTagContext->endUserDevices;

    if (!context)
    {
        return TAG_ERROR_SECURITY_CONTEXT_NULL;
    }

    TagCryptoEcdhDeinit(context);

    while (endUserDevice)
    {
        if (endUserDevice->authParam != NULL)
        {
            TagAuthConnectionDeinit(endUserDevice->authParam);
            endUserDevice->authParam = NULL;
        }
        endUserDevice = endUserDevice->next;
    }

    if (context->cloudPublicKey.p)
    {
        TagCryptoSecurityBufferFree(&context->cloudPublicKey);
    }
    if (context->randomValue.p)
    {
        TagCryptoSecurityBufferFree(&context->randomValue);
    }
    if (context->privacyIdSeed.p)
    {
        TagCryptoSecurityBufferFree(&context->privacyIdSeed);
    }
    if (context->privacyIdIv.p)
    {
        TagCryptoSecurityBufferFree(&context->privacyIdIv);
    }

    memset(context, 0, sizeof(TagSecurityContext_t));

    return TAG_ERROR_NONE;
}

TagError_t TagCryptoX963KdfWithSha256(
    const uint8_t *input_z_buf, size_t input_z_len,
    const uint8_t *input_info_buf, size_t input_info_len,
    uint8_t *output_buf, size_t output_len)
{
    TagError_t err;
    int counter = 1;
    uint8_t *hash_input_buf;
    uint8_t hash_output_buf[TAG_SECURITY_SHA256_LEN];
    size_t written_len, copy_len, hash_input_len;

    if (!input_z_buf || (input_z_len == 0))
    {
        TAG_LOG_E("KDF:invalid input_z with %d@%p", (int)input_z_len, input_z_buf);
        return TAG_ERROR_SECURITY_KDF_INVALID_Z;
    }

    if (!input_info_buf && (input_info_len != 0))
    {
        TAG_LOG_E("KDF:input_info is null but info_len is not zero %d", (int)input_info_len);
        return TAG_ERROR_SECURITY_KDF_INVALID_INFO;
    }

    if (!output_buf || (output_len == 0))
    {
        TAG_LOG_E("KDF:invalid output with %d@%p", (int)output_len, output_buf);
        return TAG_ERROR_SECURITY_KDF_INVALID_OUT;
    }

    written_len = 0;
    hash_input_len = input_z_len + 4 + input_info_len;
    hash_input_buf = (uint8_t *)TagMalloc(hash_input_len);
    if (!hash_input_buf)
    {
        return TAG_ERROR_SECURITY_KDF_HASH_MEM_ALLOC;
    }

    memcpy(hash_input_buf, input_z_buf, input_z_len);
    memcpy(hash_input_buf + input_z_len + 4, input_info_buf, input_info_len);

    while (written_len < output_len)
    {
        hash_input_buf[input_z_len] = (counter & 0xff000000) >> 24;
        hash_input_buf[input_z_len + 1] = (counter & 0x00ff0000) >> 16;
        hash_input_buf[input_z_len + 2] = (counter & 0x0000ff00) >> 8;
        hash_input_buf[input_z_len + 3] = (counter & 0x000000ff);

        err = TagCryptoSha256(hash_input_buf, hash_input_len,
                              hash_output_buf, TAG_SECURITY_SHA256_LEN);
        if (err != TAG_ERROR_NONE)
        {
            TagFree(hash_input_buf);
            return TAG_ERROR_SECURITY_KDF_SHA;
        }

        if ((output_len - written_len) < TAG_SECURITY_SHA256_LEN)
        {
            copy_len = output_len - written_len;
        }
        else
        {
            copy_len = TAG_SECURITY_SHA256_LEN;
        }

        memcpy(output_buf + written_len, hash_output_buf, copy_len);

        written_len += copy_len;
        counter++;
    }

    TagFree(hash_input_buf);
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t tagCryptoUrlEncode(char *buf, size_t buf_len)
{
    size_t i;
    if (!buf)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }
    if (!buf_len)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_LEN;
    }
    for (i = 0; i < buf_len; i++)
    {
        switch (buf[i])
        {
        case '+':
            buf[i] = '-';
            break;
        case '/':
            buf[i] = '_';
            break;
        default:
            break;
        }
    }
    return TAG_ERROR_NONE;
}

STATIC_FUNCTION TagError_t tagCryptoUrlDecode(char *buf, size_t buf_len)
{
    size_t i;
    if (!buf)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_INPUT;
    }
    if (!buf_len)
    {
        return TAG_ERROR_SECURITY_BUFFER_INVALID_LEN;
    }
    for (i = 0; i < buf_len; i++)
    {
        switch (buf[i])
        {
        case '-':
            buf[i] = '+';
            break;
        case '_':
            buf[i] = '/';
            break;
        default:
            break;
        }
    }
    return TAG_ERROR_NONE;
}

TagError_t TagCryptoBase64Encode(const uint8_t *src, size_t src_len,
                                 uint8_t *dst, size_t dst_len,
                                 size_t *out_len)
{
    int ret;
    if (!src || (src_len == 0))
    {
        TAG_LOG_E("B64E:invalid src with %d@%p", (int)src_len, src);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!dst || (dst_len == 0))
    {
        TAG_LOG_E("B64E:invalid dst with %d@%p", (int)dst_len, dst);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!out_len)
    {
        TAG_LOG_E("B64E:length output buffer is null");
        return TAG_ERROR_INVALID_ARG;
    }

    ret = mbedtls_base64_encode(dst, dst_len, out_len, src, src_len);
    if (ret)
    {
        TAG_LOG_E("B64E:mbedtls_base64_encode = -0x%04X", -ret);
        return TAG_ERROR_SECURITY_BASE64_ENCODE;
    }
    return TAG_ERROR_NONE;
}

TagError_t TagCryptoBase64Decode(const uint8_t *src, size_t src_len,
                                 uint8_t *dst, size_t dst_len,
                                 size_t *out_len)
{
    int ret;
    if (!src || (src_len == 0))
    {
        TAG_LOG_E("B64D:invalid src with %d@%p", (int)src_len, src);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!dst || (dst_len == 0))
    {
        TAG_LOG_E("B64D:invalid dst with %d@%p", (int)dst_len, dst);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!out_len)
    {
        TAG_LOG_E("B64D:length output buffer is null");
        return TAG_ERROR_INVALID_ARG;
    }
    ret = mbedtls_base64_decode(dst, dst_len, out_len, src, src_len);
    if (ret)
    {
        TAG_LOG_E("B64D:mbedtls_base64_decode = -0x%04X", -ret);
        return TAG_ERROR_SECURITY_BASE64_DECODE;
    }
    return TAG_ERROR_NONE;
}

TagError_t TagCryptoBase64EncodeUrlsafe(const uint8_t *src, size_t src_len,
                                        uint8_t *dst, size_t dst_len,
                                        size_t *out_len)
{
    int ret;
    if (!src || (src_len == 0))
    {
        TAG_LOG_E("B64EU:invalid src with %d@%p", (int)src_len, src);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!dst || (dst_len == 0))
    {
        TAG_LOG_E("B64EU:invalid dst with %d@%p", (int)dst_len, dst);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!out_len)
    {
        TAG_LOG_E("B64EU:length output buffer is null");
        return TAG_ERROR_INVALID_ARG;
    }
    ret = mbedtls_base64_encode(dst, dst_len, out_len, src, src_len);
    if (ret)
    {
        TAG_LOG_E("B64EU:mbedtls_base64_encode = -0x%04X", -ret);
        return TAG_ERROR_SECURITY_BASE64_URL_ENCODE;
    }
    ret = tagCryptoUrlEncode((char *)dst, *out_len);
    if (ret)
    {
        TAG_LOG_E("B64EU:url_encode fail %d", ret);
        return TAG_ERROR_SECURITY_BASE64_URL_ENCODE;
    }
    return TAG_ERROR_NONE;
}

TagError_t TagCryptoBase64DecodeUrlsafe(const uint8_t *src, size_t src_len,
                                        uint8_t *dst, size_t dst_len,
                                        size_t *out_len)
{
    uint8_t *src_dup = NULL;
    size_t align_len;
    size_t i;
    int ret;
    TagError_t err;
    if (!src || (src_len == 0))
    {
        TAG_LOG_E("B64DU:invalid src with %d@%p", (int)src_len, src);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!dst || (dst_len == 0))
    {
        TAG_LOG_E("B64DU:invalid dst with %d@%p", (int)dst_len, dst);
        return TAG_ERROR_INVALID_ARG;
    }
    if (!out_len)
    {
        TAG_LOG_E("B64DU:outlen is null");
        return TAG_ERROR_INVALID_ARG;
    }

    align_len = TAG_SECURITY_B64_ALIGN_LEN(src_len);
    src_dup = (uint8_t *)TagMalloc(align_len + 1);
    if (src_dup == NULL)
    {
        TAG_LOG_E("B64DU:malloc failed for align buffer");
        return TAG_ERROR_MEM_ALLOC;
    }
    memcpy(src_dup, src, src_len);
    /* consider '=' removed from tail */
    for (i = src_len; i < align_len; i++)
    {
        src_dup[i] = '=';
    }
    src_dup[align_len] = '\0';
    err = tagCryptoUrlDecode((char *)src_dup, align_len);
    if (err != TAG_ERROR_NONE)
    {
        TAG_LOG_E("B64DU:url_decode fail %d", err);
        TagFree(src_dup);
        return TAG_ERROR_SECURITY_BASE64_URL_DECODE;
    }
    ret = mbedtls_base64_decode(dst, dst_len, out_len, (const uint8_t *)src_dup, align_len);
    if (ret)
    {
        TAG_LOG_E("B64DU:mbedtls_base64_decode = -0x%04X", -ret);
        TagFree(src_dup);
        return TAG_ERROR_SECURITY_BASE64_DECODE;
    }
    TagFree(src_dup);

    return TAG_ERROR_NONE;
}
