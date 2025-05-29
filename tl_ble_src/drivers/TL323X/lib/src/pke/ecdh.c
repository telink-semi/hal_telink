/********************************************************************************************************
 * @file    ecdh.c
 *
 * @brief   This is the source file for TL323X
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
#include <stdio.h>
#include "lib/include/pke/pke.h"
#include "lib/include/pke/ecdh.h"
#include "lib/include/crypto_common/utility.h"

#ifdef SUPPORT_ECDH
/**
 * @brief           ECDH compute key
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid.
 * @param[in]       local_prikey         - local private key, big-endian.
 * @param[in]       peer_pubkey          - peer public key, big-endian.
 * @param[out]      key                  - output key.
 * @param[in]       key_len              - byte length of output key.
 * @param[in]       kdf                  - KDF function to get key.
 * @return          ECDH_SUCCESS(success)     other:error
 */
unsigned int ecdh_compute_key(const eccp_curve_t *curve, const unsigned char *local_prikey, const unsigned char *peer_pubkey, unsigned char *key, unsigned int key_len,
                              KDF_FUNC kdf)
{
    unsigned int k[ECCP_MAX_WORD_LEN], px[ECCP_MAX_WORD_LEN], Py[ECCP_MAX_WORD_LEN];
    unsigned int p_len, p_wlen, n_len, n_wlen;
    unsigned int ret;

    if ((NULL == curve) || (NULL == local_prikey) || (NULL == peer_pubkey) || (NULL == key))
    {
        return ECDH_POINTER_NULL;
    }
    else if (0u == key_len)
    {
        return ECDH_INVALID_INPUT;
    }
    else
    {
        // handle other
    }

    p_len = get_byte_len(curve->eccp_p_bitLen);
    p_wlen = get_word_len(curve->eccp_p_bitLen);
    n_len = get_byte_len(curve->eccp_n_bitLen);
    n_wlen = get_word_len(curve->eccp_n_bitLen);

    // make sure private key is in [1, n-1]
    k[n_wlen - 1U] = 0u;
    reverse_byte_array(local_prikey, (unsigned char *)k, n_len);
    ret = uint32_integer_check(k, curve->eccp_n, n_wlen, ECDH_ZERO_ALL, ECDH_INTEGER_TOO_BIG, ECDH_SUCCESS);
    if (ECDH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // check public key
    px[p_wlen - 1U] = 0u;
    Py[p_wlen - 1U] = 0u;
    reverse_byte_array(peer_pubkey, (unsigned char *)px, p_len);
    reverse_byte_array(&peer_pubkey[p_len], (unsigned char *)Py, p_len);
    ret = eccp_pointverify(curve, px, Py);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = eccp_pointmul(curve, k, px, Py, px, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    reverse_byte_array((unsigned char *)px, (unsigned char *)px, p_len);

    if (NULL != kdf)
    {
        kdf(px, p_len, key, key_len);
    }
    else
    {
        if (key_len > p_len)
        {
            key_len = p_len;
        }
        else
        {
        }

        memcpy_(key, (unsigned char *)px, key_len);
    }

    return ECDH_SUCCESS;
}

#endif
