/*! @file dh.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_DH

#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/dh.h"

/**
 * @brief           DH compute key
 * @param[in]       dh_para              - DH_PARA struct pointer.
 * @param[in]       local_prikey         - local private key, big-endian.
 * @param[in]       peer_pubkey          - peer public key, big-endian.
 * @param[out]      key                  - output key.
 * @return          DH_SUCCESS(success)     other:error
 * @note
 *        1.local_prikey occupies (dh_para->q_bits+7)/8 bytes.
 *        2. peer_pubkey and key occupy (dh_para->p_bits+7)/8 bytes.
 */
unsigned int dh_compute_key(const dh_para_t *dh_para, const unsigned char *local_prikey, const unsigned char *peer_pubkey, unsigned char *key)
{
    unsigned int tmp[DH_MAX_WORD_LEN];
    unsigned int prikey[DH_MAX_WORD_LEN];
    unsigned int pubkey[DH_MAX_WORD_LEN];
    unsigned int step_bytes, p_len, q_len, p_wlen, q_wlen;
    unsigned int ret;

    if ((NULL == dh_para) || (NULL == local_prikey) || (NULL == peer_pubkey) || (NULL == key))
    {
        return DH_POINTER_NULL;
    }
    else
    {
        ;
    }

    p_len = get_byte_len(dh_para->p_bits);
    q_len = get_byte_len(dh_para->q_bits);
    p_wlen = get_word_len(dh_para->p_bits);
    q_wlen = get_word_len(dh_para->q_bits);

    prikey[q_wlen - 1u] = 0u;
    reverse_byte_array(local_prikey, (unsigned char *)prikey, q_len);

    // get tmp = q-1
    uint32_copy(tmp, dh_para->q, q_wlen);
    tmp[0u] -= 1u;

    // make sure private key is in [2, q-2]
    if (get_valid_bits(prikey, q_wlen) <= 1)
    {
        return DH_INVALID_INPUT;
    }
    else if (uint32_big_num_cmp(prikey, q_wlen, tmp, q_wlen) >= 0)
    {
        return DH_INVALID_INPUT;
    }
    else
    {
        ;
    }

    // get tmp = p-1
    uint32_copy(tmp, dh_para->p, p_wlen);
    tmp[0u] -= 1u;

    pubkey[p_wlen - 1u] = 0u;
    reverse_byte_array(peer_pubkey, (unsigned char *)pubkey, p_len);

    // check public key
    ret = dh_check_public_key(dh_para, tmp, pubkey);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    ret = pke_modexp((unsigned int *)(rPKE_B(3u, step_bytes)), prikey, pubkey, tmp, p_wlen, q_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    reverse_byte_array((unsigned char *)tmp, (unsigned char *)key, p_len);

    return DH_SUCCESS;
}

#endif
