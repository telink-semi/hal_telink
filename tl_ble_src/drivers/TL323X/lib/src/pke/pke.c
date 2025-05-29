/********************************************************************************************************
 * @file    pke.c
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
#include "lib/include/pke/pke.h"
#include "lib/include/trng/trng.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng_basic.h"

#ifdef SUPPORT_STATIC_ANALYSIS
#ifdef CONFIG_UNIT_TEST
volatile pke_reg_t *g_pke_reg = (pke_reg_t *)PKE_BASE_ADDR;
#else
volatile pke_reg_t *const g_pke_reg = (pke_reg_t *)PKE_BASE_ADDR;
#endif
#endif

/**
 * @brief           Compute out = (a+b) mod modulus or out = (a-b) mod modulus
 * @param[in]       modulus              - Modulus
 * @param[in]       a                    - Integer a
 * @param[in]       b                    - Integer b
 * @param[out]      out                  - Result of (a+b) mod modulus or (a-b) mod modulus
 * @param[in]       wlen                 - Word length of modulus, a, and b
 * @param[in]       micro_code           - Must be MICROCODE_MODADD or MICROCODE_MODSUB
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. a and b must be less than modulus
 *        2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
 */
FLAG_STATIC unsigned int pke_modadd_modsub_internal(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen,
                                                    unsigned int micro_code);

// Maintenance API
/**
 * @brief           to clear finished and interrupt tag
 * @return          None
 */
void pke_clr_irq_status(void)
{
    pke_clear_interrupt();
}

/**
 * @brief           get pke irq status
 * @return          None
 */
unsigned int pke_get_irq_status(void)
{
    return rPKE_RISR;
}

/**
 * @brief           to start PKE calculation
 * @return          None
 */
void pke_opr_start(void)
{
    pke_start();
}

/**
 * @brief           Computes out = (a + b) mod modulus.
 * @param[in]       modulus              - Modulus value.
 * @param[in]       a                    - Integer a, must be less than modulus.
 * @param[in]       b                    - Integer b, must be less than modulus.
 * @param[out]      out                  - Result of (a + b) mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b, must not be bigger than OPERAND_MAX_WORD_LEN.
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. a and b must be less than modulus.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_mod_add(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wlen, MICROCODE_MODADD);
}

/**
 * @brief           Computes out = (a + b) mod modulus.
 * @param[in]       modulus              - Modulus value.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of (a + b) mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. a and b must be less than modulus.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_mod_sub(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wlen, MICROCODE_MODSUB);
}

/**
 * @brief           Computes out = a * b (mod modulus).
 * @param[in]       modulus              - Modulus value.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a * b mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. a, b must be less than modulus.
 *        3. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_mod_mul(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_modmul(modulus, a, b, out, wlen);
}

/**
 * @brief           Computes the modular inverse: ainv = a^(-1) mod modulus.
 * @param[in]       modulus              - Modulus value.
 * @param[in]       a                    - Integer a.
 * @param[out]      ainv                 - ainv = a^(-1) mod modulus.
 * @param[in]       mod_wlen             - Word length of modulus and ainv.
 * @param[in]       a_wlen               - Word length of a.
 * @return          PKE_SUCCESS on success, other values indicate an error or inverse does not exist.
 * @note
 *        1. Please make sure a_wlen <= mod_wlen <= OPERAND_MAX_WORD_LEN and a < modulus.
 */
unsigned int pke_mod_inv(const unsigned int *modulus, const unsigned int *a, unsigned int *ainv, unsigned int mod_wlen, unsigned int a_wlen)
{
    return pke_modinv(modulus, a, ainv, mod_wlen, a_wlen);
}

/**
 * @brief           ECCP curve point multiplication (random point), Q = [k]P.
 * @param[in]       curve                - Pointer to eccp_curve_t curve struct.
 * @param[in]       k                    - Scalar.
 * @param[in]       px                   - x coordinate of point P (unsigned char big-endian big number).
 * @param[in]       Py                   - y coordinate of point P (unsigned char big-endian big number).
 * @param[out]      qx                   - x coordinate of point Q (unsigned char big-endian big number).
 * @param[out]      qy                   - y coordinate of point Q (unsigned char big-endian big number).
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure k is in [1, n-1], where n is the order of the ECCP curve.
 *        2. Please make sure the input point P is on the curve.
 *        3. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 *        4. Even if the input point P is valid, the output may be the infinite point, in which case it will return an error.
 */
unsigned int pke_eccp_point_mul(const eccp_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    return eccp_pointmul(curve, k, px, Py, qx, qy);
}

/**
 * @brief           ECCP curve point addition, Q = P1 + P2.
 * @param[in]       curve                - Pointer to eccp_curve_t curve struct.
 * @param[in]       x1                   - x coordinate of point P1 (unsigned char big-endian big number).
 * @param[in]       y1                   - y coordinate of point P1 (unsigned char big-endian big number).
 * @param[in]       x2                   - x coordinate of point P2 (unsigned char big-endian big number).
 * @param[in]       y2                   - y coordinate of point P2 (unsigned char big-endian big number).
 * @param[out]      qx                   - x coordinate of point Q = P1 + P2 (unsigned char big-endian big number).
 * @param[out]      qy                   - y coordinate of point Q = P1 + P2 (unsigned char big-endian big number).
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure input points P1 and P2 are both on the curve.
 *        2. Please make sure the bit length of the curve is not greater than ECCP_MAX_BIT_LEN.
 *        3. Even if the input points P1 and P2 are valid, it will return an error in the following cases:
 *        3.1. P1 = P2. Return PKE_NO_MODINV.
 *        3.2. P1 = -P2. Return PKE_NO_MODINV. Actually, the output point is the neutral point (point at infinity).
 */
unsigned int pke_eccp_point_add(const eccp_curve_t *curve, const unsigned int *x1, const unsigned int *y1, const unsigned int *x2, const unsigned int *y2, unsigned int *qx,
                                unsigned int *qy)
{
    return eccp_pointadd(curve, x1, y1, x2, y2, qx, qy);
}

/**
 * @brief           Checks whether the input point P is on the ECCP curve or not.
 * @param[in]       curve                - Pointer to eccp_curve_t curve struct.
 * @param[in]       px                   - x coordinate of point P (unsigned char big-endian big number).
 * @param[in]       Py                   - y coordinate of point P (unsigned char big-endian big number).
 * @return          PKE_SUCCESS on success, point is on the curve), other values indicate an error or not on the curve.
 * @note
 *        1. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 */
unsigned int pke_eccp_point_verify(const eccp_curve_t *curve, const unsigned int *px, const unsigned int *Py)
{
    return eccp_pointverify(curve, px, Py);
}

#ifdef SUPPORT_C25519
/**
 * @brief           C25519 point multiplication (random point), Q = [k]P.
 * @param[in]       curve                - Pointer to c25519 curve struct.
 * @param[in]       k                    - Scalar.
 * @param[in]       p                    - u coordinate of point P (unsigned char big-endian).
 * @param[out]      q                    - u coordinate of point Q (unsigned char big-endian).
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure the input point P is on the curve.
 *        2. Even if the input point P is valid, the output may be the infinite point, in which case it will return an error.
 *        3. Please make sure the curve is C25519.
 */
unsigned int pke_x25519_point_mul(const mont_curve_t *curve, const unsigned int *k, const unsigned int *p, unsigned int *q)
{
    return x25519_pointmul(curve, k, p, q);
}

/**
 * @brief           Edwards25519 curve point multiplication (random point), Q = [k]P.
 * @param[in]       curve                - Pointer to edwards25519 curve struct.
 * @param[in]       k                    - Scalar.
 * @param[in]       px                   - x coordinate of point P (unsigned char big-endian).
 * @param[in]       Py                   - y coordinate of point P (unsigned char big-endian).
 * @param[out]      qx                   - x coordinate of point Q (unsigned char big-endian).
 * @param[out]      qy                   - y coordinate of point Q (unsigned char big-endian).
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure the input point P is on the curve.
 *        2. Even if the input point P is valid, the output may be the neutral point (0, 1), which is valid.
 *        3. Please make sure the curve is Edwards25519.
 *        4. k could not be zero.
 */
unsigned int pke_ed25519_point_mul(const edward_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    return ed25519_pointMul(curve, k, px, Py, qx, qy);
}

/**
 * @brief           Edwards25519 point addition, Q = P1 + P2.
 * @param[in]       curve                - Pointer to edwards25519 curve struct.
 * @param[in]       x1                   - x coordinate of point P1 (unsigned char big-endian).
 * @param[in]       y1                   - y coordinate of point P1 (unsigned char big-endian).
 * @param[in]       x2                   - x coordinate of point P2 (unsigned char big-endian).
 * @param[in]       y2                   - y coordinate of point P2 (unsigned char big-endian).
 * @param[out]      qx                   - x coordinate of point Q = P1 + P2 (unsigned char big-endian).
 * @param[out]      qy                   - y coordinate of point Q = P1 + P2 (unsigned char big-endian).
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure the input points P1 and P2 are both on the curve.
 *        2. The output point may be the neutral point (0, 1), which is valid.
 *        3. Please make sure the curve is Edwards25519.
 */
unsigned int pke_ed25519_point_add(const edward_curve_t *curve, const unsigned int *x1, const unsigned int *y1, const unsigned int *x2, const unsigned int *y2, unsigned int *qx,
                                   unsigned int *qy)
{
    return ed25519_pointAdd(curve, x1, y1, x2, y2, qx, qy);
}
#endif

/**
 * @brief           Compute out = a - b.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a - b.
 * @param[in]       wlen                 - Word length of a, b, and out.
 * @return          PKE_SUCCESS on success, other values indicate an error.
 * @note
 *        1. Please make sure a > b.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int sub_u32(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_sub(a, b, out, wlen);
}

// APIs
#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
/**
 * @brief           Perform a = a * (2^n) or a = a << n.
 * @param[in,out]   a                    - Input/big integer.
 * @param[in]       a_wlen               - Word length of a.
 * @param[in]       n                    - Exponent n.
 * @return          Result word length.
 */
unsigned int big_mul_2n(unsigned int a[], int32_t a_wlen, unsigned char n)
{
    int32_t i;
    unsigned char flag = 0;

    if (0 == a_wlen)
    {
        return 0u;
    }
    else
    {
    }

    if (a[a_wlen - 1] & (0xFFFFFFFF << (32u - ((unsigned int)n)))) // need carry
    {
        a[a_wlen] = a[a_wlen - 1] >> (32u - ((unsigned int)n));
        flag = (unsigned char)1;
    }
    else
    {
    }

    for (i = a_wlen - 1; i > 0; i--)
    {
        a[i] <<= n;
        a[i] |= (a[i - 1] >> (32u - ((unsigned int)n)));
    }
    a[i] <<= n;

    if (((unsigned char)0) != flag)
    {
        return (unsigned int)(a_wlen + 1);
    }
    else
    {
        return (unsigned int)a_wlen;
    }
}

/**
 * @brief           Get J0.
 * @param[in]       n0                   - A unsigned int odd integer.
 * @return          J0, (-n0^(-1) mod 2^w), here w is 32 actually.
 */
unsigned int get_j0(unsigned int n0)
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
 * @brief           Get PKE RAM A slot.
 * @param[in]       index                - Index of slot (0, 1, 2, ...).
 * @param[in]       step                 - Byte length of slot.
 * @return          Unsigned int pointer to the slot.
 */
unsigned int *rPKE_A(unsigned int index, unsigned int step)
{
#ifdef CONFIG_UNIT_TEST
    return (unsigned int *)(&(PKE_RAM[(0x0400U + (index * step)) / 4]));
#else
    return (unsigned int *)(PKE_BASE_ADDR + 0x0400U + (index * step));
#endif
}

/**
 * @brief           Get PKE RAM B slot.
 * @param[in]       index                - Index of slot (0, 1, 2, ...).
 * @param[in]       step                 - Byte length of slot.
 * @return          Pointer to the slot as unsigned int.
 */
unsigned int *rPKE_B(unsigned int index, unsigned int step)
{
#ifdef CONFIG_UNIT_TEST
    return (unsigned int *)(&(PKE_RAM[(0x1000U + (index * step)) / 4]));
#else
    return (unsigned int *)(PKE_BASE_ADDR + 0x1000U + (index * step));
#endif
}

/**
 * @brief           Get PKE IP version.
 * @return          PKE IP version.
 */
unsigned int pke_get_version(void)
{
    return rPKE_VERSION;
}

/**
 * @brief           Get PKE driver version (software version).
 * @return          PKE driver version.
 */
unsigned int pke_get_driver_version(void)
{
    // the meaning of the version(for example, if the return value is 0x23080301)
    // the first 3 bytes:  23.08.03 ---- date
    // the last byte:      01       ---- first version on the day
    return (((unsigned int)0x24U) << 24U) | (((unsigned int)0x08U) << 16U) | (((unsigned int)0x15U) << 8U) | 0x01U;
}

/**
 * @brief           Clear finished and interrupt tag.
 * @return          None.
 */
void pke_clear_interrupt(void)
{
    MEM_VOLATILE unsigned int mask = ~((unsigned int)1);

#if 1
    rPKE_RISR &= mask; // write 0 to clear
#else
    MEM_VOLATILE unsigned int flag = 1u;

    if (rPKE_RISR & flag)
    {
        rPKE_RISR &= mask; // write 0 to clear
    }
    else
    {
        ;
    }
#endif
}

/**
 * @brief           to enable PKE interrupt
 * @return          None
 */
void pke_enable_interrupt(void)
{
    MEM_VOLATILE unsigned int flag = (unsigned int)1;

    rPKE_IMCR |= flag;
}

/**
 * @brief           to disable PKE interrupt
 * @return          None
 */
void pke_disable_interrupt(void)
{
    MEM_VOLATILE unsigned int mask = ~((unsigned int)1);

    rPKE_IMCR &= mask;
}

/**
 * @brief           Set operand width.
 * @param[in]       bitLen               - Bit length of operand.
 * @return          Uint bytes of hardware operand.
 * @note            Please make sure 0 < bitLen <= OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_set_operand_width(unsigned int bitLen)
{
    MEM_VOLATILE unsigned int mask = ~(0x07FFFFU);
    unsigned int cfg = 0U, len;
    unsigned int step_bytes = 0U;
    const unsigned int buf[5] = {1u, 2u, 4u, 8u, 16u};
    unsigned int i;

    len = (bitLen + 255U) >> 8;

    for (i = 0u; i < 5u; i++)
    {
        if (len <= buf[i])
        {
            cfg = i + 2u;
            step_bytes = (((unsigned int)0x08u) << cfg) + 4U;
            break;
        }
        else
        {
        }
    }

    cfg = (cfg << 16) | (bitLen);

    rPKE_CFG &= mask;
    rPKE_CFG |= cfg;

    return step_bytes;
}

/**
 * @brief           Get current operand byte length.
 * @return          Current operand byte length.
 */
unsigned int pke_get_operand_bytes(void)
{
    unsigned int step_bytes;

#if 1
    unsigned int t = ((rPKE_CFG) >> 16) & 0x07U;

    if ((t > 1U) && (t < 7U))
    {
        step_bytes = ((unsigned int)0x08U) << t;
        step_bytes += 0x04U;
    }
    else
    {
        step_bytes = 0x24U; // default value
    }
#else
    switch (((rPKE_CFG) >> 16) & 0x07U)
    {
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
 * @brief           Set operation micro code.
 * @param[in]       addr                 - Specific micro code address.
 * @return          None.
 */
void pke_set_microcode(unsigned int addr)
{
    rPKE_MC_PTR = addr;
}

/**
 * @brief           Get execution configuration.
 * @return          Current execution configuration value.
 */
unsigned int pke_get_exe_cfg(void)
{
    return rPKE_EXE_CONF;
}

/**
 * @brief           Set execution configuration.
 * @param[in]       cfg                  - Specific configuration value.
 * @return          None.
 */
void pke_set_exe_cfg(unsigned int cfg)
{
    rPKE_EXE_CONF = cfg;
}

/**
 * @brief           Start PKE calculation.
 * @return          None.
 */
void pke_start(void)
{
    MEM_VOLATILE unsigned int flag = PKE_START_CALC;

    rPKE_CTRL |= flag;
}

/**
 * @brief           Get calculation return code.
 * @return          0 (success), other values indicate an error.
 */
unsigned int pke_check_rt_code(void)
{
    MEM_VOLATILE unsigned int mask = 0x07u;

    return (rPKE_RT_CODE & mask);
}

/**
 * @brief           Wait until the operation is complete.
 * @return          None.
 */
void pke_wait_till_done(void)
{
    MEM_VOLATILE unsigned int flag = 1u;

    while (0u == (rPKE_RISR & flag))
    {
    }
}

/**
 * @brief           Set operation micro code, start hardware, wait until done, and return the result code.
 * @param[in]       micro_code           - Specific micro code.
 * @return          PKE_SUCCESS (success), other values indicate an error or inverse does not exist.
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
 * @brief           Compute the modular inverse: ainv = a^(-1) mod modulus.
 * @param[in]       modulus              - Modulus.
 * @param[in]       a                    - Integer a.
 * @param[out]      ainv                 - ainv = a^(-1) mod modulus.
 * @param[in]       mod_wlen             - Word length of modulus and ainv.
 * @param[in]       a_wlen               - Word length of a.
 * @return          PKE_SUCCESS (success), other values indicate an error or inverse does not exist.
 * @note
 *        1. Please make sure a_wlen <= mod_wlen <= OPERAND_MAX_WORD_LEN and a < modulus.
 */
unsigned int pke_modinv(const unsigned int *modulus, const unsigned int *a, unsigned int *ainv, unsigned int mod_wlen, unsigned int a_wlen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    // pke_set_operand_width(mod_wlen<<5);
    step_bytes = pke_set_operand_width(get_valid_bits(modulus, mod_wlen));
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                     mod_wlen); // B3 modulus
    if (step_words > mod_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[mod_wlen]), step_words - mod_wlen);
    }
    else
    {
    }

    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), a,
                     a_wlen); // B0 a
    if (step_words > a_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[a_wlen]), step_words - a_wlen);
    }
    else
    {
    }

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODINV);
    if (PKE_SUCCESS == ret)
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), ainv,
                         mod_wlen); // A0 ainv
    }
    else if (PKE_NO_MODINV != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), a_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wlen << 2);
#endif
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Compute out = (a+b) mod modulus or out = (a-b) mod modulus
 * @param[in]       modulus              - Modulus
 * @param[in]       a                    - Integer a
 * @param[in]       b                    - Integer b
 * @param[out]      out                  - Result of (a+b) mod modulus or (a-b) mod modulus
 * @param[in]       wlen                 - Word length of modulus, a, and b
 * @param[in]       micro_code           - Must be MICROCODE_MODADD or MICROCODE_MODSUB
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. a and b must be less than modulus
 *        2. wordLen must not be bigger than OPERAND_MAX_WORD_LEN
 */
FLAG_STATIC unsigned int pke_modadd_modsub_internal(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen,
                                                    unsigned int micro_code)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(wlen << 5);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                     wlen);                                              // B3 modulus
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a, wlen); // A0
                                                                         // a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b, wlen); // B0
                                                                         // b

    if (step_words > wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[wlen]), step_words - wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(0u, step_bytes))[wlen]), step_words - wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[wlen]), step_words - wlen);
    }
    else
    {
    }

    ret = pke_set_micro_code_start_wait_return_code(micro_code);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), wlen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out,
                         wlen); // A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Compute out = (a + b), (a - b), or (a * b) mod modulus.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of (a + b), (a - b), or (a * b) mod modulus.
 * @param[in]       micro_code           - Could be MICROCODE_MODADD, MICROCODE_MODSUB, or MICROCODE_MODMUL.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Please set hardware operand width to 256u before calling this function.
 *        2. Please make sure the modulus is set in A0 before calling this function.
 *        3. If micro_code is MICROCODE_MODMUL, please make sure the pre-calculated
 *           mont parameter H(R^2 mod modulus) is set in B0, and call micro code
 *           MICROCODE_MGMR_PRE_N0 before calling this function.
 *        4. Actually, micro_code could be MICROCODE_INTADD or MICROCODE_INTSUB, then out = (a + b) or (a - b)
 *           without modulus and pre-calculated mont parameters.
 *        5. All operands are of
 *           256 bits for SM2, SM9, etc.
 */
unsigned int pke_mod_add_sub_mul_256bits_internal(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int micro_code)
{
    unsigned int ret;

    pke_load_operand((unsigned int *)(rPKE_A(0u, 36u)), a, 8u); // A0 a
    pke_load_operand((unsigned int *)(rPKE_B(0u, 36u)), b, 8u); // B0 b

    uint32_clear((unsigned int *)(&(rPKE_A(0u, 36u))[8u]), 1u);
    uint32_clear((unsigned int *)(&(rPKE_B(0u, 36u))[8u]), 1u);

    ret = pke_set_micro_code_start_wait_return_code(micro_code);
    if (PKE_SUCCESS == ret)
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, 36u)), out, 8u); // A0 result
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Compute out = (a + b) or (a - b) mod modulus.
 * @param[in]       modulus              - Modulus.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of (a + b) or (a - b) mod modulus.
 * @param[in]       micro_code           - Must be MICROCODE_MODADD or MICROCODE_MODSUB.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Please set hardware operand width to 256u before calling this function.
 *        2. Please make sure micro_code is MICROCODE_MODADD or MICROCODE_MODSUB.
 *        3. All operands are of 256 bits for SM2, SM9, etc.
 */
unsigned int pke_modadd_modsub_256bits(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int micro_code)
{
    pke_load_operand_256bits((unsigned int *)(rPKE_A(0u, 36u)),
                             modulus); // A0 modulus

    return pke_mod_add_sub_mul_256bits_internal(a, b, out, micro_code);
}

/**
 * @brief           Compute out = (a + b) mod modulus.
 * @param[in]       modulus              - Modulus.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of (a + b) mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. a and b must be less than modulus.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modadd(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wlen, MICROCODE_MODADD);
}

/**
 * @brief           Compute out = (a - b) mod modulus.
 * @param[in]       modulus              - Modulus.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of (a - b) mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. a and b must be less than modulus.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modsub(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    return pke_modadd_modsub_internal(modulus, a, b, out, wlen, MICROCODE_MODSUB);
}

/**
 * @brief           Compute out = a + b.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a + b.
 * @param[in]       wlen                 - Word length of a, b, and out.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. a + b may overflow.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_add(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
#if 0
    return pke_add_sub_internal(a, b, out, wlen, MICROCODE_INTADD);
#else
    unsigned int i, carry, temp, temp2;

    carry = 0u;
    for (i = 0u; i < wlen; i++)
    {
        temp2 = a[i];
        temp = a[i] + b[i];
        out[i] = temp + carry;
        if ((temp < temp2) || (out[i] < carry))
        {
            carry = 1u;
        }
        else
        {
            carry = 0u;
        }
    }

    return PKE_SUCCESS;
#endif
}

/**
 * @brief           Compute out = a - b.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a - b.
 * @param[in]       wlen                 - Word length of a, b, and out.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Please make sure a > b.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_sub(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
#if 0
    return pke_add_sub_internal(a, b, out, wlen, MICROCODE_INTSUB);
#else
    unsigned int i, carry, tmp, tmp2;

    carry = 0u;
    for (i = 0u; i < wlen; i++)
    {
        tmp = a[i] - b[i];
        tmp2 = tmp - carry;
        if ((tmp > a[i]) || (tmp2 > tmp))
        {
            carry = 1u;
        }
        else
        {
            carry = 0u;
        }
        out[i] = tmp2;
    }

    return PKE_SUCCESS;
#endif
}

/**
 * @brief           Compute out = a * b.
 * @param[in]       a                    - Integer a.
 * @param[in]       a_wordLen            - Word length of a.
 * @param[in]       b                    - Integer b.
 * @param[in]       b_wordLen            - Word length of b.
 * @param[out]      out                  - Result of a * b.
 * @param[in]       out_wordLen          - Word length of out.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Please make sure the out buffer word length is bigger than ((2 * max_bit_len(a, b) + 0x1F) >> 5).
 *        2. Please make sure a_wordLen and b_wordLen are not bigger than OPERAND_MAX_WORD_LEN / 2.
 */
unsigned int pke_mul_internal(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int a_wordLen, unsigned int b_wordLen, unsigned int out_wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(out_wordLen << 5); // for pke lp
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a,
                     a_wordLen); // A0 a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b,
                     b_wordLen); // B0 b

    uint32_clear((unsigned int *)(&(rPKE_A(0u, step_bytes))[a_wordLen]), step_words - a_wordLen);
    uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[b_wordLen]), step_words - b_wordLen);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_INTMUL);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), a_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), b_wordLen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), out,
                         out_wordLen); // A1 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Compute out = a * b.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a * b.
 * @param[in]       ab_wordLen           - Word length of a and b.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Please make sure the out buffer word length is bigger than ((2 * max_bit_len(a, b) + 0x1F) >> 5).
 *        2. Please make sure ab_wordLen is not bigger than OPERAND_MAX_WORD_LEN / 2.
 */
#if 1
unsigned int pke_mul(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int ab_wordLen)
{
    unsigned int bitLen, tempLen;

    bitLen = get_valid_bits(a, ab_wordLen);
    tempLen = get_valid_bits(b, ab_wordLen);

    bitLen = get_max_len(bitLen, tempLen);
    tempLen = get_word_len(bitLen << 1);
    if (tempLen < (ab_wordLen << 1))
    {
        tempLen = (ab_wordLen << 1) - 1u;
    }
    else
    {
        tempLen = (ab_wordLen << 1);
    }

    return pke_mul_internal(a, b, out, ab_wordLen, ab_wordLen, tempLen);
}
#else
unsigned int pke_mul(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int ab_wordLen)
{
    uint64_t UV;
    unsigned int i, j, *U, *V;
    unsigned int bitLen, tempLen;

    bitLen = get_valid_bits(a, ab_wordLen);
    tempLen = get_valid_bits(b, ab_wordLen);

    bitLen = get_max_len(bitLen, tempLen);
    tempLen = get_word_len(bitLen << 1);
    if (tempLen < (ab_wordLen << 1))
    {
        tempLen = (ab_wordLen << 1) - 1u;
    }
    else
    {
        tempLen = (ab_wordLen << 1);
    }

    uint32_clear(out, tempLen);

    V = (unsigned int *)(&UV);
    U = V + 1u;
    for (i = 0u; i < ab_wordLen; i++)
    {
        *U = 0u;
        for (j = 0u; j < ab_wordLen; j++)
        {
            UV = ((uint64_t)a[i]) * b[j] + out[i + j] + (*U);
            out[i + j] = (*V);
        }
        out[i + j] = (*U);
    }

    return PKE_SUCCESS;
}
#endif

#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
/**
 * @brief           Calculate H(R^2 mod modulus) and n0' (- modulus^(-1) mod 2^w) for modMul, modExp, pointMul, etc. Here w is the bit width of word, i.e., 32.
 * @param[in]       modulus              - Modulus.
 * @param[in]       bitLen               - Bit length of modulus (must be a multiple of 32).
 * @param[out]      H                    - R^2 mod modulus.
 * @param[out]      n0                   - Modulus^(-1) mod 2^w, where w is 32.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. Please make sure the word length of buffer H is equal to the word length of modulus, and n0 only needs one word.
 *        3. bitLen must not be bigger than OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_pre_calc_mont_without_mgmr_microcode(const unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
    unsigned int wlen, tmp_len, i, j;
    unsigned int *A0;
    unsigned int *B0;
    unsigned int *h;
    unsigned int *n;
    unsigned int exe_cfg_bak;
    unsigned int step_bytes;

    exe_cfg_bak = pke_get_exe_cfg();

    wlen = get_word_len(bitLen);
    step_bytes = pke_set_operand_width(wlen << 5);

    pke_set_operand_width(bitLen);

    // get and set -N^(-1) mod 2^32
    i = get_j0(modulus[0]);
    *((unsigned int *)(rPKE_B(4u, step_bytes))) = i;
    if (n0)
    {
        *n0 = i;
    }
    else
    {
    }

    A0 = (unsigned int *)(rPKE_A(0u, step_bytes));
    B0 = (unsigned int *)(rPKE_B(0u, step_bytes));
    h = (unsigned int *)(rPKE_A(3u, step_bytes));
    n = (unsigned int *)(rPKE_B(3u, step_bytes));

    // h = R mod n
    uint32_clear(h, step_bytes >> 2);
    uint32_copy(n, modulus, wlen);
    pke_sub(h, n, h, wlen);

    // h = A0 = 2R mod n
    tmp_len = big_mul_2n(h, (int32_t)wlen, (unsigned char)1);
    if ((tmp_len > wlen) || (uint32_big_num_cmp(h, wlen, n, wlen) >= 0))
    {
        pke_sub(h, n, h, wlen);
    }
    else
    {
    }
    uint32_copy(A0, h, wlen);

    if ((step_bytes >> 2) > wlen)
    {
        uint32_clear(A0 + wlen, (step_bytes >> 2) - wlen);
        uint32_clear(B0 + wlen, (step_bytes >> 2) - wlen);
        uint32_clear(n + wlen, (step_bytes >> 2) - wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_MONT);

    tmp_len = wlen << 5; // tmp_len = RbitLen-1
    i = get_valid_bits(&tmp_len, 1u) - 1u;
    j = 1u << (i - 1u);
    for (; i > 0u; i--)
    {
        // A0 = A0^2 mod n
        uint32_copy(B0, A0, wlen);
        pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

        if (tmp_len & j)
        {
            // A0 = A0*2R mod n
            uint32_copy(B0, h, wlen);
            pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
        }
        else
        {
        }

        j >>= 1;
    }

    uint32_copy(h, (unsigned int *)(rPKE_A(0u, step_bytes)), wlen);
    if (NULL != H)
    {
        uint32_copy(H, (unsigned int *)(rPKE_A(0u, step_bytes)), wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(exe_cfg_bak);

    return PKE_SUCCESS;
}
#endif

/**
 * @brief           Calculate H(R^2 mod modulus) and n0' (- modulus^(-1) mod 2^w) for modMul, modExp, pointMul, etc. Here w is the bit width of word, i.e., 32.
 * @param[in]       modulus              - Modulus.
 * @param[in]       bitLen               - Bit length of modulus.
 * @param[out]      H                    - R^2 mod modulus.
 * @param[out]      n0                   - - modulus^(-1) mod 2^w, where w is 32.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. Please make sure the word length of buffer H is equal to the word length of modulus, and n0 only needs one word.
 *        3. bitLen must not be bigger than OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_pre_calc_mont(const unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
    unsigned int step_bytes, step_words;
    unsigned int wlen = get_word_len(bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                     wlen); // B3 modulus

    if (step_words > wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[wlen]), step_words - wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(3u, step_bytes))[wlen]), step_words - wlen);
    }
    else
    {
    }

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MGMR_PRE);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), 1u << 2);
#endif
        return ret;
    }

    if (NULL != H)
    {
        pke_read_operand((unsigned int *)(rPKE_A(3u, step_bytes)), H,
                         wlen); // A3 H
    }
    else
    {
    }

    if (NULL != n0)
    {
        pke_read_operand((unsigned int *)(rPKE_B(4u, step_bytes)), n0, 1u); // B4 n0
    }
    else
    {
    }

    return PKE_SUCCESS;
}

/**
 * @brief           Like function pke_pre_calc_mont(), but this one is without output.
 * @param[in]       modulus              - Modulus.
 * @param[in]       wlen                 - Word length of modulus.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_pre_calc_mont_no_output(const unsigned int *modulus, unsigned int wlen)
{
    return pke_pre_calc_mont(modulus, get_valid_bits(modulus, wlen), NULL, NULL);
}

/**
 * @brief           Like function pke_pre_calc_mont(), but this one is for modexp.
 * @param[in]       modulus              - Modulus.
 * @param[in]       bitLen               - Bit length of modulus.
 * @param[out]      H                    - R^2 mod modulus.
 * @param[out]      n0                   - - modulus^(-1) mod 2^w, where w is 32.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. Please make sure the word length of buffer H is equal to the word length of modulus, and n0 only needs one word.
 *        3. bitLen must not be bigger than OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_pre_calc_mont_for_modexp(const unsigned int *modulus, unsigned int bitLen, unsigned int *H, unsigned int *n0)
{
#ifndef PKE_CONFIG_ALL_MODEXP_PRE_CALC_WITH_MGMR_MICROCODE
    if (bitLen & 31u)
    {
        return pke_pre_calc_mont(modulus, bitLen, H, n0);
    }
    else
    {
        return pke_pre_calc_mont_without_mgmr_microcode(modulus, bitLen, H, n0);
    }
#else
    return pke_pre_calc_mont(modulus, bitLen, H, n0);
#endif
}

/**
 * @brief           Load modulus and pre-calculated Mont parameters H(R^2 mod modulus) and n0'(- modulus^(-1) mod 2^w) for hardware operation.
 * @param[in]       modulus              - Modulus.
 * @param[in]       modulus_h            - R^2 mod modulus.
 * @param[in]       modulus_n0           - - modulus^(-1) mod 2^w, where w is 32.
 * @param[in]       bitLen               - Bit length of modulus.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. bitLen must not be bigger than OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_load_modulus_and_pre_monts(const unsigned int *modulus, const unsigned int *modulus_h, const unsigned int *modulus_n0, const unsigned int bitLen)
{
    unsigned int step_bytes, step_words;
    unsigned int wlen = get_word_len(bitLen);

    step_bytes = pke_set_operand_width(bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                     wlen); // B3 modulus
    pke_load_operand((unsigned int *)(rPKE_A(3u, step_bytes)), modulus_h,
                     wlen); // A3 h
    if (step_words > wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[wlen]), step_words - wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(3u, step_bytes))[wlen]), step_words - wlen);
    }
    else
    {
    }

    pke_load_operand((unsigned int *)(rPKE_B(4u, step_bytes)), modulus_n0, 1u);

    return PKE_SUCCESS;
}

/**
 * @brief           Load modulus and pre-calculated Mont parameters H(R^2 mod modulus) of 256 bits and n0'(- modulus^(-1) mod 2^w) for hardware operation.
 * @param[in]       modulus              - Modulus.
 * @param[in]       modulus_h            - R^2 mod modulus.
 * @param[in]       modulus_n0           - - modulus^(-1) mod 2^w, where w is 32.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. All operands are of 256 bits for SM2, SM9, etc.
 */
unsigned int pke_load_modulus_and_pre_monts_256bits(const unsigned int *modulus, const unsigned int *modulus_h, const unsigned int *modulus_n0)
{
#if 0
   rPKE_CFG &= mask;
   rPKE_CFG |= 0x00020100u;
#else
    pke_set_operand_width(256u);
#endif
    pke_load_operand_256bits((unsigned int *)(rPKE_B(3u, 36u)), modulus);   // A0 p
    pke_load_operand_256bits((unsigned int *)(rPKE_A(3u, 36u)), modulus_h); // B0
                                                                            // h
    uint32_clear((unsigned int *)(&(rPKE_B(3u, 36u))[8u]), 1u);
    uint32_clear((unsigned int *)(&(rPKE_A(3u, 36u))[8u]), 1u);

    pke_load_operand((unsigned int *)(rPKE_B(4u, 36u)), modulus_n0, 1u);

    return PKE_SUCCESS;
}

/**
 * @brief           Set modulus and pre-calculated Mont parameters H(R^2 mod modulus) and n0'(- modulus^(-1) mod 2^w) for hardware operation.
 * @param[in]       modulus              - Modulus.
 * @param[in]       modulus_h            - R^2 mod modulus.
 * @param[in]       modulus_n0           - - modulus^(-1) mod 2^w, where w is 32.
 * @param[in]       bitLen               - Bit length of modulus.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. bitLen must not be bigger than OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_set_modulus_and_pre_monts(const unsigned int *modulus, const unsigned int *modulus_h, const unsigned int *modulus_n0, unsigned int bitLen)
{
    if ((NULL == modulus_h) || (NULL == modulus_n0))
    {
        return pke_pre_calc_mont(modulus, bitLen, NULL, NULL);
    }
    else
    {
        return pke_load_modulus_and_pre_monts(modulus, modulus_h, modulus_n0, bitLen);
    }
}

/**
 * @brief           Perform out = a * b (mod modulus).
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a * b mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. a, b must be less than modulus.
 *        3. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 *        4. Before calling this function, please make sure the modulus and the pre-calculated Mont arguments of modulus are located in the right address.
 */
unsigned int pke_modmul_internal(const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), a, wlen); // A0
                                                                         // a
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), b, wlen); // B0
                                                                         // b
    if (step_words > wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(0u, step_bytes))[wlen]), step_words - wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[wlen]), step_words - wlen);
    }
    else
    {
    }

    // pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), wlen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out,
                         wlen); // A0 out

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Perform out = a * b (mod modulus).
 * @param[in]       modulus              - Modulus.
 * @param[in]       a                    - Integer a.
 * @param[in]       b                    - Integer b.
 * @param[out]      out                  - Result of a * b mod modulus.
 * @param[in]       wlen                 - Word length of modulus, a, and b.
 * @return          PKE_SUCCESS (success), other values indicate an error.
 * @note
 *        1. Modulus must be odd.
 *        2. a, b must be less than modulus.
 *        3. wlen must not be bigger than OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modmul(const unsigned int *modulus, const unsigned int *a, const unsigned int *b, unsigned int *out, unsigned int wlen)
{
    unsigned int ret;

    ret = pke_pre_calc_mont(modulus, get_valid_bits(modulus, wlen), NULL, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

        return pke_modmul_internal(a, b, out, wlen);
    }
}

/**
 * @brief           Mod exponentiation, this could be used for RSA encrypting, decrypting, signing, and verifying.
 * @param[in]       exponent             - exponent
 * @param[in]       base                 - base number
 * @param[out]      out                  - result of base^(exponent) mod modulus
 * @param[in]       mod_wordLen          - word length of modulus and base number
 * @param[in]       exp_wordLen          - word length of exponent
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please set hardware operand width before calling this function.
 *        2. Before calling this function, please make sure the modulus and the pre-calculated Mont arguments of modulus are located in the right address.
 *        3. Modulus must be odd.
 *        4. Please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modexp_internal(const unsigned int *exponent, const unsigned int *base, unsigned int *out, unsigned int mod_wordLen, unsigned int exp_wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

#if 1
    step_bytes = pke_get_operand_bytes();
#else
    step_bytes = pke_set_operand_width(mod_wordLen << 5);
#endif
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), exponent,
                     exp_wordLen); // A1 exponent
    if (step_words > exp_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(1u, step_bytes))[exp_wordLen]), step_words - exp_wordLen);
    }
    else
    {
    }

    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), base,
                     mod_wordLen); // B0 base

    if (step_words > mod_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_CFG_MODEXP);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODEXP);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), exp_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wordLen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out,
                         mod_wordLen); // A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Mod exponentiation, this could be used for RSA encrypting, decrypting, signing, and verifying.
 * @param[in]        modulus              - modulus
 * @param[in]        exponent             - exponent
 * @param[in]        base                 - base number
 * @param[out]       out                  - result of base^(exponent) mod modulus
 * @param[in]        mod_wordLen          - word length of modulus and base number
 * @param[in]        exp_wordLen          - word length of exponent
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Before calling this function, please make sure the pre-calculated Mont arguments of modulus are located in the right address.
 *        2. Modulus must be odd.
 *        3. Please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modexp(const unsigned int *modulus, const unsigned int *exponent, const unsigned int *base, unsigned int *out, unsigned int mod_wordLen, unsigned int exp_wordLen)
{
    unsigned int step_bytes, step_words;
    unsigned int ret;

    step_bytes = pke_set_operand_width(mod_wordLen << 5);
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), exponent,
                     exp_wordLen); // A1 exponent
    if (step_words > exp_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(1u, step_bytes))[exp_wordLen]), step_words - exp_wordLen);
    }
    else
    {
    }

    pke_load_operand((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                     mod_wordLen); // B3 modulus
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), base,
                     mod_wordLen); // B0 base

    if (step_words > mod_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_CFG_MODEXP);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODEXP);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), exp_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wordLen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), out,
                         mod_wordLen); // A0 result

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Check input before mod exponentiation.
 * @param[in]        modulus              - modulus
 * @param[in]        exponent             - exponent
 * @param[in]        base                 - base number
 * @param[out]       out                  - result of base^(exponent) mod modulus
 * @param[in]        mod_wordLen          - word length of modulus and base number
 * @param[in]        exp_wordLen          - word length of exponent
 * @return          PKE_SUCCESS (input is valid, allow to calculate)
 *        PKE_FINISHED (mod exponent finished)
 *        other values indicate an error
 * @note
 *        1. Modulus must be odd.
 *        2. Please make sure exp_wordLen <= mod_wordLen <= OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_modexp_check_input(const unsigned int *modulus, const unsigned int *exponent, const unsigned int *base, unsigned int *out, unsigned int mod_wordLen,
                                    unsigned int exp_wordLen)
{
    int32_t flag;

    // base should be in [0,modulus]
    flag = uint32_big_num_cmp(base, mod_wordLen, modulus, mod_wordLen);
    if (flag > 0)
    {
        return PKE_INVALID_INPUT;
    }
    else
    {
    }

    // if base is 0 or n
    if ((1u == uint32_bignum_check_zero(base, mod_wordLen)) || (0 == flag))
    {
        if (1u == uint32_bignum_check_zero(exponent, exp_wordLen)) // 0^0 mod n
        {
            return PKE_INVALID_INPUT;
        }
        else // if a is 0, e is not 0, the output is 0
        {
            uint32_clear(out, mod_wordLen);
            return PKE_FINISHED;
        }
    }
    else if (1u == uint32_bignum_check_zero(exponent, exp_wordLen)) // base is in [1,modulus-1], e is
                                                                    // 0, the output is 1
    {
        pke_set_operand_uint32_value(out, mod_wordLen, 1u);
        return PKE_FINISHED;
    }
    else
    {
    }

    return PKE_SUCCESS;
}

/**
 * @brief           Mod exponentiation (for high-level use, operands are all unsigned char big-endian big numbers). This could be used for RSA encrypting, decrypting,
 *                  signing, and verifying.
 * @param[in]        modulus              - modulus (unsigned char big-endian big number)
 * @param[in]        exponent             - exponent (unsigned char big-endian big number)
 * @param[in]        base                 - base number (unsigned char big-endian big number)
 * @param[out]       out                  - result of base^(exponent) mod modulus (unsigned char big-endian big number)
 * @param[in]        mod_bitLen           - real bit length of modulus and base number
 * @param[in]        exp_bitLen           - real bit length of exponent
 * @param[in]        calc_pre_monts       - if it is 0, no need to calculate the pre-calculated Mont arguments of modulus; otherwise, calculate.
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. This is for high-level application or protocol to use RSA mod exponent directly.
 *        2. All operands of this API are unsigned char big-endian big numbers.
 *        3. Modulus must be odd.
 *        4. Please make sure exp_bitLen <= mod_bitLen <= OPERAND_MAX_BIT_LEN.
 */
unsigned int pke_modexp_u8(const unsigned char *modulus, const unsigned char *exponent, const unsigned char *base, unsigned char *out, unsigned int mod_bitLen,
                           unsigned int exp_bitLen, unsigned int calc_pre_monts)
{
    unsigned int step_bytes, step_words;
    unsigned int mod_byteLen = get_byte_len(mod_bitLen);
    unsigned int mod_wordLen = get_word_len(mod_bitLen);
    unsigned int exp_byteLen = get_byte_len(exp_bitLen);
    unsigned int exp_wordLen = get_word_len(exp_bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(mod_bitLen);
    step_words = step_bytes >> 2;

    pke_load_operand_U8((unsigned int *)(rPKE_B(3u, step_bytes)), modulus,
                        mod_byteLen); // B3 modulus
    if (step_words > mod_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(3u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[mod_wordLen]), step_words - mod_wordLen);
    }
    else
    {
    }

    if (0U != calc_pre_monts)
    {
        (void)pke_pre_calc_mont((unsigned int *)(rPKE_B(3u, step_bytes)), mod_bitLen, NULL, NULL);
    }
    else
    {
    }

    pke_load_operand_U8((unsigned int *)(rPKE_A(1u, step_bytes)), exponent,
                        exp_byteLen); // A1 exponent
    if (step_words > exp_wordLen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(1u, step_bytes))[exp_wordLen]), step_words - exp_wordLen);
    }
    else
    {
    }

    pke_load_operand_U8((unsigned int *)(rPKE_B(0u, step_bytes)), base,
                        mod_byteLen); // B0 base

    ret = pke_modexp_check_input((const unsigned int *)(rPKE_B(3u, step_bytes)), (const unsigned int *)(rPKE_A(1u, step_bytes)), (const unsigned int *)(rPKE_B(0u, step_bytes)),
                                 (unsigned int *)(rPKE_A(0u, step_bytes)), mod_wordLen, exp_wordLen);
    if (PKE_FINISHED == ret)
    {
        goto END;
    }
    else if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        // handle other
    }

    pke_set_exe_cfg(PKE_EXE_CFG_MODEXP);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODEXP);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), mod_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), exp_wordLen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), mod_wordLen << 2);
#endif
        return ret;
    }
    else
    {
    }

END:

    pke_read_operand_U8((unsigned int *)(rPKE_A(0u, step_bytes)), out,
                        mod_byteLen); // A0 result

    return PKE_SUCCESS;
}

/**
 * @brief           Compute c = a mod b.
 * @param[in]        a                    - integer a (unsigned char big-endian big number)
 * @param[in]        a_wlen               - word length of integer a
 * @param[in]        b                    - integer b, modulus (unsigned char big-endian big number)
 * @param[in]        b_h                  - H parameter of b (unsigned char big-endian big number)
 * @param[in]        b_n0                 - - modulus ^(-1) mod 2^w, here w is 32 actually
 * @param[in]        b_wlen               - word length of integer b and b_h
 * @param[out]       c                    - result of a mod b (unsigned char big-endian big number)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. b must be odd, and please make sure b_wlen is the real word length of b.
 *        2. Real bit length of a cannot be bigger than 2*(real bit length of b), so a_wlen cannot be bigger than 2*b_wlen.
 *        3. Please make sure a_wlen <= 2*OPERAND_MAX_WORD_LEN and b_wlen <= OPERAND_MAX_WORD_LEN.
 */
unsigned int pke_mod(const unsigned int *a, unsigned int a_wlen, const unsigned int *b, const unsigned int *b_h, const unsigned int *b_n0, unsigned int b_wlen, unsigned int *c)
{
    unsigned int step_bytes;
    int32_t flag;
    unsigned int bBitLen, bitLen, tmp_len;
    unsigned int *t1, *t2;
    const unsigned int *t1_tmp;
    unsigned int ret;
    unsigned int wlen = a_wlen;

    flag = uint32_big_num_cmp(a, wlen, b, b_wlen);
    if (flag < 0)
    {
        wlen = get_valid_words(a, wlen);
        uint32_copy(c, a, wlen);
        uint32_clear(&c[wlen], b_wlen - wlen);

        return PKE_SUCCESS;
    }
    else if (0 == flag)
    {
        uint32_clear(c, b_wlen);

        return PKE_SUCCESS;
    }
    else
    {
        // handle other
    }

    bBitLen = get_valid_bits(b, b_wlen);
    step_bytes = pke_set_operand_width(bBitLen);

    t1 = (unsigned int *)(rPKE_A(1u, step_bytes));
    t2 = (unsigned int *)(rPKE_B(2u, step_bytes));
    t1_tmp = (const unsigned int *)t1;

    bitLen = bBitLen & 0x1Fu;

    // get t2 = a high part mod b
    if (0u != bitLen)
    {
        tmp_len = wlen - b_wlen + 1u;
        uint32_copy(t2, &a[b_wlen - 1u], tmp_len);
        (void)big_div_2n(t2, tmp_len, bitLen);
        if (tmp_len < b_wlen)
        {
            uint32_clear(&t2[tmp_len], b_wlen - tmp_len);
        }
        else if (uint32_big_num_cmp(t2, b_wlen, b, b_wlen) >= 0)
        {
            ret = pke_sub(t2, b, t2, b_wlen);
            if (PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
        tmp_len = wlen - b_wlen;
        if (uint32_big_num_cmp(&a[b_wlen], tmp_len, b, b_wlen) >= 0)
        {
            ret = pke_sub(&a[b_wlen], b, t2, b_wlen);
            if (PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        else
        {
            uint32_copy(t2, &a[b_wlen], tmp_len);
            uint32_clear(&t2[tmp_len], b_wlen - tmp_len);
        }
    }

    // set the pre-calculated mont parameters
    ret = pke_set_modulus_and_pre_monts(b, b_h, b_n0, bBitLen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get t1 = 1000...000 mod b
    uint32_clear(t1, b_wlen);
    if (0u != bitLen)
    {
        t1[b_wlen - 1u] = ((unsigned int)1u) << (bitLen);
    }
    else
    {
    }

    ret = pke_sub(t1, b, t1, b_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get t2 = a_high * 1000..000 mod b
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    ret = pke_modmul_internal(t1, t2, t2, b_wlen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get t1 = a low part mod b
    if (0u != bitLen)
    {
        uint32_copy(t1, a, b_wlen);
        t1[b_wlen - 1u] &= (((unsigned int)1u) << bitLen) - 1u;
        if (uint32_big_num_cmp(t1, b_wlen, b, b_wlen) >= 0)
        {
            ret = pke_sub(t1, b, t1, b_wlen);
            if (PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
        if (uint32_big_num_cmp(a, b_wlen, b, b_wlen) >= 0)
        {
            ret = pke_sub(a, b, t1, b_wlen);
            if (PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
            }
        }
        else
        {
            t1_tmp = (const unsigned int *)a;
        }
    }

    return pke_modadd(b, t1_tmp, t2, c, b_wlen);
}

/********************************** ECCp functions
 * *************************************/

/**
 * @brief           ECCP curve point multiplication (random point), Q = [k]P.
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[in]        k                    - scalar
 * @param[in]        px                   - x coordinate of point P (unsigned char big-endian big number)
 * @param[in]        Py                   - y coordinate of point P (unsigned char big-endian big number)
 * @param[out]       qx                   - x coordinate of point Q (unsigned char big-endian big number)
 * @param[out]       qy                   - y coordinate of point Q (unsigned char big-endian big number)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure k is in [1, n-1], where n is the order of the ECCP curve.
 *        2. Please make sure the input point P is on the curve.
 *        3. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 *        4. Even if the input point P is valid, the output may be the infinite point, in which case it will return an error.
 */
unsigned int eccp_pointmul(const eccp_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->eccp_p_bitLen);
    unsigned int n_wlen = get_word_len(curve->eccp_n_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, get_max_len(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), px,
                     p_wlen); // B0 px
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), Py,
                     p_wlen); // B1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a,
                     p_wlen); // A5 a
    pke_load_operand((unsigned int *)(rPKE_A(4u, step_bytes)), k,
                     n_wlen); // A4 k

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(1u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(5u, step_bytes))[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    if (step_words > n_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(4u, step_bytes))[n_wlen]), step_words - n_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_MUL);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PMUL);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), n_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(5u, step_bytes)), n_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), p_wlen << 2);
#endif
        return ret;
    }
    else
    {
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), qx,
                     p_wlen); // A0 qx
    if (NULL != qy)
    {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), qy,
                         p_wlen); // A1 qy
    }
    else
    {
    }

    return PKE_SUCCESS;
}

/**
 * @brief           ECCP curve point addition, Q = P1 + P2.
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[in]        x1                   - x coordinate of point P1 (unsigned char big-endian big number)
 * @param[in]        y1                   - y coordinate of point P1 (unsigned char big-endian big number)
 * @param[in]        x2                   - x coordinate of point P2 (unsigned char big-endian big number)
 * @param[in]        y2                   - y coordinate of point P2 (unsigned char big-endian big number)
 * @param[out]       qx                   - x coordinate of point Q = P1 + P2 (unsigned char big-endian big number)
 * @param[out]       qy                   - y coordinate of point Q = P1 + P2 (unsigned char big-endian big number)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure input points P1 and P2 are both on the curve.
 *        2. Please make sure the bit length of the curve is not greater than ECCP_MAX_BIT_LEN.
 *        3. Even if the input points P1 and P2 are valid, it will return an error in the following cases:
 *        3.1. P1 = P2. Return PKE_NO_MODINV.
 *        3.2. P1 = -P2. Return PKE_NO_MODINV. Actually, the output point is the neutral point (point at infinity).
 */
unsigned int eccp_pointadd(const eccp_curve_t *curve, const unsigned int *x1, const unsigned int *y1, const unsigned int *x2, const unsigned int *y2, unsigned int *qx,
                           unsigned int *qy)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->eccp_p_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, get_max_len(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    // pke_pre_calc_mont() may cover A1, so load A1(x1) here
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), x1,
                     p_wlen); // A0 x1
    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), y1,
                     p_wlen); // A1 y1
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), x2,
                     p_wlen); // B0 x2
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), y2,
                     p_wlen); // B1 y2
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a,
                     p_wlen); // A5 a

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(0u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(1u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(1u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(5u, step_bytes))[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_ADD);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PADD);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), p_wlen << 2);
#endif
        return ret;
    }
    else
    {
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), qx,
                     p_wlen); // A0 qx
    if (NULL != qy)
    {
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), qy,
                         p_wlen); // A1 qy
    }
    else
    {
    }

    return PKE_SUCCESS;
}

/**
 * @brief           ECCP curve point addition, Q = P1 + P2.
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[in]        x1                   - x coordinate of point P1 (unsigned char big-endian big number)
 * @param[in]        y1                   - y coordinate of point P1 (unsigned char big-endian big number)
 * @param[in]        x2                   - x coordinate of point P2 (unsigned char big-endian big number)
 * @param[in]        y2                   - y coordinate of point P2 (unsigned char big-endian big number)
 * @param[out]       qx                   - x coordinate of point Q = P1 + P2 (unsigned char big-endian big number)
 * @param[out]       qy                   - y coordinate of point Q = P1 + P2 (unsigned char big-endian big number)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure input points P1 and P2 are both on the curve.
 *        2. Please make sure the bit length of the curve is not greater than ECCP_MAX_BIT_LEN.
 *        3. If P1 = -P2, it will return PKE_NO_MODINV. Actually, the output point is the neutral point (point at infinity).
 */
unsigned int eccp_pointadd_safe(const eccp_curve_t *curve, const unsigned int *x1, const unsigned int *y1, const unsigned int *x2, const unsigned int *y2, unsigned int *qx,
                                unsigned int *qy)
{
    unsigned int step_bytes;
    unsigned int p_wlen = get_word_len(curve->eccp_p_bitLen);
    unsigned int n_wlen = get_word_len(curve->eccp_n_bitLen);
    unsigned int ret;

    step_bytes = pke_set_operand_width(get_max_len(curve->eccp_p_bitLen, curve->eccp_n_bitLen));

#ifdef PKE_SEC
    if (0 == uint32_BigNumCmp_sec(x1, p_wlen, x2, p_wlen))
    {
        if (0 == uint32_BigNumCmp_sec(y1, p_wlen, y2, p_wlen))
#else
    if (0 == uint32_big_num_cmp(x1, p_wlen, x2, p_wlen))
    {
        if (0 == uint32_big_num_cmp(y1, p_wlen, y2, p_wlen))
#endif
        {
#ifdef ECCP_POINT_DOUBLE
            ret = eccp_pointDouble(curve, x1, y1, qx, qy);
#else
            uint32_clear((unsigned int *)(rPKE_A(4u, step_bytes)), n_wlen);
            ((unsigned int *)(rPKE_A(4u, step_bytes)))[0] = 2u;
            ret = eccp_pointmul(curve, (unsigned int *)(rPKE_A(4u, step_bytes)), x1, y1, qx, qy);
#endif
        }
        else
        {
            ret = PKE_NO_MODINV;
        }
    }
    else
    {
        ret = eccp_pointadd(curve, x1, y1, x2, y2, qx, qy);
    }

    return ret;
}

#ifdef ECCP_POINT_DOUBLE
/**
 * @brief           ECCP curve point double, Q=[2]P
 * @param[in]       curve                - eccp_curve_t curve struct pointer
 * @param[in]       px                   - x coordinate of point P
 * @param[in]       Py                   - y coordinate of point P
 * @param[out]      qx                   - x coordinate of point Q=[2]P
 * @param[out]      qy                   - y coordinate of point Q=[2]P
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. please make sure the input point P is on the curve
 *        2. please make sure bit length of the curve is not bigger than ECCP_MAX_BIT_LEN
 */
unsigned int eccp_pointDouble(eccp_curve_t *curve, unsigned int *px, unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->eccp_p_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, get_max_len(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    // pke_pre_calc_mont() may cover A1, so load A1(px) and other paras here
    pke_load_operand((unsigned int *)(rPKE_A(0u, step_bytes)), px,
                     p_wlen); // A0 px
    pke_load_operand((unsigned int *)(rPKE_A(1u, step_bytes)), Py,
                     p_wlen); // A1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a,
                     p_wlen); // A5 a

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_A(0u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(1u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(5u, step_bytes))[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_DBL);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PDBL);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), p_wlen << 2);
#endif
        return ret;
    }
    else
    {
        pke_read_operand((unsigned int *)(rPKE_A(0u, step_bytes)), qx,
                         p_wlen); // A0 qx
        pke_read_operand((unsigned int *)(rPKE_A(1u, step_bytes)), qy,
                         p_wlen); // A1 qy

        return PKE_SUCCESS;
    }
}
#endif

/**
 * @brief           Check whether the input point P is on the ECCP curve or not.
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[in]        px                   - x coordinate of point P (unsigned char big-endian big number)
 * @param[in]        Py                   - y coordinate of point P (unsigned char big-endian big number)
 * @return          PKE_SUCCESS (success, point is on the curve), other values indicate an error or not on the curve
 * @note
 *        1. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 *        2. After calculation, A1 and A2 will be changed!
 */
unsigned int eccp_pointverify(const eccp_curve_t *curve, const unsigned int *px, const unsigned int *Py)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->eccp_p_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->eccp_p, curve->eccp_p_h, curve->eccp_p_n0, get_max_len(curve->eccp_p_bitLen, curve->eccp_n_bitLen));
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    // pke_pre_calc_mont() may cover A1, so load A1(px) and other paras here
    pke_load_operand((unsigned int *)(rPKE_B(0u, step_bytes)), px,
                     p_wlen); // B0 px
    pke_load_operand((unsigned int *)(rPKE_B(1u, step_bytes)), Py,
                     p_wlen); // B1 Py
    pke_load_operand((unsigned int *)(rPKE_A(5u, step_bytes)), curve->eccp_a,
                     p_wlen); // A5 a
    pke_load_operand((unsigned int *)(rPKE_A(4u, step_bytes)), curve->eccp_b,
                     p_wlen); // A4 b

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&(rPKE_B(0u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_B(1u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(5u, step_bytes))[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&(rPKE_A(4u, step_bytes))[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_ECCP_POINT_VER);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_PVER);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(5u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(4u, step_bytes)), p_wlen << 2);
#endif
        return ret;
    }
    else
    {
        return PKE_SUCCESS;
    }
}

/**
 * @brief           Get ECCP public key from private key (the key pair could be used in SM2/ECDSA/ECDH, etc.).
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[in]        priKey               - private key (unsigned char big-endian)
 * @param[out]       pubKey               - public key (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 */
unsigned int eccp_get_pubkey_from_prikey(const eccp_curve_t *curve, const unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int step_bytes;
    unsigned int n_len = get_byte_len(curve->eccp_n_bitLen);
    unsigned int n_wlen = get_word_len(curve->eccp_n_bitLen);
    unsigned int p_len = get_byte_len(curve->eccp_p_bitLen);
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int *x;
    unsigned int *y;
    unsigned int ret;

    step_bytes = pke_set_operand_width(curve->eccp_p_bitLen);
    x = (unsigned int *)(rPKE_A(0u, step_bytes));
    y = (unsigned int *)(rPKE_A(1u, step_bytes));

    k[n_wlen - 1u] = 0u; // clear if curve->eccp_n_bitLen is not a multiple of
                         // 32
    reverse_byte_array(priKey, (unsigned char *)k, n_len);

    // make sure k in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, n_wlen, PKE_ZERO_ALL, PKE_INTEGER_TOO_BIG, PKE_SUCCESS);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#ifdef SUPPORT_SM2
    if (curve == sm2_curve)
    {
        if ((k[0] == (sm2_curve->eccp_n[0] - 1u)) && (0 == uint32_big_num_cmp(k + 1u, n_wlen - 1u, (curve->eccp_n) + 1u, n_wlen - 1u)))
        {
            return PKE_INTEGER_TOO_BIG;
        }
        else
        {
        }
    }
    else
    {
    }
#endif

    // get pubKey
    ret = eccp_pointmul(curve, k, curve->eccp_Gx, curve->eccp_Gy, x, y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        reverse_byte_array((unsigned char *)x, pubKey, p_len);
        reverse_byte_array((unsigned char *)y, pubKey + p_len, p_len);

        return PKE_SUCCESS;
    }
}

/**
 * @brief           Get ECCP key pair (the key pair could be used in SM2/ECDSA/ECDH).
 * @param[in]        curve                - pointer to eccp_curve_t curve struct
 * @param[out]       priKey               - private key (unsigned char big-endian)
 * @param[out]       pubKey               - public key (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the bit length of the curve is not bigger than ECCP_MAX_BIT_LEN.
 */
unsigned int eccp_getkey(const eccp_curve_t *curve, unsigned char *priKey, unsigned char *pubKey)
{
    unsigned int tmp_len;
    unsigned int n_len = get_byte_len(curve->eccp_n_bitLen);
    unsigned int ret;

ECCP_GETKEY_LOOP:

    ret = get_rand(priKey, n_len);
    if (TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // make sure k has the same bit length as n
    tmp_len = (curve->eccp_n_bitLen) & 7u;
    if (0u != tmp_len)
    {
        priKey[0] &= (1u << (tmp_len)) - 1u;
    }
    else
    {
    }

    ret = eccp_get_pubkey_from_prikey(curve, priKey, pubKey);
    if ((PKE_ZERO_ALL == ret) || (PKE_INTEGER_TOO_BIG == ret))
    {
        goto ECCP_GETKEY_LOOP;
    }
    else
    {
        return ret;
    }
}

/****************************** ECCp functions finished *********************************/

#ifdef SUPPORT_C25519
/**************************** X25519 & Ed25519 functions ********************************/

/**
 * @brief           C25519 point multiplication (random point), Q = [k]P.
 * @param[in]       curve                - pointer to c25519 curve struct
 * @param[in]        k                    - scalar
 * @param[in]        p                    - u coordinate of point P (unsigned char big-endian)
 * @param[out]       q                    - u coordinate of point Q (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the input point P is on the curve.
 *        2. Even if the input point P is valid, the output may be the infinite point, in which case it will return an error.
 *        3. Please make sure the curve is C25519.
 */
unsigned int x25519_pointmul(const mont_curve_t *curve, const unsigned int *k, const unsigned int *p, unsigned int *q)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->p_bitLen);
    unsigned int n_wlen = get_word_len(curve->n_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    pke_load_operand((unsigned int *)rPKE_A(0u, step_bytes), p,
                     p_wlen); // A0 p
    pke_load_operand((unsigned int *)rPKE_B(0u, step_bytes), curve->a24,
                     p_wlen);                                            // B0 a24
    pke_load_operand((unsigned int *)rPKE_A(4u, step_bytes), k, n_wlen); // A4 k

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&rPKE_A(0u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_B(0u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_B(3u, step_bytes)[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    if (step_words > n_wlen)
    {
        uint32_clear((unsigned int *)(&rPKE_A(4u, step_bytes)[n_wlen]), step_words - n_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_C25519_PMUL);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(4u, step_bytes)), n_wlen << 2);
#endif
        return ret;
    }
    else
    {
    }

    pke_read_operand((unsigned int *)rPKE_A(1u, step_bytes), q,
                     p_wlen); // A1 q

    return PKE_SUCCESS;
}

/**
 * @brief           Decode X25519 or Ed25519 scalar for point multiplication.
 * @param[in]        k                    - scalar (unsigned char big-endian)
 * @param[out]       out                  - big scalar in little-endian (unsigned char little-endian)
 * @return          None
 * @note
 *        1. This function is for X25519 or Ed25519.
 */
void x25519_ed25519_decode_scalar(const unsigned char *k, unsigned char *out)
{
    if (k != out)
    {
        memcpy_(out, k, C25519_BYTE_LEN);
    }
    else
    {
    }

// actually this is internal interface,the caller ensures out is not NULL
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != out)
    {
#endif
        out[0] &= (unsigned char)0xF8;                    // clear lowest 3 bits
        out[C25519_BYTE_LEN - 1u] &= (unsigned char)0x7F; // clear highest 1 bit
        out[C25519_BYTE_LEN - 1u] |= (unsigned char)0x40; // set second highest bit as 1
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           Decode an Ed25519 encoded point.
 * @param[in]        in_y                 - encoded Ed25519 point (unsigned char big-endian)
 * @param[out]       out_x                - x coordinate of the input point (unsigned char big-endian)
 * @param[out]       out_y                - y coordinate of the input point (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 */
unsigned int ed25519_decode_point(const unsigned char in_y[32], unsigned char out_x[32], unsigned char out_y[32])
{
    unsigned int u[Ed25519_WORD_LEN];
    unsigned int v[Ed25519_WORD_LEN];
    unsigned int t[Ed25519_WORD_LEN];
    unsigned int t2[Ed25519_WORD_LEN];
    unsigned int t3[Ed25519_WORD_LEN];
    unsigned int ret;

    // get y
    memcpy_((unsigned char *)u, in_y, Ed25519_BYTE_LEN);
    u[Ed25519_WORD_LEN - 1U] &= 0x7FFFFFFFu;

    // make sure y < prime p
    if (uint32_big_num_cmp(u, Ed25519_WORD_LEN, ed25519->p, Ed25519_WORD_LEN) >= 0)
    {
        return PKE_INVALID_INPUT;
    }
    else
    {
    }

    // set type
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    // set pre-calculated paras
    ret = pke_set_modulus_and_pre_monts(ed25519->p, ed25519->p_h, ed25519->p_n0, ed25519->p_bitLen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(u, u, v, Ed25519_WORD_LEN); // v = y^2
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    uint32_clear(t, Ed25519_WORD_LEN);
    t[0] = 1u;
    ret = pke_modsub(ed25519->p, v, t, u, Ed25519_WORD_LEN); // u = y^2 - 1
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(ed25519->d, v, v, Ed25519_WORD_LEN); // v = d*y^2
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modadd(ed25519->p, v, t, v, Ed25519_WORD_LEN); // v = d*y^2 + 1
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(v, v, t2, Ed25519_WORD_LEN); // t2 = v^2
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(v, t2, t3, Ed25519_WORD_LEN); // t3 = v^3
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t3, u, t, Ed25519_WORD_LEN); // t = u*v^3
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t2, t2, t2, Ed25519_WORD_LEN); // t2 = v^4
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t2, t3, t2, Ed25519_WORD_LEN); // t2 = v^7
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t2, u, t2, Ed25519_WORD_LEN); // t2 = u*v^7
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // t3 = (p-5)/8
    uint32_copy(t3, ed25519->p, Ed25519_WORD_LEN);
    t3[0] -= 5u;
    (void)big_div_2n(t3, Ed25519_WORD_LEN, 3u);

// t2 = (u*v^7 )^((p-5)/8)
#if 0
    ret = mod_exp(t2, t3, ed25519->p, t2);
#else
    ret = pke_modexp(ed25519->p, t3, t2, t2, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t2, t, t, Ed25519_WORD_LEN); // t = x = (u*v^3)*(u*v^7 )^((p-5)/8)
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t, t, t2, Ed25519_WORD_LEN); // t2 = x^2
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = pke_modmul_internal(t2, v, t2, Ed25519_WORD_LEN); // t2 = v*x^2
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    if (0 == uint32_big_num_cmp(t2, Ed25519_WORD_LEN, u,
                                Ed25519_WORD_LEN)) // if v x^2 = u (mod p), x is a square root.
    {
        goto result;
    }
    else
    {
    }

    ret = pke_sub(ed25519->p, u, t3, Ed25519_WORD_LEN); // t3 = -u mod p
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else if (0 == uint32_big_num_cmp(t2, Ed25519_WORD_LEN, t3, Ed25519_WORD_LEN))
    {
        // v = (p-1)/4
        uint32_copy(v, ed25519->p, Ed25519_WORD_LEN);
        v[0] -= 1u;
        (void)big_div_2n(v, Ed25519_WORD_LEN, 2u);

        // t2 = 2
        pke_set_operand_uint32_value(t2, Ed25519_WORD_LEN, 2);

// u = 2^((p-1)/4)
#if 0
        ret = mod_exp(t2, v, ed25519->p, u);
#else
        ret = pke_modexp(ed25519->p, v, t2, u, Ed25519_WORD_LEN, Ed25519_WORD_LEN);
        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = pke_modmul_internal(t, u, t, Ed25519_WORD_LEN); // t = x*(2^((p-1)/4))
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        goto result;
    }
    else
    {
        // handle other
    }

    return PKE_INVALID_INPUT; // root not exist

result:

    // if x=0 and x is odd, decode fail
    if ((1u == uint32_bignum_check_zero(t, Ed25519_WORD_LEN)) && (0u != (((unsigned int)(in_y[Ed25519_BYTE_LEN - 1u])) & 0x80u)))
    {
        return PKE_INVALID_INPUT;
    }
    else
    {
    }

    // get out_x
    if ((t[0] & 1u) == (((unsigned int)(in_y[Ed25519_BYTE_LEN - 1u])) >> 7))
    {
        memcpy_(out_x, (unsigned char *)t, Ed25519_BYTE_LEN);
    }
    else
    {
        ret = pke_sub(ed25519->p, t, v, Ed25519_WORD_LEN); // v = -x mod p
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            memcpy_(out_x, (unsigned char *)v, Ed25519_BYTE_LEN);
        }
    }

    // get out_y
    memcpy_(out_y, in_y, Ed25519_BYTE_LEN);
    out_y[Ed25519_BYTE_LEN - 1u] &= ((unsigned char)0x7F);

    return PKE_SUCCESS;
}

/**
 * @brief           Edwards25519 curve point multiplication (random point), Q = [k]P.
 * @param[in]        curve                - pointer to edwards25519 curve struct
 * @param[in]        k                    - scalar
 * @param[in]        px                   - x coordinate of point P (unsigned char big-endian)
 * @param[in]        Py                   - y coordinate of point P (unsigned char big-endian)
 * @param[out]       qx                   - x coordinate of point Q (unsigned char big-endian)
 * @param[out]       qy                   - y coordinate of point Q (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the input point P is on the curve.
 *        2. Even if the input point P is valid, the output may be the neutral point (0, 1), which is valid.
 *        3. Please make sure the curve is Edwards25519.
 *        4. k could not be zero.
 *        5. Please set hardware operand width to 256u before calling this function.
 *        6. Before calling this function, please ensure that the modulus and the
 *           pre-calculated montgomery arguments of the modulus are located in the correct addresses.
 */
unsigned int ed25519_pointMul_internal(const edward_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    unsigned int ret;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL == curve)
    {
        ret = PKE_POINTER_NULL;
    }
    else
    {
#endif
        pke_load_operand((unsigned int *)rPKE_A(1u, 36u), px, 8u);       // A1 px
        pke_load_operand((unsigned int *)rPKE_A(2u, 36u), Py, 8u);       // A2 Py
        pke_load_operand((unsigned int *)rPKE_B(0u, 36u), curve->d, 8u); // B0 d
        pke_load_operand((unsigned int *)rPKE_A(0u, 36u), k, 8u);        // A0 k
        uint32_clear((unsigned int *)(&rPKE_A(1u, 36u)[8u]), 1u);
        uint32_clear((unsigned int *)(&rPKE_A(2u, 36u)[8u]), 1u);
        uint32_clear((unsigned int *)(&rPKE_B(3u, 36u)[8u]), 1u);
        uint32_clear((unsigned int *)(&rPKE_A(3u, 36u)[8u]), 1u);
        uint32_clear((unsigned int *)(&rPKE_A(0u, 36u)[8u]), 1u);

        pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

        ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PMUL);
        if (PKE_SUCCESS != ret)
        {
#ifdef PKE_SEC
            get_rand_fast((unsigned char *)(rPKE_A(0u, step_bytes)), n_wlen << 2);
            get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), p_wlen << 2);
            get_rand_fast((unsigned char *)(rPKE_A(2u, step_bytes)), p_wlen << 2);
            get_rand_fast((unsigned char *)(rPKE_A(3u, step_bytes)), p_wlen << 2);
            get_rand_fast((unsigned char *)(rPKE_B(3u, step_bytes)), p_wlen << 2);
#endif
            return ret;
        }
        else
        {
        }

        pke_read_operand((unsigned int *)rPKE_A(1u, 36u), qx, 8u); // A1 qx
        if (NULL != qy)
        {
            pke_read_operand((unsigned int *)rPKE_A(2u, 36u), qy, 8u); // A2 qx
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif

    return ret;
}

/**
 * @brief           Edwards25519 curve point multiplication (random point), Q = [k]P.
 * @param[in]        curve                - pointer to edwards25519 curve struct
 * @param[in]        k                    - scalar
 * @param[in]        px                   - x coordinate of point P (unsigned char big-endian)
 * @param[in]        Py                   - y coordinate of point P (unsigned char big-endian)
 * @param[out]       qx                   - x coordinate of point Q (unsigned char big-endian)
 * @param[out]       qy                   - y coordinate of point Q (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the input point P is on the curve.
 *        2. Even if the input point P is valid, the output may be the neutral point(0, 1), which is valid.
 *        3. Please make sure the curve is Edwards25519.
 *        4. k could not be zero.
 */
unsigned int ed25519_pointMul(const edward_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    unsigned int ret;

#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL == curve)
    {
        ret = PKE_POINTER_NULL;
    }
    else
    {
#endif
        ret = pke_load_modulus_and_pre_monts_256bits(curve->p, curve->p_h, curve->p_n0);
        if (PKE_SUCCESS == ret)
        {
            ret = ed25519_pointMul_internal(curve, k, px, Py, qx, qy);
        }
        else
        {
        }
#ifdef SUPPORT_STATIC_ANALYSIS
    }
#endif

    return ret;
}

/**
 * @brief           Edwards25519 point addition, Q = P1 + P2.
 * @param[in]        curve                - pointer to edwards25519 curve struct
 * @param[in]        x1                   - x coordinate of point P1 (unsigned char big-endian)
 * @param[in]        y1                   - y coordinate of point P1 (unsigned char big-endian)
 * @param[in]        x2                   - x coordinate of point P2 (unsigned char big-endian)
 * @param[in]        y2                   - y coordinate of point P2 (unsigned char big-endian)
 * @param[out]       qx                   - x coordinate of point Q = P1 + P2 (unsigned char big-endian)
 * @param[out]       qy                   - y coordinate of point Q = P1 + P2 (unsigned char big-endian)
 * @return          PKE_SUCCESS (success), other values indicate an error
 * @note
 *        1. Please make sure the input points P1 and P2 are both on the curve.
 *        2. The output point may be the neutral point (0, 1), which is valid.
 *        3. Please make sure the curve is Edwards25519.
 */
unsigned int ed25519_pointAdd(const edward_curve_t *curve, const unsigned int *x1, const unsigned int *y1, const unsigned int *x2, const unsigned int *y2, unsigned int *qx,
                              unsigned int *qy)
{
    unsigned int step_bytes, step_words;
    unsigned int p_wlen = get_word_len(curve->p_bitLen);
    unsigned int ret;

    // set ecc_p, ecc_p_h, ecc_p_n0, etc.
    ret = pke_set_modulus_and_pre_monts(curve->p, curve->p_h, curve->p_n0, curve->p_bitLen);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    step_bytes = pke_get_operand_bytes();
    step_words = step_bytes >> 2;

    // pke_pre_calc_mont() may cover some addresses, so load parameters here
    pke_load_operand((unsigned int *)rPKE_A(1u, step_bytes), x1,
                     p_wlen); // A1 x1
    pke_load_operand((unsigned int *)rPKE_A(2u, step_bytes), y1,
                     p_wlen); // A2 y1
    pke_load_operand((unsigned int *)rPKE_B(1u, step_bytes), x2,
                     p_wlen); // B1 x2
    pke_load_operand((unsigned int *)rPKE_B(2u, step_bytes), y2,
                     p_wlen); // B2 y2
    pke_load_operand((unsigned int *)rPKE_B(0u, step_bytes), curve->d,
                     p_wlen); // B0 d

    if (step_words > p_wlen)
    {
        uint32_clear((unsigned int *)(&rPKE_A(1u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_A(2u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_B(1u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_B(2u, step_bytes)[p_wlen]), step_words - p_wlen);
        uint32_clear((unsigned int *)(&rPKE_B(0u, step_bytes)[p_wlen]), step_words - p_wlen);
    }
    else
    {
    }

    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_Ed25519_PADD);
    if (PKE_SUCCESS != ret)
    {
#ifdef PKE_SEC
        get_rand_fast((unsigned char *)(rPKE_A(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_A(2u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(0u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(1u, step_bytes)), p_wlen << 2);
        get_rand_fast((unsigned char *)(rPKE_B(2u, step_bytes)), p_wlen << 2);
#endif
        return ret;
    }
    else
    {
    }

    pke_read_operand((unsigned int *)rPKE_A(1u, step_bytes), qx,
                     p_wlen); // A1 qx
    pke_read_operand((unsigned int *)rPKE_A(2u, step_bytes), qy,
                     p_wlen); // A2 qy

    return PKE_SUCCESS;
}

/**************************** X25519 & Ed25519 finished *********************************/
#endif
