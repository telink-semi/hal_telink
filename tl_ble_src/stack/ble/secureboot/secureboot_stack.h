/********************************************************************************************************
 * @file    secureboot_stack.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef SHA224_SHA256_H
#define SHA224_SHA256_H


typedef unsigned int   U32;
typedef unsigned short U16;
typedef unsigned char  U8;
typedef signed char    S8;


/* type to hold the SHA256 context */
typedef struct
{   U32 count[2];   
    U32 *hash;    //hash[8];
    U32 wbuf[16];
}SHA256_Ctx;

/* type to hold the SHA224 context */
typedef SHA256_Ctx SHA224_Ctx;


/* type to hold the HMAC_SHA256 context */
typedef struct
{     
    U32 K0[16];
    SHA256_Ctx sha256_ctx[1];
}HMAC_SHA256_Ctx;

/* type to hold the HMAC_SHA224 context */
typedef HMAC_SHA256_Ctx HMAC_SHA224_Ctx;


void SHA256_Init(SHA256_Ctx * ctx, U8 digest[32]);
void SHA256_Process(SHA256_Ctx * ctx, U8 * message, U32 byteLen);
void SHA256_Done(SHA256_Ctx * ctx);
void SHA256_Hash(U8 * message, U32 byteLen, U8 digest[32]);

void SHA224_Init(SHA224_Ctx * ctx, U8 digest[32]);
void SHA224_Process(SHA224_Ctx * ctx, U8 * message, U32 byteLen);
void SHA224_Done(SHA224_Ctx * ctx);
void SHA224_Hash(U8 * message, U32 byteLen, U8 digest[32]);



void HMAC_SHA256_Init(HMAC_SHA256_Ctx *ctx, U8 *key, U32 keyByteLen, U8 *mac);
void HMAC_SHA256_Process(HMAC_SHA256_Ctx *ctx, const U8 *input, U32 byteLen);
void HMAC_SHA256_Done(HMAC_SHA256_Ctx *ctx);
void HMAC_SHA256(U8 *key, U32 keyByteLen, U8 *msg, U32 msgByteLen, U8 *mac);

void HMAC_SHA224_Init(HMAC_SHA224_Ctx *ctx, U8 *key, U32 keyByteLen, U8 *mac);
void HMAC_SHA224_Process(HMAC_SHA224_Ctx *ctx, const U8 *input, U32 byteLen);
void HMAC_SHA224_Done(HMAC_SHA224_Ctx *ctx);
void HMAC_SHA224(U8 *key, U32 keyByteLen, U8 *msg, U32 msgByteLen, U8 *mac);

unsigned int sign_verify(unsigned int data_adr,unsigned int data_size, unsigned char *pub_key, unsigned char *sign);

#endif
