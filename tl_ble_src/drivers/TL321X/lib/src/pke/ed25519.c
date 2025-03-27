/********************************************************************************************************
 * @file    ed25519.c
 *
 * @brief   This is the source file for TL321X
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


#ifdef SUPPORT_C25519

#include "lib/include/pke/ed25519.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"
#include "lib/include/hash/hash.h"



//"SigEd25519 no Ed25519 collisions"
unsigned char Ed25519_sign_string[] = {
    0x53,0x69,0x67,0x45,0x64,0x32,0x35,0x35,0x31,0x39,0x20,0x6e,0x6f,0x20,0x45,0x64,
    0x32,0x35,0x35,0x31,0x39,0x20,0x63,0x6f,0x6c,0x6c,0x69,0x73,0x69,0x6f,0x6e,0x73};


extern unsigned int curve25519_p[8];
extern unsigned int curve25519_p_h[8];
extern unsigned int curve25519_p_n0[1];
extern unsigned int curve25519_n[8];
extern unsigned int curve25519_n_h[8];
extern unsigned int curve25519_n_n0[1];


//unsigned int ed25519_p       = {0xFFFFFFEDu,0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu,0xFFFFFFFFu,0x7FFFFFFFu,};
//unsigned int ed25519_p_h     = {0x000005A4u,0u,0u,0u,0u,0u,0u,0u,};
//unsigned int ed25519_p_n0    = {0x286BCA1Bu};
unsigned int ed25519_d[]       = {0x135978A3u,0x75EB4DCAu,0x4141D8ABu,0x00700A4Du,0x7779E898u,0x8CC74079u,0x2B6FFE73u,0x52036CEEu,};
unsigned int ed25519_Gx[]      = {0x8F25D51Au,0xC9562D60u,0x9525A7B2u,0x692CC760u,0xFDD6DC5Cu,0xC0A4E231u,0xCD6E53FEu,0x216936D3u,};
unsigned int ed25519_Gy[]      = {0x66666658u,0x66666666u,0x66666666u,0x66666666u,0x66666666u,0x66666666u,0x66666666u,0x66666666u,};
//unsigned int ed25519_n[]     = {0x5CF5D3EDu,0x5812631Au,0xA2F79CD6u,0x14DEF9DEu,0x00000000u,0x00000000u,0x00000000u,0x10000000u,};
//unsigned int ed25519_n_h[]   = {0x449C0F01u,0xA40611E3u,0x68859347u,0xD00E1BA7u,0x17F5BE65u,0xCEEC73D2u,0x7C309A3Du,0x0399411Bu,};
//unsigned int ed25519_n_n0[1] = {0x12547E1Bu};

edward_curve_t ed25519[1] = {
    {
        255,
        253,
        curve25519_p,
        curve25519_p_h,
#if defined(PKE_LP)
        curve25519_p_n0,
#endif
        ed25519_d,
        ed25519_Gx,
        ed25519_Gy,
        curve25519_n,
        curve25519_n_h,
#if defined(PKE_LP)
        curve25519_n_n0,
#endif
        NULL,
    },
};


extern unsigned int ed25519_decode_point(unsigned char in_y[32], unsigned char out_x[32], unsigned char out_y[32]);

extern void x25519_decode_scalar(unsigned char *k, unsigned char *out, unsigned int bytes);



/**
 * @brief       edwards25519 curve point mul(random point), Q=[k]P, secure version
 * @param[in]   curve            - edwards25519 curve struct pointer
 * @param[in]   k                - scalar, it could be 0 here
 * @param[in]   Px               - x coordinate of point P
 * @param[in]   Py               - y coordinate of point P
 * @param[out]  Qx               - x coordinate of point Q
 * @param[out]  Qy               - y coordinate of point Q
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P is on the curve
      -# 2.even if the input point P is valid, the output may be neutral point (0, 1), it is valid
      -# 3.please make sure the curve is edwards25519
      -# 4.k could be zero here.
  @endverbatim
 */
unsigned int ed25519_pointMul_s(edward_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py,
        unsigned int *Qx, unsigned int *Qy)
{
    unsigned int pWordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->n_bitLen);

    if(0u != uint32_BigNum_Check_Zero(k, nWordLen))
    {
        uint32_clear(Qx, pWordLen);
        pke_set_operand_uint32_value(Qy, pWordLen, 1);

        return PKE_SUCCESS;
    }
    else
    {
        return ed25519_pointMul(curve, k, Px, Py, Qx, Qy);
    }
}


/**
 * @brief       get Ed25519 public key from private key
 * @param[in]   prikey            - private key, 32 bytes, little-endian
 * @param[out]  pubkey            - public key, 32 bytes, little-endian
 * @return      0:success     other:error
*/
unsigned int ed25519_get_pubkey_from_prikey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int h[16];
    unsigned int ret;

    if((NULL == prikey) || (NULL == pubkey))
    {
        return EdDSA_POINTOR_NULL;
    }
    else
    {;}

    ret = hash(HASH_SHA512, prikey, 32, (unsigned char *)h);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //decode to get the scalar
    x25519_decode_scalar((unsigned char *)h, (unsigned char *)h, Ed25519_BYTE_LEN);

    ret = ed25519_pointMul_s(ed25519, h, ed25519->Gx, ed25519->Gy, h, h+8);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //encode pubkey
    memcpy_(pubkey, (unsigned char *)(h+8), Ed25519_BYTE_LEN);
    if(0u != (h[0]&1u))
    {
        pubkey[Ed25519_BYTE_LEN-1u] |= (unsigned char)0x80;
    }
    else
    {;}

    return EdDSA_SUCCESS;
}


/**
 * @brief       generate Ed25519 random key pair
 * @param[out]  prikey            - private key, 32 bytes, little-endian
 * @param[out]  pubkey            - public key, 32 bytes, little-endian
 * @return      0:success     other:error
 * @note
*/
unsigned int ed25519_getkey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int ret;

    if((NULL == prikey) || (NULL == pubkey))
    {
        return EdDSA_POINTOR_NULL;
    }
    else
    {;}

    ret = get_rand(prikey, Ed25519_BYTE_LEN);
    if(TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        return ed25519_get_pubkey_from_prikey(prikey, pubkey);
    }
}


/**
 * @brief       Ed25519 sign
 * @param[in]   mode               - Ed25519 signature mode
 * @param[in]   prikey             - private key, 32 bytes, little-endian
 * @param[in]   pubkey             - public key, 32 bytes, little-endian, if no pubkey, please set it to be NULL
 * @param[in]   ctx                - 0-255 bytes
 * @param[in]   ctxByteLen         - byte length of ctx
 * @param[in]   M                  - message, requirements are determined by mode
 * @param[in]   MByteLen           - byte length of M, M could be empty, so it could be 0
 * @param[out]  RS                 - signature
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if no public key, please set pubkey to be NULL, it will be generated inside
      -# 2.if mode is not Ed25519_PH_WITH_PH_M, M could be empty(please set M to be NULL), 
           so no need to check M and MByteLen, otherwise, M is sha512 value, occupies 64 bytes, 
           and MByteLen is not involved
      -# 3.if mode is Ed25519_DEFAULT, ctx is not involved, no need to check ctx and ctxByteLen
      -# 4.if mode is Ed25519_CTX, ctx can not be empty(ctx length is from 1 to 255)
      -# 5.if mode is Ed25519_PH or Ed25519_PH_WITH_PH_M, ctx length is from 0 to 255, default 
           length is 0, thus ctx could be empty
  @endverbatim
*/
unsigned int ed25519_sign(Ed25519_MODE mode, unsigned char prikey[32], unsigned char pubkey[32], unsigned char *ctx, unsigned char ctxByteLen,
        unsigned char *M, unsigned int MByteLen, unsigned char RS[64])
{
    unsigned int h[16];
    unsigned int *s = h;
    unsigned char *prefix = (unsigned char *)(h + Ed25519_WORD_LEN);

    unsigned int *r = h+Ed25519_WORD_LEN;
    unsigned int k[Ed25519_WORD_LEN<<1];
    unsigned int PH_M[Ed25519_WORD_LEN<<1];

    HASH_CTX sha512_ctx[1];
    unsigned int ret;
    unsigned char phflag;

    if(mode > Ed25519_PH_WITH_PH_M)
    {
        return EdDSA_INVALID_INPUT;
    }
    else if((NULL == prikey) || (NULL == RS))
    {
        return EdDSA_POINTOR_NULL;
    }
    else
    {;}

    //if mode is not Ed25519_PH_WITH_PH_M, M could be empty, 
    //so M could be NUll, MByteLen could be 0, no need to check them
    if(Ed25519_PH_WITH_PH_M != mode)
    {
        if(NULL == M)
        {
            MByteLen = 0;
        }
        else
        {;}
    }
    else
    {
        if(NULL == M)
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {
            MByteLen = 64u;
        }
    }

    if(Ed25519_CTX == mode)             //in this case ctx can not be empty
    {
        if((NULL == ctx) || (((unsigned char)0) == ctxByteLen))
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {;}
    }
    else if((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))         //in this case ctx could be empty
    {
        if(NULL == ctx)
        {
            ctxByteLen = 0;
        }
        else
        {;}
    }
    else                                //Ed25519_DEFAULT mode, ctx is useless
    {;}

    /*************** get private scalar s and prefix ***************/
    ret = hash(HASH_SHA512, prikey, Ed25519_BYTE_LEN, (unsigned char *)h);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //decode to get the scalar s
    x25519_decode_scalar((unsigned char *)h, (unsigned char *)h, Ed25519_BYTE_LEN);

    /************************* set flag F **************************/
    if(Ed25519_CTX == mode)
    {
        phflag = 0;
    }
    else if((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))
    {
        phflag = 1;
    }
    else
    {;}

    //PH_M
    if(Ed25519_PH == mode)
    {
        ret = hash(HASH_SHA512, M, MByteLen, (unsigned char *)PH_M);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    /******* get k = SHA-512(dom2(F, C) || prefix || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //dom2(phflag, ctx)
    if(Ed25519_DEFAULT != mode)
    {
        ret = hash_update(sha512_ctx, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    //prefix
    ret = hash_update(sha512_ctx, prefix, Ed25519_BYTE_LEN);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //PH(M)
    if(Ed25519_PH == mode)
    {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    }
    else
    {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    /************************ get R = [r]B *************************/
    //r = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h, ed25519->n_n0,
            Ed25519_WORD_LEN, h+Ed25519_WORD_LEN);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h,
            Ed25519_WORD_LEN, h+Ed25519_WORD_LEN);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    uint32_copy(&(k[Ed25519_WORD_LEN-1u]), h+Ed25519_WORD_LEN, Ed25519_WORD_LEN);
#if defined(PKE_LP)
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, r);
#else
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, r);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = ed25519_pointMul_s(ed25519, r, ed25519->Gx, ed25519->Gy, k, k+Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    memcpy_(RS, (unsigned char *)(k+Ed25519_WORD_LEN), Ed25519_BYTE_LEN);
    if(0u != (k[0] & 1u))
    {
        RS[Ed25519_BYTE_LEN-1u] |= (unsigned char)0x80;
    }
    else
    {;}

    /******* get k = SHA-512(dom2(F, C) || R || A || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //dom2(phflag, ctx)
    if(Ed25519_DEFAULT != mode)
    {
        ret = hash_update(sha512_ctx, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    //R
    ret = hash_update(sha512_ctx, RS, Ed25519_BYTE_LEN);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //pubkey(A)
    if(NULL == pubkey)
    {
        ret = ed25519_pointMul_s(ed25519, s, ed25519->Gx, ed25519->Gy, k, k+Ed25519_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else if(0u != (k[0] & 1u))
        {
            k[(Ed25519_WORD_LEN<<1)-1u] |= 0x80000000u;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)(k+Ed25519_WORD_LEN), Ed25519_BYTE_LEN);
    }
    else
    {
        ret = hash_update(sha512_ctx, pubkey, Ed25519_BYTE_LEN);
    }
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //PH(M)
    if(Ed25519_PH == mode)
    {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    }
    else
    {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    /***************** get S = (r + k * s) mod n *******************/
    //PH_M = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, PH_M);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, PH_M);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    uint32_copy(k+Ed25519_WORD_LEN-1, PH_M, Ed25519_WORD_LEN);
#if defined(PKE_LP)
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, PH_M);
#else
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, PH_M);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //k = s mod n
#if defined(PKE_LP)
    ret = pke_mod(s, Ed25519_WORD_LEN, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, k);
#else
    ret = pke_mod(s, Ed25519_WORD_LEN, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, k);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //k = k*s
    ret = pke_modmul(ed25519->n, PH_M, k, k, Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //k = (r+k*s)mod n
    ret = pke_modadd(ed25519->n, k, r, k, Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    memcpy_(RS+Ed25519_BYTE_LEN, (unsigned char *)k, Ed25519_BYTE_LEN);

    return EdDSA_SUCCESS;
}


/**
 * @brief       Ed25519 verify
 * @param[in]   mode               - Ed25519 signature mode
 * @param[in]   pubkey             - public key, 32 bytes, little-endian
 * @param[in]   ctx                - 0-255 bytes
 * @param[in]   ctxByteLen         - byte length of ctx
 * @param[in]   M                  - message, requirements are determined by mode
 * @param[in]   MByteLen           - byte length of M, M could be empty, so it could be 0
 * @param[in]   RS                 - signature
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.if mode is not Ed25519_PH_WITH_PH_M, M could be empty(please set M to be NULL), 
           so no need to check M and MByteLen, otherwise, M is sha512 value, occupies 64 bytes, 
           and MByteLen is not involved
      -# 2.if mode is Ed25519_DEFAULT, ctx is not involved, no need to check ctx and ctxByteLen
      -# 3.if mode is Ed25519_CTX, ctx can not be empty(ctx length is from 1 to 255)
      -# 4.if mode is Ed25519_PH or Ed25519_PH_WITH_PH_M, ctx length is from 0 to 255, default 
           length is 0, thus ctx could be empty
      -# 5.if mode is Ed25519_PH_WITH_PH_M, the M is sha512 value, big-endian, occupies 64 bytes, 
           and MByteLen is not involved
  @endverbatim
*/
unsigned int ed25519_verify(Ed25519_MODE mode, unsigned char pubkey[32], unsigned char *ctx, unsigned char ctxByteLen, unsigned char *M,
        unsigned int MByteLen, unsigned char RS[64])
{
    unsigned int k[Ed25519_WORD_LEN<<1];
    unsigned int S[Ed25519_WORD_LEN];
    unsigned int PH_M[Ed25519_WORD_LEN<<1];

    unsigned int pub_x[Ed25519_WORD_LEN], *pub_y=S;
    unsigned int *x=PH_M, *y=PH_M+Ed25519_WORD_LEN;

    HASH_CTX sha512_ctx[1];
    unsigned int ret;
    unsigned char phflag;

    if(mode > Ed25519_PH_WITH_PH_M)
    {
        return EdDSA_INVALID_INPUT;
    }
    else if((NULL == pubkey) || (NULL == RS))
    {
        return EdDSA_POINTOR_NULL;
    }
    else
    {;}

    //if mode is not Ed25519_PH_WITH_PH_M, M could be empty, 
    //so M could be NUll, MByteLen could be 0, no need to check them
    if(Ed25519_PH_WITH_PH_M != mode)
    {
        if(NULL == M)
        {
            MByteLen = 0u;
        }
        else
        {;}
    }
    else
    {
        if(NULL == M)
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {
            MByteLen = 64u;
        }
    }

    if(Ed25519_CTX == mode)             //in this case ctx can not be empty
    {
        if((NULL == ctx) || (((unsigned char)0) == ctxByteLen))
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {;}
    }
    else if((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))         //in this case ctx could be empty
    {
        if(NULL == ctx)
        {
            ctxByteLen = 0;
        }
        else
        {;}
    }
    else                                //Ed25519_DEFAULT mode, ctx is useless
    {;}

    //get S (S should be less than order of the base point)
    memcpy_((unsigned char *)S, RS+Ed25519_BYTE_LEN, Ed25519_BYTE_LEN);
    if(uint32_BigNumCmp(S, Ed25519_WORD_LEN, ed25519->n, Ed25519_WORD_LEN) >= 0)
    {
        return EdDSA_INVALID_INPUT;
    }
    else
    {;}

    /************************* set flag F **************************/
    if(Ed25519_CTX == mode)
    {
        phflag = 0;
    }
    else if((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))
    {
        phflag = 1;
    }
    else
    {;}

    //PH_M
    if(Ed25519_PH == mode)
    {
        ret = hash(HASH_SHA512, M, MByteLen, (unsigned char *)PH_M);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    /******* get k = SHA-512(dom2(F, C) || R || A || PH(M)) ********/
    ret = hash_init(sha512_ctx, HASH_SHA512);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //dom2(phflag, ctx)
    if(Ed25519_DEFAULT != mode)
    {
        ret = hash_update(sha512_ctx, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&phflag, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, (unsigned char *)&ctxByteLen, 1);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_update(sha512_ctx, ctx, ctxByteLen);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {;}

    //R
    ret = hash_update(sha512_ctx, RS, Ed25519_BYTE_LEN);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //pubkey(A)
    ret = hash_update(sha512_ctx, pubkey, Ed25519_BYTE_LEN);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //PH(M)
    if(Ed25519_PH == mode)
    {
        ret = hash_update(sha512_ctx, (unsigned char *)PH_M, 64);
    }
    else
    {
        ret = hash_update(sha512_ctx, M, MByteLen);
    }
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = hash_final(sha512_ctx, (unsigned char *)k);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //k = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN-1u]), Ed25519_WORD_LEN+1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, x);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    uint32_copy(&(k[Ed25519_WORD_LEN-1u]), x, Ed25519_WORD_LEN);
#if defined(PKE_LP)
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
#else
    ret = pke_mod(k, (Ed25519_WORD_LEN<<1)-1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, x);
#endif
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        uint32_copy(k, x, Ed25519_WORD_LEN);
    }

    //get [S]B
    ret = ed25519_pointMul_s(ed25519, S, ed25519->Gx, ed25519->Gy, x, y);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get [k]A'
    ret = ed25519_decode_point((unsigned char *)pubkey, (unsigned char *)pub_x,(unsigned char *)pub_y);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = ed25519_pointMul_s(ed25519, k, pub_x, pub_y, pub_x, pub_y);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //get R
    ret = ed25519_decode_point((unsigned char *)RS, (unsigned char *)k,(unsigned char *)(k+Ed25519_WORD_LEN));
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //R + [k]A'
    ret = ed25519_pointAdd(ed25519, k, k+Ed25519_WORD_LEN, pub_x, pub_y, k, k+Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //check whether [S]B = R + [k]A
    if(((int32_t)0 != uint32_BigNumCmp(k, Ed25519_WORD_LEN, x, Ed25519_WORD_LEN)) ||
       ((int32_t)0 != uint32_BigNumCmp(k+Ed25519_WORD_LEN, Ed25519_WORD_LEN, y, Ed25519_WORD_LEN)))
    {
        return EdDSA_VERIFY_FAIL;
    }
    else
    {
        return EdDSA_SUCCESS;
    }
}

#endif

