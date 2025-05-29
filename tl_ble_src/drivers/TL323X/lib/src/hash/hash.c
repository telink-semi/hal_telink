/********************************************************************************************************
 * @file    hash.c
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
#include "lib/include/crypto_common/utility.h"
#include "lib/include/hash/hash.h"

// HASH IV definition
#ifndef HASH_CPU_BIG_ENDIAN

#ifdef SUPPORT_HASH_SM3
extern unsigned int const g_sm3_iv[8];
unsigned int const g_sm3_iv[8] = {
    0x6F168073U,
    0xB9B21449U,
    0xD7422417U,
    0x00068ADAU,
    0xBC306FA9U,
    0xAA383116U,
    0x4DEE8DE3U,
    0x4E0EFBB0U,
};
#endif

#ifdef SUPPORT_HASH_MD5
extern unsigned int const g_md5_iv[4];
unsigned int const g_md5_iv[4] = {
    0x67452301U,
    0xefcdab89U,
    0x98badcfeU,
    0x10325476U,
};
#endif

#ifdef SUPPORT_HASH_SHA256
extern unsigned int const g_sha256_iv[8];
unsigned int const g_sha256_iv[8] = {
    0x67E6096AU,
    0x85AE67BBU,
    0x72F36E3CU,
    0x3AF54FA5U,
    0x7F520E51U,
    0x8C68059BU,
    0xABD9831FU,
    0x19CDE05BU,
};
#endif

#ifdef SUPPORT_HASH_SHA384
extern unsigned int const g_sha384_iv[16];
unsigned int const g_sha384_iv[16] = {
    0x5D9DBBCBU,
    0xD89E05C1U,
    0x2A299A62U,
    0x07D57C36U,
    0x5A015991U,
    0x17DD7030U,
    0xD8EC2F15U,
    0x39590EF7U,
    0x67263367U,
    0x310BC0FFU,
    0x874AB48EU,
    0x11155868U,
    0x0D2E0CDBU,
    0xA78FF964U,
    0x1D48B547U,
    0xA44FFABEU,
};
#endif

#ifdef SUPPORT_HASH_SHA512
extern unsigned int const g_sha512_iv[16];
unsigned int const g_sha512_iv[16] = {
    0x67E6096AU,
    0x08C9BCF3U,
    0x85AE67BBU,
    0x3BA7CA84U,
    0x72F36E3CU,
    0x2BF894FEU,
    0x3AF54FA5U,
    0xF1361D5FU,
    0x7F520E51U,
    0xD182E6ADU,
    0x8C68059BU,
    0x1F6C3E2BU,
    0xABD9831FU,
    0x6BBD41FBU,
    0x19CDE05BU,
    0x79217E13U,
};
#endif

#ifdef SUPPORT_HASH_SHA1
extern unsigned int const g_sha1_iv[5];
unsigned int const g_sha1_iv[5] = {
    0x01234567U,
    0x89ABCDEFU,
    0xFEDCBA98U,
    0x76543210U,
    0xF0E1D2C3U,
};
#endif

#ifdef SUPPORT_HASH_SHA224
extern unsigned int const g_sha224_iv[8];
unsigned int const g_sha224_iv[8] = {
    0xD89E05C1U,
    0x07D57C36U,
    0x17DD7030U,
    0x39590EF7U,
    0x310BC0FFU,
    0x11155868U,
    0xA78FF964U,
    0xA44FFABEU,
};
#endif

#ifdef SUPPORT_HASH_SHA512_224
extern unsigned int const g_sha512_224_iv[16];
unsigned int const g_sha512_224_iv[16] = {
    0xC8373D8CU,
    0xA24D5419U,
    0x6699E173U,
    0xD6D4DC89U,
    0xAEB7FA1DU,
    0x829CFF32U,
    0x14D59D67U,
    0xCF9F2F58U,
    0x692B6D0FU,
    0xA84DD47BU,
    0x736FE377U,
    0x4289C404U,
    0xA8859D3FU,
    0xC8361D6AU,
    0xADE61211U,
    0xA192D691U,
};
#endif

#ifdef SUPPORT_HASH_SHA512_256
extern unsigned int const g_sha512_256_iv[16];
unsigned int const g_sha512_256_iv[16] = {
    0x94213122U,
    0x2CF72BFCU,
    0xA35F559FU,
    0xC2644CC8U,
    0x6BB89323U,
    0x51B1536FU,
    0x19773896U,
    0xBDEA4059U,
    0xE23E2896U,
    0xE3FF8EA8U,
    0x251E5EBEU,
    0x92398653U,
    0xFC99012BU,
    0xAAB8852CU,
    0xDC2DB70EU,
    0xA22CC581U,
};
#endif

#else

#ifdef SUPPORT_HASH_SM3
extern unsigned int const g_sm3_iv[8];
unsigned int const g_sm3_iv[8] = {
    0x7380166fU,
    0x4914b2b9U,
    0x172442d7U,
    0xda8a0600U,
    0xa96f30bcU,
    0x163138aaU,
    0xe38dee4dU,
    0xb0fb0e4eU,
};
#endif

#ifdef SUPPORT_HASH_MD5
extern unsigned int const g_md5_iv[4];
unsigned int const g_md5_iv[4] = {
    0x01234567U,
    0x89ABCDEFU,
    0xFEDCBA98U,
    0x76543210U,
};
#endif

#ifdef SUPPORT_HASH_SHA256
extern unsigned int const g_sha256_iv[8];
unsigned int const g_sha256_iv[8] = {
    0x6a09e667U,
    0xbb67ae85U,
    0x3c6ef372U,
    0xa54ff53aU,
    0x510e527fU,
    0x9b05688cU,
    0x1f83d9abU,
    0x5be0cd19U,
};
#endif

#ifdef SUPPORT_HASH_SHA384
extern unsigned int const g_sha384_iv[16];
unsigned int const g_sha384_iv[16] = {
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
#endif

#ifdef SUPPORT_HASH_SHA512
extern unsigned int const g_sha512_iv[16];
unsigned int const g_sha512_iv[16] = {
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
#endif

#ifdef SUPPORT_HASH_SHA1
extern unsigned int const g_sha1_iv[5];
unsigned int const g_sha1_iv[5] = {
    0x67452301U,
    0xefcdab89U,
    0x98badcfeU,
    0x10325476U,
    0xc3d2e1f0U,
};
#endif

#ifdef SUPPORT_HASH_SHA224
extern unsigned int const g_sha224_iv[8];
unsigned int const g_sha224_iv[8] = {
    0xc1059ed8U,
    0x367cd507U,
    0x3070dd17U,
    0xf70e5939U,
    0xffc00b31U,
    0x68581511U,
    0x64f98fa7U,
    0xbefa4fa4U,
};
#endif

#ifdef SUPPORT_HASH_SHA512_224
extern unsigned int const g_sha512_224_iv[16];
unsigned int const g_sha512_224_iv[16] = {
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
#endif

#ifdef SUPPORT_HASH_SHA512_256
extern unsigned int const g_sha512_256_iv[16];
unsigned int const g_sha512_256_iv[16] = {
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
#endif

#endif

/**
 * @brief           check whether the hash algorithm is valid or not
 * @param[in]       alg                  - specific hash algorithm
 * @return          HASH_SUCCESS(valid), other(invalid)
 */
unsigned int check_hash_alg(hash_alg_e alg)
{
    unsigned int ret;

    switch ((unsigned char)alg)
    {
#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
#endif

#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
#endif

        ret = HASH_SUCCESS;
        break;

    default:
        ret = HASH_INPUT_INVALID;
        break;
    }

    return ret;
}

/**
 * @brief           get hash block word length
 * @param[in]       alg                  - specific hash algorithm
 * @return          hash block word length
 * @note
 *        1. please make sure alg is valid
 */
unsigned char hash_get_block_word_len(hash_alg_e alg)
{
    unsigned char block_words = (unsigned char)0;

    switch ((unsigned char)alg)
    {
#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
#endif

#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
#endif

#if (defined(SUPPORT_HASH_SM3) || defined(SUPPORT_HASH_MD5) || defined(SUPPORT_HASH_SHA1) || defined(SUPPORT_HASH_SHA256) || defined(SUPPORT_HASH_SHA224))
        block_words = (unsigned char)16;
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
#endif

#if (defined(SUPPORT_HASH_SHA384) || defined(SUPPORT_HASH_SHA512) || defined(SUPPORT_HASH_SHA512_224) || defined(SUPPORT_HASH_SHA512_256))
        block_words = (unsigned char)32;
        break;
#endif

    default:
        break;
    }

    return block_words;
}

/**
 * @brief           get hash iterator word length
 * @param[in]       alg                  - specific hash algorithm
 * @return          hash iterator word length
 * @note
 *        1. please make sure alg is valid
 */
unsigned char hash_get_iterator_word_len(hash_alg_e alg)
{
    unsigned char iterator_words = 0;

    switch ((unsigned char)alg)
    {
#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
        iterator_words = 4;
        break;
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
        iterator_words = 5;
        break;
#endif

#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
#endif

#if (defined(SUPPORT_HASH_SM3) || defined(SUPPORT_HASH_SHA256) || defined(SUPPORT_HASH_SHA224))
        iterator_words = 8;
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
#endif

#if (defined(SUPPORT_HASH_SHA384) || defined(SUPPORT_HASH_SHA512) || defined(SUPPORT_HASH_SHA512_224) || defined(SUPPORT_HASH_SHA512_256))
        iterator_words = 16;
        break;
#endif

    default:
        break;
    }

    return iterator_words;
}

/**
 * @brief           get hash digest word length
 * @param[in]       alg                  - specific hash algorithm
 * @return          hash digest word length
 * @note
 *        1. please make sure alg is valid
 */
unsigned char hash_get_digest_word_len(hash_alg_e alg)
{
    unsigned char digest_words = 0;

    switch ((unsigned char)alg)
    {
#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
        digest_words = 4;
        break;
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
        digest_words = 5;
        break;
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
#endif

#if (defined(SUPPORT_HASH_SHA224) || defined(SUPPORT_HASH_SHA512_224))
        digest_words = 7;
        break;
#endif

#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
#endif

#if (defined(SUPPORT_HASH_SM3) || defined(SUPPORT_HASH_SHA256) || defined(SUPPORT_HASH_SHA512_256))
        digest_words = 8;
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
        digest_words = 12;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
        digest_words = 16;
        break;
#endif

    default:
        break;
    }

    return digest_words;
}

/**
 * @brief           get hash IV pointer
 * @param[in]       alg                  - specific hash algorithm
 * @return          IV address
 */
const unsigned int *hash_get_iv(hash_alg_e alg)
{
    const unsigned int *iv;

    switch ((unsigned char)alg)
    {
#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
        iv = g_sm3_iv;
        break;
#endif

#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
        iv = g_md5_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
        iv = g_sha256_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
        iv = g_sha384_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
        iv = g_sha1_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
        iv = g_sha512_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
        iv = g_sha224_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
        iv = g_sha512_224_iv;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
        iv = g_sha512_256_iv;
        break;
#endif

    default:
        iv = NULL;
        break;
    }

    return iv;
}

/**
 * @brief           input hash IV
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       hash_iterator_words  - iterator word length
 * @return          none
 */
void hash_set_iv(hash_alg_e alg, unsigned int hash_iterator_words)
{
    hash_set_iterator(hash_get_iv(alg), hash_iterator_words);
}

/**
 * @brief           hash message total byte length a = a+b
 * @param[in,out]   a                    - big number a, total byte length of hash message
 * @param[in]       a_words              - word length of buffer a
 * @param[in]       b                    - integer to be added to a
 * @return          0(success), other(error, hash total length overflow)
 */
unsigned int hash_total_byte_len_add_uint32(unsigned int *a, unsigned int a_words, unsigned int b)
{
    unsigned int i, ret;
    unsigned int tmp = b;

    for (i = 0U; i < a_words; i++)
    {
        a[i] += tmp;
        if (a[i] < tmp)
        {
            tmp = 1U;
        }
        else
        {
            break;
        }
    }

    if (i == a_words)
    {
        ret = 1U;
    }
    else if (0U != (a[a_words - 1U] & 0xE0000000U)) // bit length overflow
    {
        ret = 1U;
    }
    else
    {
        ret = 0U;
    }

    return ret;
}

/**
 * @brief           start HASH iteration calc
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @return          none
 */
void hash_start_calculate(hash_ctx_t *ctx)
{
    // if it is the first time to calculate, set the IV
    if (0U != (ctx->first_update_flag))
    {
        hash_set_iv(ctx->alg, ctx->iterator_word_len);

        ctx->first_update_flag = (unsigned char)0; // clear the flag
    }
    else
    {
    }

    hash_start();
}

/**
 * @brief           init HASH
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 */
unsigned int hash_init(hash_ctx_t *ctx, hash_alg_e alg)
{
    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {
        // handle other;
    }

    // clear the context
    memset_((unsigned char *)ctx, 0, sizeof(hash_ctx_t));

#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_cpu_mode();
    hash_disable_interruption();
    hash_set_last_block(0); // set not the last block
    hash_set_alg(alg);

    hash_clear_msg_len();
#endif

    // set context config
    ctx->alg = alg;
    ctx->block_byte_len = (unsigned char)(hash_get_block_word_len(alg) << 2);
    ctx->iterator_word_len = (unsigned char)(hash_get_iterator_word_len(alg));
    ctx->digest_byte_len = (unsigned char)(hash_get_digest_word_len(alg) << 2);
    ctx->status.busy = 0U;
    ctx->first_update_flag = (unsigned char)1;
    ctx->finish_flag = (unsigned char)0;

    return HASH_SUCCESS;
}

/**
 * @brief           init HASH with iv and updated message length
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       iv                   - iv or iterator after updating some blocks
 * @param[in]       byte_length_h        - high 32 bit of updated message byte length
 * @param[in]       byte_length_l        - low 32 bit of updated message byte length,
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. updated message byte length must be a multiple of block byte length
 */
unsigned int hash_init_with_iv_and_updated_length(hash_ctx_t *ctx, hash_alg_e alg, const unsigned int *iv, unsigned int byte_length_h, unsigned int byte_length_l)
{
    unsigned int ret;
    unsigned char block_byte_len = (unsigned char)(hash_get_block_word_len(alg) << 2);

    if (0U != block_byte_len)
    {
        if (0U != (byte_length_l % CAST2UINT32(block_byte_len)))
        {
            return HASH_INPUT_INVALID;
        }
        else
        {
        }
    }
    else
    {
        return HASH_INPUT_INVALID;
    }

    ret = hash_init(ctx, alg);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ctx->first_update_flag = (unsigned char)0;
    ctx->total[0] = byte_length_l;
    ctx->total[1] = byte_length_h;

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    memcpy_(ctx->iterator, iv, CAST2UINT32(ctx->iterator_word_len) << 2);
#else
    hash_set_iterator(iv, ctx->iterator_word_len);
#endif

    return HASH_SUCCESS;
}

/**
 * @brief           hash iterate calc with some blocks
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[in]       msg                  - message of some blocks
 * @param[in]       block_count          - count of blocks
 * @return          none
 * @note
 *        1. please make sure the three parameters is valid
 */
void hash_calc_blocks(hash_ctx_t *ctx, const unsigned char *msg, unsigned int block_count)
{
#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    // set the input iterator data
    if (((unsigned char)1) != ctx->first_update_flag)
    {
        hash_set_iterator(ctx->iterator, ctx->iterator_word_len);
    }
    else
    {
        ;
    }
#endif

    while (0U != (block_count--))
    {
        hash_input_msg_u8(msg, ctx->block_byte_len);

        hash_start_calculate(ctx);

        msg = &(msg[ctx->block_byte_len]);

        hash_wait_till_done();
    }

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    // if message update not done, get the new iterator hash value
    if (((unsigned char)1) != ctx->finish_flag)
    {
        hash_get_iterator((unsigned char *)(ctx->iterator), ctx->iterator_word_len);
    }
    else
    {
        ;
    }
#endif
}

/**
 * @brief           hash iterate calc with padding
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[in]       msg                  - message that contains the last block(maybe not full)
 * @param[in]       msg_len              - byte length of msg
 * @return          none
 * @note
 *        1. msg contains the last byte of the total message while the total message length is not a
 *           multiple of hash block length, otherwise byte length of msg is zero.
 *        2. at present this function does not support the case that byte length of msg is a multiple
 *           of hash block length. actually msg_len here must be less than the hash block byte length,
 *           namely, this function is just for the remainder message, and will do padding, finally get
 *           digest.
 *        3. before calling this function, some blocks(could be 0 block) must be calculated
 */
void hash_calc_rand_len_msg(hash_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    // set the input iterator data
    if (((unsigned char)1) != ctx->first_update_flag)
    {
        hash_set_iterator(ctx->iterator, ctx->iterator_word_len);
    }
    else
    {
        ;
    }
#endif

    hash_set_last_block(1);

    hash_input_msg_u8(msg, msg_len);

    hash_start_calculate(ctx);

    hash_wait_till_done();
}

/**
 * @brief           hash update message
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the three parameters are valid, and ctx is initialize
 */
unsigned int hash_update(hash_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
    unsigned int count;
    unsigned char left, fill;

    if ((NULL == ctx))
    {
        return HASH_BUFFER_NULL;
    }
    else if ((NULL == msg) || (0U == msg_len))
    {
        return HASH_SUCCESS;
    }
    else
    {
        // handle other;
    }

    ctx->status.busy = 1U; // start to update processing

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_cpu_mode();
    hash_set_last_block(0); // set not the last block
    hash_set_alg(ctx->alg);
#endif

    left = (unsigned char)(ctx->total[0] % (ctx->block_byte_len)); // byte length of valid message left in block buffer
    fill = (ctx->block_byte_len) - left;                           // byte length that block buffer need to fill a block

    // update total byte length
    if (0U != (hash_total_byte_len_add_uint32(ctx->total, CAST2UINT32(ctx->block_byte_len) / 32U, msg_len)))
    {
        return HASH_LEN_OVERFLOW;
    }
    else
    {
    }

    if (0U != left)
    {
        if (msg_len >= fill)
        {
            memcpy_(&(ctx->hash_buffer[left]), msg, fill);
            hash_calc_blocks(ctx, ctx->hash_buffer, 1);
            msg_len -= fill;
            msg = &(msg[fill]);
        }
        else
        {
            memcpy_(&(ctx->hash_buffer[left]), msg, msg_len);
            goto END;
        }
    }
    else
    {
    }

    // process some blocks
    count = msg_len / (ctx->block_byte_len);
    if (0U != count)
    {
        hash_calc_blocks(ctx, msg, count);
    }
    else
    {
    }

    // process the remainder
    msg_len = msg_len % (ctx->block_byte_len);
    if (0U != msg_len)
    {
        msg = &(msg[(ctx->block_byte_len) * count]);
        memcpy_(ctx->hash_buffer, msg, msg_len);
    }
    else
    {
    }

END:
    ctx->status.busy = 0U; // update end, status becomes idle

    return HASH_SUCCESS;
}

/**
 * @brief           message update done, get the digest
 * @param[in]       ctx                  - hash_ctx_t context pointer
 * @param[out]      digest               - hash digest
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the ctx is valid and initialized
 *        2. please make sure the digest buffer is sufficient
 */
unsigned int hash_final(hash_ctx_t *ctx, unsigned char *digest)
{
    unsigned char tmp;

    if ((NULL == ctx) || (NULL == digest))
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_cpu_mode();

    hash_disable_interruption();

    // hash_set_last_block(0);//set not the last block

    hash_set_alg(ctx->alg);
#endif

    // set total length of message
    hash_set_msg_total_byte_len(ctx->total, CAST2UINT32(ctx->block_byte_len) / 32U);

    ctx->finish_flag = ((unsigned char)1); // the last block calc

    // get the byte length of the remainder msg(less than one block)
    tmp = (unsigned char)(ctx->total[0] % (ctx->block_byte_len));

    // input the remainder msg(less than one block)
    hash_calc_rand_len_msg(ctx, ctx->hash_buffer, tmp);

    // get the hash result
    hash_get_iterator(digest, CAST2UINT32(ctx->digest_byte_len) >> 2);

    // clear the context
    memset_((unsigned char *)ctx, 0, sizeof(hash_ctx_t));

    return HASH_SUCCESS;
}

/**
 * @brief           input whole message and get its digest
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the input message, it could be 0
 * @param[out]      digest               - hash digest
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 */
unsigned int hash(hash_alg_e alg, const unsigned char *msg, unsigned int msg_len, unsigned char *digest)
{
    hash_ctx_t ctx[1];
    unsigned int ret;

    ret = hash_init(ctx, alg);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = hash_update(ctx, msg, msg_len);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = hash_final(ctx, digest);
    if (HASH_SUCCESS != ret)
    {
        memset_(digest, 0, ctx->digest_byte_len);
    }
    else
    {
    }

    // clear the context
    memset_((unsigned char *)ctx, 0, sizeof(hash_ctx_t));

    return ret;
}

#ifdef SUPPORT_HASH_NODE
/**
 * @brief           input whole message and get its digest(node style)
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      digest               - hash digest
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length
 */
unsigned int hash_node_steps(hash_alg_e alg, const hash_node_t *node, unsigned int node_num, unsigned char *digest)
{
    hash_ctx_t ctx[1];
    unsigned int i, ret;

    ret = hash_init(ctx, alg);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    for (i = 0U; i < node_num; i++)
    {
        ret = hash_update(ctx, node[i].msg_addr, node[i].msg_len);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }

    return hash_final(ctx, digest);
}
#endif

#ifdef HASH_DMA_FUNCTION
/**
 * @brief           init dma hash with iv and updated message length
 * @param[in]       ctx                  - hash_dma_ctx_t context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       iv                   - iv or iterator after updating some blocks
 * @param[in]       byte_length_h        - high 32 bit of updated message byte length
 * @param[in]       byte_length_l        - low 32 bit of updated message byte length,
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure alg is valid
 *        2. updated message byte length must be a multiple of block byte length
 */
unsigned int hash_dma_init_with_iv_and_updated_length(hash_dma_ctx_t *ctx, hash_alg_e alg, const unsigned int *iv, unsigned int byte_length_h, unsigned int byte_length_l,
                                                      hash_callback callback)
{
    unsigned char block_word_len;

    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {
        // handle other;
    }

    block_word_len = hash_get_block_word_len(alg);
    if (0U != block_word_len)
    {
        if (0U != (byte_length_l % (CAST2UINT32(block_word_len) << 2)))
        {
            return HASH_INPUT_INVALID;
        }
        else
        {
        }
    }
    else
    {
        return HASH_INPUT_INVALID;
    }

    // clear the context
    memset_((unsigned char *)ctx, 0, sizeof(hash_dma_ctx_t));

    // init context
    ctx->alg = alg;
    ctx->block_word_len = block_word_len;
    ctx->callback = callback;
    ctx->total[0] = byte_length_l;
    ctx->total[1] = byte_length_h;

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    ctx->iterator_word_len = hash_get_iterator_word_len(alg);
    if (NULL != iv)
    {
        ctx->first_update_flag = (unsigned char)0;
        memcpy_((ctx->iterator), iv, CAST2UINT32(ctx->iterator_word_len) << 2);
    }
    else
    {
        ctx->first_update_flag = (unsigned char)1;
    }
#else
    hash_set_dma_mode();
    hash_disable_interruption();
    hash_set_alg(alg);
    hash_set_last_block(0); // set not the last block
    hash_set_iv(alg, hash_get_iterator_word_len(alg));
    hash_set_dma_output_len((unsigned int)hash_get_digest_word_len(alg) << 2);
    hash_clear_msg_len();

    // set IV
    if (NULL != iv)
    {
        hash_set_iterator(iv, hash_get_iterator_word_len(alg));
    }
    else
    {
        hash_set_iv(alg, hash_get_iterator_word_len(alg));
    }
#endif

    return HASH_SUCCESS;
}

/**
 * @brief           init dma hash
 * @param[in]       ctx                  - hash_dma_ctx_t context pointer
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 */
unsigned int hash_dma_init(hash_dma_ctx_t *ctx, hash_alg_e alg, hash_callback callback)
{
    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if (HASH_SUCCESS != check_hash_alg(alg))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {
        // handle other;
    }

    // init context
    ctx->alg = alg;
    ctx->block_word_len = hash_get_block_word_len(alg);
    ctx->callback = callback;
    uint32_clear(ctx->total, sizeof(ctx->total) / 4U);

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    ctx->iterator_word_len = hash_get_iterator_word_len(alg);
    ctx->first_update_flag = (unsigned char)1;
#else
    hash_set_dma_mode();
    hash_disable_interruption();
    hash_set_alg(alg);
    hash_set_last_block(0); // set not the last block
    hash_set_iv(alg, hash_get_iterator_word_len(alg));
    hash_set_dma_output_len((unsigned int)hash_get_digest_word_len(alg) << 2);
    hash_clear_msg_len();
#endif

    return HASH_SUCCESS;
}

/**
 * @brief           dma hash update some message blocks
 * @param[in]       ctx                  - hash_dma_ctx_t context pointer
 * @param[in]       msg                  - message blocks
 * @param[in]       msg_len            - byte length of the input message, must be a multiple of hash block byte length
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialize
 */
unsigned int hash_dma_update_blocks(hash_dma_ctx_t *ctx, const unsigned int *msg, unsigned int msg_len)
{
    if (NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if ((NULL == msg) || (0U == msg_len))
    {
        return HASH_SUCCESS;
    }
    else if (0U != (msg_len % (CAST2UINT32(ctx->block_word_len) << 2)))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {
        // handle other;
    }

    // update total byte length
    if (0U != (hash_total_byte_len_add_uint32(ctx->total, CAST2UINT32(ctx->block_word_len) / 8U, msg_len)))
    {
        return HASH_LEN_OVERFLOW;
    }
    else
    {
    }

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_last_block(0);
    hash_set_alg(ctx->alg);
    hash_set_dma_output_len(0);
    hash_set_dma_mode();

    // set the input iterator data
    if (((unsigned char)0) != (ctx->first_update_flag))
    {
        hash_set_iv(ctx->alg, ctx->iterator_word_len);

        ctx->first_update_flag = (unsigned char)0; // clear the flag
    }
    else
    {
        hash_set_iterator(ctx->iterator, ctx->iterator_word_len);
    }
#endif

    hash_dma_operate(msg, NULL, msg_len, ctx->callback);

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    // get the new iterator hash value
    hash_get_iterator((unsigned char *)(ctx->iterator), (unsigned int)(ctx->iterator_word_len));
#endif

    return HASH_SUCCESS;
}

/**
 * @brief           dma hash final(input the remainder message and get the digest)
 * @param[in]       ctx                  - hash_dma_ctx_t context pointer
 * @param[in]       msg                  - remainder message
 * @param[in]       msg_len              - byte length of the remainder message
 * @param[out]      digest               - hash digest
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid, and ctx is initialized
 *        2. if remainder_msg is NULL, or remainder_bytes is zero, in this case input valid,
 *           means the message is NULL
 */
unsigned int hash_dma_final(hash_dma_ctx_t *ctx, const unsigned int *msg, unsigned int msg_len, unsigned int *digest)
{
    unsigned int blocks_bytes;

    if ((NULL == ctx) || (NULL == digest))
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

    if ((0u == msg_len))
    {
        msg = digest;
        msg_len = 0U;
    }
    else
    {
    }

    // update total byte length
    if (0U != hash_total_byte_len_add_uint32(ctx->total, CAST2UINT32(ctx->block_word_len) / 8U, msg_len))
    {
        return HASH_LEN_OVERFLOW;
    }
    else
    {
    }

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_clear_msg_len();
    hash_set_alg(ctx->alg);
    hash_disable_interruption();
    hash_set_last_block(0);
    hash_set_dma_mode();

    // set the input iterator data
    if (0U != ctx->first_update_flag)
    {
        hash_set_iv(ctx->alg, CAST2UINT32(ctx->iterator_word_len));

        ctx->first_update_flag = (unsigned char)0; // clear the flag
    }
    else
    {
        hash_set_iterator(ctx->iterator, CAST2UINT32(ctx->iterator_word_len));
    }
#endif

    // update some whole blocks
    blocks_bytes = msg_len - (msg_len % (CAST2UINT32(ctx->block_word_len) << 2));
    msg_len = msg_len - blocks_bytes;

    if (0U != blocks_bytes)
    {
        hash_dma_operate(msg, NULL, blocks_bytes, ctx->callback);
        msg = &(msg[blocks_bytes / 4U]);
    }
    else
    {
    }

    // set total length of message
    hash_set_msg_total_byte_len(ctx->total, CAST2UINT32(ctx->block_word_len) / 8U);
    hash_set_last_block(1);
    hash_set_dma_output_len(CAST2UINT32(hash_get_digest_word_len(ctx->alg)) << 2);

    // update the remainder message(maybe empty)
    hash_dma_operate(msg, digest, msg_len, ctx->callback);

    return HASH_SUCCESS;
}

/**
 * @brief           dma hash digest calculate
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       msg                  - message
 * @param[in]       msg_len            - byte length of the message, it could be 0
 * @param[out]      digest               - hash digest
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the four parameters are valid
 */
unsigned int hash_dma(hash_alg_e alg, unsigned int *msg, unsigned int msg_len, unsigned int *digest, hash_callback callback)
{
    unsigned int ret;
    hash_dma_ctx_t ctx[1];

    if ((NULL == msg))
    {
        msg = digest;
        msg_len = 0U;
    }
    else
    {
    }

    if (NULL == digest)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {
    }

    ret = hash_dma_init(ctx, alg, callback);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    return hash_dma_final(ctx, msg, msg_len, digest);
}

#ifdef SUPPORT_HASH_DMA_NODE
/**
 * @brief           input whole message and get its digest(dma node style)
 * @param[in]       alg                  - specific hash algorithm
 * @param[in]       node                 - message node pointer
 * @param[in]       node_num             - number of hash nodes, i.e. number of message segments.
 * @param[out]      digest               - hash digest
 * @param[in]       callback             - callback function pointer
 * @return          HASH_SUCCESS(success), other(error)
 * @note
 *        1. please make sure the digest buffer is sufficient
 *        2. if the whole message consists of some segments, every segment is a node, a node includes
 *           address and byte length.
 *        3. for every node or segment except the last, its message length must be a multiple of block length
 */
unsigned int hash_dma_node_steps(hash_alg_e alg, const hash_dma_node_t *node, unsigned int node_num, unsigned int *digest, hash_callback callback)
{
    hash_dma_ctx_t ctx[1];
    unsigned int i, ret;

    ret = hash_dma_init(ctx, alg, callback);
    if (HASH_SUCCESS == ret)
    {
        for (i = 0; i < (node_num - 1U); i++)
        {
            ret = hash_dma_update_blocks(ctx, node[i].msg_addr, node[i].msg_len);
            if (HASH_SUCCESS != ret)
            {
                break;
            }
            else
            {
            }
        }

        if (HASH_SUCCESS == ret)
        {
            ret = hash_dma_final(ctx, node[i].msg_addr, node[i].msg_len, digest);
        }
    }
    else
    {
    }

    return ret;
}
#endif

#endif
