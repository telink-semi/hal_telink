
/*! @file rsa_common.c */
#include "lib/include/pke/pke_config.h"

#if (defined(SUPPORT_RSASSA_PSS) || defined(SUPPORT_RSAES_OAEP))

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/hash_hmac/hash_kdf.h"
#include "../../crypto_include/pke/rsa.h"

/**
 * @brief           RSA PKCS#1_v2.2 MGF1(a mask generation function based on a hash function)
 * @param[in]       hash_alg             - specific hash algorithm for MGF1.
 * @param[in]       seed                 - seed.
 * @param[in]       seed_bytes           - byte length of seed.
 * @param[in]       in                   - this is to XOR mask, and this could be NULL.
 * @param[out]      out                  - if in is NULL, this is mask directly, otherwise,this is (mask XOR in).
 * @param[in]       mask_bytes           - input, if in is NULL, this is byte length of out(mask), otherwise,
 *                                         this is byte length of in or out(mask XOR in).
 * @return          0:success     other:error
 * @note
 *        1.out = mask XOR in, if in is NULL, out is mask directly.
 */
unsigned int rsa_pkcs1_mgf1_with_xor_in(hash_alg_e hash_alg, const unsigned char *seed, unsigned int seed_bytes, const unsigned char *in, unsigned char *out,
                                        unsigned int mask_bytes)
{
    unsigned char counter_buf[4] = {0, 0, 0, 0};
    const unsigned char *counter = counter_buf;
    unsigned int ret;
#if 0
    hash_node_t digest_node[2] = 
    {
        {seed, seed_bytes},
        {counter, 4u},
    };
#else
    hash_node_t digest_node[2];

    digest_node[0].msg_addr = seed;
    digest_node[0].msg_len = seed_bytes;
    digest_node[1].msg_addr = counter;
    digest_node[1].msg_len = 4u;
#endif

    ret = ansi_x9_63_kdf_node_with_xor_in(hash_alg, digest_node, 2, counter_buf, in, out, mask_bytes, 1);
    if (HASH_SUCCESS == ret)
    {
        ret = RSA_SUCCESS;
    }
    else
    {
    }

    return ret;
}

#endif
