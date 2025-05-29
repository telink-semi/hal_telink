/********************************************************************************************************
 * @file    dh.c
 *
 * @brief   This is the source file for tl322x
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


#ifdef SUPPORT_DH

    #include "lib/include/pke/dh.h"
    #include "lib/include/crypto_common/utility.h"

/**
 * @brief       DH compute key
 * @param[in]   dh_para         - DH_PARA struct pointer.
 * @param[in]   local_prikey    - local private key, big-endian.
 * @param[in]   peer_pubkey     - peer public key, big-endian.
 * @param[out]  key             - output key.
 * @return      DH_SUCCESS(success)     other:error
 * @note
   @verbatim
      -# 1.local_prikey occupies (dh_para->q_bits+7)/8 bytes.
      -# 2. peer_pubkey and key occupy (dh_para->p_bits+7)/8 bytes.
   @endverbatim
 */
unsigned int dh_compute_key(DH_PARA *dh_para, unsigned char *local_prikey, unsigned char *peer_pubkey, unsigned char *key)
{
    unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int prikey[DH_MAX_WORD_LEN];
    unsigned int pubkey[DH_MAX_WORD_LEN];
    unsigned int step_bytes, pByteLen, qByteLen, pWordLen, qWordLen;
    unsigned int ret;

    if ((NULL == dh_para) || (NULL == local_prikey) || (NULL == peer_pubkey) || (NULL == key)) {
        return DH_POINTER_NULL;
    } else {
        ;
    }

    pByteLen = GET_BYTE_LEN(dh_para->p_bits);
    qByteLen = GET_BYTE_LEN(dh_para->q_bits);
    pWordLen = GET_WORD_LEN(dh_para->p_bits);
    qWordLen = GET_WORD_LEN(dh_para->q_bits);

    prikey[qWordLen - 1u] = 0u;
    reverse_byte_array(local_prikey, (unsigned char *)prikey, qByteLen);

    //get tmp = q-1
    uint32_copy(tmp, dh_para->q, qWordLen);
    tmp[0u] -= 1u;

    //make sure private key is in [2, q-2]
    if (get_valid_bits(prikey, qWordLen) <= 1) {
        return DH_INVALID_INPUT;
    } else if (uint32_BigNumCmp(prikey, qWordLen, tmp, qWordLen) >= 0) {
        return DH_INVALID_INPUT;
    } else {
        ;
    }

    //get tmp = p-1
    uint32_copy(tmp, dh_para->p, pWordLen);
    tmp[0u] -= 1u;

    pubkey[pWordLen - 1u] = 0u;
    reverse_byte_array(peer_pubkey, (unsigned char *)pubkey, pByteLen);

    //check public key
    ret = dh_check_public_key(dh_para, tmp, pubkey);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    ret        = pke_modexp((unsigned int *)(rPKE_B(3u, step_bytes)), prikey, pubkey, tmp, pWordLen, qWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    reverse_byte_array((unsigned char *)tmp, (unsigned char *)key, pByteLen);

    return DH_SUCCESS;
}

#endif
