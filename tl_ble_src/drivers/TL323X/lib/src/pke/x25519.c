/********************************************************************************************************
 * @file    x25519.c
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
#include <string.h>
#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/pke.h"
#include "lib/include/trng/trng.h"
#include "lib/include/pke/x25519.h"
#include "lib/include/pke/ed25519.h"
#ifdef SUPPORT_C25519

/**
 * @brief           Decode X25519 u coordinate for point multiplication.
 *                  This function decodes the u-coordinate, applies necessary bit masking, and
 *                  performs a modulus operation with p.
 * @param[in]       u                    - u-coordinate in little-endian format.
 * @param[in]       p                    - modulus value in little-endian format (256 bits).
 * @param[out]      out                  - decoded big scalar in little-endian format.
 * @param[in]       bytes                - Byte length of the input and output arrays (32 bytes for X25519).
 * @return          PKE_SUCCESS on success, error code otherwise.
 * @note            All operands are of 256 bits for X25519.
 */
static unsigned int x25519_decode_u(const unsigned char *u, const unsigned int *p, unsigned int *out)
{
    unsigned int ret;
    unsigned char *out_u8 = (unsigned char *)out;

    if (u != ((unsigned char *)out))
    {
        memcpy_(out_u8, u, 32u);
    }
    else
    {
    }

    out_u8[32u - 1u] &= (unsigned char)0x7F; // Clear highest bit

    // Mod p
    if (uint32_big_num_cmp((unsigned int *)out, 8u, p, 8u) >= 0)
    {
#if 1
        ret = pke_sub((unsigned int *)out, p, (unsigned int *)out, 8u);
#else
        ret = pke_mod_add_sub_mul_256bits_internal((unsigned int *)out, p, (unsigned int *)out, MICROCODE_INTSUB);
#endif
    }
    else
    {
        ret = PKE_SUCCESS;
    }

    return ret;
}

/**
 * @brief           Generate X25519 public key from a private key.
 *                  This function generates the corresponding public key for a given private key
 *                  using scalar multiplication.
 * @param[in]       prikey               - Private key (32 bytes in little-endian format).
 * @param[out]      pubkey               - Public key (32 bytes in little-endian format).
 * @return          X25519_SUCCESS on success, error code otherwise.
 */
unsigned int x25519_get_pubkey_from_prikey(const unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int t[C25519_WORD_LEN];
    unsigned int ret;

    if ((NULL == prikey) || (NULL == pubkey))
    {
        ret = X25519_POINTER_NULL;
    }
    else
    {
        x25519_ed25519_decode_scalar(prikey, (unsigned char *)t);

        // It could be proved that here t is not a multiple of c25519->n, so no need
        // to compare
        //(t mod c25519->n) with c25519->n

        ret = x25519_pointmul(c25519, t, c25519->u, t);
        if (PKE_SUCCESS == ret)
        {
            memcpy_(pubkey, (unsigned char *)t, C25519_BYTE_LEN);
            ret = X25519_SUCCESS;
        }
        else
        {
        }
    }

    return ret;
}

/**
 * @brief           Generate a random key pair for X25519.
 *                  This function generates a new private key and computes the corresponding
 *                  public key.
 * @param[out]      prikey               - Generated private key (32 bytes in little-endian format).
 * @param[out]      pubkey               - Corresponding public key (32 bytes in little-endian format).
 * @return          X25519_SUCCESS on success, error code otherwise.
 */
unsigned int x25519_getkey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int ret;

    if ((NULL == prikey) || (NULL == pubkey))
    {
        ret = X25519_POINTER_NULL;
    }
    else
    {
        ret = get_rand(prikey, C25519_BYTE_LEN);
        if (TRNG_SUCCESS == ret)
        {
            ret = x25519_get_pubkey_from_prikey(prikey, pubkey);
        }
        else
        {
        }
    }

    return ret;
}

/**
 * @brief           Perform X25519 key agreement.
 *                  This function computes the shared secret between local and peer keys using
 *                  scalar multiplication, and optionally applies a Key Derivation Function (KDF)
 *                  to derive the final key material.
 * @param[in]       local_prikey         - Local private key (32 bytes in little-endian format).
 * @param[in]       peer_pubkey          - Peer public key (32 bytes in little-endian format).
 * @param[out]      key                  - Derived key.
 * @param[in]       key_len              - Byte length of the output key.
 * @param[in]       kdf                  - Key Derivation Function. Set to NULL if no KDF is used.
 * @return          X25519_SUCCESS on success, error code otherwise.
 */
unsigned int x25519_compute_key(const unsigned char local_prikey[32], const unsigned char peer_pubkey[32], unsigned char *key, unsigned int key_len, KDF_FUNC kdf)
{
    unsigned int k[C25519_WORD_LEN], u[C25519_WORD_LEN];
    unsigned int ret;

    if ((NULL == local_prikey) || (NULL == peer_pubkey) || (NULL == key))
    {
        ret = X25519_POINTER_NULL;
    }
    else if ((0u == key_len) || ((NULL == kdf) && (key_len > C25519_BYTE_LEN)))
    {
        ret = X25519_INVALID_INPUT;
    }
    else
    {
        ret = x25519_decode_u(peer_pubkey, c25519->p, u); // Decode u
        if (PKE_SUCCESS == ret)
        {
            // u could not be zero, otherwise it will return PKE_NO_MODINV no matter
            // what the scalar is.
            if (1u == uint32_bignum_check_zero(u, C25519_WORD_LEN))
            {
                ret = X25519_INVALID_INPUT;
            }
            else
            {
            }
        }
        else
        {
        }

        if (PKE_SUCCESS == ret)
        {
            x25519_ed25519_decode_scalar(local_prikey,
                                         (unsigned char *)k); // Decode scalar
            ret = x25519_pointmul(c25519, k, u, u);
        }
        else
        {
        }

        if (PKE_SUCCESS == ret)
        {
            // Make sure u is not zero
            if (0u != uint32_bignum_check_zero(u, C25519_WORD_LEN))
            {
                ret = X25519_ZERO_ALL;
            }
            else
            {
            }
        }
        else
        {
        }

        if (PKE_SUCCESS == ret)
        {
            if (NULL != kdf)
            {
                (void)kdf(u, C25519_BYTE_LEN, key, key_len);
            }
            else
            {
                memcpy_(key, (unsigned char *)u, key_len);
            }
            ret = X25519_SUCCESS;
        }
        else
        {
        }
    }

    return ret;
}

#endif
