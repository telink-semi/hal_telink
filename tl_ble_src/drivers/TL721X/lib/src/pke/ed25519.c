/********************************************************************************************************
 * @file    ed25519.c
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
#include "string.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/pke.h"
#include "lib/include/hash/hash.h"
#include "lib/include/pke/ed25519.h"
#include "lib/include/trng/trng.h"

#if 1

char *Ed25519_sign_string = "SigEd25519 no Ed25519 collisions";


//Curve25519 parameters
unsigned int curve25519_p[8] = {
    0xFFFFFFED,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF,
    0x7FFFFFFF,
};
unsigned int curve25519_p_h[8] = {
    0x000005A4,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
unsigned int curve25519_p_n0[1] = {0x286BCA1B};
unsigned int curve25519_n[]     = {
    0x5CF5D3ED,
    0x5812631A,
    0xA2F79CD6,
    0x14DEF9DE,
    0x00000000,
    0x00000000,
    0x00000000,
    0x10000000,
};
unsigned int curve25519_n_h[8] = {
    0x449C0F01,
    0xA40611E3,
    0x68859347,
    0xD00E1BA7,
    0x17F5BE65,
    0xCEEC73D2,
    0x7C309A3D,
    0x0399411B,
};
unsigned int curve25519_n_n0[1] = {0x12547E1B};


//unsigned int  ed25519_p       = {0xFFFFFFED,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x7FFFFFFF,};
//unsigned int  ed25519_p_h     = {0x000005A4,0,0,0,0,0,0,0,};
//unsigned int  ed25519_p_n0    = {0x286BCA1B};
unsigned int ed25519_d[] = {
    0x135978A3,
    0x75EB4DCA,
    0x4141D8AB,
    0x00700A4D,
    0x7779E898,
    0x8CC74079,
    0x2B6FFE73,
    0x52036CEE,
};
unsigned int ed25519_Gx[] = {
    0x8F25D51A,
    0xC9562D60,
    0x9525A7B2,
    0x692CC760,
    0xFDD6DC5C,
    0xC0A4E231,
    0xCD6E53FE,
    0x216936D3,
};
unsigned int ed25519_Gy[] = {
    0x66666658,
    0x66666666,
    0x66666666,
    0x66666666,
    0x66666666,
    0x66666666,
    0x66666666,
    0x66666666,
};
//unsigned int  ed25519_n[]     = {0x5CF5D3ED,0x5812631A,0xA2F79CD6,0x14DEF9DE,0x00000000,0x00000000,0x00000000,0x10000000,};
//unsigned int  ed25519_n_h[]   = {0x449C0F01,0xA40611E3,0x68859347,0xD00E1BA7,0x17F5BE65,0xCEEC73D2,0x7C309A3D,0x0399411B,};
//unsigned int  ed25519_n_n0[1] = {0x12547E1B};

edward_curve_t ed25519[1] = {
    {
     255,
     (unsigned int *)curve25519_p,
     (unsigned int *)curve25519_p_h,
     (unsigned int *)curve25519_p_n0,
     (unsigned int *)ed25519_d,
     (unsigned int *)ed25519_Gx,
     (unsigned int *)ed25519_Gy,
     (unsigned int *)curve25519_n,
     (unsigned int *)curve25519_n_h,
     (unsigned int *)curve25519_n_n0,
     NULL,
     },
};

/**
 * @brief      set operand with an unsigned int value
 * @param[in]  a                - modulus.
 * @param[in]  wordLen          - integer a.
 * @param[in]  b                - ainv = a^(-1) mod modulus.
 * @return     none
 * @note
  @verbatim
      -# 1. aWordLen can not be 0
  @endverbatim
 */
static void pke_set_operand_uint32_value(unsigned int *a, unsigned int aWordLen, unsigned int b)
{
    unsigned int i = aWordLen;

    while (i > 1) {
        a[--i] = 0;
    }

    a[0] = b;
}

/**
 * @brief     decode X25519 scalar for point multiplication
 * @param[in]  k                - null.
 * @param[out] out              - big scalar in little-endian
 * @param[in]  bytes            - byte length of k and out
 * @return     none
 */
void x25519_decode_scalar(unsigned char *k, unsigned char *out, unsigned int bytes)
{
    if (k != out) {
        memcpy_(out, (void *)k, bytes);
    } else {
        ;
    }

    out[0] &= 0xF8;         //clear lowest 3 bits
    out[bytes - 1] &= 0x7F; //clear highest 1 bit
    out[bytes - 1] |= 0x40; //set second highest bit as 1
}

/**
 * @brief      edwards25519 curve point mul(random point), Q=[k]P, secure version
 * @param[in]  curve                - edwards25519 curve struct pointer.
 * @param[in]  k                    - scalar, it could be 0 here.
 * @param[in]  Px                   - x coordinate of point P.
 * @param[in]  Py                   - y coordinate of point P.
 * @param[out] Qx                   - x coordinate of point Q.
 * @param[out] Qy                   - y coordinate of point Q.
 * @return      PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. please make sure input point P is on the curve
      -# 2. even if the input point P is valid, the output may be neutral point (0, 1), it is valid
      -# 3. please make sure the curve is edwards25519
      -# 4. k could be zero here.
  @endverbatim
 */
unsigned int ed25519_pointMul_s(edward_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int wordLen = GET_WORD_LEN(curve->p_bitLen);

    if (uint32_BigNum_Check_Zero(k, wordLen)) {
        uint32_clear(Qx, wordLen);
        pke_set_operand_uint32_value(Qy, wordLen, 1);

        return PKE_SUCCESS;
    } else {
        return ed25519_pointMul(curve, k, Px, Py, Qx, Qy);
    }
}

/**
 * @brief      get Ed25519 public key from private key
 * @param[in]  prikey                - private key, 32 bytes, little-endian.
 * @param[out] pubkey                - public key, 32 bytes, little-endian.
 * @return      PKE_SUCCESS(success), other(error)
 */
unsigned int ed25519_get_pubkey_from_prikey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int h[16];
    unsigned int ret;

    if (NULL == prikey || NULL == pubkey) {
        return EdDSA_POINTOR_NULL;
    } else {
        ;
    }

    ret = hash(HASH_SHA512, prikey, 32, (unsigned char *)h);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //decode to get the scalar
    x25519_decode_scalar((unsigned char *)h, (unsigned char *)h, Ed25519_BYTE_LEN);

    ret = ed25519_pointMul_s((edward_curve_t *)ed25519, h, ed25519->Gx, ed25519->Gy, h, h + 8);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //encode pubkey
    memcpy_(pubkey, h + 8, Ed25519_BYTE_LEN);
    if (h[0] & 1) {
        pubkey[Ed25519_BYTE_LEN - 1] |= 0x80;
    } else {
        ;
    }

    return EdDSA_SUCCESS;
}

/**
 * @brief      generate Ed25519 random key pair
 * @param[out] prikey                - private key, 32 bytes, little-endian.
 * @param[out] pubkey                - public key, 32 bytes, little-endian.
 * @return     PKE_SUCCESS(success), other(error)
 */
unsigned int ed25519_getkey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int ret;

    if (NULL == prikey || NULL == pubkey) {
        return EdDSA_POINTOR_NULL;
    } else {
        ;
    }

    ret = get_rand(prikey, Ed25519_BYTE_LEN);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
        return ed25519_get_pubkey_from_prikey(prikey, pubkey);
    }
}

/**
 * @brief      Ed25519 sign
 * @param[in]  mode                - Ed25519 signature mode
 * @param[in]  prikey              - private key, 32 bytes, little-endian
 * @param[in]  pubkey              - public key, 32 bytes, little-endian, if no pubkey, please set it to be NULL
 * @param[in]  ctx                 - 0-255 bytes
 * @param[in]  ctxByteLen          - byte length of ctx
 * @param[in]  M                   - message, M could be empty, in this case please set M to be NULL
 * @param[in]  MByteLen            - byte length of M, M could be empty, so it could be 0
 * @param[out] RS                  - signature
 * @return      PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. if no public key, please set pubkey to be NULL, it will be generated inside
      -# 2. M could be empty(please set M to be NULL), so no need to check M and MByteLen
      -# 3. if mode is Ed25519_DEFAULT, ctx is not involved, no need to check ctx and ctxByteLen
      -# 4. if mode is Ed25519_CTX, ctx can not be empty(ctx length is from 1 to 255)
      -# 5. if mode is Ed25519_PH, ctx length is from 0 to 255, default length is 0, thus ctx could be empty
  @endverbatim
 */
unsigned int ed25519_sign(Ed25519_MODE mode, unsigned char prikey[32], unsigned char pubkey[32], unsigned char *ctx, unsigned char ctxByteLen, unsigned char *M, unsigned int MByteLen, unsigned char RS[64])
{
    unsigned int   h[16];
    unsigned int  *s      = h;
    unsigned char *prefix = (unsigned char *)(h + Ed25519_WORD_LEN);

    unsigned int *r = h + Ed25519_WORD_LEN;
    unsigned int  k[Ed25519_WORD_LEN << 1];
    unsigned int  PH_M[Ed25519_WORD_LEN << 1];

    HASH_CTX      sha512_ctx[1];
    unsigned int  ret;
    unsigned char phflag, tmp;

    if (mode > Ed25519_PH) {
        return EdDSA_INVALID_INPUT;
    } else if (NULL == prikey || NULL == RS) {
        return EdDSA_POINTOR_NULL;
    } else {
        ;
    }

    //M could be empty, so M could be NUll, MByteLen could be 0, no need to check them
    if (NULL == M) {
        MByteLen = 0;
    } else {
        ;
    }

    if (Ed25519_CTX == mode) //in this case ctx can not be empty
    {
        if (NULL == ctx || 0 == ctxByteLen) {
            return EdDSA_INVALID_INPUT;
        } else {
            ;
        }
    } else if (Ed25519_PH == mode) //in this case ctx could be empty
    {
        if (NULL == ctx) {
            ctxByteLen = 0;
        } else {
            ;
        }
    } else //Ed25519_DEFAULT mode, ctx is useless
    {
        ;
    }

    /*************** get private scalar s and prefix ***************/
    ret = hash(HASH_SHA512, prikey, Ed25519_BYTE_LEN, (unsigned char *)h);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //decode to get the scalar s
    x25519_decode_scalar((unsigned char *)h, (unsigned char *)h, Ed25519_BYTE_LEN);

    /************************* set flag F **************************/
    if (Ed25519_CTX == mode) {
        phflag = 0;
    } else if (Ed25519_PH == mode) {
        phflag = 1;
    } else {
        ;
    }

    //PH_M
    if (Ed25519_PH == mode) {
        ret = hash(HASH_SHA512, M, MByteLen, (unsigned char *)PH_M);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    /******* get k = SHA-512(dom2(F, C) || prefix || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //dom2(phflag, ctx)
    if (Ed25519_DEFAULT != mode) {
        tmp = strlen(Ed25519_sign_string);
        ret = hash_update(sha512_ctx, (unsigned char *)Ed25519_sign_string, tmp);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    //prefix
    ret = hash_update(sha512_ctx, prefix, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //PH(M)
    if (Ed25519_PH == mode) {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    } else {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    /************************ get R = [r]B *************************/
    //r = k mod n
    ret = pke_mod(k + Ed25519_WORD_LEN - 1, Ed25519_WORD_LEN + 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, h + Ed25519_WORD_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    uint32_copy(k + Ed25519_WORD_LEN - 1, h + Ed25519_WORD_LEN, Ed25519_WORD_LEN);
    ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, r);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = ed25519_pointMul_s((edward_curve_t *)ed25519, r, ed25519->Gx, ed25519->Gy, k, k + Ed25519_WORD_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    memcpy_(RS, k + Ed25519_WORD_LEN, Ed25519_BYTE_LEN);
    if (k[0] & 1) {
        RS[Ed25519_BYTE_LEN - 1] |= 0x80;
    } else {
        ;
    }

    /******* get k = SHA-512(dom2(F, C) || R || A || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //dom2(phflag, ctx)
    if (Ed25519_DEFAULT != mode) {
        tmp = strlen(Ed25519_sign_string);
        ret = hash_update(sha512_ctx, (unsigned char *)Ed25519_sign_string, tmp);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    //R
    ret = hash_update(sha512_ctx, RS, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //pubkey(A)
    if (NULL == pubkey) {
        ret = ed25519_pointMul_s((edward_curve_t *)ed25519, s, ed25519->Gx, ed25519->Gy, k, k + Ed25519_WORD_LEN);
        if (PKE_SUCCESS != ret) {
            return ret;
        } else if (k[0] & 1) {
            k[(Ed25519_WORD_LEN << 1) - 1] |= 0x80000000;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)(k + Ed25519_WORD_LEN), Ed25519_BYTE_LEN);
    } else {
        ret = hash_update(sha512_ctx, pubkey, Ed25519_BYTE_LEN);
    }
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //PH(M)
    if (Ed25519_PH == mode) {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    } else {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    /***************** get S = (r + k * s) mod n *******************/
    //PH_M = k mod n
    ret = pke_mod(k + Ed25519_WORD_LEN - 1, Ed25519_WORD_LEN + 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, PH_M);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    uint32_copy(k + Ed25519_WORD_LEN - 1, PH_M, Ed25519_WORD_LEN);
    ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, PH_M);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //k = s mod n
    ret = pke_mod(s, Ed25519_WORD_LEN, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, k);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //k = k*s
    ret = pke_modmul(ed25519->n, PH_M, k, k, Ed25519_WORD_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //k = (r+k*s)mod n
    ret = pke_modadd(ed25519->n, k, r, k, Ed25519_WORD_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    memcpy_(RS + Ed25519_BYTE_LEN, k, Ed25519_BYTE_LEN);

    return EdDSA_SUCCESS;
}

/**
 * @brief       Ed25519 verify
 * @param[in]  mode                - Ed25519 signature mode
 * @param[in]  pubkey              - public key, 32 bytes, little-endian, if no pubkey, please set it to be NULL
 * @param[in]  ctx                 - 0-255 bytes
 * @param[in]  ctxByteLen          - byte length of ctx
 * @param[in]  M                   - message, M could be empty, in this case please set M to be NULL
 * @param[in]  MByteLen            - byte length of M, M could be empty, so it could be 0
 * @param[out] RS                  - signature
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. M could be empty(please set M to be NULL), so no need to check M and MByteLen
      -# 2. if mode is Ed25519_DEFAULT, ctx is not involved, no need to check ctx and ctxByteLen
      -# 3. if mode is Ed25519_CTX, ctx can not be empty(ctx length is from 1 to 255)
      -# 4. if mode is Ed25519_PH, ctx length is from 0 to 255, default length is 0, thus ctx could be empty
  @endverbatim
 */
unsigned int ed25519_verify(Ed25519_MODE mode, unsigned char pubkey[32], unsigned char *ctx, unsigned char ctxByteLen, unsigned char *M, unsigned int MByteLen, unsigned char RS[64])
{
    unsigned int k[Ed25519_WORD_LEN << 1];
    unsigned int S[Ed25519_WORD_LEN];
    unsigned int PH_M[Ed25519_WORD_LEN << 1];

    unsigned int  pub_x[Ed25519_WORD_LEN], *pub_y = S;
    unsigned int *x = PH_M, *y = PH_M + Ed25519_WORD_LEN;

    HASH_CTX      sha512_ctx[1];
    unsigned int  ret;
    unsigned char phflag, tmp;

    if (mode > Ed25519_PH) {
        return EdDSA_INVALID_INPUT;
    } else if (NULL == pubkey || NULL == RS) {
        return EdDSA_POINTOR_NULL;
    } else {
        ;
    }

    //M could be empty, so M could be NUll, MByteLen could be 0, no need to check them
    if (NULL == M) {
        MByteLen = 0;
    } else {
        ;
    }

    if (Ed25519_CTX == mode) //in this case ctx can not be empty
    {
        if (NULL == ctx || 0 == ctxByteLen) {
            return EdDSA_INVALID_INPUT;
        } else {
            ;
        }
    } else if (Ed25519_PH == mode) //in this case ctx could be empty
    {
        if (NULL == ctx) {
            ctxByteLen = 0;
        } else {
            ;
        }
    } else //Ed25519_DEFAULT mode, ctx is useless
    {
        ;
    }

    //get S (S should be less than order of the base point)
    memcpy_(S, RS + Ed25519_BYTE_LEN, Ed25519_BYTE_LEN);
    if (uint32_BigNumCmp(S, Ed25519_WORD_LEN, ed25519->n, Ed25519_WORD_LEN) >= 0) {
        return EdDSA_INVALID_INPUT;
    } else {
        ;
    }

    /************************* set flag F **************************/
    if (Ed25519_CTX == mode) {
        phflag = 0;
    } else if (Ed25519_PH == mode) {
        phflag = 1;
    } else {
        ;
    }

    //PH_M
    if (Ed25519_PH == mode) {
        ret = hash(HASH_SHA512, M, MByteLen, (unsigned char *)PH_M);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    /******* get k = SHA-512(dom2(F, C) || R || A || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //dom2(phflag, ctx)
    if (Ed25519_DEFAULT != mode) {
        tmp = strlen(Ed25519_sign_string);
        ret = hash_update(sha512_ctx, (unsigned char *)Ed25519_sign_string, tmp);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if (HASH_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }
    } else {
        ;
    }

    //R
    ret = hash_update(sha512_ctx, RS, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //pubkey(A)
    ret = hash_update(sha512_ctx, pubkey, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //PH(M)
    if (Ed25519_PH == mode) {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    } else {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //k = k mod n
    ret = pke_mod(k + Ed25519_WORD_LEN - 1, Ed25519_WORD_LEN + 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    uint32_copy(k + Ed25519_WORD_LEN - 1, x, Ed25519_WORD_LEN);
    ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        uint32_copy(k, x, Ed25519_WORD_LEN);
    }

    //get [S]B
    ret = ed25519_pointMul_s((edward_curve_t *)ed25519, S, ed25519->Gx, ed25519->Gy, x, y);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get [k]A'
    ret = ed25519_decode_point((unsigned char *)pubkey, (unsigned char *)pub_x, (unsigned char *)pub_y);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = ed25519_pointMul_s((edward_curve_t *)ed25519, k, pub_x, pub_y, pub_x, pub_y);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get R
    ret = ed25519_decode_point((unsigned char *)RS, (unsigned char *)k, (unsigned char *)(k + Ed25519_WORD_LEN));
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //R + [k]A'
    ret = ed25519_pointAdd((edward_curve_t *)ed25519, k, k + Ed25519_WORD_LEN, pub_x, pub_y, k, k + Ed25519_WORD_LEN);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //check whether [S]B = R + [k]A��
    if (uint32_BigNumCmp(k, Ed25519_WORD_LEN, x, Ed25519_WORD_LEN) ||
        uint32_BigNumCmp(k + Ed25519_WORD_LEN, Ed25519_WORD_LEN, y, Ed25519_WORD_LEN)) {
        return EdDSA_VERIFY_FAIL;
    } else {
        return EdDSA_SUCCESS;
    }
}

#endif
