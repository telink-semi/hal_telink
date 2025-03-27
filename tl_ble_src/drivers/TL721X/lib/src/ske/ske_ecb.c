/********************************************************************************************************
 * @file    ske_ecb.c
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


#include "lib/include/ske/ske_ecb.h"




#ifdef SUPPORT_SKE_MODE_ECB


unsigned int ske_lp_ecb_init(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx)
{
    return ske_lp_init(alg, SKE_MODE_ECB, crypto, key, sp_key_idx, NULL);
}

unsigned int ske_lp_ecb_update_blocks(unsigned char *in, unsigned char *out, unsigned int bytes)
{
    return ske_lp_update_blocks(in, out, bytes);
}

unsigned int ske_lp_ecb_final(void)
{
    return ske_lp_final();
}

unsigned int ske_lp_ecb_crypto(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx,
        unsigned char *in, unsigned char *out, unsigned int bytes)
{
    return ske_lp_crypto(alg, SKE_MODE_ECB, crypto, key, sp_key_idx, NULL, in, out, bytes);
}


#ifdef SKE_LP_DMA_FUNCTION
unsigned int ske_lp_dma_ecb_init(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx)
{
    return ske_lp_dma_init(alg, SKE_MODE_ECB, crypto, key, sp_key_idx, NULL);
}

unsigned int ske_lp_dma_ecb_update_blocks(unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    return ske_lp_dma_update_blocks(in, out, words, callback);
}

unsigned int ske_lp_dma_ecb_final(void)
{
    return ske_lp_dma_final();
}

unsigned int ske_lp_dma_ecb_crypto(SKE_ALG alg, SKE_CRYPTO crypto, unsigned char *key, unsigned short sp_key_idx,
        unsigned int *in, unsigned int *out, unsigned int words, SKE_CALLBACK callback)
{
    return ske_lp_dma_crypto(alg, SKE_MODE_ECB, crypto, key, sp_key_idx, NULL, in, out, words, callback);
}
#endif


#endif

