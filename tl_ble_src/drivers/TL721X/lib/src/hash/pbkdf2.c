/********************************************************************************************************
 * @file    pbkdf2.c
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
#include "lib/include/crypto_common/utility.h"
#include "lib/include/hash/pbkdf2.h"



#define PBKDF2_HIGH_SPEED



#ifdef PBKDF2_HIGH_SPEED
/**
 * @brief       dma md5 digest calculate
 * @param[out]  ctx_bak            - hmac ctx.
 * @param[in]   ctx                - hmac ctx to be backup.
 * @return      0:success     other:error
 */
void pbkdf2_hmac_backup(HMAC_CTX *ctx_bak, HMAC_CTX *ctx)
{
    memcpy_((unsigned char *)ctx_bak, (unsigned char *)ctx, sizeof(HMAC_CTX));

#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_get_iterator((unsigned char *)(ctx_bak->hash_ctx->hash_buffer), ctx_bak->hash_ctx->iterator_word_len);
#endif
}
#endif


#ifdef PBKDF2_HIGH_SPEED
/**
 * @brief       dma md5 digest calculate
 * @param[out]  ctx            - hmac ctx to be recover.
 * @param[in]   ctx_bak        - hmac ctx.
 * @return      0:success     other:error
 */
void pbkdf2_hmac_recover(HMAC_CTX *ctx, HMAC_CTX *ctx_bak)
{
    memcpy_((unsigned char *)ctx, (unsigned char *)ctx_bak, sizeof(HMAC_CTX));

#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
    hash_set_iterator((unsigned int *)(ctx_bak->hash_ctx->hash_buffer), ctx_bak->hash_ctx->iterator_word_len);
#endif
}
#endif

/**
 * @brief       pbkdf2 function(using hmac as PRF)
 * @param[in]   hash_alg         - specific hash algorithm.
 * @param[in]   pwd              - password, as the key of hmac.
 * @param[in]   sp_key_idx       - index of secure port key(password).
 * @param[in]   pwd_bytes        - byte length of password, it could be 0.
 * @param[in]   salt             - salt.
 * @param[in]   salt_bytes       - byte length of salt, it could be 0.
 * @param[in]   iter             - iteration times.
 * @param[out]  out              - derived key.
 * @param[in]   out_bytes        - byte length of derived key.
 * @return      0:success     other:error
 */
unsigned int pbkdf2_hmac(HASH_ALG hash_alg, unsigned char *pwd, unsigned short sp_key_idx, unsigned int pwd_bytes, unsigned char *salt, unsigned int salt_bytes,
        unsigned int iter, unsigned char *out, unsigned int out_bytes)
{
    unsigned int i;
    int j;
    unsigned int digest_words, digest_bytes, tmp_bytes;
    unsigned char counter[4] = {0,0,0,1};
    unsigned int tmp[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int mac[HASH_DIGEST_MAX_WORD_LEN];
    unsigned int ret;

    HMAC_CTX ctx[1];

#ifdef PBKDF2_HIGH_SPEED
    HMAC_CTX ctx_bak[1];
#endif

    if(HASH_SUCCESS != check_hash_alg(hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if(NULL == out)
    {
        return HASH_BUFFER_NULL;
    }
    else
    {;}

    if(NULL == pwd)
    {
        pwd_bytes = 0;
    }
    else
    {;}

    if(NULL == salt)
    {
        salt_bytes = 0;
    }
    else
    {;}

    digest_words = hash_get_digest_word_len(hash_alg);
    digest_bytes = 4*digest_words;

#ifdef PBKDF2_HIGH_SPEED
    ret  = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
    if(HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    pbkdf2_hmac_backup(ctx_bak, ctx);
#endif

    while(out_bytes)
    {
        if(out_bytes > digest_bytes)
        {
            tmp_bytes = digest_bytes;
        }
        else
        {
            tmp_bytes = out_bytes;
        }

        //get U1
#ifdef PBKDF2_HIGH_SPEED
        pbkdf2_hmac_recover(ctx, ctx_bak);
        ret = 0;
#else
        ret  = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
#endif
        ret |= hash_update(ctx->hash_ctx, salt, salt_bytes);//hmac_update(ctx, salt, salt_bytes);
        ret |= hash_update(ctx->hash_ctx, counter, 4);      //hmac_update(ctx, counter, 4);
        ret |= hmac_final(ctx, (unsigned char *)mac);
        if(HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {;}

        uint32_copy(tmp, mac, digest_words);

        for(i=1; i<iter; i++)
        {
#ifdef PBKDF2_HIGH_SPEED
            pbkdf2_hmac_recover(ctx, ctx_bak);
            ret = 0;
#else
            ret  = hmac_init(ctx, hash_alg, pwd, sp_key_idx, pwd_bytes);
#endif
            ret |= hash_update(ctx->hash_ctx, (unsigned char *)tmp, digest_bytes);
            ret |= hmac_final(ctx, (unsigned char *)tmp);
            if(HASH_SUCCESS != ret)
            {
                goto END;
            }
            else
            {;}

            for(j=0; j<(int)digest_words; j++)
            {
                mac[j] ^= tmp[j];
            }
        }

        //add 1
        for(j=3; j>-1; j--)
        {
            counter[j] += 1;
            if(counter[j] != 0)
            {
                break;
            }
        }

        memcpy_(out, (unsigned char *)mac, tmp_bytes);

        out += tmp_bytes;
        out_bytes -= tmp_bytes;
    }

    ret = HASH_SUCCESS;

END:

    return ret;
}

