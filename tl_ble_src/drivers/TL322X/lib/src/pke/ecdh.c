/********************************************************************************************************
 * @file    ecdh.c
 *
 * @brief   This is the source file for tl322x
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#include "lib/include/pke/pke_config.h"


#ifdef SUPPORT_ECDH

    #include "lib/include/pke/ecdh.h"
    #include "lib/include/crypto_common/utility.h"

/**
 * @brief       ECDH compute key
 * @param[in]   curve           - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   local_prikey    - local private key, big-endian.
 * @param[in]   peer_pubkey     - peer public key, big-endian.
 * @param[out]  key             - output key.
 * @param[in]   keyByteLen      - byte length of output key.
 * @param[in]   kdf             - KDF function to get key.
 * @return      ECDH_SUCCESS(success)     other:error
 */
unsigned int ecdh_compute_key(eccp_curve_t *curve, unsigned char *local_prikey, unsigned char *peer_pubkey, unsigned char *key, unsigned int keyByteLen, KDF_FUNC kdf)
{
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int Px[ECCP_MAX_WORD_LEN];
    unsigned int Py[ECCP_MAX_WORD_LEN];
    unsigned int pByteLen, pWordLen, nByteLen, nWordLen;
    unsigned int ret;

    if ((NULL == curve) || (NULL == local_prikey) || (NULL == peer_pubkey) || (NULL == key)) {
        return ECDH_POINTOR_NULL;
    } else if (0u == keyByteLen) {
        return ECDH_INVALID_INPUT;
    } else {
        ;
    }

    pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);

    //make sure private key is in [1, n-1]
    k[nWordLen - 1] = 0u;
    reverse_byte_array((unsigned char *)local_prikey, (unsigned char *)k, nByteLen);
    ret = uint32_integer_check(k, curve->eccp_n, nWordLen, ECDH_ZERO_ALL, ECDH_INTEGER_TOO_BIG, ECDH_SUCCESS);
    if (ECDH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //check public key
    Px[pWordLen - 1] = 0u;
    Py[pWordLen - 1] = 0u;
    reverse_byte_array(peer_pubkey, (unsigned char *)Px, pByteLen);
    reverse_byte_array(peer_pubkey + pByteLen, (unsigned char *)Py, pByteLen);
    ret = eccp_pointVerify(curve, Px, Py);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = eccp_pointMul(curve, k, Px, Py, Px, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    reverse_byte_array((unsigned char *)Px, (unsigned char *)Px, pByteLen);

    if (NULL != kdf) {
        kdf(Px, pByteLen, key, keyByteLen);
    } else {
        if (keyByteLen > pByteLen) {
            keyByteLen = pByteLen;
        } else {
            ;
        }

        memcpy_(key, (unsigned char *)Px, keyByteLen);
    }

    return ECDH_SUCCESS;
}

#endif
