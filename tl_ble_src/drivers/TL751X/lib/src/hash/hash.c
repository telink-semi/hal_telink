/********************************************************************************************************
 * @file    hash.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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



//HASH IV definition
#if 1

#ifdef SUPPORT_HASH_SM3
unsigned int const SM3_IV[8]         ={0x6F168073,0xB9B21449,0xD7422417,0x00068ADA,0xBC306FA9,0xAA383116,0x4DEE8DE3,0x4E0EFBB0,};
#endif

#ifdef SUPPORT_HASH_MD5
unsigned int const MD5_IV[4]         ={0x67452301,0xefcdab89,0x98badcfe,0x10325476,};
#endif

#ifdef SUPPORT_HASH_SHA256
unsigned int const SHA256_IV[8]      ={0x67E6096A,0x85AE67BB,0x72F36E3C,0x3AF54FA5,0x7F520E51,0x8C68059B,0xABD9831F,0x19CDE05B,};
#endif

#ifdef SUPPORT_HASH_SHA384
unsigned int const SHA384_IV[16]     ={0x5D9DBBCB,0xD89E05C1,0x2A299A62,0x07D57C36,0x5A015991,0x17DD7030,0xD8EC2F15,0x39590EF7,
                                   0x67263367,0x310BC0FF,0x874AB48E,0x11155868,0x0D2E0CDB,0xA78FF964,0x1D48B547,0xA44FFABE,};
#endif

#ifdef SUPPORT_HASH_SHA512
unsigned int const SHA512_IV[16]     ={0x67E6096A,0x08C9BCF3,0x85AE67BB,0x3BA7CA84,0x72F36E3C,0x2BF894FE,0x3AF54FA5,0xF1361D5F,
                                   0x7F520E51,0xD182E6AD,0x8C68059B,0x1F6C3E2B,0xABD9831F,0x6BBD41FB,0x19CDE05B,0x79217E13,};
#endif

#ifdef SUPPORT_HASH_SHA1
unsigned int const SHA1_IV[5]        ={0x01234567,0x89ABCDEF,0xFEDCBA98,0x76543210,0xF0E1D2C3,};
#endif

#ifdef SUPPORT_HASH_SHA224
unsigned int const SHA224_IV[8]      ={0xD89E05C1,0x07D57C36,0x17DD7030,0x39590EF7,0x310BC0FF,0x11155868,0xA78FF964,0xA44FFABE,};
#endif

#ifdef SUPPORT_HASH_SHA512_224
unsigned int const SHA512_224_IV[16] ={0xC8373D8C,0xA24D5419,0x6699E173,0xD6D4DC89,0xAEB7FA1D,0x829CFF32,0x14D59D67,0xCF9F2F58,
                                   0x692B6D0F,0xA84DD47B,0x736FE377,0x4289C404,0xA8859D3F,0xC8361D6A,0xADE61211,0xA192D691,};
#endif

#ifdef SUPPORT_HASH_SHA512_256
unsigned int const SHA512_256_IV[16] ={0x94213122,0x2CF72BFC,0xA35F559F,0xC2644CC8,0x6BB89323,0x51B1536F,0x19773896,0xBDEA4059,
                                   0xE23E2896,0xE3FF8EA8,0x251E5EBE,0x92398653,0xFC99012B,0xAAB8852C,0xDC2DB70E,0xA22CC581,};
#endif

#else

#ifdef SUPPORT_HASH_SM3
unsigned int const SM3_IV[8]         ={0x7380166f,0x4914b2b9,0x172442d7,0xda8a0600,0xa96f30bc,0x163138aa,0xe38dee4d,0xb0fb0e4e,};
#endif

#ifdef SUPPORT_HASH_MD5
unsigned int const MD5_IV[4]         ={0x01234567,0x89ABCDEF,0xFEDCBA98,0x76543210,};
#endif

#ifdef SUPPORT_HASH_SHA256
unsigned int const SHA256_IV[8]      ={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19,};
#endif

#ifdef SUPPORT_HASH_SHA384
unsigned int const SHA384_IV[16]     ={0xcbbb9d5d,0xc1059ed8,0x629a292a,0x367cd507,0x9159015a,0x3070dd17,0x152fecd8,0xf70e5939,
                                   0x67332667,0xffc00b31,0x8eb44a87,0x68581511,0xdb0c2e0d,0x64f98fa7,0x47b5481d,0xbefa4fa4,};
#endif

#ifdef SUPPORT_HASH_SHA512
unsigned int const SHA512_IV[16]     ={0x6a09e667,0xf3bcc908,0xbb67ae85,0x84caa73b,0x3c6ef372,0xfe94f82b,0xa54ff53a,0x5f1d36f1,
                                   0x510e527f,0xade682d1,0x9b05688c,0x2b3e6c1f,0x1f83d9ab,0xfb41bd6b,0x5be0cd19,0x137e2179,};
#endif

#ifdef SUPPORT_HASH_SHA1
unsigned int const SHA1_IV[5]        ={0x67452301,0xefcdab89,0x98badcfe,0x10325476,0xc3d2e1f0,};
#endif

#ifdef SUPPORT_HASH_SHA224
unsigned int const SHA224_IV[8]      ={0xc1059ed8,0x367cd507,0x3070dd17,0xf70e5939,0xffc00b31,0x68581511,0x64f98fa7,0xbefa4fa4,};
#endif

#ifdef SUPPORT_HASH_SHA512_224
unsigned int const SHA512_224_IV[16] ={0x8C3D37C8,0x19544DA2,0x73E19966,0x89DCD4D6,0x1DFAB7AE,0x32FF9C82,0x679DD514,0x582F9FCF,
                                   0x0F6D2B69,0x7BD44DA8,0x77E36F73,0x04C48942,0x3F9D85A8,0x6A1D36C8,0x1112E6AD,0x91D692A1,};
#endif

#ifdef SUPPORT_HASH_SHA512_256
unsigned int const SHA512_256_IV[16] ={0x22312194,0xFC2BF72C,0x9F555FA3,0xC84C64C2,0x2393B86B,0x6F53B151,0x96387719,0x5940EABD,
                                   0x96283EE2,0xA88EFFE3,0xBE5E1E25,0x53863992,0x2B0199FC,0x2C85B8AA,0x0EB72DDC,0x81C52CA2,};
#endif

#endif




/**
 * @brief       check whether the hash algorithm is valid or not
 * @param[in]   hash_alg            - specific hash algorithm.
 * @return      0:success     other:error
 */
unsigned int check_hash_alg(HASH_ALG hash_alg)
{
    unsigned int ret;

    switch(hash_alg)
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
 * @brief       get hash block word length
 * @param[in]   hash_alg                    - specific hash algorithm.
 * @return      hash block word length
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hash_get_block_word_len(HASH_ALG hash_alg)
{
    unsigned int block_words= 0U;

    switch(hash_alg)
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
        block_words = 16;
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
        block_words = 32;
        break;
#endif

    default:
        break;
    }

    return block_words;
}


/**
 * @brief       get hash iterator word length
 * @param[in]   hash_alg                    - specific hash algorithm.
 * @return      hash block word length
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hash_get_iterator_word_len(HASH_ALG hash_alg)
{
    unsigned int iterator_words = 0U;

    switch(hash_alg)
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
 * @brief       get hash digest word length
 * @param[in]   hash_alg                    - specific hash algorithm.
 * @return      hash block word length
 * @note
  @verbatim
      -# 1. please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hash_get_digest_word_len(HASH_ALG hash_alg)
{
    unsigned int digest_words= 0U;

    switch(hash_alg)
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
 * @brief       get hash IV pointer
 * @param[in]   hash_alg                    - specific hash algorithm.
 * @return      IV address
 */
const unsigned int *hash_get_IV(HASH_ALG hash_alg)
{
    const unsigned int *iv;

    switch(hash_alg)
    {
#ifdef SUPPORT_HASH_SM3
    case HASH_SM3:
        iv = SM3_IV;
        break;
#endif

#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
        iv = MD5_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
        iv = SHA256_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
        iv = SHA384_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
        iv = SHA1_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
        iv = SHA512_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
        iv = SHA224_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
        iv = SHA512_224_IV;
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
        iv = SHA512_256_IV;
        break;
#endif

    default:
        iv = NULL;
    }

    return iv;
}

/**
 * @brief       input hash IV
 * @param[in]   hash_alg                    - specific hash algorithm.
 * @param[in]   hash_iterator_words         - iterator word length.
 * @return      none
 */
void hash_set_IV(HASH_ALG hash_alg, unsigned int hash_iterator_words)
{
    hash_set_iterator(hash_get_IV(hash_alg), hash_iterator_words);
}

/**
 * @brief       hash message total byte length a = a+b
 * @param[in]   a                - big number a, total byte length of hash message.
 * @param[in]   a_words          - word length of buffer a.
 * @param[in]   b                - integer to be added to a.
 * @return      0:success     other:error
 */
unsigned int hash_total_byte_len_add_uint32(unsigned int *a, unsigned int a_words, unsigned int b)
{
    unsigned int i;

    for(i=0; i<a_words; i++)
    {
        a[i] += b;
        if(a[i] < b)
        {
            b = 1;
        }
        else
        {
            break;
        }
    }

    if(i == a_words)
    {
        return 1;
    }
    else if(a[a_words-1] & 0xE0000000)  //bit length overflow
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

#if 0
/**
 * @brief       transform hash message total byte length to bit length
 * @param[in/out]   a              - big number a.
 * @param[in]       a_words        - word length of buffer a.
 * @return      none
 */
void hash_total_bytelen_2_bitlen(unsigned int *a, unsigned int a_words)
{
    int i;

    for(i = a_words-1; i>0; i--)
    {
        a[i] <<= 3;
        a[i] |= a[i-1]>>(32-3);
    }

    a[i] <<= 3;
}
#endif

/**
 * @brief       start HASH iteration calc
 * @param[in]   ctx              - big number a, total byte length of hash message.
 * @return      none
 */
void hash_start_calculate(HASH_CTX *ctx)
{
    //if it is the first time to calculate, set the IV
    if(ctx->first_update_flag)
    {
        hash_set_IV(ctx->hash_alg, ctx->iterator_word_len);

        ctx->first_update_flag = 0;   //clear the flag
    }
    else
    {;}

    hash_start();
}


/**
 * @brief       init HASH
 * @param[in]   ctx                     - HASH_CTX context pointer.
 * @param[in]   hash_alg                - specific hash algorithm.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  please make sure hash_alg is valid.
  @endverbatim
 */
unsigned int hash_init(HASH_CTX *ctx, HASH_ALG hash_alg)
{
    if(NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if(HASH_SUCCESS != check_hash_alg(hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {;}
    
    //clear the context
    memset_(ctx, 0, sizeof(HASH_CTX));
    //uint32_clear(ctx->total, sizeof(ctx->total)/4);

    hash_set_cpu_mode();
    hash_disable_interruption();
    hash_set_last_block(0);//set not the last block
    hash_set_alg(hash_alg);

    hash_clear_msg_len();

    //set context config
    ctx->hash_alg = hash_alg;
    ctx->block_byte_len = hash_get_block_word_len(hash_alg)<<2;
    ctx->iterator_word_len = hash_get_iterator_word_len(hash_alg);
    ctx->digest_byte_len = hash_get_digest_word_len(hash_alg)<<2;
    ctx->status.busy = 0;
    ctx->first_update_flag = 1;
    ctx->finish_flag = 0;

    return HASH_SUCCESS;
}


/**
 * @brief       hash iterate calc with some blocks
 * @param[in]   ctx                     - HASH_CTX context pointer.
 * @param[in]   msg                     - message of some blocks.
 * @param[in]   block_count             - count of blocks.
 * @return      none
 * @note
  @verbatim
      -# 1.  please make sure the three parameters is valid.
  @endverbatim
 */
void hash_calc_blocks(HASH_CTX *ctx, const unsigned char *msg, unsigned int block_count)
{
    unsigned int block_word_len = (ctx->block_byte_len)>>2;

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    //set the input iterator data
    if(1 != ctx->first_update_flag)
    {
        hash_set_iterator(ctx->iterator, ctx->iterator_word_len);
    }
    else
    {;}
#endif

    while(block_count--)
    {
        hash_input_msg(msg, block_word_len);

        hash_start_calculate(ctx);

        msg += ctx->block_byte_len;

        hash_wait_till_done();
    }

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    //if message update not done, get the new iterator hash value
    if(1 != ctx->finish_flag)
    {
        hash_get_iterator((unsigned char *)(ctx->iterator), ctx->iterator_word_len);
    }
    else
    {;}
#endif
}


/**
 * @brief       hash iterate calc with some blocks
 * @param[in]   ctx                - HASH_CTX context pointer.
 * @param[in]   msg                - message that contains the last block(maybe not full).
 * @param[in]   msg_bytes          - byte length of msg.
 * @return      none
 * @note
  @verbatim
      -# 1.  msg contains the last byte of the total message while the total message length is not a
        multiple of hash block length, otherwise byte length of msg is zero.
      -# 2.  at present this function does not support the case that byte length of msg is a multiple
        of hash block length. actually msg_bytes here must be less than the hash block byte length,
        namely, this function is just for the remainder message, and will do padding, finally get
        digest.
      -# 3.  before calling this function, some blocks(could be 0 block) must be calculated.
  @endverbatim
 */
void hash_calc_rand_len_msg(HASH_CTX *ctx, const unsigned char *msg, unsigned int msg_bytes)
{
#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    //set the input iterator data
    if(1 != ctx->first_update_flag)
    {
        hash_set_iterator(ctx->iterator, ctx->iterator_word_len);
    }
    else
    {;}
#endif

    hash_set_last_block(1);

    hash_input_msg(msg, (msg_bytes+3)/4);

    hash_start_calculate(ctx);

    hash_wait_till_done();
}


/**
 * @brief       hash iterate calc with some blocks
 * @param[in]   ctx                - HASH_CTX context pointer.
 * @param[in]   msg                - message.
 * @param[in]   msg_bytes          - byte length of the input message.
 * @return      none
 * @note
  @verbatim
      -# 1.  please make sure the three parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hash_update(HASH_CTX *ctx, const unsigned char *msg, unsigned int msg_bytes)
{
    unsigned int count;
    unsigned char left, fill;

    if((NULL == ctx))
    {
        return HASH_BUFFER_NULL;
    }
    else if((NULL == msg) || (0 == msg_bytes))
    {
        return HASH_SUCCESS;
    }
    else
    {;}

    ctx->status.busy = 1;                             //start to update processing

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_cpu_mode();
    hash_set_last_block(0);//set not the last block
    hash_set_alg(ctx->hash_alg);
#endif

    left = ctx->total[0] % (ctx->block_byte_len);    //byte length of valid message left in block buffer
    fill = (ctx->block_byte_len) - left;             //byte length that block buffer need to fill a block

    //update total byte length
    if(hash_total_byte_len_add_uint32(ctx->total, ctx->block_byte_len/32, msg_bytes))
    {
        return HASH_LEN_OVERFLOW;
    }
    else
    {;}

    if(left)
    {
        if(msg_bytes >= fill)
        {
            memcpy_(ctx->hash_buffer + left, msg, fill);
            hash_calc_blocks(ctx, ctx->hash_buffer, 1);
            msg_bytes -= fill;
            msg += fill;
        }
        else
        {
            memcpy_(ctx->hash_buffer + left, msg, msg_bytes);
            goto END;
        }
    }
    else
    {;}

    //process some blocks
    count = msg_bytes/(ctx->block_byte_len);
    if(count)
    {
        hash_calc_blocks(ctx, msg, count);
    }
    else
    {;}

    //process the remainder
    msg_bytes = msg_bytes % (ctx->block_byte_len);
    if(msg_bytes)
    {
        msg += (ctx->block_byte_len)*count;
        memcpy_(ctx->hash_buffer,msg, msg_bytes);
    }
    else
    {;}

END:
    ctx->status.busy = 0;   //update end, status becomes idle

    return HASH_SUCCESS;
}


/**
 * @brief       message update done, get the digest
 * @param[in]   ctx                - HASH_CTX context pointer.
 * @param[out]  digest             - hash digest.
 * @return      none
 * @note
  @verbatim
      -# 1.  please make sure the ctx is valid and initialized.
      -# 2.  please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int hash_final(HASH_CTX *ctx, unsigned char *digest)
{
    unsigned char tmp;

    if((NULL == ctx) || (NULL == digest))
    {
        return HASH_BUFFER_NULL;
    }
    else
    {;}

#ifdef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_cpu_mode();

    hash_disable_interruption();

    //hash_set_last_block(0);//set not the last block

    hash_set_alg(ctx->hash_alg);
#endif

    // set total length of message
    hash_set_msg_total_byte_len(ctx->total, ctx->block_byte_len/32);

    ctx->finish_flag = 1;

    //get the byte length of the remainder msg(less than one block)
    tmp = ctx->total[0] % (ctx->block_byte_len);

    //input the remainder msg(less than one block)
    hash_calc_rand_len_msg(ctx, ctx->hash_buffer, tmp);

    //get the hash result
    hash_get_iterator(digest, (ctx->digest_byte_len)>>2);

    //clear the context
    memset_(ctx, 0, sizeof(HASH_CTX));

    return HASH_SUCCESS;
}


/**
 * @brief       message update done, get the digest
 * @param[in]   hash_alg           - specific hash algorithm.
 * @param[in]   msg                - message.
 * @param[in]   msg_bytes          - byte length of the input message, it could be 0.
 * @param[out]  digest             - hash digest.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  please make sure the digest buffer is sufficient.
  @endverbatim
 */
unsigned int hash(HASH_ALG hash_alg, unsigned char *msg, unsigned int msg_bytes, unsigned char *digest)
{
    HASH_CTX ctx[1];
    unsigned int ret;

    ret = hash_init(ctx,  hash_alg);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = hash_update(ctx, msg, msg_bytes);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = hash_final(ctx, digest);
    if(HASH_SUCCESS != ret)
    {
        memset_(digest, 0, ctx->digest_byte_len);
    }
    else
    {;}

    //clear the context
    memset_(ctx, 0, sizeof(HASH_CTX));

    return ret;
}

#ifdef HASH_DMA_FUNCTION
/**
 * @brief       message update done, get the digest
 * @param[in]   ctx           - HASH_DMA_CTX context pointer.
 * @param[in]   hash_alg      - specific hash algorithm.
 * @param[in]   callback      - callback function pointer.
 * @return      0:success     other:error
 */
unsigned int hash_dma_init(HASH_DMA_CTX *ctx, HASH_ALG hash_alg, HASH_CALLBACK callback)
{
    if(NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if(HASH_SUCCESS != check_hash_alg(hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {;}

    //init context
    ctx->hash_alg = hash_alg;
    ctx->block_word_len = hash_get_block_word_len(hash_alg);
    ctx->callback = callback;
    uint32_clear(ctx->total, sizeof(ctx->total)/4);

    hash_set_dma_mode();

    hash_disable_interruption();

    hash_set_alg(hash_alg);

    hash_set_last_block(0);//set not the last block

    hash_set_IV(hash_alg, hash_get_iterator_word_len(hash_alg));

    hash_set_dma_output_len(hash_get_digest_word_len(hash_alg)<<2);

    hash_clear_msg_len();

    return HASH_SUCCESS;
}


/**
 * @brief       dma hash update some message blocks
 * @param[in]   ctx                - HASH_DMA_CTX context pointer.
 * @param[in]   msg                - message blocks.
 * @param[in]   msg_words          - word length of the input message, must be a multiple of hash block word length.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hash_dma_update_blocks(HASH_DMA_CTX *ctx, unsigned int *msg, unsigned int msg_words)
{
    if(NULL == ctx)
    {
        return HASH_BUFFER_NULL;
    }
    else if((NULL == msg) || (0 == msg_words))
    {
        return HASH_SUCCESS;
    }
    else if(msg_words % ctx->block_word_len)
    {
        return HASH_INPUT_INVALID;
    }
    else
    {;}

    //update total byte length
    if(hash_total_byte_len_add_uint32(ctx->total, ctx->block_word_len/8, msg_words<<2))
    {
        return HASH_LEN_OVERFLOW;
    }
    else
    {;}

    hash_dma_operate(msg, NULL, msg_words<<2, ctx->callback);

    return HASH_SUCCESS;
}


/**
 * @brief       dma hash update some message blocks
 * @param[in]   ctx                - HASH_DMA_CTX context pointer.
 * @param[in]   remainder_msg      - message blocks.
 * @param[in]   remainder_bytes    -byte length of the input message, must be a multiple of hash block byte length.
 * @param[in]   digest              - hash digest.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  please make sure the four parameters are valid, and ctx is initialized.
  @endverbatim
 */
unsigned int hash_dma_final(HASH_DMA_CTX *ctx, unsigned int *remainder_msg, unsigned int remainder_bytes, unsigned int *digest)
{
    HASH_CTX final_ctx[1];
    unsigned int ret;

    if((NULL == ctx) || (NULL == digest))
    {
        return HASH_BUFFER_NULL;
    }
    else
    {;}

    if((NULL == remainder_msg))
    {
        remainder_bytes = 0;
    }
    else
    {;}

    if(remainder_bytes >= ((unsigned int)(ctx->block_word_len<<2)))
    {
        return HASH_INPUT_INVALID;
    }
    else
    {;}

    //for some remainder_msg length, transfer to CPU style
    if(remainder_bytes >= ((unsigned int)(ctx->block_word_len<<2)-3))
    {
        ret = hash_init(final_ctx, ctx->hash_alg);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        //do not input IV, since IV or iterator is held in hardware
        final_ctx->first_update_flag = 0;

        //keep total message length
        uint32_copy(final_ctx->total, ctx->total, HASH_TOTAL_LEN_MAX_WORD_LEN);

        ret = hash_update(final_ctx, (unsigned char *)remainder_msg, remainder_bytes);
        if(HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = hash_final(final_ctx, (unsigned char *)digest);
        if(HASH_SUCCESS != ret)
        {
            memset_(digest, 0, final_ctx->digest_byte_len);
            return ret;
        }
        else
        {;}
    }
    else  //for other cases, keep DMA style
    {
        // update total byte length
        if(hash_total_byte_len_add_uint32(ctx->total, ctx->block_word_len/8, remainder_bytes))
        {
            return HASH_LEN_OVERFLOW;
        }
        else
        {;}

        // set total length of message
        hash_set_msg_total_byte_len(ctx->total, ctx->block_word_len/8);

        hash_set_last_block(1);

        hash_set_dma_output_len(hash_get_digest_word_len(ctx->hash_alg)<<2);

        hash_dma_operate(remainder_msg, digest, remainder_bytes, NULL);
    }

    return HASH_SUCCESS;
}


/**
 * @brief       dma hash digest calculate
 * @param[in]   hash_alg       - specific hash algorithm.
 * @param[in]   msg            - message.
 * @param[in]   msg_bytes      - byte length of the message, it could be 0.
 * @param[in]   digest         - hash digest.
 * @param[in]   callback       - callback function pointer.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.  please make sure the four parameters are valid.
  @endverbatim
 */
unsigned int hash_dma(HASH_ALG hash_alg, unsigned int *msg, unsigned int msg_bytes, unsigned int *digest, HASH_CALLBACK callback)
{
    unsigned int blocks_words, remainder_bytes;
    unsigned int ret;
    HASH_DMA_CTX ctx[1];

    if((NULL == msg))
    {
        msg_bytes = 0;
    }
    else
    {;}

    if(NULL == digest)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {;}

    ret = hash_dma_init(ctx, hash_alg, callback);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    remainder_bytes = msg_bytes % (ctx->block_word_len<<2);
    blocks_words = (msg_bytes - remainder_bytes)/4;

    ret = hash_dma_update_blocks(ctx, (unsigned int *)msg, blocks_words);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        return hash_dma_final(ctx, (unsigned int *)(msg+blocks_words), remainder_bytes, digest);
    }
}

#endif


