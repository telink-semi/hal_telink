/********************************************************************************************************
 * @file    x25519.c
 *
 * @brief   This is the source file for TL721X
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

    //void x25519_decode_scalar( unsigned char *k, unsigned char *out, unsigned int bytes);


    //Curve25519 parameters
    #if 0
unsigned int  curve25519_p[8]    = {0xFFFFFFED,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x7FFFFFFF,};
unsigned int  curve25519_p_h[8]  = {0x000005A4,0,0,0,0,0,0,0,};
unsigned int  curve25519_p_n0[1] = {0x286BCA1B};
unsigned int  curve25519_n[]     = {0x5CF5D3ED,0x5812631A,0xA2F79CD6,0x14DEF9DE,0x00000000,0x00000000,0x00000000,0x10000000,};
unsigned int  curve25519_n_h[8]  = {0x449C0F01,0xA40611E3,0x68859347,0xD00E1BA7,0x17F5BE65,0xCEEC73D2,0x7C309A3D,0x0399411B,};
unsigned int  curve25519_n_n0[1] = {0x12547E1B};
    #else
extern unsigned int curve25519_p[8];
extern unsigned int curve25519_p_h[8];
extern unsigned int curve25519_p_n0[1];
extern unsigned int curve25519_n[];
extern unsigned int curve25519_n_h[8];
extern unsigned int curve25519_n_n0[1];
    #endif
unsigned int curve25519_a24[8] = {
    0x0001DB41,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
//unsigned int  curve25519_B[]   = {0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,};
unsigned int curve25519_u[] = {
    0x00000009,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
};
unsigned int curve25519_v[] = {
    0x7ECED3D9,
    0x29E9C5A2,
    0x6D7C61B2,
    0x923D4D7E,
    0x7748D14C,
    0xE01EDD2C,
    0xB8A086B4,
    0x20AE19A1,
};

unsigned int curve25519_h = 8;


mont_curve_t c25519[1] = {
    {
     255,
     (unsigned int *)curve25519_p,
     (unsigned int *)curve25519_p_h,
     (unsigned int *)curve25519_p_n0,
     (unsigned int *)curve25519_a24,
     (unsigned int *)curve25519_u,
     (unsigned int *)curve25519_v,
     (unsigned int *)curve25519_n,
     (unsigned int *)curve25519_n_h,
     (unsigned int *)curve25519_n_n0,
     (unsigned int *)&curve25519_h,
     },
};


    #if 0
 /**
  * @brief     Decode X25519 scalar for point multiplication
  * @param[in] k                 - Input scalar value for decoding.
  * @param[out] out              - Output, big scalar in little-endian format.
  * @param[in] bytes             - Input byte length of k and out.
  * @return     none
  */
void x25519_decode_scalar(unsigned char *k, unsigned char *out, unsigned int bytes)
{
    if(k != out)
    {
        memcpy_(out, k, bytes);
    }
    else
    {;}

    out[0] &= 0xF8;            //clear lowest 3 bits
    out[bytes - 1] &= 0x7F;    //clear highest 1 bit
    out[bytes - 1] |= 0x40;    //set second highest bit as 1
}
    #endif


/**
 * @brief     Decode X25519 u coordinate for point multiplication
 * @param[in]  u                - Input u coordinate for point multiplication.
 * @param[in]  p                - Modulus (p) in little-endian format.
 * @param[out] out              - Output big scalar in little-endian format.
 * @param[in]  bytes            - Byte length of u, p, and out.
 * @return     none
 */
unsigned int x25519_decode_u(unsigned char *u, unsigned int *p, unsigned char *out, unsigned int bytes)
{
    unsigned char ret;

    if (u != out) {
        memcpy_(out, u, bytes);
    } else {
        ;
    }

    //clear highest bit
    out[bytes - 1] &= 0x7F;

    //mod p
    if (uint32_BigNumCmp((unsigned int *)out, (bytes + 3) / 4, p, (bytes + 3) / 4) >= 0) {
        ret = pke_sub((unsigned int *)out, p, (unsigned int *)out, (bytes + 3) / 4);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief     Get X25519 public key from private key
 * @param[in]  prikey           - Input private key, 32 bytes, little-endian format.
 * @param[out] pubkey           - Output public key, 32 bytes, little-endian format.
 * @return     X25519_SUCCESS(success); other(error)
 */
unsigned int x25519_get_pubkey_from_prikey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int t[C25519_WORD_LEN];
    unsigned int ret;

    if (NULL == prikey || NULL == pubkey) {
        return X25519_POINTER_NULL;
    } else {
        ;
    }

    x25519_decode_scalar(prikey, (unsigned char *)t, C25519_BYTE_LEN);

    //it could be proved that here t is not a multiple of c25519->n, so no need to compare
    //(t mod c25519->n) with c25519->n

    ret = x25519_pointMul((mont_curve_t *)c25519, t, c25519->u, t);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    memcpy_(pubkey, t, C25519_BYTE_LEN);

    return X25519_SUCCESS;
}

/**
 * @brief     Get X25519 random key pair
 * @param[out] prikey           - Output private key, 32 bytes, little-endian format.
 * @param[out] pubkey           - Output public key, 32 bytes, little-endian format.
 * @return    X25519_SUCCESS(success); other(error)
 */
unsigned int x25519_getkey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int ret;

    if (NULL == prikey || NULL == pubkey) {
        return X25519_POINTER_NULL;
    } else {
        ;
    }

    ret = get_rand(prikey, C25519_BYTE_LEN);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
        return x25519_get_pubkey_from_prikey(prikey, pubkey);
    }
}

/**
 * @brief     X25519 key agreement
 * @param[in] local_prikey      - Input local private key, 32 bytes, little-endian format.
 * @param[in] peer_pubkey       - Input peer public key, 32 bytes, little-endian format.
 * @param[out] key              - Output derived key.
 * @param[in] keyByteLen        - Input byte length of the output key.
 * @param[in] kdf               - Input KDF function; set to NULL if not used.
 * @return     X25519_SUCCESS(success); other(error)
 */
unsigned int x25519_compute_key(unsigned char local_prikey[32], unsigned char peer_pubkey[32], unsigned char *key, unsigned int keyByteLen, KDF_FUNC kdf)
{
    unsigned int k[C25519_WORD_LEN], u[C25519_WORD_LEN];
    unsigned int ret;

    if (NULL == local_prikey || NULL == peer_pubkey || NULL == key) {
        return X25519_POINTER_NULL;
    } else if (0 == keyByteLen) {
        return X25519_INVALID_INPUT;
    } else {
        ;
    }

    //decode u
    ret = x25519_decode_u(peer_pubkey, c25519->p, (unsigned char *)u, C25519_BYTE_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //u could not be zero, otherwise it will return PKE_NO_MODINV no matter what the scalar is.
    if (uint32_BigNum_Check_Zero(u, C25519_WORD_LEN)) {
        return X25519_INVALID_INPUT;
    } else {
        ;
    }

    //decode scalar
    x25519_decode_scalar(local_prikey, (unsigned char *)k, C25519_BYTE_LEN);

    ret = x25519_pointMul((mont_curve_t *)c25519, k, u, u);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //make sure u is not zero
    k[0] = 0;
    for (ret = 0; ret < C25519_WORD_LEN; ret++) {
        k[0] |= u[ret];
    }
    if (0 == k[0]) {
        return X25519_ZERO_ALL;
    } else {
        ;
    }

    if (kdf) {
        kdf(u, C25519_BYTE_LEN, key, keyByteLen);
    } else {
        if (keyByteLen > C25519_BYTE_LEN) {
            keyByteLen = C25519_BYTE_LEN;
        } else {
            ;
        }

        memcpy_(key, u, keyByteLen);
    }

    return X25519_SUCCESS;
}

#endif
