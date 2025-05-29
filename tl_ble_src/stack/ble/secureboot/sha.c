/********************************************************************************************************
 * @file    sha.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "tl_common.h"
#include "drivers.h"
#include "secureboot_stack.h"

#define SHA256_U32_SPEEDUP

U32 const SHA224_H0[8] = {0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4};
U32 const SHA256_H0[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
U32 const SHA256_K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

void convert(U32 *a, U8 bytelen)
{
    U8  tmp;
    U8 *p = (U8 *)a;

    while (bytelen > 0) {
        tmp      = *p;
        *p       = *(p + 3);
        *(p + 3) = tmp;
        p += 1;
        tmp      = *p;
        *p       = *(p + 1);
        *(p + 1) = tmp;
        bytelen -= 4;
        p += 3;
    }
}

#ifdef SHA256_U32_SPEEDUP
void SHA256_U32Copy(U32 *dst, U32 *src, U32 wordLen)
{
    U32 i;

    for (i = 0; i < wordLen; i++) {
        dst[i] = src[i];
    }
}
#endif


#ifdef SHA256_U32_SPEEDUP
    #define SHA256_ROTR(x, n) (((*(x)) >> (n)) | ((*(x)) << (32 - (n))))
#else
U32 SHA256_ROTR(U32 x[1], U8 n)
{
    return ((*x) >> n) | ((*x) << (32 - n));
}
#endif


#ifdef SHA256_U32_SPEEDUP
    #define SHA256_SHR(x, n) ((*(x)) >> (n))
#else
U32 SHA256_SHR(U32 x[1], U8 n)
{
    return (*x) >> n;
}
#endif


void SHA256_byteLen_add(U32 a[2], U32 byteLen)
{
    a[1] += byteLen;
    if (a[1] < byteLen) {
        a[0] += 1;
    }
}

U8 SHA256_block_byteLen(SHA256_Ctx *ctx)
{
    return ctx->count[1] & 0x3f;
}

void SHA256_block(SHA256_Ctx *ctx, U8 byteLen)
{
    S8  i, j;
    U32 SHA256_abcdefgh[8], *W, *T;

    //initialize abcdefgh
#ifdef SHA256_U32_SPEEDUP
    SHA256_U32Copy(SHA256_abcdefgh, ctx->hash, 8);
#else
    memcpy(SHA256_abcdefgh, ctx->hash, 32);
#endif

    //convert data
    convert(ctx->wbuf, byteLen);

    for (i = 0; i < 64; i++) {
        //compute W                  (i%16) == (i&15)
        W = ctx->wbuf + (i & 15);
        if (i > 15) {
            //W += sigma1(Wt_2)
            T = ctx->wbuf + ((i - 2) & 15);
            (*W) += (SHA256_ROTR(T, 17) ^ SHA256_ROTR(T, 19) ^ SHA256_SHR(T, 10));

            //W += sigma0(Wt_15)
            T = ctx->wbuf + ((i - 15) & 15);
            (*W) += (SHA256_ROTR(T, 7) ^ SHA256_ROTR(T, 18) ^ SHA256_SHR(T, 3));

            //W += (Wt_7)
            (*W) += ctx->wbuf[(i - 7) & 15];
        }

        j = 7 - (i & 7); //(i%8) == (i&7)

        //compute T1
        SHA256_abcdefgh[j] += ((SHA256_abcdefgh[(j + 5) & 7]) & (SHA256_abcdefgh[(j + 6) & 7])) ^ ((~(SHA256_abcdefgh[(j + 5) & 7])) & (SHA256_abcdefgh[(j + 7) & 7]));
        SHA256_abcdefgh[j] += ((*W) + SHA256_K[i]);
        SHA256_abcdefgh[j] += (SHA256_ROTR(SHA256_abcdefgh + ((j + 5) & 7), 6) ^ SHA256_ROTR(SHA256_abcdefgh + ((j + 5) & 7), 11) ^ SHA256_ROTR(SHA256_abcdefgh + ((j + 5) & 7), 25));

        // e=d+T1
        SHA256_abcdefgh[(j + 4) & 7] += SHA256_abcdefgh[j];

        // a=T1+T2
        SHA256_abcdefgh[j] += ((SHA256_abcdefgh[(j + 1) & 7]) & (SHA256_abcdefgh[(j + 2) & 7])) ^ ((SHA256_abcdefgh[(j + 1) & 7]) & (SHA256_abcdefgh[(j + 3) & 7])) ^ ((SHA256_abcdefgh[(j + 2) & 7]) & (SHA256_abcdefgh[(j + 3) & 7]));
        SHA256_abcdefgh[j] += (SHA256_ROTR(SHA256_abcdefgh + ((j + 1) & 7), 2) ^ SHA256_ROTR(SHA256_abcdefgh + ((j + 1) & 7), 13) ^ SHA256_ROTR(SHA256_abcdefgh + ((j + 1) & 7), 22));
    }

    //get ctx->hash for now
    for (i = 0; i < 8; i++) {
        ctx->hash[i] += SHA256_abcdefgh[i];
    }
}

void SHA256_Init(SHA256_Ctx *ctx, U8 digest[32])
{
    ctx->hash = (U32 *)digest;

#ifdef SHA256_U32_SPEEDUP
    ctx->count[0] = ctx->count[1] = 0;
    SHA256_U32Copy(ctx->hash, (U32 *)(u32)SHA256_H0, 8);
#else
    memset(ctx->count, 0, 8);
    memcpy(digest, SHA256_H0, 32);
#endif
}

void SHA256_Process(SHA256_Ctx *ctx, U8 *message, U32 byteLen)
{
#ifdef SHA256_U32_SPEEDUP
    U8  leftlen, filllen;
    U32 i, Cycle;

    leftlen = SHA256_block_byteLen(ctx);
    SHA256_byteLen_add(ctx->count, byteLen);

    if (leftlen + byteLen < 64) {
        memcpy(((U8 *)ctx->wbuf) + leftlen, message, byteLen);
        return;
    }

    if (leftlen) {
        filllen = 64 - leftlen;
        memcpy(((U8 *)ctx->wbuf) + leftlen, message, filllen);
        SHA256_block(ctx, 64);
        byteLen -= filllen;
        message += filllen;
    }

    Cycle = byteLen >> 6;
    for (i = 0; i < Cycle; i++) {
        memcpy(((U8 *)ctx->wbuf), message, 64);
        SHA256_block(ctx, 64);
        message += 64;
    }

    leftlen = SHA256_block_byteLen(ctx);
    memcpy(((U8 *)ctx->wbuf), message, leftlen);
#else
    U8 filllen, leftlen, rightlen;

    while (byteLen) {
        leftlen  = SHA256_block_byteLen(ctx);
        rightlen = 64 - leftlen;
        filllen  = byteLen < rightlen ? byteLen : rightlen;
        memcpy(((U8 *)ctx->wbuf) + leftlen, message, filllen);
        SHA256_byteLen_add(ctx->count, filllen);
        message += filllen;
        byteLen -= filllen;
        if (!SHA256_block_byteLen(ctx)) {
            SHA256_block(ctx, 64);
        }
    }
#endif
}

void SHA256_Done(SHA256_Ctx *ctx)
{
    U8 byteLen;

    byteLen                        = SHA256_block_byteLen(ctx);
    *((U8 *)(ctx->wbuf) + byteLen) = 0x80;

    memset((U8 *)(ctx->wbuf) + byteLen + 1, 0, byteLen < 56 ? 55 - byteLen : 63 - byteLen);
    if (byteLen > 55) {
        SHA256_block(ctx, 64);
        memset((U8 *)ctx->wbuf, 0, 56);
    }

    ctx->wbuf[14] = ctx->count[0] << 3;
    ctx->wbuf[14] |= (ctx->count[1] >> 29);
    ctx->wbuf[15] = ctx->count[1] << 3;

    SHA256_block(ctx, 56);

    //convert result
    convert(ctx->hash, 32);
}

void SHA256_Hash(U8 *message, U32 byteLen, U8 digest[32])
{
    SHA256_Ctx ctx[1];

    SHA256_Init(ctx, digest);
    SHA256_Process(ctx, message, byteLen);
    SHA256_Done(ctx);
}

void SHA224_Init(SHA224_Ctx *ctx, U8 digest[32])
{
    ctx->hash = (U32 *)digest;

#ifdef SHA256_U32_SPEEDUP
    ctx->count[0] = ctx->count[1] = 0;
    SHA256_U32Copy(ctx->hash, (U32 *)(u32)SHA224_H0, 8);
#else
    memset(ctx->count, 0, 8);
    memcpy(digest, SHA224_H0, 32);
#endif
}

void SHA224_Process(SHA224_Ctx *ctx, U8 *message, U32 byteLen)
{
    SHA256_Process(ctx, message, byteLen);
}

void SHA224_Done(SHA224_Ctx *ctx)
{
    SHA256_Done(ctx);
    memset(ctx->hash + 7, 0, 4);
}

void SHA224_Hash(U8 *message, U32 byteLen, U8 digest[32])
{
    SHA224_Ctx ctx[1];

    SHA224_Init(ctx, digest);
    SHA256_Process(ctx, message, byteLen);
    SHA224_Done(ctx);
}

void HMAC_SHA256_Init(HMAC_SHA256_Ctx *ctx, U8 *key, U32 keyByteLen, U8 *mac)
{
    U32 i;

    //get K0
    if (keyByteLen <= 64) {
        memcpy(ctx->K0, key, keyByteLen);
        memset(((U8 *)(ctx->K0)) + keyByteLen, 0, 64 - keyByteLen);
    } else {
        SHA256_Hash((U8 *)key, keyByteLen, (U8 *)(ctx->K0));
        memset(((U8 *)(ctx->K0)) + 32, 0, 64 - 32);
    }

    //get K0 ^ ipad
    for (i = 0; i < 16; i++) {
        ctx->K0[i] ^= 0x36363636;
    }

    SHA256_Init(ctx->sha256_ctx, mac);

    SHA256_Process(ctx->sha256_ctx, (U8 *)(ctx->K0), 64);
}

void HMAC_SHA256_Process(HMAC_SHA256_Ctx *ctx, const U8 *input, U32 byteLen)
{
    SHA256_Process(ctx->sha256_ctx, (U8 *)(u32)input, byteLen);
}

void HMAC_SHA256_Done(HMAC_SHA256_Ctx *ctx)
{
    U32  i;
    U32 *mac = ctx->sha256_ctx->hash;
    U8   digest[32];

    //set mac as hash((K0^ipad)||message)
    SHA256_Done(ctx->sha256_ctx);

    //get K0 ^ opad
    for (i = 0; i < 16; i++) {
        ctx->K0[i] ^= (0x36363636 ^ 0x5c5c5c5c);
    }

    SHA256_Init(ctx->sha256_ctx, digest);
    SHA256_Process(ctx->sha256_ctx, (U8 *)(ctx->K0), 64);
    SHA256_Process(ctx->sha256_ctx, (U8 *)mac, 32);
    SHA256_Done(ctx->sha256_ctx);

    memcpy(mac, digest, 32);
}

void HMAC_SHA256(U8 *key, U32 keyByteLen, U8 *msg, U32 msgByteLen, U8 *mac)
{
    HMAC_SHA256_Ctx ctx[1];

    HMAC_SHA256_Init(ctx, key, keyByteLen, mac);

    HMAC_SHA256_Process(ctx, msg, msgByteLen);

    HMAC_SHA256_Done(ctx);
}

void HMAC_SHA224_Init(HMAC_SHA224_Ctx *ctx, U8 *key, U32 keyByteLen, U8 *mac)
{
    U32 i;

    //get K0
    if (keyByteLen <= 64) {
        memcpy(ctx->K0, key, keyByteLen);
        memset(((U8 *)(ctx->K0)) + keyByteLen, 0, 64 - keyByteLen);
    } else {
        SHA224_Hash((U8 *)key, keyByteLen, (U8 *)(ctx->K0));
        memset(((U8 *)(ctx->K0)) + 28, 0, 64 - 28);
    }

    //get K0 ^ ipad
    for (i = 0; i < 16; i++) {
        ctx->K0[i] ^= 0x36363636;
    }

    SHA224_Init(ctx->sha256_ctx, mac);

    SHA224_Process(ctx->sha256_ctx, (U8 *)(ctx->K0), 64);
}

void HMAC_SHA224_Process(HMAC_SHA224_Ctx *ctx, const U8 *input, U32 byteLen)
{
    SHA224_Process(ctx->sha256_ctx, (U8 *)(u32)input, byteLen);
}

void HMAC_SHA224_Done(HMAC_SHA224_Ctx *ctx)
{
    U32  i;
    U32 *mac = ctx->sha256_ctx->hash;
    U8   digest[32];

    //set mac as hash((K0^ipad)||message)
    SHA224_Done(ctx->sha256_ctx);

    //get K0 ^ opad
    for (i = 0; i < 16; i++) {
        ctx->K0[i] ^= (0x36363636 ^ 0x5c5c5c5c);
    }

    SHA224_Init(ctx->sha256_ctx, digest);
    SHA224_Process(ctx->sha256_ctx, (U8 *)(ctx->K0), 64);
    SHA224_Process(ctx->sha256_ctx, (U8 *)mac, 28);
    SHA224_Done(ctx->sha256_ctx);

    memcpy(mac, digest, 28);
}

void HMAC_SHA224(U8 *key, U32 keyByteLen, U8 *msg, U32 msgByteLen, U8 *mac)
{
    HMAC_SHA224_Ctx ctx[1];

    HMAC_SHA224_Init(ctx, key, keyByteLen, mac);

    HMAC_SHA224_Process(ctx, msg, msgByteLen);

    HMAC_SHA224_Done(ctx);
}
