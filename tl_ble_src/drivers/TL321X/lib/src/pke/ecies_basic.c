/********************************************************************************************************
 * @file    ecies_basic.c
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


#ifdef SUPPORT_ECIES

    #include "lib/include/pke/ecies.h"
    #include "lib/include/crypto_common/utility.h"
    #include "lib/include/trng/trng.h"
    #include "lib/include/hash/hash_kdf.h"

/*************************************************************************************
 *                       KDF function Implementation
 *************************************************************************************/
/**
 * @brief       ANSI-X9.63 KDF function core, this will be used in ANSI-X963 KDF,
 *              IEEE 1363a KDF2, ISO 18033-2 kdf1, ISO 18033-2 kdf2, etc.
 * @param[in]   hash_alg            - hash algorithm used in KDF.
 * @param[in]   Z                   - shared secret value, such as DH shared secret value,
 *                                    the initial key to be extended in ECIES.
 * @param[in]   Z_bytes             - byte length of Z.
 * @param[in]   counter             - initial counter value, 4 bytes, big-endian.
 * @param[in]   shared_info         - additional shared information, this is optional.
 * @param[in]   shared_info_bytes   - byte length of shared_info.
 * @param[out]  k1                  - extended key k1, actually the whole output is k1||k2.
 * @param[in]   k1_bytes            - byte length of k1.
 * @param[in]   k2                  - extended key k2, actually the whole output is k1||k2.
 * @param[in]   k2_bytes            - byte length of k2.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.for standard ANSI-X9.63, the initial counter is {0x00,0x00,0x00,0x01}.
      -# 2.shared_info is optional, if there is no such item, please set this parameter to NULL,
           and set shared_info_bytes to 0.
      -# 3.the whole output is k1||k2, if your output is entire, please set k1 and k1_bytes as your
           output parameters, and set K2 and k2_bytes to NULL and 0 respectively.
  @endverbatim
 */
unsigned int ansi_x963_kdf_core(HASH_ALG hash_alg, unsigned char *Z, unsigned int Z_bytes, unsigned char *counter, unsigned char *shared_info, unsigned int shared_info_bytes, unsigned char *k1, unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    HASH_NODE hash_node[3] = {
        {Z,           Z_bytes          },
        {counter,     4                },
        {shared_info, shared_info_bytes},
    };

    return ansi_x9_63_kdf_node(hash_alg, hash_node, 3, counter, k1, k1_bytes, k2, k2_bytes);
}

/**
 * @brief       ANSI-X963 KDF function.
 *              this function refers to ANSI-X9.63-2001, or SEC1-v2-2009 section 3.6.1, or rfc8418 section-2.1
 * @param[in]   hash_alg            - hash algorithm used in KDF.
 * @param[in]   Z                   - shared secret value, such as DH shared secret value,
 *                                    the initial key to be extended in ECIES.
 * @param[in]   Z_bytes             - byte length of Z.
 * @param[in]   shared_info         - additional shared information, this is optional.
 * @param[in]   shared_info_bytes   - byte length of shared_info.
 * @param[out]  k1                  - extended key k1, actually the whole output is k1||k2.
 * @param[in]   k1_bytes            - byte length of k1.
 * @param[out]  k2                  - extended key k2, actually the whole output is k1||k2.
 * @param[in]   k2_bytes            - byte length of k2.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.for standard ANSI-X9.63, the initial counter is {0x00,0x00,0x00,0x01}.
      -# 2.shared_info is optional, if there is no such item, please set this parameter to NULL,
           and set shared_info_bytes to 0.
      -# 3.the whole output is k1||k2, if your output is entire, please set k1 and k1_bytes as your
           output parameters, and set K2 and k2_bytes to NULL and 0 respectively.
  @endverbatim
 */
unsigned int ansi_x963_2001_kdf(HASH_ALG hash_alg, unsigned char *Z, unsigned int Z_bytes, unsigned char *shared_info, unsigned int shared_info_bytes, unsigned char *k1, unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char counter[4] = {0x00, 0x00, 0x00, 0x01};

    return ansi_x963_kdf_core(hash_alg, Z, Z_bytes, counter, shared_info, shared_info_bytes, k1, k1_bytes, k2, k2_bytes);
}

/**
 * @brief       NIST-SP800-56A-Concatenation-KDF function
 *              this function refers to NIST.SP.800-56Ar1.pdf  chapter 5.8.1
 * @param[in]   hash_alg            - hash algorithm used in KDF.
 * @param[in]   Z                   - shared secret value, such as DH shared secret value,
 *                                    the initial key to be extended in ECIES.
 * @param[in]   Z_bytes             - byte length of Z.
 * @param[in]   other_info          - additional other information.
 * @param[in]   other_info_bytes    - byte length of other_info.
 * @param[out]  k1                  - extended key k1, actually the whole output is k1||k2.
 * @param[in]   k1_bytes            - byte length of k1.
 * @param[out]  k2                  - extended key k2, actually the whole output is k1||k2.
 * @param[in]   k2_bytes            - byte length of k2.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.for standard ANSI-X9.63, the initial counter is {0x00,0x00,0x00,0x01}.
      -# 2.other_info is not optional, it consist of some items by concatenation, please see NIST-SP800-56Ar1.
      -# 3.the whole output is k1||k2, if your output is entire, please set k1 and k1_bytes as your
           output parameters, and set K2 and k2_bytes to NULL and 0 respectively.
  @endverbatim
 */
unsigned int nist_sp800_56a_concatenation_kdf(HASH_ALG hash_alg, unsigned char *Z, unsigned int Z_bytes, unsigned char *other_info, unsigned int other_info_bytes, unsigned char *k1, unsigned int k1_bytes, unsigned char *k2, unsigned int k2_bytes)
{
    unsigned char counter[4] = {0x00, 0x00, 0x00, 0x01};

    return ansi_x963_kdf_core(hash_alg, Z, Z_bytes, counter, other_info, other_info_bytes, k1, k1_bytes, k2, k2_bytes);
}

/*************************************************************************************
 *                       KDF structure Implementation
 *  xxx_imp is kdf structure function interface.
 *************************************************************************************/

void kdf_base_init(E_KDF_BASE *base_ctx, E_KDF_TYPE kdf_type)
{
    memset_((unsigned char *)base_ctx, 0, sizeof(E_KDF_BASE));

    base_ctx->kdf_type = kdf_type;
}

unsigned int ansi_x963_kdf_imp(E_KDF_BASE *self)
{
    E_KDF_ANSI_X963_2001_CTX *kdf_ctx = (E_KDF_ANSI_X963_2001_CTX *)self;
    unsigned int              ret;
    //print_buf_U8(kdf_ctx->base.input,kdf_ctx->base.input_bytes, "msg");print_buf_U8(kdf_ctx->shared_info,kdf_ctx->shared_info_bytes, "msg");
    ret = ansi_x963_2001_kdf(kdf_ctx->hash_alg, kdf_ctx->base.input, kdf_ctx->base.input_bytes, kdf_ctx->shared_info, kdf_ctx->shared_info_bytes, kdf_ctx->base.out1, kdf_ctx->base.out1_bytes, kdf_ctx->base.out2, kdf_ctx->base.out2_bytes);
    if (HASH_SUCCESS == ret) { //    print_buf_U8(kdf_ctx->base.out1,kdf_ctx->base.out1_bytes, "out1");print_buf_U8(kdf_ctx->base.out2,kdf_ctx->base.out2_bytes, "out2");
        return ECIES_SUCCESS;
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       ANSI-X963 KDF CTX init.
 * @param[in]   kdf_ctx             - ctx to be initialized.
 * @param[in]   shared_info         - optional, shared secret value.
 * @param[in]   shared_info_bytes   - byte length of shared_info.
 * @param[out]  hash_alg            - hash algorithm used in KDF.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.after this initialization, before kdf calculation, the following fields of
 *       (E_KDF_BASE *)kdf_ctx must be set by hand.
 *       input, input_bytes, out1, out1_bytes.
 *       out2 and out2_bytes could be ignored if the whole kdf output is in out1, otherwise
 *       these two also must be initialized.
  @endverbatim
 */
void ansi_x963_2001_kdf_init(E_KDF_ANSI_X963_2001_CTX *kdf_ctx, unsigned char *shared_info, unsigned int shared_info_bytes, HASH_ALG hash_alg)
{
    kdf_base_init((E_KDF_BASE *)kdf_ctx, X963_KDF);

    kdf_ctx->base.kdf_fun_imp = ansi_x963_kdf_imp;

    kdf_ctx->shared_info       = shared_info;
    kdf_ctx->shared_info_bytes = shared_info_bytes;
    kdf_ctx->hash_alg          = hash_alg;
}

/*************************************************************************************
 *                       ENC structure Implementation
 *************************************************************************************/

void enc_base_init(E_ENC_BASE *base_ctx, unsigned int key_bytes, unsigned char *input, unsigned int input_bytes, ENC_TYPE type)
{
    memset_((unsigned char *)base_ctx, 0, sizeof(E_ENC_BASE));

    //no base_ctx->key, since enc key depends on ENC_TYPE
    //no base_ctx->output, since this depends on 1st part of the whole output(when encrypting)
    //no base_ctx->output_bytes, since this is encryption or decryption output
    base_ctx->key_bytes   = key_bytes;
    base_ctx->input       = input;
    base_ctx->input_bytes = input_bytes;
    base_ctx->enc_type    = type;
}

unsigned int xor_enc_imp(E_ENC_BASE *self)
{
    E_XOR_ENC_CTX *ctx = (E_XOR_ENC_CTX *)self;

    uint8_XOR(ctx->base.key, ctx->base.input, ctx->base.output, ctx->base.key_bytes);
    ctx->base.output_bytes = ctx->base.key_bytes;

    return ECIES_SUCCESS;
}

/**
 * @brief       XOR Encryption ENC CTX init.
 * @param[in]   enc_ctx      - ctx to be initialized
 * @param[in]   input        - internal plaintext or ciphertext
 * @param[in]   input_bytes  - byte length of key and input
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.in XOR encryption, the encryption key length is equal to the message length
      -# 2.after this initialization, before encryption or decryption, the following fields of
           (E_ENC_BASE *)enc_ctx must be set by hand.
           key, output
  @endverbatim
 */
void e_xor_enc_init(E_XOR_ENC_CTX *enc_ctx, unsigned char *input, unsigned int input_bytes)
{
    enc_base_init((E_ENC_BASE *)enc_ctx, input_bytes, input, input_bytes, XOR_ENC);
    enc_ctx->base.enc_fun_imp = xor_enc_imp;
    enc_ctx->base.dec_fun_imp = xor_enc_imp;
}

/*************************************************************************************
 *                       MAC structure Implementation
 *************************************************************************************/

void mac_base_init(E_MAC_BASE *base_ctx, unsigned char *key, unsigned int key_bytes)
{
    memset_((unsigned char *)base_ctx, 0, sizeof(E_MAC_BASE));

    base_ctx->key       = key;
    base_ctx->key_bytes = key_bytes;
}

unsigned int hmac_imp(E_MAC_BASE *self)
{
    E_HMAC_CTX  *ctx;
    HASH_NODE    node[2];
    unsigned int ret;

    ctx = (E_HMAC_CTX *)self;

    node[0].msg_addr  = ctx->base.msg;
    node[0].msg_bytes = ctx->base.msg_bytes;
    node[1].msg_addr  = ctx->base.appendix;
    node[1].msg_bytes = ctx->base.appendix_bytes;
    //print_buf_U8(node[0].msg_addr, node[0].msg_bytes, "msg");print_buf_U8(node[1].msg_addr, node[1].msg_bytes, "msg");print_buf_U8(ctx->base.key, ctx->base.key_bytes, "key");
    ret = hmac_node_steps(ctx->hash_alg, ctx->base.key, 0, ctx->base.key_bytes, node, 2, ctx->base.mac); //print_buf_U8(ctx->base.mac, ctx->base.mac_bytes, "mac");
    if (HASH_SUCCESS == ret) {
        return ECIES_SUCCESS;
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       HMAC MAC CTX init.
 * @param[in]   mac_ctx          - ctx to be initialized
 * @param[in]   key_buffer       - hmac key buffer, to store hmac key
 * @param[in]   key_bytes        - hmac key byte length
 * @param[out]  appendix         - appendix followed by cipher
 * @param[in]   appendix_bytes   - byte length of appendix
 * @param[in]   hash_alg         - hash algorithm used in hmac
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.after this initialization, before mac calculation, the following fields of
          (E_MAC_BASE *)mac_ctx must be set by hand.
          msg, msg_bytes, mac
  @endverbatim
 */
void e_hmac_init(E_HMAC_CTX *mac_ctx, unsigned char *key_buffer, unsigned int key_bytes, unsigned char *appendix, unsigned int appendix_bytes, HASH_ALG hash_alg)
{
    mac_base_init((E_MAC_BASE *)mac_ctx, key_buffer, key_bytes);

    //no base_ctx->msg and base_ctx->msg_bytes, since this is the cipher
    //no mac, since this depends on the encrypting.
    mac_ctx->base.mac_bytes      = hash_get_digest_word_len(hash_alg) << 2;
    mac_ctx->base.appendix       = appendix;
    mac_ctx->base.appendix_bytes = appendix_bytes;
    mac_ctx->base.mac_imp        = hmac_imp;

    mac_ctx->hash_alg = hash_alg;
}

/*************************************************************************************
  *                       ECIES function Implementation
  *************************************************************************************/

    #ifdef ECIES_SUPPORT_EC_POINT_COMPRESSED
/**
 * @brief    private function.
 *           lucas sequences:
 *           U_0 = 0, U_1 = 1, and U_k = P * U_{k - 1} - Q * U_{k - 2} for k >= 2
 *           V_0 = 2, V_1 = P, and V_k = P * V_{k - 1} - Q * V_{k - 2} for k >= 2
 *           ref: ANSI-X963-2001 D1.3 Generating Lucas Sequences
 * @param[in]   p          - input, modulus, a odd prime.
 * @param[in]   P          - input, initial value of lucas sequence parameter P (P is upper case !).
 * @param[in]   Q          - input, initial value of lucas sequence parameter Q.
 * @param[in]   k          - input, the subscript value of a lucas sequence.
 * @param[in]   pBitLen    - bit length of p.
 * @param[out]  u          - output, U_k mod p.
 * @param[out]  v          - output, V_k mod p.
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. k can not be zero
  @endverbatim
 */
unsigned int lucas_sequences(unsigned int *p, unsigned int *P, unsigned int *Q, unsigned int *k, unsigned int pBitLen, unsigned int *u, unsigned int *v)
{
    unsigned int delta[ECCP_MAX_WORD_LEN];
    unsigned int inv_2[ECCP_MAX_WORD_LEN]; //2^(-1) mod p
    unsigned int tmp1[ECCP_MAX_WORD_LEN];
    unsigned int tmp2[ECCP_MAX_WORD_LEN];
    unsigned int tmp3[ECCP_MAX_WORD_LEN];
    unsigned int pWordLen = GET_WORD_LEN(pBitLen);
    unsigned int i, ret;

    //get inv_2 = 2 ^ (-1) mod p
    inv_2[0] = 2;
    ret      = pke_modinv(p, inv_2, inv_2, pWordLen, 1);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    /*********** get delta = (P ^ 2 - 4 * Q) mod p ***********/
    // delta = (P ^ 2) mod p
    ret = pke_modmul(p, P, P, delta, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    // tmp1 = (4 * Q) mod p
    uint32_clear(tmp1, pWordLen);
    tmp1[0] = 4;
    ret     = pke_modmul_internal(tmp1, Q, tmp1, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    // delta = (P ^ 2) - (4 * Q) mod p
    ret = pke_modsub(p, delta, tmp1, delta, pWordLen);
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    /*********** traversal k binary ***********/
    //set u1 and v1
    uint32_clear(u, pWordLen);   //u = 1
    u[0] = 1;
    uint32_copy(v, P, pWordLen); //v = P

    i = get_valid_bits((unsigned int *)k, pWordLen);
    if (0 == i) {
        //set u0 and v0
        u[0] = 0;                  //u = 0
        uint32_clear(v, pWordLen); //v = 2
        v[0] = 2;

        ret = PKE_SUCCESS;
        goto END;
    } else {
        ;
    }

    i--;
    while (0U != (i--)) {
        /*********** (u,v)=(uv mod p, (v^2 + delta*u^2)/2 mod p) ***********/
        //tmp3 = (u * v) mod p ------ (u)
        ret = pke_modmul_internal(u, v, tmp3, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // tmp1 = (v ^ 2) mod p
        ret = pke_modmul_internal(v, v, tmp1, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //tmp2 = u ^ 2 mod p
        ret = pke_modmul_internal(u, u, tmp2, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //tmp2 = delta * (u ^ 2) mod p
        ret = pke_modmul_internal(delta, tmp2, tmp2, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //tmp2 = (v ^ 2) + delta * (u ^ 2) mod p
        ret = pke_modadd(p, tmp1, tmp2, tmp2, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //v = tmp2/2 mod p
        ret = pke_modmul_internal(tmp2, inv_2, v, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }
        uint32_copy(u, tmp3, pWordLen);

        if (get_bit_value_by_index((unsigned int *)k, i)) {
            /*********** (u, v) = ((Pu + v)/2 mod p, (Pv + delta * u)/2 mod p) ***********/
            //tmp1 = P * u mod p
            ret = pke_modmul_internal(P, u, tmp1, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //tmp2 = (P * u + v) mod p
            ret = pke_modadd(p, tmp1, v, tmp2, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //tmp3 = (P * u + v)/2 mod p ---- (u)
            ret = pke_modmul_internal(tmp2, inv_2, tmp3, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //v = (P * v + delta * u) / 2 mod p
            // tmp2 = (P * v) mod p
            ret = pke_modmul_internal(P, v, tmp2, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //tmp1 = (delta * u) mod p
            ret = pke_modmul_internal(delta, u, tmp1, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //tmp2 = (P * v + delta * u) mod p
            ret = pke_modadd(p, tmp2, tmp1, tmp2, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            //v = (P * v + delta * u)/2 mod p
            ret = pke_modmul_internal(tmp2, inv_2, v, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }
            uint32_copy(u, tmp3, pWordLen);
        } else {
            ;
        }
    }

    ret = PKE_SUCCESS;

END:

    uint32_clear(delta, ECCP_MAX_WORD_LEN);
    uint32_clear(inv_2, ECCP_MAX_WORD_LEN);
    uint32_clear(tmp1, ECCP_MAX_WORD_LEN);
    uint32_clear(tmp2, ECCP_MAX_WORD_LEN);
    uint32_clear(tmp3, ECCP_MAX_WORD_LEN);

    return ret;
}

/**
 * @brief    private function.
 *           Quadratic residue:
 *           x ^ 2 = a mod p  -----> x = ?
 *           ref: ANSI-X963-2001 D1.4 Finding Square Roots Modulo a Prime
 * @param[in]   p          - input, modulus, a odd prime.
 * @param[in]   P          - input, integer a with 0 < a < p.
 * @param[in]   pBitLen    - input, bit length of p.
 * @param[in]   x          - output, a square root (mod p) of a if one exists.
 * @return     PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. p must be a prime.
  @endverbatim
 */
unsigned int quadratic_residue(unsigned int *p, unsigned int *a, unsigned int pBitLen, unsigned int *x)
{
    unsigned int tmp[ECCP_MAX_WORD_LEN];
    unsigned int u[ECCP_MAX_WORD_LEN];
    unsigned int gama[ECCP_MAX_WORD_LEN];
    unsigned int i[ECCP_MAX_WORD_LEN];
    unsigned int flex_num[ECCP_MAX_WORD_LEN];
    unsigned int pWordLen = GET_WORD_LEN(pBitLen);
    unsigned int ret;

    uint32_clear(flex_num, ECCP_MAX_WORD_LEN);

        //set mod p
        #if (defined(PKE_LP) || defined(PKE_SECURE))
    ret = pke_pre_calc_mont(p, pBitLen, NULL, NULL);
        #else
    ret = pke_pre_calc_mont(p, pBitLen, NULL);
        #endif
    if (PKE_SUCCESS != ret) {
        goto END;
    } else {
        ;
    }

    // p = 3 mod 4 ?
    if ((p[0] & 0x00000003U) == 0x00000003U) {
        // 4 * u + 3 = p ---- u = ?
        // u = p - 3
        uint32_copy(u, p, pWordLen);

        // u = u / 4
        (void)Big_Div2n(u, pWordLen, 2);

        //tmp = u + 1
        flex_num[0] = 1;
        ret         = pke_add(u, flex_num, tmp, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //u = a ^ (u+1) mod p
        ret = pke_modexp(p, tmp, a, u, pWordLen, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

            // tmp = (u ^ 2) mod p
        #if (defined(PKE_LP) || defined(PKE_SECURE))
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        #endif
        ret = pke_modmul_internal(u, u, tmp, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        if (0 == uint32_BigNumCmp(tmp, pWordLen, a, pWordLen)) {
            uint32_copy(x, u, pWordLen);
            ret = PKE_SUCCESS;
        } else {
            ret = PKE_ERROR;
        }
    } else if ((p[0] & 0x00000007U) == 0x00000005U) // p = 5 mod 8 ?
    {
        // p = 8 * u + 5 ---- u = ?
        // u = p - 5
        uint32_copy(u, p, pWordLen);

        // u = u / 8
        (void)Big_Div2n(u, pWordLen, 3);

        // tmp = (2 * a) mod p
        ret = pke_modadd(p, a, a, tmp, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // gama = (tmp ^ u) mod p
        ret = pke_modexp(p, u, tmp, gama, pWordLen, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

            /*********** i = 2 * a * (gama ^ 2) mod p ***********/
            // u = gama ^ 2 mod p
        #if (defined(PKE_LP) || defined(PKE_SECURE))
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        #endif
        ret = pke_modmul_internal(gama, gama, u, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // i = tmp * u mod p
        ret = pke_modmul_internal(tmp, u, i, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        /*********** x = a * gama * (i - 1) mod p ***********/
        // tmp = (i - 1)
        flex_num[0] = 1;
        ret         = pke_sub(i, flex_num, tmp, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // u = gama * (i - 1) mod p
        ret = pke_modmul_internal(gama, tmp, u, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // x = a * u mod p
        ret = pke_modmul_internal(a, u, x, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        // tmp = x ^ 2 mod p
        ret = pke_modmul_internal(x, x, tmp, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        if (0 == uint32_BigNumCmp(tmp, pWordLen, a, pWordLen)) {
            ret = PKE_SUCCESS;
        } else {
            ret = PKE_ERROR;
        }
    } else if ((p[0] & 0x00000003U) == 0x00000001U) // p = 1 mod 4
    {
        //set i = 2 * u + 1, here p = 4 * u + 1
        uint32_copy(i, p, pWordLen);
        (void)Big_Div2n(i, pWordLen, 1);
        i[0] |= 1;

            //set x = 4 * a mod p
        #if (defined(PKE_LP) || defined(PKE_SECURE))
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
        #endif
        flex_num[0] = 4;
        ret         = pke_modmul_internal(flex_num, a, x, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        //flex_num = 2^(-1) mod p
        flex_num[0] = 2;
        ret         = pke_modinv(p, flex_num, flex_num, pWordLen, 1);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        while (1) {
            //generate random num in [0, p - 1]
            ret = get_rand((unsigned char *)tmp, GET_BYTE_LEN(pBitLen));
            if (TRNG_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            if (tmp[pWordLen - 1] >= p[pWordLen - 1]) {
                tmp[pWordLen - 1] = p[pWordLen - 1] - 1;
            } else {
                ;
            }

            // u --> U gama--> V
            ret = lucas_sequences(p, tmp, a, i, pBitLen, u, gama);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            // tmp = gama ^ 2 mod p
            ret = pke_modmul_internal(gama, gama, tmp, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }

            // gama ^ 2 = 4 * a mod p ?
            if (0 == uint32_BigNumCmp(tmp, pWordLen, x, pWordLen)) {
                //x = V/2 mod p
                ret = pke_modmul_internal(gama, flex_num, x, pWordLen);
                if (PKE_SUCCESS != ret) {
                    goto END;
                } else {
                    ;
                }

                ret = PKE_SUCCESS;
                break;
            } else if ((Bigint_Check_1(u, pWordLen) != 1u) && (Bigint_Check_p_1(u, p, pWordLen) != 1u)) {
                ret = PKE_ERROR;
                break;
            } else {
                ;
            }
        }
    } else {
        ret = PKE_ERROR;
    }

END:

    uint32_clear(flex_num, ECCP_MAX_WORD_LEN);
    uint32_clear(tmp, ECCP_MAX_WORD_LEN);
    uint32_clear(u, ECCP_MAX_WORD_LEN);
    uint32_clear(gama, ECCP_MAX_WORD_LEN);
    uint32_clear(i, ECCP_MAX_WORD_LEN);

    return ret;
}
    #endif


/**
 * @brief       An elliptic curve point P = (x , y) that is not the point at infinity shall
                be represented as an octet string in one of the following two forms:
 * @param[in]   curve        - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   x            - curve point x, big-endian.
 * @param[in]   y            - curve point y, big-endian.
 * @param[in]   point_form   - curve point format.
 * @param[out]  result       - curve point representation.
 * @param[out]  r_bytes      - byte length of result
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.the result space needs 1+2*pByteLen bytes while point_form is POINT_UNCOMPRESSED,
           or 1+pByteLen bytes while point_form is POINT_COMPRESSED, here pByteLen is byte
           length of curve parameter p
  @endverbatim
 */
unsigned int point_to_octet_string_conversion(eccp_curve_t *curve, unsigned char *x, unsigned char *y, EC_POINT_FORM point_form, unsigned char *result, unsigned int *r_bytes)
{
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    memcpy_(result + 1, (unsigned char *)x, pByteLen);

    switch (point_form) {
    #ifdef ECIES_SUPPORT_EC_POINT_COMPRESSED
    case POINT_COMPRESSED:
        result[0] = POINT_COMPRESSED | (y[pByteLen - 1] & 0x00000001);
        *r_bytes  = 1 + pByteLen;
        ret       = PKE_SUCCESS;
        break;
    #endif

    case POINT_UNCOMPRESSED:
        result[0] = POINT_UNCOMPRESSED;
        memcpy_(result + 1 + pByteLen, (unsigned char *)y, pByteLen);
        *r_bytes = 1 + 2 * pByteLen;
        ret      = PKE_SUCCESS;
        break;

    default:
        ret = PKE_ERROR;
        break;
    }

    return ret;
}

/**
 * @brief       The representation value of the curve point (x, y) obtained through the
                point_to_octet_string_conversion function is inversely released from the
                x_p coordinate and y_p coordinate
 * @param[in]   curve        - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   encode       - curve point representation.
 * @param[out]  x            - x coordinate of curve point, U32 little-endian.
 * @param[out]  y            - y coordinate of curve point, U32 little-endian.
 * @return      0:success     other:error
 */
unsigned int octet_string_to_point_conversion(eccp_curve_t *curve, unsigned char *encode, unsigned int *x, unsigned int *y)
{
    unsigned int z[ECCP_MAX_WORD_LEN];
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int ret      = PKE_ERROR;

    //little endian -> big endian
    reverse_byte_array(encode + 1, (unsigned char *)x, pByteLen);

    #ifdef ECIES_SUPPORT_EC_POINT_COMPRESSED
    if ((POINT_COMPRESSED == encode[0]) || ((POINT_COMPRESSED + 1) == encode[0])) {
        // z = x^3 + ax + b = x(x^2+a) + b mod p
        ret = pke_modmul(curve->eccp_p, x, x, z, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = pke_modadd(curve->eccp_p, curve->eccp_a, z, z, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = pke_modmul_internal(x, z, z, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = pke_modadd(curve->eccp_p, curve->eccp_b, z, z, pWordLen);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        ret = quadratic_residue(curve->eccp_p, z, curve->eccp_p_bitLen, y);
        if (PKE_SUCCESS != ret) {
            goto END;
        } else {
            ;
        }

        if ((y[0] & 0x00000001) != (encode[0] - POINT_COMPRESSED)) {
            // y = p - y
            ret = pke_sub(curve->eccp_p, y, y, pWordLen);
            if (PKE_SUCCESS != ret) {
                goto END;
            } else {
                ;
            }
        } else {
            ;
        }

        ret = PKE_SUCCESS;
        goto END;
    } else {
        ;
    }
    #endif

    if (POINT_UNCOMPRESSED == encode[0]) {
        reverse_byte_array(encode + 1 + pByteLen, (unsigned char *)y, pByteLen);
        ret = PKE_SUCCESS;
        goto END;
    } else {
        ;
    }

END:

    uint32_clear(z, pWordLen);
    if (PKE_SUCCESS != ret) {
        ret = PKE_ERROR;
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       get the 1st part(a point) byte length in ECIES ciphertext.
 * @param[in]   curve            - ecc curve struct pointer, please make sure it is valid.
 * @param[in]   cipher           - ECIES ciphertext.
 * @param[in]   cipher_bytes     - byre length of cipher.
 * @param[out]  point_bytes      - the 1st part(a point) byte length in cipher.
 * @return      0:success     other:error
 */
unsigned int ansi_x963_get_point_byte_len_from_ciphertext(eccp_curve_t *curve, unsigned char *cipher, unsigned int cipher_bytes, unsigned int *point_bytes)
{
    unsigned int pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int ret;
    (void)cipher_bytes;

    switch (cipher[0]) {
    #ifdef ECIES_SUPPORT_EC_POINT_COMPRESSED
    case POINT_COMPRESSED:
    case (POINT_COMPRESSED + 1):
        *point_bytes = pByteLen + 1;
        ;
        ret = PKE_SUCCESS;
        break;
    #endif

    case POINT_UNCOMPRESSED:
        *point_bytes = 2 * pByteLen + 1;
        ;
        ret = PKE_SUCCESS;
        break;

    default:
        ret = PKE_ERROR;
        break;
    }

    return ret;
}

/*************************************************************************************
 *                       ECIES structure Implementation
 *************************************************************************************/

void ecies_ansi_x963_ctx_init(ECIES_STD *ctx, ECIES_ENC_MAC_KEY_ORDER enc_mac_key_order)
{
    ctx->type_flag                     = ANSI_X963;
    ctx->enc_mac_key_order             = enc_mac_key_order;
    ctx->get_point_len_from_ciphertext = ansi_x963_get_point_byte_len_from_ciphertext;
    ctx->point_decompress              = octet_string_to_point_conversion;
    ctx->point_compress                = point_to_octet_string_conversion;
}

#endif
