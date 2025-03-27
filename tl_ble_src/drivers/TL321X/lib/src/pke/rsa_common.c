/********************************************************************************************************
 * @file    rsa_common.c
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


#if (defined(SUPPORT_RSASSA_PSS) || defined(SUPPORT_RSAES_OAEP))

#include "lib/include/pke/rsa.h"
#include "lib/include/hash/hash_kdf.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"




/**
 * @brief       RSA PKCS#1_v2.2 MGF1(a mask generation function based on a hash function)
 * @param[in]   hash_alg     - specific hash algorithm for MGF1.
 * @param[in]   seed         - seed.
 * @param[in]   seed_bytes   - byte length of seed.
 * @param[in]   in           - this is to XOR mask, and this could be NULL.
 * @param[out]  out          - if in is NULL, this is mask directly, otherwise,this is (mask XOR in).
 * @param[in]   mask_bytes   - input, if in is NULL, this is byte length of out(mask), otherwise,
 *                             this is byte length of in or out(mask XOR in).
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.out = mask XOR in, if in is NULL, out is mask directly.
  @endverbatim
 */
unsigned int rsa_pkcs1_mgf1_with_xor_in(HASH_ALG hash_alg, unsigned char *seed, unsigned int seed_bytes, unsigned char *in,
        unsigned char *out, unsigned int mask_bytes)
{
    unsigned char counter[4] = {0,0,0,0};
    unsigned int ret;
    HASH_NODE hash_node[2] = 
    {
        {seed, seed_bytes},
        {counter, 4},
    };

    ret = ansi_x9_63_kdf_node_with_xor_in(hash_alg, hash_node, 2, counter, in, out, mask_bytes, 1);
    if(HASH_SUCCESS == ret)
    {
        return RSA_SUCCESS;
    }
    else
    {
        return ret;
    }
}

#endif

