/********************************************************************************************************
 * @file    soft_sha384_sha512.c
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

#include <lib/include/hash/soft_sha384_sha512.h>
#include <stdio.h>
#include "lib/include/crypto_common/utility.h"


//this simple implementation is for little-endian CPU platform default.

#ifdef SUPPORT_STATIC_ANALYSIS
extern unsigned int SHA384_H0[16];
extern unsigned int SHA512_H0[16];
extern unsigned int SHA512_224_H0[16];
extern unsigned int SHA512_256_H0[16];
extern unsigned int SHA512_K[160];
#endif

#define SHA512_U32_SPEEDUP

#if 0
extern void print_buf(char *buf,int len, char *name);
#endif

unsigned int SHA384_H0[16] = {
    0xcbbb9d5dU,
    0xc1059ed8U,
    0x629a292aU,
    0x367cd507U,
    0x9159015aU,
    0x3070dd17U,
    0x152fecd8U,
    0xf70e5939U,
    0x67332667U,
    0xffc00b31U,
    0x8eb44a87U,
    0x68581511U,
    0xdb0c2e0dU,
    0x64f98fa7U,
    0x47b5481dU,
    0xbefa4fa4U,
};
unsigned int SHA512_H0[16] = {
    0x6a09e667U,
    0xf3bcc908U,
    0xbb67ae85U,
    0x84caa73bU,
    0x3c6ef372U,
    0xfe94f82bU,
    0xa54ff53aU,
    0x5f1d36f1U,
    0x510e527fU,
    0xade682d1U,
    0x9b05688cU,
    0x2b3e6c1fU,
    0x1f83d9abU,
    0xfb41bd6bU,
    0x5be0cd19U,
    0x137e2179U,
};
unsigned int SHA512_224_H0[16] = {
    0x8C3D37C8U,
    0x19544DA2U,
    0x73E19966U,
    0x89DCD4D6U,
    0x1DFAB7AEU,
    0x32FF9C82U,
    0x679DD514U,
    0x582F9FCFU,
    0x0F6D2B69U,
    0x7BD44DA8U,
    0x77E36F73U,
    0x04C48942U,
    0x3F9D85A8U,
    0x6A1D36C8U,
    0x1112E6ADU,
    0x91D692A1U,
};
unsigned int SHA512_256_H0[16] = {
    0x22312194U,
    0xFC2BF72CU,
    0x9F555FA3U,
    0xC84C64C2U,
    0x2393B86BU,
    0x6F53B151U,
    0x96387719U,
    0x5940EABDU,
    0x96283EE2U,
    0xA88EFFE3U,
    0xBE5E1E25U,
    0x53863992U,
    0x2B0199FCU,
    0x2C85B8AAU,
    0x0EB72DDCU,
    0x81C52CA2U,
};
unsigned int SHA512_K[160] = {
    0x428a2f98U,
    0xd728ae22U,
    0x71374491U,
    0x23ef65cdU,
    0xb5c0fbcfU,
    0xec4d3b2fU,
    0xe9b5dba5U,
    0x8189dbbcU,
    0x3956c25bU,
    0xf348b538U,
    0x59f111f1U,
    0xb605d019U,
    0x923f82a4U,
    0xaf194f9bU,
    0xab1c5ed5U,
    0xda6d8118U,
    0xd807aa98U,
    0xa3030242U,
    0x12835b01U,
    0x45706fbeU,
    0x243185beU,
    0x4ee4b28cU,
    0x550c7dc3U,
    0xd5ffb4e2U,
    0x72be5d74U,
    0xf27b896fU,
    0x80deb1feU,
    0x3b1696b1U,
    0x9bdc06a7U,
    0x25c71235U,
    0xc19bf174U,
    0xcf692694U,
    0xe49b69c1U,
    0x9ef14ad2U,
    0xefbe4786U,
    0x384f25e3U,
    0x0fc19dc6U,
    0x8b8cd5b5U,
    0x240ca1ccU,
    0x77ac9c65U,
    0x2de92c6fU,
    0x592b0275U,
    0x4a7484aaU,
    0x6ea6e483U,
    0x5cb0a9dcU,
    0xbd41fbd4U,
    0x76f988daU,
    0x831153b5U,
    0x983e5152U,
    0xee66dfabU,
    0xa831c66dU,
    0x2db43210U,
    0xb00327c8U,
    0x98fb213fU,
    0xbf597fc7U,
    0xbeef0ee4U,
    0xc6e00bf3U,
    0x3da88fc2U,
    0xd5a79147U,
    0x930aa725U,
    0x06ca6351U,
    0xe003826fU,
    0x14292967U,
    0x0a0e6e70U,
    0x27b70a85U,
    0x46d22ffcU,
    0x2e1b2138U,
    0x5c26c926U,
    0x4d2c6dfcU,
    0x5ac42aedU,
    0x53380d13U,
    0x9d95b3dfU,
    0x650a7354U,
    0x8baf63deU,
    0x766a0abbU,
    0x3c77b2a8U,
    0x81c2c92eU,
    0x47edaee6U,
    0x92722c85U,
    0x1482353bU,
    0xa2bfe8a1U,
    0x4cf10364U,
    0xa81a664bU,
    0xbc423001U,
    0xc24b8b70U,
    0xd0f89791U,
    0xc76c51a3U,
    0x0654be30U,
    0xd192e819U,
    0xd6ef5218U,
    0xd6990624U,
    0x5565a910U,
    0xf40e3585U,
    0x5771202aU,
    0x106aa070U,
    0x32bbd1b8U,
    0x19a4c116U,
    0xb8d2d0c8U,
    0x1e376c08U,
    0x5141ab53U,
    0x2748774cU,
    0xdf8eeb99U,
    0x34b0bcb5U,
    0xe19b48a8U,
    0x391c0cb3U,
    0xc5c95a63U,
    0x4ed8aa4aU,
    0xe3418acbU,
    0x5b9cca4fU,
    0x7763e373U,
    0x682e6ff3U,
    0xd6b2b8a3U,
    0x748f82eeU,
    0x5defb2fcU,
    0x78a5636fU,
    0x43172f60U,
    0x84c87814U,
    0xa1f0ab72U,
    0x8cc70208U,
    0x1a6439ecU,
    0x90befffaU,
    0x23631e28U,
    0xa4506cebU,
    0xde82bde9U,
    0xbef9a3f7U,
    0xb2c67915U,
    0xc67178f2U,
    0xe372532bU,
    0xca273eceU,
    0xea26619cU,
    0xd186b8c7U,
    0x21c0c207U,
    0xeada7dd6U,
    0xcde0eb1eU,
    0xf57d4f7fU,
    0xee6ed178U,
    0x06f067aaU,
    0x72176fbaU,
    0x0a637dc5U,
    0xa2c898a6U,
    0x113f9804U,
    0xbef90daeU,
    0x1b710b35U,
    0x131c471bU,
    0x28db77f5U,
    0x23047d84U,
    0x32caab7bU,
    0x40c72493U,
    0x3c9ebe0aU,
    0x15c9bebcU,
    0x431d67c4U,
    0x9c100d4cU,
    0x4cc5d4beU,
    0xcb3e42b6U,
    0x597f299cU,
    0xfc657e2aU,
    0x5fcb6fabU,
    0x3ad6faecU,
    0x6c44198cU,
    0x4a475817U,
};

//reverse byte order in unsigned int integer(change endian).
//bytelen must be a multiple of 4.
void SHA512_convert(unsigned int *a, unsigned char bytelen)
{
#if 1
    unsigned char i = bytelen >> 2;

    #ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
    #endif
        while (i > ((unsigned char)0)) {
            i--;
            a[i] = (a[i] << 24) | ((a[i] << 8) & 0xFF0000U) | ((a[i] >> 8) & 0xFF00U) | (a[i] >> 24);
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
#else
    unsigned char *p;
    unsigned char  bytes = bytelen;
    unsigned char  tmp;

    #ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
    #endif
        p = (unsigned char *)a;
        while (bytes > ((unsigned char)0)) {
            tmp  = p[0];
            p[0] = p[3];
            p[3] = tmp;
            p    = &p[1];
            tmp  = p[0];
            p[0] = p[1];
            p[1] = tmp;
            bytes -= (unsigned char)4;
            p = &p[3];
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
#endif
}

//unsigned int copy
void SHA512_Copy_16_Words(unsigned int *dst, unsigned int *src)
{
#if 0
    unsigned int i;
#endif

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != dst) && (NULL != src)) {
#endif
#if 0
        for(i=0U; i<16U; i++)
        {
            dst[i] = src[i];
        }
#else
        dst[0]  = src[0];
        dst[1]  = src[1];
        dst[2]  = src[2];
        dst[3]  = src[3];
        dst[4]  = src[4];
        dst[5]  = src[5];
        dst[6]  = src[6];
        dst[7]  = src[7];
        dst[8]  = src[8];
        dst[9]  = src[9];
        dst[10] = src[10];
        dst[11] = src[11];
        dst[12] = src[12];
        dst[13] = src[13];
        dst[14] = src[14];
        dst[15] = src[15];
#endif
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

//for U64 integer a,b,c, get c = a ^ b
#if 1 //#ifdef SHA512_U32_SPEEDUP
    #define SHA512_XOR(a, b, c)       \
        {                             \
            (c)[0] = (a)[0] ^ (b)[0]; \
            (c)[1] = (a)[1] ^ (b)[1]; \
        }
#else
void SHA512_XOR(unsigned int a[2], unsigned int b[2], unsigned int c[2])
{
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != b) && (NULL != c)) {
    #endif
        c[0] = a[0] ^ b[0];
        c[1] = a[1] ^ b[1];
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
}
#endif


//for U64 integer x, left rotation n bits(0<n<32), obtained y
#if 1 //#ifdef SHA512_U32_SPEEDUP
    #define SHA512_ROTL(x, y, n)                                                    \
        {                                                                           \
            (y)[0] = (((x)[0]) << (n)) | (((x)[1]) >> (((unsigned char)32) - (n))); \
            (y)[1] = (((x)[1]) << (n)) | (((x)[0]) >> (((unsigned char)32) - (n))); \
        }
#else
void SHA512_ROTL(unsigned int x[2], unsigned int y[2], unsigned char n)
{
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != x) && (NULL != y)) {
    #endif
        if ((n > (unsigned char)0) && (n < (unsigned char)32)) {
            y[0] = ((x[0]) << (n)) | ((x[1]) >> (((unsigned char)32) - n));
            y[1] = ((x[1]) << (n)) | ((x[0]) >> (((unsigned char)32) - n));
        } else {
            //nothing to do, just for static analysis.
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
}
#endif


//for U64 integer x, right rotation n bits(0<n<32), obtained y
#if 1 //#ifdef SHA512_U32_SPEEDUP
    #define SHA512_ROTR(x, y, n)                                                    \
        {                                                                           \
            (y)[1] = (((x)[1]) >> (n)) | (((x)[0]) << (((unsigned char)32) - (n))); \
            (y)[0] = (((x)[0]) >> (n)) | (((x)[1]) << (((unsigned char)32) - (n))); \
        }
#else
void SHA512_ROTR(unsigned int x[2], unsigned int y[2], unsigned char n)
{
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != x) && (NULL != y)) {
    #endif
        if ((n > (unsigned char)0) && (n < (unsigned char)32)) {
            y[1] = ((x[1]) >> n) | ((x[0]) << (((unsigned char)32) - n));
            y[0] = ((x[0]) >> n) | ((x[1]) << (((unsigned char)32) - n));
        } else {
            //nothing to do, just for static analysis.
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
}
#endif


//for U64 integer x, right shift n bits(0<n<32), left pad 0, obtained y
#if 1 //#ifdef SHA512_U32_SPEEDUP
    #define SHA512_SHR(x, y, n)                                                     \
        {                                                                           \
            (y)[1] = (((x)[1]) >> (n)) | (((x)[0]) << (((unsigned char)32) - (n))); \
            (y)[0] = ((x)[0]) >> (n);                                               \
        }
#else
void SHA512_SHR(unsigned int x[2], unsigned int y[2], unsigned char n)
{
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != x) && (NULL != y)) {
    #endif
        if ((n > (unsigned char)0) && (n < (unsigned char)32)) {
            y[1] = ((x[1]) >> n) | ((x[0]) << (((unsigned char)32) - n));
            y[0] = x[0] >> n;
        } else {
            //since 0<n<32 for SHA512
    #if 0
            y[1] = (x[0])>>(n-((unsigned char)32));
            y[0] = 0U;
    #endif
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
}
#endif


//c = a + b mod 2^64
//b and c can not points the same buffer
#if 1 //#ifdef SHA512_U32_SPEEDUP
    #define SHA512_mod_add(a, b, c)   \
        {                             \
            (c)[1] = (a)[1] + (b)[1]; \
            (c)[0] = (a)[0] + (b)[0]; \
            if ((c)[1] < (b)[1]) {    \
                (c)[0] += 1U;         \
            }                         \
        }
#else
void SHA512_mod_add(unsigned int a[2], unsigned int b[2], unsigned int c[2])
{
    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != a) && (NULL != b) && (NULL != c)) {
    #endif
        c[1] = a[1] + b[1];
        c[0] = a[0] + b[0];
        if (c[1] < b[1]) {
            c[0] += 1U;
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif
}
#endif


//for U128 integer a, get a=a+byteLen. this is for updating total byte length
//here a[0] is high 32 bit, and a[1] is low 32 bit
void SHA512_byteLen_add(unsigned int a[4], unsigned int byteLen)
{
    unsigned int carry = byteLen;
#if 0
    unsigned char i;
#else
    unsigned int i;
#endif

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != a) {
#endif
        i = 4U;
        while (i > 0U) {
            i--;
            a[i] += carry;
            if (a[i] < carry) {
                carry = 1U;
            } else {
                break;
            }
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

//get byte length of current message stored in block buffer
unsigned char SHA512_block_byteLen(SHA512_Ctx *ctx)
{
    unsigned char ret = (unsigned char)0;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        ret = (unsigned char)((ctx->count[3]) & 0x7fU);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif

    return ret;
}

//for a message block stored in block buffer, update and get new internal output.
void SHA512_block(SHA512_Ctx *ctx, unsigned char byteLen)
{
    unsigned int  SHA512_tmp[2], SHA512_tmp1[2], abcdefgh[16], *W, *T;
    unsigned char i, j;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        //initialize abcdefgh
#if 1
        SHA512_Copy_16_Words(abcdefgh, ctx->hash);
#else
    memcpy_((unsigned char *)abcdefgh, (unsigned char *)ctx->hash, 64U);
#endif

        //convert data
        SHA512_convert(ctx->wbuf, byteLen);

#if 0
        print_buf((unsigned char *)ctx->wbuf, 128, "message");
        print_buf((unsigned char *)abcdefgh, 64, "abcdefgh 000");
#endif

        for (i = (unsigned char)0; i < (unsigned char)80; i++) {
            //compute W[i]
            W = &ctx->wbuf[(((unsigned int)i) << 1) & 31U]; //(i%16)*2 = (i*2)%32. for (i%16) = i-16k, so (i%16)*2 = 2i-32k,  then (i*2)%32 = (i<<1)&31
            if (i > ((unsigned char)15)) {
                //W += sigma1(Wt_2)
                T = &(ctx->wbuf[((((unsigned int)i) - 2U) << 1) & 31U]);

                SHA512_ROTR(T, SHA512_tmp, ((unsigned char)19));
                SHA512_ROTL(T, SHA512_tmp1, ((unsigned char)3));
                SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);
                SHA512_SHR(T, SHA512_tmp1, ((unsigned char)6));
                SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);

                SHA512_mod_add(W, SHA512_tmp, W);

                //W += sigma0(Wt_15)
                T = &(ctx->wbuf[((((unsigned int)i) - 15U) << 1) & 31U]);

                SHA512_ROTR(T, SHA512_tmp, ((unsigned char)1));
                SHA512_ROTR(T, SHA512_tmp1, ((unsigned char)8));
                SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);
                SHA512_SHR(T, SHA512_tmp1, ((unsigned char)7));
                SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);

                SHA512_mod_add(W, SHA512_tmp, W);

                //compute W[i] for now
                SHA512_mod_add(W, &(ctx->wbuf[((((unsigned int)i) - 7U) << 1) & 31U]), W);
            }

            //actually j is in [0,2,4,6,8,10,12,14].
            j = ((unsigned char)14) - ((i << 1) & ((unsigned char)15)); //(7-(i%8))*2 = 14-(i*2)%16

            /******** compute T1 ********/
            //Ch(e,f,g)
            SHA512_tmp[0] = ((abcdefgh[(j + ((unsigned char)10)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)12)) & ((unsigned char)15)])) ^ ((~(abcdefgh[(j + ((unsigned char)10)) & ((unsigned char)15)])) & (abcdefgh[(j + ((unsigned char)14)) & ((unsigned char)15)]));
            SHA512_tmp[1] = ((abcdefgh[(j + ((unsigned char)11)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)13)) & ((unsigned char)15)])) ^ ((~(abcdefgh[(j + ((unsigned char)11)) & ((unsigned char)15)])) & (abcdefgh[(j + ((unsigned char)15)) & ((unsigned char)15)]));

            SHA512_mod_add(&abcdefgh[j], SHA512_tmp, &abcdefgh[j]);
            SHA512_mod_add(&abcdefgh[j], W, &abcdefgh[j]);
            SHA512_mod_add(&abcdefgh[j], (unsigned int *)&SHA512_K[i << 1], &abcdefgh[j]);

            SHA512_ROTR(&abcdefgh[(j + ((unsigned char)10)) & ((unsigned char)15)], SHA512_tmp, (unsigned char)14);
            SHA512_ROTR(&abcdefgh[(j + ((unsigned char)10)) & ((unsigned char)15)], SHA512_tmp1, (unsigned char)18);
            SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);
            SHA512_ROTL(&abcdefgh[(j + ((unsigned char)10)) & ((unsigned char)15)], SHA512_tmp1, (unsigned char)23);
            SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);
#if 0
            print_buf(SHA512_tmp, 8, "\r\n SHA512_tmp");
#endif

            SHA512_mod_add(&abcdefgh[j], SHA512_tmp, &abcdefgh[j]);

            // e=d+T1
            SHA512_mod_add(&abcdefgh[(j + ((unsigned char)8)) & ((unsigned char)15)], &abcdefgh[j], &abcdefgh[(j + ((unsigned char)8)) & ((unsigned char)15)]);

            /******** compute T2 ********/
            //Maj(a,b,c)
            SHA512_tmp[0] = ((abcdefgh[(j + ((unsigned char)2)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)4)) & ((unsigned char)15)])) ^ ((abcdefgh[(j + ((unsigned char)2)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)6)) & ((unsigned char)15)])) ^ ((abcdefgh[(j + ((unsigned char)4)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)6)) & ((unsigned char)15)]));
            SHA512_tmp[1] = ((abcdefgh[(j + ((unsigned char)3)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)5)) & ((unsigned char)15)])) ^ ((abcdefgh[(j + ((unsigned char)3)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)7)) & ((unsigned char)15)])) ^ ((abcdefgh[(j + ((unsigned char)5)) & ((unsigned char)15)]) & (abcdefgh[(j + ((unsigned char)7)) & ((unsigned char)15)]));
            SHA512_mod_add(&abcdefgh[j], SHA512_tmp, &abcdefgh[j]);

            SHA512_ROTR(&abcdefgh[(j + ((unsigned char)2)) & ((unsigned char)15)], SHA512_tmp, (unsigned char)28);
            SHA512_ROTL(&abcdefgh[(j + ((unsigned char)2)) & ((unsigned char)15)], SHA512_tmp1, (unsigned char)30);
            SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);
            SHA512_ROTL(&abcdefgh[(j + ((unsigned char)2)) & ((unsigned char)15)], SHA512_tmp1, (unsigned char)25);
            SHA512_XOR(SHA512_tmp, SHA512_tmp1, SHA512_tmp);

            SHA512_mod_add(&abcdefgh[j], SHA512_tmp, &abcdefgh[j]);
#if 0
            print_buf((unsigned char *)abcdefgh, 64, "abcdefgh");
#endif
        }

        //get ctx->hash for now
#if 1 //this relusts in better performance
        for (i = (unsigned char)0; i < (unsigned char)8; i++) {
            SHA512_mod_add(&ctx->hash[i << 1], abcdefgh + (i << 1), ctx->hash + (i << 1));
        }
#else
    SHA512_mod_add(&ctx->hash[0], abcdefgh + (0), ctx->hash + (0));
    SHA512_mod_add(&ctx->hash[2], abcdefgh + (2), ctx->hash + (2));
    SHA512_mod_add(&ctx->hash[4], abcdefgh + (4), ctx->hash + (4));
    SHA512_mod_add(&ctx->hash[6], abcdefgh + (6), ctx->hash + (6));
    SHA512_mod_add(&ctx->hash[8], abcdefgh + (8), ctx->hash + (8));
    SHA512_mod_add(&ctx->hash[10], abcdefgh + (10), ctx->hash + (10));
    SHA512_mod_add(&ctx->hash[12], abcdefgh + (12), ctx->hash + (12));
    SHA512_mod_add(&ctx->hash[14], abcdefgh + (14), ctx->hash + (14));
#endif
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

//input message
void SHA512_Process(SHA512_Ctx *ctx, unsigned char *message, unsigned int byteLen)
{
#ifdef SHA512_U32_SPEEDUP
    unsigned char *msg   = message;
    unsigned int   bytes = byteLen;
    unsigned char  leftlen, filllen;
    unsigned int   i, Cycle;

    #ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != message)) {
    #endif
        leftlen = SHA512_block_byteLen(ctx);
        SHA512_byteLen_add(ctx->count, bytes);

        if (bytes < (128U - ((unsigned int)leftlen))) {
            memcpy_(((unsigned char *)ctx->wbuf) + leftlen, msg, bytes);
            return;
        }

        if (((unsigned char)0) != leftlen) {
            filllen = ((unsigned char)128) - leftlen;
            memcpy_(((unsigned char *)ctx->wbuf) + leftlen, msg, filllen);
            SHA512_block(ctx, (unsigned char)128);
            bytes -= filllen;
            msg = &msg[filllen];
        }

        Cycle = bytes >> 7;
        for (i = 0U; i < Cycle; i++) {
            memcpy_(((unsigned char *)ctx->wbuf), msg, 128U);
            SHA512_block(ctx, (unsigned char)128);
            msg = &msg[128];
        }

        leftlen = SHA512_block_byteLen(ctx);
        if (((unsigned char)0) != leftlen) {
            memcpy_(((unsigned char *)ctx->wbuf), msg, leftlen);
        }
    #ifdef SUPPORT_STATIC_ANALYSIS
    }
    #endif

#else
    unsigned char filllen, leftlen, rightlen;

    while (byteLen) {
        leftlen  = SHA512_block_byteLen(ctx);
        rightlen = ((unsigned char)128) - leftlen;
        filllen  = byteLen < rightlen ? byteLen : rightlen;
        memcpy_(((unsigned char *)ctx->wbuf) + leftlen, message, filllen);
        SHA512_byteLen_add(ctx->count, filllen);
        message += filllen;
        byteLen -= filllen;
        if (!SHA512_block_byteLen(ctx)) {
            SHA512_block(ctx, (unsigned char)128);
        }
    }
#endif
}

/* init with inputting iv, and updated message length(must be a 
 * multiple of SHA512 block length, i.e. 128bytes).
 */
void SHA512_Init_with_iv_and_updated_length(SHA512_Ctx *ctx, unsigned char iv[64], unsigned int byte_length_h, unsigned int byte_length_l)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        ctx->count[0] = 0U;
        ctx->count[1] = 0U;
        ctx->count[2] = byte_length_h;
        ctx->count[3] = byte_length_l;

        if (NULL != iv) {
            memcpy_((unsigned char *)ctx->hash, iv, 64U);
        } else {
            SHA512_Copy_16_Words(ctx->hash, (unsigned int *)SHA512_H0);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

//init
void SHA512_Init(SHA512_Ctx *ctx)
{
    SHA512_Init_with_iv_and_updated_length(ctx, NULL, 0, 0);
}

//padding and get digest
void SHA512_Done(SHA512_Ctx *ctx, unsigned char digest[64])
{
    unsigned int byteLen;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != digest)) {
#endif
        byteLen                                 = (unsigned int)SHA512_block_byteLen(ctx); //left bytes, in [0,127]
        ((unsigned char *)(ctx->wbuf))[byteLen] = (unsigned char)0x80;                     //begin to pad
        byteLen += 1U;

        //now byteLen is in [1,128], pad the remainder with 0, if byteLen is greater than 112, update this block.
        memset_((unsigned char *)(ctx->wbuf) + byteLen, 0, (byteLen <= 112U) ? (112U - byteLen) : (128U - byteLen));
        if (byteLen > 112U) {
            SHA512_block(ctx, (unsigned char)128);
            memset_((unsigned char *)ctx->wbuf, 0, 112U);
        }

        //pad total message bit length(16 bytes)
        ctx->wbuf[28] = (ctx->count[0] << 3) | (ctx->count[1] >> 29);
        ctx->wbuf[29] = (ctx->count[1] << 3) | (ctx->count[2] >> 29);
        ctx->wbuf[30] = (ctx->count[2] << 3) | (ctx->count[3] >> 29);
        ctx->wbuf[31] = ctx->count[3] << 3;

        SHA512_block(ctx, (unsigned char)112);

        //convert result
        SHA512_convert(ctx->hash, 64);

        memcpy_(digest, (unsigned char *)ctx->hash, 64U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

//get message digest(one-off style)
void SHA512_Hash(unsigned char *message, unsigned int byteLen, unsigned char digest[64])
{
    SHA512_Ctx ctx[1];

    SHA512_Init(ctx);
    SHA512_Process(ctx, message, byteLen);
    SHA512_Done(ctx, digest);
}

/* init with inputting iv, and updated message length(must be a 
 * multiple of SHA384 block length, i.e. 128bytes).
 */
void SHA384_Init_with_iv_and_updated_length(SHA384_Ctx *ctx, unsigned char iv[64], unsigned int byte_length_h, unsigned int byte_length_l)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        ctx->count[0] = 0U;
        ctx->count[1] = 0U;
        ctx->count[2] = byte_length_h;
        ctx->count[3] = byte_length_l;

        if (NULL != iv) {
            memcpy_((unsigned char *)ctx->hash, iv, 64U);
        } else {
            SHA512_Copy_16_Words(ctx->hash, (unsigned int *)SHA384_H0);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA384_Init(SHA384_Ctx *ctx)
{
    SHA384_Init_with_iv_and_updated_length(ctx, NULL, 0, 0);
}

void SHA384_Process(SHA384_Ctx *ctx, unsigned char *message, unsigned int byteLen)
{
    SHA512_Process(ctx, message, byteLen);
}

void SHA384_Done(SHA384_Ctx *ctx, unsigned char digest[48])
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != digest)) {
#endif
        SHA512_Done(ctx, (unsigned char *)ctx->hash);
        memcpy_(digest, (unsigned char *)ctx->hash, 48U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA384_Hash(unsigned char *message, unsigned int byteLen, unsigned char digest[48])
{
    SHA384_Ctx ctx[1];

    SHA384_Init(ctx);
    SHA512_Process(ctx, message, byteLen);
    SHA384_Done(ctx, digest);
}

/* init with inputting iv, and updated message length(must be a 
 * multiple of SHA512_224 block length, i.e. 128bytes).
 */
void SHA512_224_Init_with_iv_and_updated_length(SHA512_224_Ctx *ctx, unsigned char iv[64], unsigned int byte_length_h, unsigned int byte_length_l)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        ctx->count[0] = 0U;
        ctx->count[1] = 0U;
        ctx->count[2] = byte_length_h;
        ctx->count[3] = byte_length_l;

        if (NULL != iv) {
            memcpy_((unsigned char *)ctx->hash, iv, 64U);
        } else {
            SHA512_Copy_16_Words(ctx->hash, (unsigned int *)SHA512_224_H0);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA512_224_Init(SHA512_224_Ctx *ctx)
{
    SHA512_224_Init_with_iv_and_updated_length(ctx, NULL, 0, 0);
}

void SHA512_224_Process(SHA512_224_Ctx *ctx, unsigned char *message, unsigned int byteLen)
{
    SHA512_Process(ctx, message, byteLen);
}

void SHA512_224_Done(SHA512_224_Ctx *ctx, unsigned char digest[28])
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != digest)) {
#endif
        SHA512_Done(ctx, (unsigned char *)ctx->hash);
        memcpy_(digest, (unsigned char *)ctx->hash, 28U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA512_224_Hash(unsigned char *message, unsigned int byteLen, unsigned char digest[28])
{
    SHA512_224_Ctx ctx[1];

    SHA512_224_Init(ctx);
    SHA512_224_Process(ctx, message, byteLen);
    SHA512_224_Done(ctx, digest);
}

/* init with inputting iv, and updated message length(must be a 
 * multiple of SHA512_256 block length, i.e. 128bytes).
 */
void SHA512_256_Init_with_iv_and_updated_length(SHA512_256_Ctx *ctx, unsigned char iv[64], unsigned int byte_length_h, unsigned int byte_length_l)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != ctx) {
#endif
        ctx->count[0] = 0U;
        ctx->count[1] = 0U;
        ctx->count[2] = byte_length_h;
        ctx->count[3] = byte_length_l;

        if (NULL != iv) {
            memcpy_((unsigned char *)ctx->hash, iv, 64U);
        } else {
            SHA512_Copy_16_Words(ctx->hash, (unsigned int *)SHA512_256_H0);
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA512_256_Init(SHA512_256_Ctx *ctx)
{
    SHA512_256_Init_with_iv_and_updated_length(ctx, NULL, 0, 0);
}

void SHA512_256_Process(SHA512_256_Ctx *ctx, unsigned char *message, unsigned int byteLen)
{
    SHA512_Process(ctx, message, byteLen);
}

void SHA512_256_Done(SHA512_256_Ctx *ctx, unsigned char digest[32])
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != digest)) {
#endif
        SHA512_Done(ctx, (unsigned char *)ctx->hash);
        memcpy_(digest, (unsigned char *)ctx->hash, 32U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void SHA512_256_Hash(unsigned char *message, unsigned int byteLen, unsigned char digest[32])
{
    SHA512_256_Ctx ctx[1];

    SHA512_256_Init(ctx);
    SHA512_256_Process(ctx, message, byteLen);
    SHA512_256_Done(ctx, digest);
}

/******************* HMAC_SHA384 *******************/
void HMAC_SHA384_Init(HMAC_SHA384_Ctx *ctx, unsigned char *key, unsigned int keyByteLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != key)) {
#endif
        //get K0
        if (keyByteLen <= 128U) {
            memcpy_((unsigned char *)(ctx->K0), key, keyByteLen);
            memset_(((unsigned char *)(ctx->K0)) + keyByteLen, 0, 128U - keyByteLen);
        } else {
            SHA384_Hash((unsigned char *)key, keyByteLen, (unsigned char *)(ctx->K0));
            memset_(((unsigned char *)(ctx->K0)) + 48, 0, 128U - 48U);
        }

        //get K0 ^ ipad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= 0x36363636U;
        }

        SHA384_Init(ctx->sha512_ctx);

        SHA512_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA384_Process(HMAC_SHA384_Ctx *ctx, unsigned char *input, unsigned int byteLen)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != input)) {
#endif
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)input, byteLen);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA384_Done(HMAC_SHA384_Ctx *ctx, unsigned char mac[48])
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != mac)) {
#endif
        //set mac as hash((K0^ipad)||message)
        SHA384_Done(ctx->sha512_ctx, mac);

        //get K0 ^ opad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= (0x36363636U ^ 0x5c5c5c5cU);
        }

        SHA384_Init(ctx->sha512_ctx);
        SHA384_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
        SHA384_Process(ctx->sha512_ctx, (unsigned char *)mac, 48U);
        SHA384_Done(ctx->sha512_ctx, mac);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA384(unsigned char *key, unsigned int keyByteLen, unsigned char *msg, unsigned int msgByteLen, unsigned char mac[48])
{
    HMAC_SHA384_Ctx ctx[1];

    HMAC_SHA384_Init(ctx, key, keyByteLen);

    HMAC_SHA384_Process(ctx, msg, msgByteLen);

    HMAC_SHA384_Done(ctx, mac);
}

/******************* HMAC_SHA512_224 *******************/
void HMAC_SHA512_224_Init(HMAC_SHA512_224_Ctx *ctx, unsigned char *key, unsigned int keyByteLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != key)) {
#endif
        //get K0
        if (keyByteLen <= 128U) {
            memcpy_((unsigned char *)(ctx->K0), key, keyByteLen);
            memset_(((unsigned char *)(ctx->K0)) + keyByteLen, 0, 128U - keyByteLen);
        } else {
            SHA512_224_Hash((unsigned char *)key, keyByteLen, (unsigned char *)(ctx->K0));
            memset_(((unsigned char *)(ctx->K0)) + 28, 0, 128U - 28U);
        }

        //get K0 ^ ipad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= 0x36363636U;
        }

        SHA512_224_Init(ctx->sha512_ctx);

        SHA512_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_224_Process(HMAC_SHA512_224_Ctx *ctx, unsigned char *input, unsigned int byteLen)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != input)) {
#endif
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)input, byteLen);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_224_Done(HMAC_SHA512_224_Ctx *ctx, unsigned char mac[28])
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != mac)) {
#endif
        //set mac as hash((K0^ipad)||message)
        SHA512_224_Done(ctx->sha512_ctx, mac);

        //get K0 ^ opad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= (0x36363636U ^ 0x5c5c5c5cU);
        }

        SHA512_224_Init(ctx->sha512_ctx);
        SHA512_224_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
        SHA512_224_Process(ctx->sha512_ctx, (unsigned char *)mac, 28U);
        SHA512_224_Done(ctx->sha512_ctx, mac);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_224(unsigned char *key, unsigned int keyByteLen, unsigned char *msg, unsigned int msgByteLen, unsigned char mac[28])
{
    HMAC_SHA512_224_Ctx ctx[1];

    HMAC_SHA512_224_Init(ctx, key, keyByteLen);

    HMAC_SHA512_224_Process(ctx, msg, msgByteLen);

    HMAC_SHA512_224_Done(ctx, mac);
}

/******************* HMAC_SHA512_256 *******************/
void HMAC_SHA512_256_Init(HMAC_SHA512_256_Ctx *ctx, unsigned char *key, unsigned int keyByteLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != key)) {
#endif
        //get K0
        if (keyByteLen <= 128U) {
            memcpy_((unsigned char *)(ctx->K0), key, keyByteLen);
            memset_(((unsigned char *)(ctx->K0)) + keyByteLen, 0, 128U - keyByteLen);
        } else {
            SHA512_256_Hash((unsigned char *)key, keyByteLen, (unsigned char *)(ctx->K0));
            memset_(((unsigned char *)(ctx->K0)) + 32, 0, 128U - 32U);
        }

        //get K0 ^ ipad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= 0x36363636U;
        }

        SHA512_256_Init(ctx->sha512_ctx);

        SHA512_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_256_Process(HMAC_SHA512_256_Ctx *ctx, unsigned char *input, unsigned int byteLen)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != input)) {
#endif
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)input, byteLen);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_256_Done(HMAC_SHA512_256_Ctx *ctx, unsigned char mac[32])
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != mac)) {
#endif
        //set mac as hash((K0^ipad)||message)
        SHA512_256_Done(ctx->sha512_ctx, mac);

        //get K0 ^ opad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= (0x36363636U ^ 0x5c5c5c5cU);
        }

        SHA512_256_Init(ctx->sha512_ctx);
        SHA512_256_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
        SHA512_256_Process(ctx->sha512_ctx, (unsigned char *)mac, 32U);
        SHA512_256_Done(ctx->sha512_ctx, mac);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_256(unsigned char *key, unsigned int keyByteLen, unsigned char *msg, unsigned int msgByteLen, unsigned char mac[32])
{
    HMAC_SHA512_256_Ctx ctx[1];

    HMAC_SHA512_256_Init(ctx, key, keyByteLen);

    HMAC_SHA512_256_Process(ctx, msg, msgByteLen);

    HMAC_SHA512_256_Done(ctx, mac);
}

/******************* HMAC_SHA512 *******************/
void HMAC_SHA512_Init(HMAC_SHA512_Ctx *ctx, unsigned char *key, unsigned int keyByteLen)
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != key)) {
#endif
        //get K0
        if (keyByteLen <= 128U) {
            memcpy_((unsigned char *)(ctx->K0), key, keyByteLen);
            memset_(((unsigned char *)(ctx->K0)) + keyByteLen, 0, 128U - keyByteLen);
        } else {
            SHA512_Hash((unsigned char *)key, keyByteLen, (unsigned char *)(ctx->K0));
            memset_(((unsigned char *)(ctx->K0)) + 64, 0, 128U - 64U);
        }

        //get K0 ^ ipad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= 0x36363636U;
        }

        SHA512_Init(ctx->sha512_ctx);

        SHA512_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_Process(HMAC_SHA512_Ctx *ctx, unsigned char *input, unsigned int byteLen)
{
#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != input)) {
#endif
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)input, byteLen);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512_Done(HMAC_SHA512_Ctx *ctx, unsigned char mac[64])
{
    unsigned int i;

#ifdef SUPPORT_STATIC_ANALYSIS
    if ((NULL != ctx) && (NULL != mac)) {
#endif
        //set mac as hash((K0^ipad)||message)
        SHA512_Done(ctx->sha512_ctx, mac);

        //get K0 ^ opad
        for (i = 0U; i < 32U; i++) {
            ctx->K0[i] ^= (0x36363636U ^ 0x5c5c5c5cU);
        }

        SHA512_Init(ctx->sha512_ctx);
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)(ctx->K0), 128U);
        SHA512_Process(ctx->sha512_ctx, (unsigned char *)mac, 64U);
        SHA512_Done(ctx->sha512_ctx, mac);
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif
}

void HMAC_SHA512(unsigned char *key, unsigned int keyByteLen, unsigned char *msg, unsigned int msgByteLen, unsigned char mac[64])
{
    HMAC_SHA512_Ctx ctx[1];

    HMAC_SHA512_Init(ctx, key, keyByteLen);

    HMAC_SHA512_Process(ctx, msg, msgByteLen);

    HMAC_SHA512_Done(ctx, mac);
}
