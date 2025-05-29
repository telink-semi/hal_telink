/********************************************************************************************************
 * @file    ecdh.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
/********* pke version:1.0 *********/
#include "lib/include/pke/eccp_curve.h"
#include "lib/include/pke/ecdh.h"

/**
 * @brief       ECDH compute key.
 * @param[in]   curve           - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   local_prikey    - local private key, big-endian.
 * @param[in]   peer_pubkey     - peer public key, big-endian.
 * @param[out]  key             - output key.
 * @param[in]   keyByteLen      - byte length of output key.
 * @param[in]   KDF             - KDF function to get key.
 * @Return      0(success), other(error)
 */
unsigned char ecdh_compute_key(eccp_curve_t *curve, unsigned char *local_prikey, unsigned char *peer_pubkey, unsigned char *key, unsigned int keyByteLen, KDF_FUNC kdf)
{
    unsigned int  k[ECC_MAX_WORD_LEN]  = {0};
    unsigned int  Px[ECC_MAX_WORD_LEN] = {0};
    unsigned int  Py[ECC_MAX_WORD_LEN] = {0};
    unsigned int  byteLen, wordLen;
    unsigned char ret;

    if (NULL == curve || NULL == local_prikey || NULL == peer_pubkey || NULL == key) {
        return ECDH_POINTOR_NULL;
    }
    if (0 == keyByteLen) {
        return ECDH_INVALID_INPUT;
    }

    byteLen = (curve->eccp_n_bitLen + 7) / 8;
    wordLen = (curve->eccp_n_bitLen + 31) / 32;

    //make sure private key is in [1, n-1]
    reverse_byte_array((unsigned char *)local_prikey, (unsigned char *)k, byteLen);
    if (ismemzero4(k, wordLen)) {
        return ECDH_INVALID_INPUT;
    }
    if (big_integer_compare(k, wordLen, curve->eccp_n, wordLen) >= 0) {
        return ECDH_INVALID_INPUT;
    }

    //check public key
    reverse_byte_array(peer_pubkey, (unsigned char *)Px, byteLen);
    reverse_byte_array(peer_pubkey + byteLen, (unsigned char *)Py, byteLen);
    ret = pke_eccp_point_verify(curve, Px, Py);
    if (PKE_SUCCESS != ret) {
        return ret;
    }

    ret = pke_eccp_point_mul(curve, k, Px, Py, Px, Py);
    if (PKE_SUCCESS != ret) {
        return ret;
    }

    reverse_byte_array((unsigned char *)Px, (unsigned char *)Px, byteLen);

    if (kdf) {
        kdf(Px, byteLen, key, keyByteLen);
    } else {
        if (keyByteLen > byteLen) {
            keyByteLen = byteLen;
        }
        memcpy(key, Px, keyByteLen);
    }

    return ECDH_SUCCESS;
}
