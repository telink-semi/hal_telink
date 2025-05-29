/********************************************************************************************************
 * @file    pke.c
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

#include "lib/include/pke/pke.h"
#include "lib/include/trng/trng.h"
#include "lib/include/crypto_common/utility.h"
#ifdef PKE_SEC
    #include "../crypto_include/crypto_common/utility_sec.h"
#endif


#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
/**
 * @brief       a = a*(2^n) or a = a<<n
 * @param[in]   a                    - input/output, big integer
 * @param[out]  aWordLen             - word length of a.
 * @param[in]   n                    - exponent n.
 * @return      result word length
 */
unsigned int Big_Mul2n(unsigned int a[], int32_t aWordLen, unsigned char n)
{
    int32_t       i;
    unsigned char flag = 0;

    if (0 == aWordLen) {
        return 0u;
    } else {
        ;
    }

    if (a[aWordLen - 1] & (0xFFFFFFFF << (32u - ((unsigned int)n)))) // need carry
    {
        a[aWordLen] = a[aWordLen - 1] >> (32u - ((unsigned int)n));
        flag        = (unsigned char)1;
    } else {
        ;
    }

    for (i = aWordLen - 1; i > 0; i--) {
        a[i] <<= n;
        a[i] |= (a[i - 1] >> (32u - ((unsigned int)n)));
    }
    a[i] <<= n;

    if (((unsigned char)0) != flag) {
        return (unsigned int)(aWordLen + 1);
    } else {
        return (unsigned int)aWordLen;
    }
}

/**
 * @brief       get J0
 * @param[in]   n0                    - a U32 odd integer
 * @return      J0, (- n0 ^(-1) mod 2^w), here w is 32 actually
 */
unsigned int get_J0(unsigned int n0)
{
    #if 0
    int32_t i;
    unsigned int t = n0;

    for(i=30;i>0;i--)
    {
        t = t*t;
        t = t*n0;
    }

    return (0u-t);
    #else
    unsigned int t;

    t = n0 + (((n0 + 2U) & 4U) << 1);

    t *= (2U - (n0 * t));
    t *= (2U - (n0 * t));
    t *= (2U - (n0 * t));

    return (~t) + 1U;
    #endif
}
#endif


/**
 * @brief       get pke IP version
 * @return      pke IP version
 */
unsigned int pke_get_version(void)
{
    return rPKE_VERSION;
}

/**
 * @brief       get pke driver version
 * @return      pke driver version(software version)
 */
unsigned int pke_get_driver_version(void)
{
    //the meaning of the version(for example, if the return value is 0x23080301)
    //the first 3 bytes:  23.08.03 ---- date
    //the last byte:      01       ---- first version on the day
    return (0x23U << 24U) | (0x10U << 16U) | (0x17U << 8U) | 0x01U;
}

/**
 * @brief       clear finished and interrupt tag
 * @return      none
 */
void pke_clear_interrupt(void)
{
    MEM_VOLATILE unsigned int mask = ~((unsigned int)1);

#if 1
    rPKE_RISR &= mask; //write 0 to clear
#else
    MEM_VOLATILE unsigned int flag = 1u;

    if (rPKE_RISR & flag) {
        rPKE_RISR &= mask; //write 0 to clear
    } else {
        ;
    }
#endif
}

/**
 * @brief       enable pke interrupt
 * @return      none
 */
void pke_enable_interrupt(void)
{
    MEM_VOLATILE unsigned int flag = (unsigned int)1;

    rPKE_IMCR |= flag;
}

/**
 * @brief       disable pke interrupt
 * @return      none
 */
void pke_disable_interrupt(void)
{
    MEM_VOLATILE unsigned int mask = ~((unsigned int)1);

    rPKE_IMCR &= mask;
}

/**
 * @brief       set operand width
 * @param[in]   bitLen         - bit length of operand.
 * @return      uint bytes of hardware operand.
 * @note
  @verbatim
      -# 1.please make sure aWordLen <= modWordLen <= OPERAND_MAX_WORD_LEN and a < modulus.
  @endverbatim
 */
unsigned int pke_set_operand_width(unsigned int bitLen)
{
    MEM_VOLATILE unsigned int mask       = ~(0x07FFFFU);
    unsigned int              cfg        = 0U, len;
    unsigned int              step_bytes = 0U;

    len = (bitLen + 255U) >> 8;

    if (1U == len) {
        cfg        = 2U;
        step_bytes = 0x24U;
    } else if (2U == len) {
        cfg        = 3U;
        step_bytes = 0x44U;
    } else if (len <= 4U) {
        cfg        = 4U;
        step_bytes = 0x84U;
    } else if (len <= 8U) {
        cfg        = 5U;
        step_bytes = 0x104U;
    } else if (len <= 16U) {
        cfg        = 6U;
        step_bytes = 0x204U;
    } else {
        ;
    }

    cfg = (cfg << 16) | (bitLen); //cfg = (cfg<<16)|(len<<8);

    rPKE_CFG &= mask;
    rPKE_CFG |= cfg;              //printf("\r\n %u, rPKE_CFG = %08x", len, rPKE_CFG);

    return step_bytes;
}

/**
 * @brief       set operation micro code
 * @return      current operand byte length
 */
unsigned int pke_get_operand_bytes(void)
{
    unsigned int step_bytes;

#if 1
    unsigned int t = ((rPKE_CFG) >> 16) & 0x07U;

    if ((t > 1U) && (t < 7U)) {
        step_bytes = 0x04U + ((0x08U) << t);
    } else {
        step_bytes = 0x24U; //default value
    }
#else
    switch (((rPKE_CFG) >> 16) & 0x07U) {
    case 2U:
        step_bytes = 0x24U;
        break;

    case 3U:
        step_bytes = 0x44U;
        break;

    case 4U:
        step_bytes = 0x84U;
        break;

    case 5U:
        step_bytes = 0x104U;
        break;

    case 6U:
        step_bytes = 0x204U;
        break;

    default:
        step_bytes = 0x24U;
    }
#endif

    return step_bytes;
}

/**
 * @brief       set operation micro code
 * @param[in]   addr      - specific micro code.
 * @return      none.
 */
void pke_set_microcode(unsigned int addr)
{
    rPKE_MC_PTR = addr;
}

/**
 * @brief       get exe config
 * @return      current exe config value.
 */
unsigned int pke_get_exe_cfg(void)
{
    return rPKE_EXE_CONF;
}

/**
 * @brief       set exe config
 * @param[in]   cfg      - specific config value.
 * @return      none
 */
void pke_set_exe_cfg(unsigned int cfg)
{
    rPKE_EXE_CONF = cfg;
}

/**
 * @brief       start pke calc
 * @return      none
 */
void pke_start(void)
{
    MEM_VOLATILE unsigned int flag = PKE_START_CALC;

    rPKE_CTRL |= flag;
}

/**
 * @brief       return calc return code
 * @return      0:success     other:error
 */
unsigned int pke_check_rt_code(void)
{
    MEM_VOLATILE unsigned int mask = 0x07u;

    return (unsigned char)(rPKE_RT_CODE & mask);
}

/**
 * @brief       wait till done
 * @return      none
 */
void pke_wait_till_done(void)
{
    MEM_VOLATILE unsigned int flag = 1u;

    while (0u == (rPKE_RISR & flag)) {
        ;
    }
}

/**
 * @brief       set operation micro code, start hardware, wait till done, and return code.
 * @param[in]   micro_code      - pecific micro code
 * @return      PKE_SUCCESS(success), other(inverse not exists or error)
 */
unsigned int pke_set_micro_code_start_wait_return_code(unsigned int micro_code)
{
    pke_set_microcode(micro_code);

    pke_clear_interrupt();

    pke_start();

    pke_wait_till_done();

    return pke_check_rt_code();
}

/**
 * @brief       ainv = a^(-1) mod modulus
 * @param[in]   modulus              - modulus.
 * @param[in]   a                    - integer a.
 * @param[out]  ainv                 - ainv = a^(-1) mod modulus.
 * @param[in]   modWordLen           - word length of modulus and ainv.
 * @param[in]   aWordLen             - word length of a.
 * @return      PKE_SUCCESS(success), other(inverse not exists or error)
 * @note
  @verbatim
      -# 1.please make sure aWordLen <= modWordLen <= OPERAND_MAX_WORD_LEN and a < modulus.
  @endverbatim
 */
unsigned int pke_modinv(unsigned int *modulus, unsigned int *a, unsigned int *ainv, unsigned int modWordLen, unsigned int aWordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    //pke_set_operand_width(modWordLen<<5);
    step_bytes = pke_set_operand_width(get_valid_bits(modulus, modWordLen));
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus, modWordLen); //B3 modulus
    if (step_words > modWordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + modWordLen, step_words - modWordLen);
    } else {
        ;
    }

    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), a, aWordLen); //B0 a
    if (step_words > aWordLen) {
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + aWordLen, step_words - aWordLen);
    } else {
        ;
    }

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODINV);
    if (PKE_SUCCESS == ret) {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), ainv, modWordLen); //A0 ainv
    } else if (PKE_NO_MODINV != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), modWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), aWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), modWordLen << 2);
#endif
    } else {
        ;
    }

    return ret;
}

/**
 * @brief       out = (a+b) mod modulus or out = (a-b) mod modulus
 * @param[in]   modulus              - modulus.
 * @param[in]   a                    - integer a.
 * @param[in]   b                    - integer b.
 * @param[out]  out                  - out = a+b mod modulus or out = (a-b) mod modulus.
 * @param[in]   wordLen              - word length of modulus, a, b.
 * @param[in]   micro_code           - must be MICROCODE_MODADD or MICROCODE_MODSUB
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.a,b must be less than modulus.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_modadd_modsub_internal(unsigned int *modulus, unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen, unsigned int micro_code)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(wordLen << 5);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus, wordLen); //B3 modulus
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a, wordLen);       //A0 a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b, wordLen);       //B0 b

    if (step_words > wordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + wordLen, step_words - wordLen);
        uint32_clear((unsigned int *)(rPKE_A(0u, step_bytes)) + wordLen, step_words - wordLen);
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + wordLen, step_words - wordLen);
    } else {
        ;
    }

    ret = pke_set_micro_code_start_wait_return_code(micro_code);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), wordLen << 2);
#endif
        return ret;
    } else {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out, wordLen); //A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief       out = (a+b) mod modulus or out = (a-b) mod modulus
 * @param[in]   modulus              - modulus.
 * @param[in]   a                    - integer a.
 * @param[in]   b                    - integer b.
 * @param[out]  out                  - out = a+b mod modulus or out = (a-b) mod modulus.
 * @param[in]   wordLen              - word length of modulus, a, b.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.a,b must be less than modulus.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_modadd(unsigned int *modulus, unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wordLen, MICROCODE_MODADD);
}

/**
 * @brief       out = (a-b) mod modulus
 * @param[in]   modulus              - modulus.
 * @param[in]   a                    - integer a.
 * @param[in]   b                    - integer b.
 * @param[out]  out                  - out = a+b mod modulus or out = (a-b) mod modulus.
 * @param[in]   wordLen              - word length of modulus, a, b.
 * @return      PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.a,b must be less than modulus.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_modsub(unsigned int *modulus, unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wordLen, MICROCODE_MODSUB);
}

/* 
 * @brief       out = a+b or out = a-b
 * @param[in]   a              - modulus.
 * @param[in]   b              - integer a.
 * @param[in]   out            - integer b.
 * @param[out]  wordLen        - out = a+b mod modulus or out = (a-b) mod modulus.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.a+b may overflow.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim

unsigned int pke_add_sub_internal(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen,
        unsigned int micro_code)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(wordLen<<5);
    step_words = step_bytes>>2;

    pke_load_operand((unsigned int *)(rPKE_A(1u,step_bytes)), (unsigned int *)a, wordLen);          //A1 a
    pke_load_operand((unsigned int *)(rPKE_B(1u,step_bytes)), (unsigned int *)b, wordLen);          //B1 b

    if(step_words > wordLen)
    {
        uint32_clear((unsigned int *)(rPKE_A(1u,step_bytes))+wordLen, step_words-wordLen);
        uint32_clear((unsigned int *)(rPKE_B(1u,step_bytes))+wordLen, step_words-wordLen);
    }
    else
    {;}

    ret = pke_set_micro_code_start_wait_return_code(micro_code);
    if(PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(1u,step_bytes)), wordLen<<2);
        get_rand_fast((unsigned char *)(rPKE_B(1u,step_bytes)), wordLen<<2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(1u,step_bytes)), out, wordLen);                //A1 result

        return PKE_SUCCESS;
    }
} */


/**
 * @brief       out = a+b
 * @param[in]   a              - modulus.
 * @param[in]   b              - integer a.
 * @param[in]   out            - integer b.
 * @param[out]  wordLen        - word length of a, b, out
 * @return      PKE_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1.a+b may overflow.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_add(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
#if 0
    return pke_add_sub_internal(a, b, out, wordLen, MICROCODE_INTADD);
#else
    unsigned int i, carry, temp, temp2;

    carry = 0u;
    for (i = 0u; i < wordLen; i++) {
        temp2  = a[i];
        temp   = a[i] + b[i];
        out[i] = temp + carry;
        if ((temp < temp2) || (out[i] < carry)) {
            carry = 1u;
        } else {
            carry = 0u;
        }
    }

    return PKE_SUCCESS;
#endif
}

/**
 * @brief       out = a-b
 * @param[in]   a              - integer a.
 * @param[in]   b              - integer b.
 * @param[out]  out            - out = a-b.
 * @param[in]   wordLen        - word length of a, b, out.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure a > b.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_sub(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
#if 0
    return pke_add_sub_internal(a, b, out, wordLen, MICROCODE_INTSUB);
#else
    unsigned int i, carry, tmp, tmp2;

    carry = 0u;
    for (i = 0u; i < wordLen; i++) {
        tmp  = a[i] - b[i];
        tmp2 = tmp - carry;
        if ((tmp > a[i]) || (tmp2 > tmp)) {
            carry = 1u;
        } else {
            carry = 0u;
        }
        out[i] = tmp2;
    }

    return PKE_SUCCESS;
#endif
}

/**
 * @brief       out = a*b
 * @param[in]   a              - integer a.
 * @param[in]   a_wordLen      - word length of a.
 * @param[in]   b              - integer b.
 * @param[in]   b_wordLen      - word length of b.
 * @param[out]  out            - out = a*b.
 * @param[in]   out_wordLen    - word length of out.
 * @return      0:PKE_SUCCESS     other:error
 * @note
  @verbatim
      -# 1.please make sure out buffer word length is bigger than (2*max_bit_len(a,b)+0x1F)>>5.
      -# 2.please make sure ab_wordLen is not bigger than OPERAND_MAX_WORD_LEN/2.
  @endverbatim
 */
unsigned int pke_mul_internal(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int a_wordLen, unsigned int b_wordLen, unsigned int out_wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(out_wordLen << 5); //for pke lp
    //step_bytes = pke_set_operand_width(GET_MAX_LEN(out_wordLen<<5,512u));  //for pke hp
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a, a_wordLen); //A0 a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b, b_wordLen); //B0 b

    uint32_clear((unsigned int *)(rPKE_A(0u, step_bytes)) + a_wordLen, step_words - a_wordLen);
    uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + b_wordLen, step_words - b_wordLen);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_INTMUL);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), a_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), b_wordLen << 2);
#endif
        return ret;
    } else {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), out, out_wordLen); //A1 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief       out = a*b
 * @param[in]   a              - integer a.
 * @param[in]   b              - integer b.
 * @param[out]  out            - out = a*b.
 * @param[in]   ab_wordLen        - word length of a, b, out.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure out buffer word length is bigger than (2*max_bit_len(a,b)+0x1F)>>5
      -# 2.please make sure ab_wordLen is not bigger than OPERAND_MAX_WORD_LEN/2
  @endverbatim
 */
#if 1
unsigned int pke_mul(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int ab_wordLen)
{
    unsigned int bitLen, tempLen;

    bitLen  = get_valid_bits(a, ab_wordLen);
    tempLen = get_valid_bits(b, ab_wordLen);

    bitLen  = GET_MAX_LEN(bitLen, tempLen);
    tempLen = GET_WORD_LEN(bitLen << 1);
    if (tempLen < (ab_wordLen << 1)) {
        tempLen = (ab_wordLen << 1) - 1u;
    } else {
        tempLen = (ab_wordLen << 1);
    }

    return pke_mul_internal(a, b, out, ab_wordLen, ab_wordLen, tempLen);
}
#else
unsigned int pke_mul(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int ab_wordLen)
{
    uint64_t     UV;
    unsigned int i, j, *U, *V;
    unsigned int bitLen, tempLen;

    bitLen  = get_valid_bits(a, ab_wordLen);
    tempLen = get_valid_bits(b, ab_wordLen);

    bitLen  = GET_MAX_LEN(bitLen, tempLen);
    tempLen = GET_WORD_LEN(bitLen << 1);
    if (tempLen < (ab_wordLen << 1)) {
        tempLen = (ab_wordLen << 1) - 1u;
    } else {
        tempLen = (ab_wordLen << 1);
    }

    uint32_clear(out, tempLen);

    V = (unsigned int *)(&UV);
    U = V + 1u;
    for (i = 0u; i < ab_wordLen; i++) {
        *U = 0u;
        for (j = 0u; j < ab_wordLen; j++) {
            UV         = ((uint64_t)a[i]) * b[j] + out[i + j] + (*U);
            out[i + j] = (*V);
        }
        out[i + j] = (*U);
    }

    return PKE_SUCCESS;
}
#endif


#if 0
/**
 * @brief       calc n0(- modulus ^(-1) mod 2^w) for modMul, and pointMul. etc
 * @return      none
 * @note
  @verbatim
      -# 1.before calling, please make sure the modulus is set in PKE_A(a, 0).
      -# 2.please make sure the modulus is odd, and word length of the modulus
        is not bigger than OPERAND_MAX_WORD_LEN.
      -# 3.the result is set in the internal register, no need to output
  @endverbatim
*/
unsigned int pke_pre_calc_mont_N0(void)
{
    return pke_set_micro_code_start_wait_return_code(MICROCODE_MGMR_PRE_N0);
}
#endif

#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
/**
 * @brief       calc H(R^2 mod modulus) and n0'( - modulus ^(-1) mod 2^w ) for modMul,modExp, and pointMul. etc
 *              here w is bit width of word, i,e. 32.
 * @param[in]   modulus              - modulus.
 * @param[in]   bitLen               - bit length of modulus, must be multiple of 32
 * @param[out]  H                    - R^2 mod modulus.
 * @param[out]  n0                   - modulus ^(-1) mod 2^w, here w is 32 actually.
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.please make sure word length of buffer H is equal to wordLen(word length of modulus),
 *        and n0 only need one word.
      -# 3.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
  @endverbatim
 */
unsigned int pke_pre_calc_mont_without_mgmr_microcode(unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
    unsigned int  wordLen, tmpLen, i, j;
    unsigned int *A0;
    unsigned int *B0;
    unsigned int *h;
    unsigned int *n;
    unsigned int  exe_cfg_bak;
    unsigned int  step_bytes;

    exe_cfg_bak = pke_get_exe_cfg();

    wordLen    = GET_WORD_LEN(bitLen);
    step_bytes = pke_set_operand_width(wordLen << 5);

    pke_set_operand_width(bitLen);

    //get and set -N^(-1) mod 2^32
    i                                           = get_J0(modulus[0]);
    *((unsigned int *)(rPKE_B(4u, step_bytes))) = i;
    if (n0) {
        *n0 = i;
    } else {
        ;
    }

    A0 = (unsigned int *)(rPKE_A(0u, step_bytes));
    B0 = (unsigned int *)(rPKE_B(0u, step_bytes));
    h  = (unsigned int *)(rPKE_A(3u, step_bytes));
    n  = (unsigned int *)(rPKE_B(3u, step_bytes));

    //h = R mod n
    uint32_clear(h, step_bytes >> 2);
    uint32_copy(n, modulus, wordLen);
    pke_sub(h, n, h, wordLen);

    //h = A0 = 2R mod n
    tmpLen = Big_Mul2n(h, wordLen, (unsigned char)1);
    if ((tmpLen > wordLen) || (uint32_BigNumCmp(h, wordLen, n, wordLen) >= 0)) {
        pke_sub(h, n, h, wordLen);
    } else {
        ;
    }
    uint32_copy(A0, h, wordLen);

    if ((step_bytes >> 2) > wordLen) {
        uint32_clear(A0 + wordLen, (step_bytes >> 2) - wordLen);
        uint32_clear(B0 + wordLen, (step_bytes >> 2) - wordLen);
        uint32_clear(n + wordLen, (step_bytes >> 2) - wordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_MONT);

    tmpLen = wordLen << 5; //tmpLen = RbitLen-1
    i      = get_valid_bits(&tmpLen, 1u) - 1u;
    j      = 1u << (i - 1u);
    for (; i > 0u; i--) {
        //A0 = A0^2 mod n
        //pke_modmul_internal((unsigned int *)(PKE_B(3,step)), (unsigned int *)(PKE_A(0,step)), (unsigned int *)(PKE_A(0,step)), (unsigned int *)(PKE_A(0,step)), wordLen);
        uint32_copy(B0, A0, wordLen);
        pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

        if (tmpLen & j) {
            //A0 = A0*2R mod n
            //pke_modmul_internal((unsigned int *)(PKE_B(3,step)), (unsigned int *)(PKE_A(0,step)), h, (unsigned int *)(PKE_A(0,step)), wordLen);
            uint32_copy(B0, h, wordLen);
            pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
        } else {
            ;
        }

        j >>= 1;
    }

    uint32_copy(h, (unsigned int *)(rPKE_A(0u, step_bytes)), wordLen);
    if (NULL != H) {
        uint32_copy(H, (unsigned int *)(rPKE_A(0u, step_bytes)), wordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(exe_cfg_bak);

    return PKE_SUCCESS;
}
#endif


/**
 * @brief       calc H(R^2 mod modulus) and n0'( - modulus ^(-1) mod 2^w ) for modMul,modExp, and pointMul. etc
 *              here w is bit width of word, i,e. 32.
 * @param[in]   modulus              - modulus.
 * @param[in]   bitLen               - bit length of modulus
 * @param[out]  H                    - R^2 mod modulus.
 * @param[out]  n0                   - modulus ^(-1) mod 2^w, here w is 32 actually.
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.please make sure word length of buffer H is equal to wordLen(word length of modulus),
 *        and n0 only need one word.
      -# 3.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
  @endverbatim
 */
unsigned int pke_pre_calc_mont(unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
    unsigned int step_bytes, step_words;
    unsigned int wordLen = GET_WORD_LEN(bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus, wordLen); //B3 modulus

    if (step_words > wordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + wordLen, step_words - wordLen);
        uint32_clear((unsigned int *)(rPKE_A(3u, step_bytes)) + wordLen, step_words - wordLen);
    } else {
        ;
    }

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MGMR_PRE);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), 1u << 2);
#endif
        return ret;
    }

    if (NULL != H) {
        pke_read_operand((unsigned int *)(rPKE_A(3u, step_bytes)), H, wordLen); //A3 H
    } else {
        ;
    }

    if (NULL != n0) {
        pke_read_operand((unsigned int *)(rPKE_B(4u, step_bytes)), n0, 1u); //B4 n0
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       like function pke_pre_calc_mont(), but this one is without output here.
 * @param[in]   modulus              - modulus.
 * @param[in]   wordLen                  - word length of modulus.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_pre_calc_mont_no_output(unsigned int *modulus, unsigned int wordLen)
{
    return pke_pre_calc_mont(modulus, get_valid_bits(modulus, wordLen), NULL, NULL);
}

/**
 * @brief       like function pke_pre_calc_mont(), but this one is for modexp
 * @param[in]   modulus              - modulus.
 * @param[in]   bitLen               - bit length of modulus.
 * @param[out]  H                    - R^2 mod modulus
 * @param[out]  n0                   - modulus ^(-1) mod 2^w, here w is 32 actually
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.please make sure word length of buffer H is equal to wordLen(word length of modulus),and n0 only need one word.   
      -# 3.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
   @endverbatim
 */
unsigned int pke_pre_calc_mont_for_modexp(unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
    (void)n0;
    (void)H;
#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
    if (bitLen & 31u) {
        return pke_pre_calc_mont(modulus, bitLen, NULL, NULL);
    } else {
        return pke_pre_calc_mont_without_mgmr_microcode(modulus, bitLen, NULL, NULL);
    }
#else
    return pke_pre_calc_mont(modulus, bitLen, NULL, NULL);
#endif
}

/**
 * @brief       like function pke_pre_calc_mont(), but this one is for modexp
 * @param[in]   modulus              - modulus.
 * @param[out]  modulus_h            - R^2 mod modulus
 * @param[out]  modulus_n0           - modulus ^(-1) mod 2^w, here w is 32 actually
 * @param[in]   bitLen               - bit length of modulus
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
   @endverbatim
 */
unsigned int pke_load_modulus_and_pre_monts(unsigned int *modulus, unsigned int *modulus_h, unsigned int *modulus_n0, unsigned int bitLen)
{
    unsigned int step_bytes, step_words;
    unsigned int wordLen = GET_WORD_LEN(bitLen);

    step_bytes = pke_set_operand_width(bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus, wordLen);   //B3 modulus
    pke_load_operand((unsigned int *)(rPKE_A(3u, step_bytes)), modulus_h, wordLen); //A3 h
    if (step_words > wordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + wordLen, step_words - wordLen);
        uint32_clear((unsigned int *)(rPKE_A(3u, step_bytes)) + wordLen, step_words - wordLen);
    } else {
        ;
    }

    pke_load_operand((unsigned int *)(rPKE_B(4u, step_bytes)), modulus_n0, 1u);

    return PKE_SUCCESS;
}

/**
 * @brief       set modulus and pre-calculated mont parameters H(R^2 mod modulus) and n0'(- modulus ^(-1) mod 2^w) for hardware operation
 * @param[in]   modulus             - modulus.
 * @param[in]  modulus_h            - R^2 mod modulus
 * @param[in]  modulus_n0           - modulus ^(-1) mod 2^w, here w is 32 actually
 * @param[in]  bitLen               - bit length of modulus
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.bitLen must not be bigger than OPERAND_MAX_BIT_LEN
   @endverbatim
 */
unsigned int pke_set_modulus_and_pre_monts(unsigned int *modulus, unsigned int *modulus_h, unsigned int *modulus_n0, unsigned int bitLen)
{
    if ((NULL == modulus_h) || (NULL == modulus_n0)) {
        return pke_pre_calc_mont(modulus, bitLen, NULL, NULL);
    } else {
        return pke_load_modulus_and_pre_monts(modulus, modulus_h, modulus_n0, bitLen);
    }
}

/**
 * @brief       set modulus and pre-calculated mont parameters H(R^2 mod modulus) and n0'(- modulus ^(-1) mod 2^w) for hardware operation.
 * @param[in]   a              - integer a.
 * @param[in]   b              - integer b.
 * @param[out]  out            - out = a*b mod modulus.
 * @param[in]   wordLen        - word length of modulus, a, b.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.a, b must less than modulus.
      -# 3.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
      -# 4.before calling this function, please make sure the modulus and the pre-calculated mont arguments
       of modulus are located in the right address.
  @endverbatim
 */
unsigned int pke_modmul_internal(unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    //step_bytes = pke_set_operand_width(wordLen<<5);
    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a, wordLen); //A0 a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b, wordLen); //B0 b
    if (step_words > wordLen) {
        uint32_clear((unsigned int *)(rPKE_A(0u, step_bytes)) + wordLen, step_words - wordLen);
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + wordLen, step_words - wordLen);
    } else {
        ;
    }

    //pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), wordLen << 2);
#endif
        return ret;
    } else {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out, wordLen); //A0 out

        return PKE_SUCCESS;
    }
}

/**
 * @brief       out = a*b mod modulus.
 * @param[in]   modulus        - modulus.
 * @param[in]   a              - integer a.
 * @param[in]   b              - integer b.
 * @param[out]  out            - out = a*b mod modulus.
 * @param[in]   wordLen        - word length of modulus, a, b.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.modulus must be odd.
      -# 2.a, b must less than modulus.
      -# 3.wordLen must not be bigger than OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_modmul(unsigned int *modulus, unsigned int *a, unsigned int *b, unsigned int *out, unsigned int wordLen)
{
    unsigned int ret;

    ret = pke_pre_calc_mont(modulus, get_valid_bits(modulus, wordLen), NULL, NULL);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

        return pke_modmul_internal(a, b, out, wordLen);
    }
}

/**
 * @brief       mod exponent, this could be used for rsa encrypting,decrypting,signing,verifying.
 * @param[in]   modulus        - modulus.
 * @param[in]   exponent       - exponent.
 * @param[in]   base           - base number.
 * @param[out]  out            - out = base^(exponent) mod modulus.
 * @param[in]   mod_wordLen    - word length of modulus and base number.
 * @param[in]   exp_wordLen    - word length of exponent.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# before calling this function, please make sure the pre-calculated mont arguments of modulus are
       located in the right address
      -# 2.modulus must be odd.
      -# 3.please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_modexp(unsigned int *modulus, unsigned int *exponent, unsigned int *base, unsigned int *out, unsigned int mod_wordLen, unsigned int exp_wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(mod_wordLen << 5);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), exponent, exp_wordLen); //A1 exponent
    if (step_words > exp_wordLen) {
        uint32_clear((unsigned int *)(rPKE_A(1u, step_bytes)) + exp_wordLen, step_words - exp_wordLen);
    } else {
        ;
    }

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus, mod_wordLen); //B3 modulus
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), base, mod_wordLen);    //B0 base

    if (step_words > mod_wordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + mod_wordLen, step_words - mod_wordLen);
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + mod_wordLen, step_words - mod_wordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_MODEXP);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODEXP);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), exp_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wordLen << 2);
#endif
        return ret;
    } else {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out, mod_wordLen); //A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief       check input before mod exponent
 * @param[in]   modulus              - modulus.
 * @param[in]   exponent             - exponent.
 * @param[in]   base                 - base number.
 * @param[out]  out                  - out = base^(exponent) mod modulus.
 * @param[in]   mod_wordLen          - word length of modulus and base number.
 * @param[in]   exp_wordLen          - word length of exponent.
 * @return      PKE_SUCCESS(input is valid, allow to calculate),PKE_FINISHED(mod exponent finished),other(error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN
  @endverbatim
 */
unsigned int pke_modexp_check_input(unsigned int *modulus, unsigned int *exponent, unsigned int *base, unsigned int *out, unsigned int mod_wordLen, unsigned int exp_wordLen)
{
    int32_t flag;

    //base should be in [0,modulus]
    flag = uint32_BigNumCmp(base, mod_wordLen, modulus, mod_wordLen);
    if (flag > 0) {
        return PKE_INVALID_INPUT;
    } else {
        ;
    }

    //if base is 0 or n
    if ((0 == flag) || (1u == uint32_BigNum_Check_Zero(base, mod_wordLen))) {
        if (1u == uint32_BigNum_Check_Zero(exponent, exp_wordLen)) //0^0 mod n
        {
            return PKE_INVALID_INPUT;
        } else                                                     //if a is 0, e is not 0, the output is 0
        {
            uint32_clear(out, mod_wordLen);
            return PKE_FINISHED;
        }
    } else if (1u == uint32_BigNum_Check_Zero(exponent, exp_wordLen)) //base is in [1,modulus-1], e is 0, the output is 1
    {
        pke_set_operand_uint32_value(out, mod_wordLen, 1u);
        return PKE_FINISHED;
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       od exponent(for high level use, operands are all U8 big-endian big number), this could be used for rsa encrypting,decrypting,signing,verifying.
 * @param[in]   modulus              - modulus.
 * @param[in]   exponent             - exponent.
 * @param[in]   base                 - base number.
 * @param[out]  out                  - out = base^(exponent) mod modulus.
 * @param[in]   mod_bitLen           - real bit length of modulus and base number.
 * @param[in]   exp_bitLen           - real bit length of exponent.
 * @param[in]   calc_pre_monts       - if it is 0, no need to calculate the pre-calculated mont arguments of modulus, otherwise calculate.
 * @return      PKE_SUCCESS(input is valid, allow to calculate),PKE_FINISHED(mod exponent finished),other(error)
 * @note
  @verbatim
      -# 1.modulus must be odd
      -# 2.please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN
      -# 3.this is for high level application or protocol to use RSA mod exponent directly. all operands of this API are U8 big-endian big number.
  @endverbatim
 */
unsigned int pke_modexp_U8(unsigned char *modulus, unsigned char *exponent, unsigned char *base, unsigned char *out, unsigned int mod_bitLen, unsigned int exp_bitLen, unsigned int calc_pre_monts)
{
    unsigned int step_bytes, step_words;
    unsigned int mod_byteLen = GET_BYTE_LEN(mod_bitLen);
    unsigned int mod_wordLen = GET_WORD_LEN(mod_bitLen);
    unsigned int exp_byteLen = GET_BYTE_LEN(exp_bitLen);
    unsigned int exp_wordLen = GET_WORD_LEN(exp_bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(mod_bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand_U8((rPKE_B(3u, step_bytes)), modulus, mod_byteLen); //B3 modulus
    if (step_words > mod_wordLen) {
        uint32_clear((unsigned int *)(rPKE_B(3u, step_bytes)) + mod_wordLen, step_words - mod_wordLen);
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + mod_wordLen, step_words - mod_wordLen);
    } else {
        ;
    }

    if (calc_pre_monts) {
        pke_pre_calc_mont((unsigned int *)(rPKE_B(3u, step_bytes)), mod_bitLen, NULL, NULL);
    } else {
        ;
    }

    pke_load_operand_U8((unsigned int *)(rPKE_A(1u, step_bytes)), exponent, exp_byteLen); //A1 exponent
    if (step_words > exp_wordLen) {
        uint32_clear((unsigned int *)(rPKE_A(1u, step_bytes)) + exp_wordLen, step_words - exp_wordLen);
    } else {
        ;
    }

    pke_load_operand_U8((unsigned int *)(rPKE_B(0u, step_bytes)), base, mod_byteLen); //B0 base

    ret = pke_modexp_check_input((unsigned int *)(rPKE_B(3u, step_bytes)), (unsigned int *)(rPKE_A(1u, step_bytes)), (unsigned int *)(rPKE_B(0u, step_bytes)), (unsigned int *)(rPKE_A(0u, step_bytes)), mod_wordLen, exp_wordLen);
    if (PKE_FINISHED == ret) {
        goto END;
    } else if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_MODEXP);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODEXP);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), exp_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wordLen << 2);
#endif
        return ret;
    } else {
        ;
    }

END:

    pke_read_operand_U8((unsigned int *)(rPKE_A(0u, step_bytes)), out, mod_byteLen); //A0 result

    return PKE_SUCCESS;
}

/**
 * @brief       c = a mod b.
 * @param[in]   a           - nteger a.
 * @param[in]   aWordLen    - word length of integer.
 * @param[in]   b           - integer b, modulus.
 * @param[in]   b_h         - out = base^(exponent) mod modulus.
 * @param[in]   b_n0        - modulus ^(-1) mod 2^w, here w is 32 actually.
 * @param[in]   bWordLen    - word length of integer b and b_h.
 * @param[out]  c           - c = a mod b.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# b must be odd, and please make sure bWordLen is real word length of b.
      -# 2.real bit length of a can not be bigger than 2*(real bit length of b), so aWordLen can
        not be bigger than 2*bWordLen.
      -# 3.please make sure aWordLen <= 2*OPERAND_MAX_WORD_LEN, bWordLen <= OPERAND_MAX_WORD_LEN.
  @endverbatim
 */
unsigned int pke_mod(unsigned int *a, unsigned int aWordLen, unsigned int *b, unsigned int *b_h, unsigned int *b_n0, unsigned int bWordLen, unsigned int *c)
{
    unsigned int  step_bytes;
    int32_t       flag;
    unsigned int  bBitLen, bitLen, tmpLen;
    unsigned int *t1, *t2;
    unsigned int *t1_tmp;
    //    unsigned int t1[OPERAND_MAX_WORD_LEN], t2[OPERAND_MAX_WORD_LEN];
    unsigned int ret;

    flag = uint32_BigNumCmp(a, aWordLen, b, bWordLen);
    if (flag < 0) {
        aWordLen = get_valid_words(a, aWordLen);
        uint32_copy(c, a, aWordLen);
        uint32_clear(c + aWordLen, bWordLen - aWordLen);

        return PKE_SUCCESS;
    } else if (0 == flag) {
        uint32_clear(c, bWordLen);

        return PKE_SUCCESS;
    } else {
        ;
    }

    bBitLen    = get_valid_bits(b, bWordLen);
    step_bytes = pke_set_operand_width(bBitLen);

    t1     = (unsigned int *)(rPKE_A(1u, step_bytes));
    t2     = (unsigned int *)(rPKE_B(2u, step_bytes));
    t1_tmp = (unsigned int *)t1;

    bitLen = bBitLen & 0x1Fu;

    //get t2 = a high part mod b
    if (0u != bitLen) {
        tmpLen = aWordLen - bWordLen + 1u;
        uint32_copy(t2, a + bWordLen - 1u, tmpLen);
        Big_Div2n(t2, tmpLen, bitLen);
        if (tmpLen < bWordLen) {
            uint32_clear(t2 + tmpLen, bWordLen - tmpLen);
        } else if (uint32_BigNumCmp(t2, bWordLen, b, bWordLen) >= 0) {
            ret = pke_sub(t2, b, t2, bWordLen);
            if (PKE_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            ;
        }
    } else {
        tmpLen = aWordLen - bWordLen;
        if (uint32_BigNumCmp(a + bWordLen, tmpLen, b, bWordLen) >= 0) {
            ret = pke_sub(a + bWordLen, b, t2, bWordLen);
            if (PKE_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            uint32_copy(t2, a + bWordLen, tmpLen);
            uint32_clear(t2 + tmpLen, bWordLen - tmpLen);
        }
    }

    //set the pre-calculated mont parameters
    ret = pke_set_modulus_and_pre_monts(b, b_h, b_n0, bBitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get t1 = 1000...000 mod b
    uint32_clear(t1, bWordLen);
    if (0u != bitLen) {
        t1[bWordLen - 1u] = 1u << (bitLen);
    } else {
        ;
    }

    ret = pke_sub(t1, b, t1, bWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get t2 = a_high * 1000..000 mod b
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal(t1, t2, t2, bWordLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //get t1 = a low part mod b
    if (0u != bitLen) {
        uint32_copy(t1, a, bWordLen);
        t1[bWordLen - 1u] &= ((1u << (bitLen)) - 1u);
        if (uint32_BigNumCmp(t1, bWordLen, b, bWordLen) >= 0) {
            ret = pke_sub(t1, b, t1, bWordLen);
            if (PKE_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            ;
        }
    } else {
        if (uint32_BigNumCmp(a, bWordLen, b, bWordLen) >= 0) {
            ret = pke_sub(a, b, t1, bWordLen);
            if (PKE_SUCCESS != ret) {
                return ret;
            } else {
                ;
            }
        } else {
            //t1 = a;
            t1_tmp = (unsigned int *)a;
        }
    }

    //return pke_modadd(b, t1, t2, c, bWordLen);
    return pke_modadd(b, t1_tmp, t2, c, bWordLen);
}

/********************************** ECCp functions *************************************/

/**
 * @brief       ECCP curve point mul(random point), Q=[k]P.
 * @param[in]   curve   - eccp_curve_t curve struct pointer.
 * @param[in]   k       - scalar.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @param[out]  Qx      - x coordinate of point Q.
 * @param[out]  Qy      - y coordinate of point Q.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure k in [1,n-1], n is order of ECCP curve.
      -# 2.please make sure input point P is on the curve.
      -# 3.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
      -# 4.even if the input point P is valid, the output may be infinite point, in this case
       it will return error.
  @endverbatim
 */
unsigned int eccp_pointMul(eccp_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, GET_MAX_LEN(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), Px, pWordLen);            //B0 Px
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), Py, pWordLen);            //B1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a, pWordLen); //A5 a
    pke_load_operand((unsigned int *)(rPKE_A(4u, step_bytes)), k, nWordLen);             //A4 k
    //pke_load_operand((unsigned int *)(rPKE_B(5u,step_bytes)), curve->eccp_n, nWordLen);          //B5 n

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_B(1u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(5u, step_bytes)) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    if (step_words > nWordLen) {
        uint32_clear((unsigned int *)(rPKE_A(4u, step_bytes)) + nWordLen, step_words - nWordLen);
        //uint32_clear((unsigned int *)(rPKE_B(5u,step_bytes))+nWordLen, step_words-nWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_MUL);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PMUL);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), nWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(5u, step_bytes)), nWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), pWordLen << 2);
#endif
        return ret;
    } else {
        ;
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), Qx, pWordLen);     //A0 Qx
    if (NULL != Qy) {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), Qy, pWordLen); //A1 Qy
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       ECCP curve point add, Q=P1+P2.
 * @param[in]   curve       - eccp_curve_t curve struct pointer.
 * @param[in]   P1x         - x coordinate of point P1.
 * @param[in]   P1y         - x coordinate of point P1.
 * @param[in]   P2x         - x coordinate of point P2.
 * @param[in]   P2y         - y coordinate of point P2.
 * @param[out]  Qx          - x coordinate of point Q=P1+P2.
 * @param[out]  Qy          - y coordinate of point Q=P1+P2.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P1 and P2 are both on the curve.
      -# 2.please make sure input point P is on the curve.
      -# 3.even if the input point P1 and P2 are valid, it will return error in the following 2 cases.
           (1). P1 = P2. return PKE_NO_MODINV.
           (2). P1 = -P2. return PKE_NO_MODINV. actually the output point is neutral point(point at infinity)
  @endverbatim
 */
unsigned int eccp_pointAdd(eccp_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, GET_MAX_LEN(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    //pke_pre_calc_mont() may cover A1, so load A1(P1x) here
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), P1x, pWordLen);           //A0 P1x
    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), P1y, pWordLen);           //A1 P1y
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), P2x, pWordLen);           //B0 P2x
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), P2y, pWordLen);           //B1 P2y
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a, pWordLen); //A5 a

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)(rPKE_A(0u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(1u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_B(1u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(5u, step_bytes)) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_ADD);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PADD);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), pWordLen << 2);
#endif
        return ret;
    } else {
        ;
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), Qx, pWordLen);     //A0 Qx
    if (NULL != Qy) {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), Qy, pWordLen); //A1 Qy
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       out = (a+b) mod modulus or out = (a-b) mod modulus
 * @param[in]   curve              - eccp_curve_t curve struct pointer
 * @param[in]   P1x                - x coordinate of point P1
 * @param[in]   P1y                - y coordinate of point P1
 * @param[in]   P2x                - x coordinate of point P2
 * @param[in]   P2y                - y coordinate of point P2
 * @param[out]   Qx                - x coordinate of point Q=P1+P2
 * @param[out]   Qy                - y coordinate of point Q=P1+P2
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P1 and P2 are both on the curve
      -# 2.please make sure bit length of the curve is not greater than ECCP_MAX_BIT_LEN
      -# 3.if P1 = -P2, it will return PKE_NO_MODINV. actually the output point is neutral point(point at infinity)
  @endverbatim
 */
unsigned int eccp_pointAdd_safe(eccp_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes;
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(GET_MAX_LEN(curve->eccp_p_bitLen, curve->eccp_n_bitLen));

#ifdef PKE_SEC
    if (0 == uint32_BigNumCmp_sec(P1x, pWordLen, P2x, pWordLen)) {
        if (0 == uint32_BigNumCmp_sec(P1y, pWordLen, P2y, pWordLen))
#else
    if (0 == uint32_BigNumCmp(P1x, pWordLen, P2x, pWordLen)) {
        if (0 == uint32_BigNumCmp(P1y, pWordLen, P2y, pWordLen))
#endif
        {
#ifdef ECCP_POINT_DOUBLE
            ret = eccp_pointDouble(curve, P1x, P1y, Qx, Qy);
#else
            uint32_clear((unsigned int *)(rPKE_A(4u, step_bytes)), nWordLen);
            ((unsigned int *)(rPKE_A(4u, step_bytes)))[0] = 2u;
            ret                                           = eccp_pointMul(curve, (unsigned int *)(rPKE_A(4u, step_bytes)), P1x, P1y, Qx, Qy);
#endif
        } else {
            ret = PKE_NO_MODINV;
        }
    } else {
        ret = eccp_pointAdd(curve, P1x, P1y, P2x, P2y, Qx, Qy);
    }

    return ret;
}


#ifdef ECCP_POINT_DOUBLE
/* function: ECCP curve point double, Q=[2]P
 * parameters:
 *     curve ---------------------- input, eccp_curve_t curve struct pointer
 *     Px ------------------------- input, x coordinate of point P
 *     Py ------------------------- input, y coordinate of point P
 *     Qx ------------------------- output, x coordinate of point Q=[2]P
 *     Qy ------------------------- output, y coordinate of point Q=[2]P
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure input point P is on the curve
 *     2. please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
 */
unsigned int eccp_pointDouble(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, GET_MAX_LEN(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    //pke_pre_calc_mont() may cover A1, so load A1(Px) and other paras here
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), Px, pWordLen);            //A0 Px
    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), Py, pWordLen);            //A1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a, pWordLen); //A5 a

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)(rPKE_A(0u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(1u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(5u, step_bytes)) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_DBL);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PDBL);
    if (PKE_SUCCESS != ret) {
    #ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), pWordLen << 2);
    #endif
        return ret;
    } else {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), Qx, pWordLen); //A0 Qx
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), Qy, pWordLen); //A1 Qy

        return PKE_SUCCESS;
    }
}
#endif


/**
 * @brief       check whether the input point P is on ECCP curve or not.
 * @param[in]   curve   - eccp_curve_t curve struct pointer.
 * @param[in]   Px      - x coordinate of point P.
 * @param[in]   Py      - y coordinate of point P.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
      -# 2.after calculation, A1 and A2 will be changed!.
  @endverbatim
 */
unsigned int eccp_pointVerify(eccp_curve_t *curve, unsigned int *Px, unsigned int *Py)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, GET_MAX_LEN(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    //pke_pre_calc_mont() may cover A1, so load A1(Px) and other paras here
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), Px, pWordLen);            //B0 Px
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), Py, pWordLen);            //B1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a, pWordLen); //A5 a
    pke_load_operand((unsigned int *)(rPKE_A(4u, step_bytes)), curve->eccp_b, pWordLen); //A4 b

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)(rPKE_B(0u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_B(1u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(5u, step_bytes)) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)(rPKE_A(4u, step_bytes)) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_VER);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PVER);
    if (PKE_SUCCESS != ret) {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), pWordLen << 2);
#endif
        return ret;
    } else {
        return PKE_SUCCESS;
    }
}

/**
 * @brief       get ECCP public key from private key(the key pair could be used in SM2/ECDSA/ECDH, etc.).
 * @param[in]   curve        - eccp_curve_t curve struct pointer.
 * @param[in]   priKey       - private key, big-endian.
 * @param[out]  pubKey       - public key, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
  @endverbatim
 */
unsigned int eccp_get_pubkey_from_prikey(eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int  step_bytes;
    unsigned int  nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int  nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    unsigned int  pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    unsigned int  k[ECCP_MAX_WORD_LEN];
    unsigned int *x;
    unsigned int *y;
    unsigned int  ret;

    step_bytes = pke_set_operand_width(curve->eccp_p_bitLen);
    x          = (unsigned int *)(rPKE_A(0u, step_bytes));
    y          = (unsigned int *)(rPKE_A(1u, step_bytes));

    k[nWordLen - 1u] = 0u; //clear if curve->eccp_n_bitLen is not a multiple of 32
    reverse_byte_array(priKey, (unsigned char *)k, nByteLen);

    //make sure k in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, nWordLen, PKE_ZERO_ALL, PKE_INTEGER_TOO_BIG, PKE_SUCCESS);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

#ifdef SUPPORT_SM2
    if (curve == sm2_curve) {
        if ((k[0] == (sm2_curve->eccp_n[0] - 1u)) && (0 == uint32_BigNumCmp(k + 1u, nWordLen - 1u, (curve->eccp_n) + 1u, nWordLen - 1u))) {
            return PKE_INTEGER_TOO_BIG;
        } else {
            ;
        }
    } else {
        ;
    }
#endif

    //get pubKey
    ret = eccp_pointMul(curve, k, curve->eccp_Gx, curve->eccp_Gy, x, y);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        reverse_byte_array((unsigned char *)x, pubKey, pByteLen);
        reverse_byte_array((unsigned char *)y, pubKey + pByteLen, pByteLen);

        return PKE_SUCCESS;
    }
}

/**
 * @brief       get ECCP key pair(the key pair could be used in SM2/ECDSA/ECDH).
 * @param[in]   curve        - eccp_curve_t curve struct pointer.
 * @param[out]  priKey       - private key, big-endian.
 * @param[out]  pubKey       - public key, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
  @endverbatim
 */
unsigned int eccp_getkey(eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int tmpLen;
    unsigned int nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);
    unsigned int ret;

ECCP_GETKEY_LOOP:

    ret = get_rand(priKey, nByteLen);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
    }

    //make sure k has the same bit length as n
    tmpLen = (curve->eccp_n_bitLen) & 7u;
    if (0u != tmpLen) {
        priKey[0] &= (1u << (tmpLen)) - 1u;
    } else {
        ;
    }

    ret = eccp_get_pubkey_from_prikey(curve, priKey, pubKey);
    if ((PKE_ZERO_ALL == ret) || (PKE_INTEGER_TOO_BIG == ret)) {
        goto ECCP_GETKEY_LOOP;
    } else {
        return ret;
    }
}

/****************************** ECCp functions finished ********************************/


#ifdef SUPPORT_C25519
/**************************** X25519 & Ed25519 functions *******************************/

/**
 * @brief       c25519 point mul(random point), Q=[k]P
 * @param[in]   curve              - c25519 curve struct pointer
 * @param[in]   k                  - scalar
 * @param[in]   Pu                 - u coordinate of point P
 * @param[out]  Qu                 - u coordinate of point Q
 * @return      PKE_SUCCESS(success)     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P is on the curve
      -# 2.even if the input point P is valid, the output may be infinite point, in this case return error.
      -# 3.please make sure the curve is c25519
  @endverbatim
 */
unsigned int x25519_pointMul(mont_curve_t *curve, unsigned int *k, unsigned int *Pu, unsigned int *Qu)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->n_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)rPKE_A(0u, step_bytes), Pu, pWordLen);         //A0 Pu
    pke_load_operand((unsigned int *)rPKE_B(0u, step_bytes), curve->a24, pWordLen); //B0 a24
    pke_load_operand((unsigned int *)rPKE_A(4u, step_bytes), k, nWordLen);          //A4 k

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)rPKE_A(0u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(0u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(3u, step_bytes) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    if (step_words > nWordLen) {
        uint32_clear((unsigned int *)rPKE_A(4u, step_bytes) + nWordLen, step_words - nWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_C25519_PMUL);
    if (PKE_SUCCESS != ret) {
    #ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), nWordLen << 2);
    #endif
        return ret;
    } else {
        ;
    }

    pke_read_operand((unsigned int *)rPKE_A(1u, step_bytes), Qu, pWordLen); //A1 Qu

    return PKE_SUCCESS;
}

    #if 0
/* function: out = a^b mod n
 * parameters:
 *     a -------------------------- input, base number, 8 words
 *     b -------------------------- input, exponent number, 8 words
 *     n -------------------------- input, modulus number, 8 words
 *     out ------------------------ output, out = a^b mod n
 * return: PKE_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure n is odd, b is not zero
 *     2. this function is used in Ed25519 to decode point
 */
unsigned int mod_exp(unsigned int a[8], unsigned int b[8], unsigned int n[8], unsigned int out[8])
{
    unsigned int t[8];
    int32_t cfg_bak, bitLen;
    unsigned int ret;

    pke_pre_calc_mont(n, 256u, NULL, NULL);

    cfg_bak = PKE_EXE_CONF;
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_MONT);

    //t = A0 = aR mod n
    ret = pke_modmul_internal(a, (unsigned int *)(rPKE_A(3u,step_bytes)), t, Ed25519_WORD_LEN);     //A3: R^2 mod n
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    bitLen = get_valid_bits(b, Ed25519_WORD_LEN);
    bitLen -= 2;
    for(; bitLen>=0; bitLen--)
    {
        ret = pke_modmul_internal((unsigned int *)(rPKE_A(0u,step_bytes)), (unsigned int *)(rPKE_A(0u,step_bytes)), (unsigned int *)(rPKE_A(0u,step_bytes)), Ed25519_WORD_LEN);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else if(b[bitLen>>5] & (1u<<(bitLen&31)))
        {
            ret = pke_modmul_internal((unsigned int *)(rPKE_A(0u,step_bytes)), t, (unsigned int *)(rPKE_A(0u,step_bytes)), Ed25519_WORD_LEN);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
        else
        {;}
    }

    //t = 1
    pke_set_operand_uint32_value(t, 8u, 1u);

    //get result
    ret = pke_modmul_internal((unsigned int *)(rPKE_A(0u,step_bytes)), t, out, Ed25519_WORD_LEN);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    pke_set_exe_cfg(cfg_bak);

    return PKE_SUCCESS;
}
    #endif


/**
 * @brief       Ed25519 decode point
 * @param[in]   in_y              - encoded Ed25519 point
 * @param[in]   out_x             - x coordinate of input point
 * @param[in]   out_y             - y coordinate of input point
 * @return      PKE_SUCCESS(success)     other:error
 */
unsigned int ed25519_decode_point(unsigned char in_y[32], unsigned char out_x[32], unsigned char out_y[32])
{
    unsigned int u[Ed25519_WORD_LEN];
    unsigned int v[Ed25519_WORD_LEN];
    unsigned int t[Ed25519_WORD_LEN];
    unsigned int t2[Ed25519_WORD_LEN];
    unsigned int t3[Ed25519_WORD_LEN];
    unsigned int ret;

    //get y
    memcpy_((unsigned char *)u, in_y, Ed25519_BYTE_LEN);
    u[Ed25519_WORD_LEN - 1] &= 0x7FFFFFFFu;

    //make sure y < prime p
    if (uint32_BigNumCmp(u, Ed25519_WORD_LEN, ed25519->p, Ed25519_WORD_LEN) >= 0) {
        return PKE_INVALID_INPUT;
    } else {
        ;
    }

    //set type
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    //set pre-calculated paras
    ret = pke_set_modulus_and_pre_monts(ed25519->p, ed25519->p_h, ed25519->p_n0, ed25519->p_bitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(u, u, v, Ed25519_WORD_LEN); //v = y^2
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    uint32_clear(t, Ed25519_WORD_LEN);
    t[0] = 1u;
    ret  = pke_modsub(ed25519->p, v, t, u, Ed25519_WORD_LEN); //u = y^2 - 1
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(ed25519->d, v, v, Ed25519_WORD_LEN); //v = d*y^2
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modadd(ed25519->p, v, t, v, Ed25519_WORD_LEN); //v = d*y^2 + 1
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(v, v, t2, Ed25519_WORD_LEN); //t2 = v^2
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(v, t2, t3, Ed25519_WORD_LEN); //t3 = v^3
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t3, u, t, Ed25519_WORD_LEN); //t = u*v^3
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t2, t2, t2, Ed25519_WORD_LEN); //t2 = v^4
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t2, t3, t2, Ed25519_WORD_LEN); //t2 = v^7
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t2, u, t2, Ed25519_WORD_LEN); //t2 = u*v^7
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //t3 = (p-5)/8
    uint32_copy(t3, ed25519->p, Ed25519_WORD_LEN);
    t3[0] -= 5u;
    Big_Div2n(t3, Ed25519_WORD_LEN, 3u);

    //t2 = (u*v^7 )^((p-5)/8)
    #if 0
    ret = mod_exp(t2, t3, ed25519->p, t2);
    #else
    ret = pke_modexp(ed25519->p, t3, t2, t2, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    #endif
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t2, t, t, Ed25519_WORD_LEN); //t = x = (u*v^3)*(u*v^7 )^((p-5)/8)
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t, t, t2, Ed25519_WORD_LEN); //t2 = x^2
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    ret = pke_modmul_internal(t2, v, t2, Ed25519_WORD_LEN); //t2 = v*x^2
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    if (0 == uint32_BigNumCmp(t2, Ed25519_WORD_LEN, u, Ed25519_WORD_LEN)) //if v x^2 = u (mod p), x is a square root.
    {
        goto result;
    } else {
        ;
    }

    ret = pke_sub(ed25519->p, u, t3, Ed25519_WORD_LEN); //t3 = -u mod p
    if (PKE_SUCCESS != ret) {
        return ret;
    } else if (0 == uint32_BigNumCmp(t2, Ed25519_WORD_LEN, t3, Ed25519_WORD_LEN)) {
        //v = (p-1)/4
        uint32_copy(v, ed25519->p, Ed25519_WORD_LEN);
        v[0] -= 1u;
        Big_Div2n(v, Ed25519_WORD_LEN, 2u);

        //t2 = 2
        pke_set_operand_uint32_value(t2, Ed25519_WORD_LEN, 2);

        //u = 2^((p-1)/4)
    #if 0
        ret = mod_exp(t2, v, ed25519->p, u);
    #else
        ret = pke_modexp(ed25519->p, v, t2, u, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    #endif
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        ret = pke_modmul_internal(t, u, t, Ed25519_WORD_LEN); //t = x*(2^((p-1)/4))
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            ;
        }

        goto result;
    } else {
        ;
    }

    return PKE_INVALID_INPUT; //root not exist

result:

    //if x=0 and x is odd, decode fail
    if ((1u == uint32_BigNum_Check_Zero(t, Ed25519_WORD_LEN)) && (0u != (((unsigned int)(in_y[Ed25519_BYTE_LEN - 1u])) & 0x80u))) {
        return PKE_INVALID_INPUT;
    } else {
        ;
    }

    //get out_x
    if ((t[0] & 1u) == (((unsigned int)(in_y[Ed25519_BYTE_LEN - 1u])) >> 7)) {
        memcpy_(out_x, (unsigned char *)t, Ed25519_BYTE_LEN);
    } else {
        ret = pke_sub(ed25519->p, t, v, Ed25519_WORD_LEN); //v = -x mod p
        if (PKE_SUCCESS != ret) {
            return ret;
        } else {
            memcpy_(out_x, (unsigned char *)v, Ed25519_BYTE_LEN);
        }
    }

    //get out_y
    memcpy_(out_y, in_y, Ed25519_BYTE_LEN);
    out_y[Ed25519_BYTE_LEN - 1u] &= ((unsigned char)0x7F);

    return PKE_SUCCESS;
}

/**
 * @brief       edwards25519 curve point mul(random point), Q=[k]P
 * @param[in]   curve                - edwards25519 curve struct pointer
 * @param[in]   k                    - scalar
 * @param[in]   Px                   - x coordinate of point P
 * @param[in]   Py                   - y coordinate of point P
 * @param[out]  Qx                   - x coordinate of point Q
 * @param[out]  Qy                   - y coordinate of point Q
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P is on the curve
      -# 2.even if the input point P is valid, the output may be neutral point (0, 1), it is valid
      -# 3.please make sure the curve is edwards25519
      -# 4.k could not be zero now.
  @endverbatim
 */
unsigned int ed25519_pointMul(edward_curve_t *curve, unsigned int *k, unsigned int *Px, unsigned int *Py, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int nWordLen = GET_WORD_LEN(curve->n_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)rPKE_A(1u, step_bytes), Px, pWordLen);       //A1 Px
    pke_load_operand((unsigned int *)rPKE_A(2u, step_bytes), Py, pWordLen);       //A2 Py
    pke_load_operand((unsigned int *)rPKE_B(0u, step_bytes), curve->d, pWordLen); //B0 d
    pke_load_operand((unsigned int *)rPKE_A(0u, step_bytes), k, nWordLen);        //A0 k

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)rPKE_A(1u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_A(2u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(3u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_A(3u, step_bytes) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    if (step_words > nWordLen) {
        uint32_clear((unsigned int *)rPKE_A(0u, step_bytes) + nWordLen, step_words - nWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PMUL);
    if (PKE_SUCCESS != ret) {
    #ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), nWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(2u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), pWordLen << 2);
    #endif
        return ret;
    } else {
        ;
    }

    pke_read_operand((unsigned int *)rPKE_A(1u, step_bytes), Qx, pWordLen);     //A1 Qx
    if (NULL != Qy) {
        pke_read_operand((unsigned int *)rPKE_A(2u, step_bytes), Qy, pWordLen); //A2 Qx
    } else {
        ;
    }

    return PKE_SUCCESS;
}

/**
 * @brief       edwards25519 point add, Q=P1+P2
 * @param[in]   curve                - edwards25519 curve struct pointer
 * @param[in]   P1x                  - x coordinate of point P1
 * @param[in]   P1y                  - y coordinate of point P1
 * @param[in]   P2x                  - x coordinate of point P2
 * @param[in]   P2y                  - y coordinate of point P2
 * @param[out]  Qx                   - x coordinate of point Q=P1+P2
 * @param[out]  Qy                   - y coordinate of point Q=P1+P2
 * @return      PKE_SUCCESS(success),     other:error
 * @note
  @verbatim
      -# 1.please make sure input point P1 and P2 are both on the curve
      -# 2.the output point may be neutral point (0, 1), it is valid
      -# 3.please make sure the curve is edwards25519
  @endverbatim
 */
unsigned int ed25519_pointAdd(edward_curve_t *curve, unsigned int *P1x, unsigned int *P1y, unsigned int *P2x, unsigned int *P2y, unsigned int *Qx, unsigned int *Qy)
{
    unsigned int step_bytes, step_words;
    unsigned int pWordLen = GET_WORD_LEN(curve->p_bitLen);
    unsigned int ret;

    //set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if (PKE_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    //pke_pre_calc_mont() may cover some addresses, so load parameters here
    pke_load_operand((unsigned int *)rPKE_A(1u, step_bytes), P1x, pWordLen);      //A1 P1x
    pke_load_operand((unsigned int *)rPKE_A(2u, step_bytes), P1y, pWordLen);      //A2 P1y
    pke_load_operand((unsigned int *)rPKE_B(1u, step_bytes), P2x, pWordLen);      //B1 P2x
    pke_load_operand((unsigned int *)rPKE_B(2u, step_bytes), P2y, pWordLen);      //B2 P2y
    pke_load_operand((unsigned int *)rPKE_B(0u, step_bytes), curve->d, pWordLen); //B0 d

    if (step_words > pWordLen) {
        uint32_clear((unsigned int *)rPKE_A(1u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_A(2u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(1u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(2u, step_bytes) + pWordLen, step_words - pWordLen);
        uint32_clear((unsigned int *)rPKE_B(0u, step_bytes) + pWordLen, step_words - pWordLen);
    } else {
        ;
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PADD);
    if (PKE_SUCCESS != ret) {
    #ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(2u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), pWordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(2u, step_bytes)), pWordLen << 2);
    #endif
        return ret;
    } else {
        ;
    }

    pke_read_operand((unsigned int *)rPKE_A(1u, step_bytes), Qx, pWordLen); //A1 Qx
    pke_read_operand((unsigned int *)rPKE_A(2u, step_bytes), Qy, pWordLen); //A2 Qy

    return PKE_SUCCESS;
}

/**************************** X25519 & Ed25519 finished ********************************/
#endif
