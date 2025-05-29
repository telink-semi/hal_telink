/*! @file sm9.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_SM9

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/hash_hmac/hash_kdf.h"
#include "../../crypto_include/pke/sm9.h"
#include "../../crypto_include/trng/trng.h"

/*
 * caution:
 * for PKE_LP, PKE_SECURE
 * 1. B0 is corrupted after modmul
 * 2. exe cfg reg is modified by pointMul, pointAdd, etc.
 */

typedef struct
{
    unsigned int re[8];
    unsigned int im[8];
} __mpc_struct;

typedef __mpc_struct mpc_t[1];
typedef __mpc_struct *mpc_ptr;

typedef struct
{
    mpc_t ft2;
    mpc_t ft;
    mpc_t f;
} __mpc6_struct;

typedef __mpc6_struct mpc6_t[1];
typedef __mpc6_struct *mpc6_ptr;

typedef struct
{
    mpc6_t fW;
    mpc6_t f;
} __mpc12_struct;

typedef __mpc12_struct mpc12_t[1];
typedef __mpc12_struct *mpc12_ptr;

typedef struct
{
    unsigned int x[8];
    unsigned int y[8];
    unsigned int z[8];
} FP_POINT;

typedef FP_POINT fp_pt_t[1];

typedef struct
{
    mpc_t x;
    mpc_t y;
    mpc_t z;
} FP2_POINT;

typedef FP2_POINT fp2_pt_t[1];

// sm9 algorithm parameters
//  const unsigned int sm9p256v1_t[2]    = {0x0058F98Au,0x60000000u};
//  const unsigned int sm9p256v1_p[8]    =
//  {0xE351457Du,0xE56F9B27u,0x1A7AEEDBu,0x21F2934Bu,0xF58EC745u,0xD603AB4Fu,0x02A3A6F1u,0xB6400000u};
//  const unsigned int sm9p256v1_p_h[8]  =
//  {0xB417E2D2u,0x27DEA312u,0xAE1A5D3Fu,0x88F8105Fu,0xD6706E7Bu,0xE479B522u,0x56F62FBDu,0x2EA795A6u};
//  #if (defined(PKE_LP) || defined(PKE_SECURE))
//  const unsigned int sm9p256v1_p_n0[1] = {0x2F2EE42Bu,};
//  #endif
//  const unsigned int sm9p256v1_a[8]    =
//  {0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
//  const unsigned int sm9p256v1_b[8]    =
//  {0x00000005u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u};
//  const unsigned int sm9p256v1_Gx[8]   =
//  {0x7C66DDDDu,0xE8C4E481u,0x09DC3280u,0xE1E40869u,0x487D01D6u,0xF5ED0704u,0x62BF718Fu,0x93DE051Du};
//  const unsigned int sm9p256v1_Gy[8]   =
//  {0x0A3EA616u,0x0C464CD7u,0xFA602435u,0x1C1C00CBu,0x5C395BBCu,0x63106512u,0x4F21E607u,0x21FE8DDAu};
//  const unsigned int sm9p256v1_n[8]    =
//  {0xD69ECF25u,0xE56EE19Cu,0x18EA8BEEu,0x49F2934Bu,0xF58EC744u,0xD603AB4Fu,0x02A3A6F1u,0xB6400000u};
//  const unsigned int sm9p256v1_n_h[8]  =
//  {0xCD750C35u,0x7598CD79u,0xBB6DAEABu,0xE4A08110u,0x7D78A1F9u,0xBFEE4BAEu,0x63695D0Eu,0x8894F5D1u};
//  #if (defined(PKE_LP) || defined(PKE_SECURE))
//  const unsigned int sm9p256v1_n_n0[1] = {0x51974B53u,};
//  #endif

// sm9 para (n-1), for private key checking
unsigned int const sm9p256v1_n_1[8] = {0xD69ECF24u, 0xE56EE19Cu, 0x18EA8BEEu, 0x49F2934Bu, 0xF58EC744u, 0xD603AB4Fu, 0x02A3A6F1u, 0xB6400000u};

//[2^128]G, for [k]G of high speed
#if !(defined(PKE_LP) || defined(PKE_SECURE))
const unsigned int sm9p256v1_2_128_G_x[8] = {0x4FF01786u, 0xC677FCD6u, 0x4BC63E2Au, 0xB9FBCA7Cu, 0xDCDB5244u, 0xDBB96A1Cu, 0xE18508BCu, 0xAA480D0Cu};
const unsigned int sm9p256v1_2_128_G_y[8] = {0x99177E0Bu, 0x9ADE138Bu, 0x647ADE95u, 0x4D452749u, 0x1E25BA7Bu, 0xBBE5D04Au, 0x4338FA7Fu, 0x765C0578u};
#endif

// const eccp_curve_t sm9_curve[1] = {
//     {
//         SM9_BASE_BIT_LEN,
//         SM9_BASE_BIT_LEN,
//         (unsigned int *)sm9p256v1_p,
//         (unsigned int *)sm9p256v1_p_h,
// #if (defined(PKE_LP) || defined(PKE_SECURE))
//         (unsigned int *)sm9p256v1_p_n0,
// #endif
//         (unsigned int *)sm9p256v1_a,
//         (unsigned int *)sm9p256v1_b,
//         (unsigned int *)sm9p256v1_Gx,
//         (unsigned int *)sm9p256v1_Gy,
//         (unsigned int *)sm9p256v1_n,
//         (unsigned int *)sm9p256v1_n_h,
// #if (defined(PKE_LP) || defined(PKE_SECURE))
//         (unsigned int *)sm9p256v1_n_n0,
// #else
//         (unsigned int *)sm9p256v1_2_128_G_x,
//         (unsigned int *)sm9p256v1_2_128_G_y,
// #endif
//     },
// };

// P1
const fp2_pt_t fp2ptP1 = {{{{{0x7C66DDDDu, 0xE8C4E481u, 0x09DC3280u, 0xE1E40869u, 0x487D01D6u, 0xF5ED0704u, 0x62BF718Fu, 0x93DE051Du},
                             {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}}},
                           {{{0x0A3EA616u, 0x0C464CD7u, 0xFA602435u, 0x1C1C00CBu, 0x5C395BBCu, 0x63106512u, 0x4F21E607u, 0x21FE8DDAu},
                             {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}}},
                           {{{0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
                             {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}}}}};

// P2
const fp2_pt_t fp2ptP2 = {{{{{0xAF82D65Bu, 0xF9B7213Bu, 0xD19C17ABu, 0xEE265948u, 0xD34EC120u, 0xD2AAB97Fu, 0x92130B08u, 0x37227552u},
                             {0xD8806141u, 0x54806C11u, 0x0F5E93C4u, 0xF1DD2C19u, 0xB441A01Fu, 0x597B6027u, 0x78640C98u, 0x85AEF3D0u}}},
                           {{{0xC999A7C7u, 0x6215BBA5u, 0xA71A0811u, 0x47EFBA98u, 0x3D278FF2u, 0x5F317015u, 0x19BE3DA6u, 0xA7CF28D5u},
                             {0x84EBEB96u, 0x856DC76Bu, 0xA347C8BDu, 0x0736A96Fu, 0x2CBEE6EDu, 0x66BA0D26u, 0x2E845C12u, 0x17509B09u}}},
                           {{{0x00000001u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u},
                             {0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}}}}};

/**
 * @brief           copy a big-endian unsigned integer array from source to destination.
 *                  this function copies wlen elements from the source array src to the
 *                  destination array dst.
 * @param[out]      dst                  - pointer to the destination unsigned int array.
 * @param[in]       src                  - pointer to the source unsigned int array.
 * @param[in]       wlen                 - number of elements (unsigned ints) to copy.
 */
void pke_sm9_copy(unsigned int *dst, const unsigned int *src, unsigned int wlen)
{
    (void)wlen;
#if 0
    unsigned int i;

    for(i=0u; i<wlen; i++)
    {
        dst[i] = src[i];
    }
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[7] = src[7];
#endif
}

/**
 * @brief           copy a big-endian unsigned integer array from source to destination
 *                  and clear any remaining elements in the destination array beyond wlen.
 *                  this function copies wlen elements from the source array src to the
 *                  destination array dst, and sets any additional elements in dst to zero.
 * @param[out]      dst                  - pointer to the destination unsigned int array.
 * @param[in]       src                  - pointer to the source unsigned int array.
 * @param[in]       wlen                 - number of elements (unsigned ints) to copy.
 */
static void pke_sm9_copy_with_clear_tail(unsigned int *dst, const unsigned int *src, unsigned int wlen)
{
    (void)wlen;
#if 0
    unsigned int i;

    for(i=0u; i<wlen; i++)
    {
        dst[i] = src[i];
    }
#else
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[7] = src[7];
#endif
    dst[8] = 0u;
}

/**
 * @brief           set the modulus p and precompute montgomery parameters.
 * @return          rsa_success if the operation is successful, other values indicate an error.
 */
static unsigned int pke_sm9_set_p_and_pre_mont(void)
{
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
    return pke_load_modulus_and_pre_monts(sm9_curve->eccp_p, sm9_curve->eccp_p_h, sm9_curve->eccp_p_n0, SM9_BASE_BIT_LEN);
}

/**
 * @brief           compute the modular inverse of a modulo p or n.
 *                  this function computes the modular inverse of a with respect to the modulus
 *                  p or n. the modulus is either p or n, both of which are prime. the inverse exists
 *                  except when a=0.
 * @param[in]       a                    - pointer to the unsigned int array representing the number to invert.
 * @param[out]      ainv                 - pointer to the unsigned int array where the modular inverse will be stored.
 * @return          rsa_success if the operation is successful, other values indicate an error.
 */
unsigned int pke_sm9_simple_modinv(unsigned int *a, unsigned int *ainv)
{
    unsigned int ret;

    // pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(3u,SM9_STEPS)),
    // (unsigned int *)modulus, mod_wlen);   //B3 modulus
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)a, SM9_BASE_WORD_LEN); // B0 a

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODINV);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, SM9_STEPS)), ainv,
                     SM9_BASE_WORD_LEN); // A0 ainv

    return PKE_SUCCESS;
}

/**
 * @brief           compute the modular multiplication of a and b modulo p or n.
 *                  this function computes the modular multiplication of a and b with respect to
 *                  the modulus p or n. the result is stored in the out array.
 * @param[in]       a                    - pointer to the unsigned int array representing the first operand.
 * @param[in]       b                    - pointer to the unsigned int array representing the second operand.
 * @param[out]      out                  - pointer to the unsigned int array where the result of the modular multiplication will be stored.
 * @return          rsa_success if the operation is successful, other values indicate an error.
 */
unsigned int pke_sm9_simple_modmul(unsigned int *a, unsigned int *b, unsigned int *out)
{
    unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int *)a, SM9_BASE_WORD_LEN); // A0 a
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)b, SM9_BASE_WORD_LEN); // B0 b

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, SM9_STEPS)), out,
                     SM9_BASE_WORD_LEN); // A0 out

    return PKE_SUCCESS;
}

/**
 * @brief           compute the modular addition of a and b modulo p or n.
 *                  this function computes the modular addition of a and b with respect to
 *                  the modulus p or n. the result is stored in the out array.
 * @param[in]       a                    - pointer to the unsigned int array representing the first operand.
 * @param[in]       b                    - pointer to the unsigned int array representing the second operand.
 * @param[out]      out                  - pointer to the unsigned int array where the result of the modular addition will be stored.
 * @return          rsa_success if the operation is successful, other values indicate an error.
 */
unsigned int pke_sm9_simple_modadd(unsigned int *a, unsigned int *b, unsigned int *out)
{
    unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int *)a, SM9_BASE_WORD_LEN); // A0 a
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)b, SM9_BASE_WORD_LEN); // B0 b

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pke_read_operand((unsigned int *)(rPKE_A(0u, SM9_STEPS)), out,
                     SM9_BASE_WORD_LEN); // A0 result

    return PKE_SUCCESS;
}

/*
void pke_sm9_simple_modsub(const unsigned int *a, const unsigned int *b,
unsigned int *out)
{
    pke_load_operand((unsigned int *)(rPKE_A(0u,SM9_STEPS)), (unsigned int *)a,
SM9_BASE_WORD_LEN);          //A1 a pke_load_operand((unsigned int
*)(rPKE_B(0u,SM9_STEPS)), (unsigned int *)b, SM9_BASE_WORD_LEN);          //B1 b

    pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_read_operand((unsigned int *)(rPKE_A(0u, SM9_STEPS)), out,
SM9_BASE_WORD_LEN);                   //A1 result
}*/

/*
void pke_sm9_simple_multiple_modsub(const unsigned int *a, const unsigned int
*b, const unsigned int *c, unsigned int *out)
{
    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), (unsigned int *)a,
SM9_BASE_WORD_LEN);          //A1 a pke_sm9_copy((unsigned int
*)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)b, SM9_BASE_WORD_LEN);          //B1 b
    pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)c,
SM9_BASE_WORD_LEN);          //B1 c
    pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(out, (unsigned int *)(rPKE_A(1u, SM9_STEPS)),
SM9_BASE_WORD_LEN);                   //A1 result
}*/

/**
 * @brief           compute the modular subtraction of a and b, followed by modular addition with c modulo p or n.
 *                  this function computes (a - b + c) mod (p or n). the result is stored in the out array.
 * @param[in]       a                    - pointer to the unsigned int array representing the first operand for subtraction.
 * @param[in]       b                    - pointer to the unsigned int array representing the second operand for subtraction.
 * @param[in]       c                    - pointer to the unsigned int array representing the operand for addition.
 * @param[out]      out                  - pointer to the unsigned int array where the result of the modular operation will be stored.
 * @return          rsa_success if the operation is successful, other values indicate an error.
 */
unsigned int pke_sm9_simple_multiple_modsub_modadd(const unsigned int *a, const unsigned int *b, const unsigned int *c, unsigned int *out)
{
    unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN); // A0 a
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), b, SM9_BASE_WORD_LEN); // B0 b
    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), c, SM9_BASE_WORD_LEN); // B0 c

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pke_sm9_copy(out, (unsigned int *)(rPKE_A(0u, SM9_STEPS)),
                 SM9_BASE_WORD_LEN); // A0 result

    return PKE_SUCCESS;
}

/*
void pke_sm9_simple_multiple_modsub_v2(const unsigned int *a, const unsigned int
*b, unsigned int *out)
{
    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), (unsigned int *)a,
SM9_BASE_WORD_LEN);          //A1 a pke_sm9_copy((unsigned int
*)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)b, SM9_BASE_WORD_LEN);          //B1 b
    pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(out, (unsigned int *)(rPKE_A(1u, SM9_STEPS)),
SM9_BASE_WORD_LEN);                   //A1 result
}*/

/**
 * @brief           subtract the input array a from the prime p and store the result in out.
 *                  this function performs the operation (p - a) mod p, where p is the prime modulus,
 *                  and stores the result in the out array.
 * @param[in]       a                    - pointer to the unsigned int array representing the number to subtract.
 * @param[out]      out                  - pointer to the unsigned int array where the result will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int pke_sm9_prime_p_sub(const unsigned int *a, unsigned int *out)
{
#if 0
//    unsigned int sm9p256v1_p[8]    = {0xE351457Du,0xE56F9B27u,0x1A7AEEDBu,0x21F2934Bu,0xF58EC745u,0xD603AB4Fu,0x02A3A6F1u,0xB6400000u};

    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(0u,SM9_STEPS)), SM9_BASE_WORD_LEN); //A1 a = p
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)a, SM9_BASE_WORD_LEN);                      //B1 b

    pke_set_micro_code_start_wait_return_code(MICROCODE_INTSUB);
#else
    unsigned int ret;

    uint32_clear((unsigned int *)(rPKE_A(0u, SM9_STEPS)),
                 SM9_BASE_WORD_LEN + 1);                                                         // A1 a = 0
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN); // B0 b

    ret = pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
#endif

    pke_sm9_copy(out, (unsigned int *)(rPKE_A(0u, SM9_STEPS)),
                 SM9_BASE_WORD_LEN); // A0 result

    return PKE_SUCCESS;
}

/**
 * @brief           convert a big-endian byte array to a little-endian unsigned int array.
 *                  this function converts a 32-byte big-endian array into an 8-element little-endian
 *                  unsigned int array.
 * @param[out]      out                  - pointer to the unsigned int array where the result will be stored.
 * @param[in]       in                   - pointer to the 32-byte big-endian input array.
 */
static void u8big_to_u32little_8(unsigned int out[], const unsigned char in[])
{
    out[7] = ((unsigned int)in[3]) | ((unsigned int)in[2] << 8u) | ((unsigned int)in[1] << 16u) | ((unsigned int)in[0] << 24u);
    out[6] = ((unsigned int)in[7]) | ((unsigned int)in[6] << 8u) | ((unsigned int)in[5] << 16u) | ((unsigned int)in[4] << 24u);
    out[5] = ((unsigned int)in[11]) | ((unsigned int)in[10] << 8u) | ((unsigned int)in[9] << 16u) | ((unsigned int)in[8] << 24u);
    out[4] = ((unsigned int)in[15]) | ((unsigned int)in[14] << 8u) | ((unsigned int)in[13] << 16u) | ((unsigned int)in[12] << 24u);
    out[3] = ((unsigned int)in[19]) | ((unsigned int)in[18] << 8u) | ((unsigned int)in[17] << 16u) | ((unsigned int)in[16] << 24u);
    out[2] = ((unsigned int)in[23]) | ((unsigned int)in[22] << 8u) | ((unsigned int)in[21] << 16u) | ((unsigned int)in[20] << 24u);
    out[1] = ((unsigned int)in[27]) | ((unsigned int)in[26] << 8u) | ((unsigned int)in[25] << 16u) | ((unsigned int)in[24] << 24u);
    out[0] = ((unsigned int)in[31]) | ((unsigned int)in[30] << 8u) | ((unsigned int)in[29] << 16u) | ((unsigned int)in[28] << 24u);
}

/**
 * @brief           convert a little-endian unsigned int array to a big-endian byte array.
 *                  this function converts an 8-element little-endian unsigned int array into a
 *                  32-byte big-endian array.
 * @param[out]      out                  - pointer to the 32-byte big-endian output array.
 * @param[in]       in                   - pointer to the 8-element little-endian input array.
 */
static void u32little_to_u8big(unsigned char out[], const unsigned int in[])
{
    unsigned int i, j;
#if 0
    unsigned int t;
    if(out == (unsigned char *)in)
    {
        for(i = 0u; i < 4u; i++)
        {
            t = in[7u-i];
            j = 28u - (i << 2);
            out[j]      = (unsigned char)((in[i] >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
            j = i<<2;
            out[j]      = (unsigned char)((t >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((t >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((t >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)(t & 0xffu);

        }
    }
    else
    {
        for(i = 0u; i < 8u; i++)
        {
            j = 28u - (i << 2);
            out[j]      = (unsigned char)((in[i] >> 24) & 0xffu);
            out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
            out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
            out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
        }
    }
#else
    for (i = 0u; i < 8u; i++)
    {
        j = 28u - (i << 2);
        out[j] = (unsigned char)((in[i] >> 24) & 0xffu);
        out[j + 1u] = (unsigned char)((in[i] >> 16) & 0xffu);
        out[j + 2u] = (unsigned char)((in[i] >> 8) & 0xffu);
        out[j + 3u] = (unsigned char)((in[i]) & 0xffu);
    }
#endif
}

/**
 * @brief           h = (ha mod (n-1)) + 1
 * @param[in]       ha                   - input, -a big number of 320bits (40 bytes)
 * @param[out]      h                    - output, -h = (ha mod (n-1)) + 1
 * @return          PKE_SUCCESS(success), other(error)
 * @note            :
 *        1. this function is called by H1() or H2(), i.e., sm9_h1_h2()
 *        2. ha is an internal big number of 320bits (40 bytes), h is a big number
 *           of 256bits.
 */
static unsigned int sm9_h1_h2_mod(unsigned char ha[40], unsigned int *h)
{
    unsigned int t[8];
    unsigned int h1[8];
    unsigned int h2[8];
    unsigned int h3[8];
    unsigned int n_compl[8] = {0x296130DBu, 0x1A911E63u, 0xE7157411u, 0xB60D6CB4u, 0x0A7138BBu, 0x29FC54B0u, 0xFD5C590Eu, 0x49BFFFFFu}; // 2^(256) mod n
    unsigned int n2[8] = {0xFFFFF8A5u, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu};
    unsigned int n2_inv[8] = {0xD81AE00Bu, 0x73FFE2F2u, 0x3CA00AC3u, 0x6EE46995u, 0x859A2700u, 0xECEE7342u, 0xE23BB01Cu, 0xB707F075u};   // n^(-1) mod n2
    unsigned int n2_compl[8] = {0x0000075Bu, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u}; // 2^(256) mod n2
    // unsigned int ret;

    // h1 = high 8 byte of ha
    h1[0] = ((unsigned int)ha[7]) | ((unsigned int)ha[6] << 8u) | ((unsigned int)ha[5] << 16u) | ((unsigned int)ha[4] << 24u);
    h1[1] = ((unsigned int)ha[3]) | ((unsigned int)ha[2] << 8u) | ((unsigned int)ha[1] << 16u) | ((unsigned int)ha[0] << 24u);
    h1[2] = 0x00000000u;
    h1[3] = 0x00000000u;
    h1[4] = 0x00000000u;
    h1[5] = 0x00000000u;
    h1[6] = 0x00000000u;
    h1[7] = 0x00000000u;

    // h2 = low 32 byte of ha
    u8big_to_u32little_8(h2, (unsigned char *)(ha + 8u));

    /************ get h3 = ha mod n ************/
    (void)pke_modmul(sm9_curve->eccp_n, h1, n_compl, h3, 8u);
    if (uint32_big_num_cmp(h2, 8u, sm9_curve->eccp_n, 8u) >= 0)
    {
        (void)pke_sub(h2, sm9_curve->eccp_n, t, 8u);
        (void)pke_sm9_simple_modadd(h3, t,
                                    h3); // pke_modadd(sm9p256v1_n, Hc, t, Hc, 8);
    }
    else
    {
        (void)pke_sm9_simple_modadd(h3, h2,
                                    h3); // pke_modadd(sm9p256v1_n, Hc, Hb, Hc, 8);
    }

    /************ get h1||h2 = (ha - h3) = k1*n ************/
    if (uint32_big_num_cmp(h2, 8u, h3, 8u) >= 0)
    {
        (void)pke_sub(h2, h3, h2, 8u);
    }
    else
    {
        (void)pke_sub(h2, h3, h2, 8u);

        if (h1[0] != 0x00000000u)
        {
            h1[0] -= 0x00000001u;
        }
        else
        {
            h1[0] = 0xFFFFFFFFu;
            h1[1] -= 0x00000001u;
        }
    }

    /************ get h1 = h1||h2 mod n2 = k1*n mod n2, n2 > n. ************/
    (void)pke_modmul(n2, h1, n2_compl, h1, 8u);
    if (uint32_big_num_cmp(h2, 8u, n2, 8u) >= 0)
    {
        (void)pke_sub(h2, n2, h2, 8u);
    }
    (void)pke_sm9_simple_modadd(h1, h2, h1); // pke_modadd(N2, Ha, Hb, Ha, 8u);

    /************ get h2 = k1 mod n2, n2 > n. ************/
    (void)pke_sm9_simple_modmul(h1, n2_inv,
                                h2); // pke_modmul(N2, Ha, N2_inv, Hb, 8u);

    /************ get h3 = h3 mod (n-1) ************/
    if (uint32_big_num_cmp(h3, 8u, sm9p256v1_n_1, 8u) >= 0)
    {
        (void)pke_sub(h3, sm9p256v1_n_1, h3, 8u);
    }

    /************ get h = k1+h3 mod (n-1) = ha mod (n-1) ************/
    (void)pke_modadd(sm9p256v1_n_1, h3, h2, h, 8u);

    // h += 1
    (void)uint32_big_num_little_endian_add_little(h, 8u, 1u, (unsigned char)1);

    return PKE_SUCCESS;
}

/**
 * @brief           H1(Z,n) or H2(Z,n), the difference of the two is that the two tag
 * @param[in]       tag                  - the tag value distinguishing between H1 and H2.
 * @param[in]       z1                   - pointer to the first part of the input byte array Z.
 * @param[in]       z1_len               - length of the first part of the input byte array Z in bytes.
 * @param[in]       z2                   - pointer to the second part of the input byte array Z.
 * @param[in]       z2_len               - length of the second part of the input byte array Z in bytes.
 * @param[out]      h                    - pointer to the unsigned int array where the resulting hash value will be stored (8 elements).
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 * @note
 *        1.values are different. Z=z1||z2, n is order of the elliptic curve, the output
 *        2.h is a big number less than n.
 */
static unsigned int sm9_h1_h2(unsigned char tag, const unsigned char *z1, unsigned int z1_len, const unsigned char *z2, unsigned int z2_len, unsigned int h[8])
{
    unsigned int ret;

#if 1
    unsigned char cnt[4] = {0x00, 0x00, 0x00, 0x02};
    unsigned char ha[40];
    hash_node_t hash_node[4] = {
        {&tag, 1u},
        {z1, z1_len},
        {z2, z2_len},
        {cnt, 4u},
    };

    ret = hash_node_steps(HASH_SM3, hash_node, 4u, ha);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
    memcpy_(ha + 32u, ha, 8u);

    cnt[3] = (unsigned char)0x01;
    ret = hash_node_steps(HASH_SM3, hash_node, 4u, ha);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
#else
    unsigned char cnt[4] = {0x00, 0x00, 0x00, 0x01};
    unsigned char ha[64];
    hash_node_t hash_node[4] = {
        {&tag, 1u},
        {z1, z1_len},
        {z2, z2_len},
        {cnt, 4u},
    };

    ret = hash_node_steps(HASH_SM3, hash_node, 4u, ha);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    cnt[3] = (unsigned char)0x02;
    ret = hash_node_steps(HASH_SM3, hash_node, 4u, ha + 32u);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
#endif

    return sm9_h1_h2_mod(ha, h);
}

/**
 * @brief           field element to octet string primitive
 * @param[in]       input                - Pointer to the mpc12_t structure containing the field element.
 * @param[out]      output               - Pointer to the buffer where the octet string will be stored.
 *                                         The buffer must have enough space to hold the resulting octet string.
 */
static void FE2OSP(mpc12_t input, unsigned char *output)
{
    u32little_to_u8big(output + 32u * 11u, input->f->f->re);
    u32little_to_u8big(output + 32u * 10u, input->f->f->im);
    u32little_to_u8big(output + 32u * 3u, input->f->ft->re);
    u32little_to_u8big(output + 32u * 2u, input->f->ft->im);
    u32little_to_u8big(output + 32u * 5u, input->f->ft2->re);
    u32little_to_u8big(output + 32u * 4u, input->f->ft2->im);
    u32little_to_u8big(output + 32u * 7u, input->fW->f->re);
    u32little_to_u8big(output + 32u * 6u, input->fW->f->im);
    u32little_to_u8big(output + 32u * 9u, input->fW->ft->re);
    u32little_to_u8big(output + 32u * 8u, input->fW->ft->im);
    u32little_to_u8big(output + 32u, input->fW->ft2->re);
    u32little_to_u8big(output, input->fW->ft2->im);
}

/**
 * @brief           octet string to field element primitive
 * @param[in]       input                - Pointer to the buffer containing the octet string.
 *                                         The buffer must contain the octet string in big-endian byte order.
 * @param[out]      output               - Pointer to the mpc12_t structure where the field element will be stored.
 */
static void OS2FEP(const unsigned char *input, mpc12_t output)
{
    u8big_to_u32little_8(output->f->f->re, input + 32u * 11u);
    u8big_to_u32little_8(output->f->f->im, input + 32u * 10u);
    u8big_to_u32little_8(output->f->ft->re, input + 32u * 3u);
    u8big_to_u32little_8(output->f->ft->im, input + 32u * 2u);
    u8big_to_u32little_8(output->f->ft2->re, input + 32u * 5u);
    u8big_to_u32little_8(output->f->ft2->im, input + 32u * 4u);
    u8big_to_u32little_8(output->fW->f->re, input + 32u * 7u);
    u8big_to_u32little_8(output->fW->f->im, input + 32u * 6u);
    u8big_to_u32little_8(output->fW->ft->re, input + 32u * 9u);
    u8big_to_u32little_8(output->fW->ft->im, input + 32u * 8u);
    u8big_to_u32little_8(output->fW->ft2->re, input + 32u);
    u8big_to_u32little_8(output->fW->ft2->im, input);
}

/**
 * @brief           Compares two 256-bit unsigned integers.
 * @param[in]       out                  - Pointer to the first array of 8 unsigned integers.
 * @param[in]       in                   - Pointer to the second array of 8 unsigned integers.
 * @return          1 if the integers are not equal, 0 if they are equal.
 */
static unsigned int uint32_cmp(unsigned int out[8], const unsigned int in[8])
{
    unsigned int i;
    for (i = 0u; i < 8u; i++)
    {
        if (out[i] != in[i])
        {
            return 1u;
        }
        else
        {
            ;
        }
    }
    return 0u;
}

/**
 * @brief           Copies a complex field element.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the complex field element will be stored.
 * @param[in]       src                  - Pointer to the mpc_t structure containing the complex field element to copy.
 */
static void mpc_fp2_copy(mpc_t dst, const mpc_t src)
{
    pke_sm9_copy(dst->re, src->re, 8u);
    pke_sm9_copy(dst->im, src->im, 8u);
}

/**
 * @brief           Clears a complex field element.
 * @param[in,out]   x                    - Pointer to the mpc_t structure containing the complex field element to clear.
 */
static void mpc_fp2_clears(mpc_ptr x)
{
#if 1
    uint32_clear(x->re, 8u);
    uint32_clear(x->im, 8u);
#else
    x->re[0] = 0u;
    x->re[1] = 0u;
    x->re[2] = 0u;
    x->re[3] = 0u;
    x->re[4] = 0u;
    x->re[5] = 0u;
    x->re[6] = 0u;
    x->re[7] = 0u;
    x->im[0] = 0u;
    x->im[1] = 0u;
    x->im[2] = 0u;
    x->im[3] = 0u;
    x->im[4] = 0u;
    x->im[5] = 0u;
    x->im[6] = 0u;
    x->im[7] = 0u;
#endif
}

/**
 * @brief           g = f^(-1)
 *
 *                  This function computes the multiplicative inverse of a complex field element represented by an mpc_t structure.
 *                  The result is stored in another mpc_t structure.
 *
 * @param[out]      g                    - Pointer to the mpc_t structure where the resulting inverse will be stored.
 * @param[in]       f                    - Pointer to the mpc_t structure containing the complex field element to invert.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_inv(mpc_t g, mpc_t f)
{
    unsigned int c1[8];
    unsigned int c2[8];
    unsigned int ret;

    (void)pke_sm9_prime_p_sub(f->im, g->im);

    (void)pke_sm9_simple_modmul(f->re, f->re, c1);
    (void)pke_sm9_simple_modmul(f->im, f->im, c2);
    (void)pke_sm9_simple_modadd(c2, c2, c2);
    (void)pke_sm9_simple_modadd(c1, c2, c1);

    ret = pke_sm9_simple_modinv(c1, c1);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)pke_sm9_simple_modmul(f->re, c1, g->re);
    (void)pke_sm9_simple_modmul(g->im, c1, g->im);

    return PKE_SUCCESS;
}

/**
 * @brief           Adds two complex field elements.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting sum will be stored.
 * @param[in]       x                    - Pointer to the first mpc_t structure containing the complex field element.
 * @param[in]       y                    - Pointer to the second mpc_t structure containing the complex field element.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_add(mpc_t dst, const mpc_t x, const mpc_t y)
{
#if 0
    (void)pke_sm9_simple_modadd(x->re, y->re, dst->re);
    (void)pke_sm9_simple_modadd(x->im, y->im, dst->im);
#else
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           dst = 3x
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element to multiply.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul_3(mpc_t dst, const mpc_t x)
{
#if 0
    (void)pke_sm9_simple_modadd(x->re, x->re, (unsigned int *)(rPKE_A(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->re);
    (void)pke_sm9_simple_modadd(x->im, x->im, (unsigned int *)(rPKE_A(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->im);
#else
    unsigned int a[8];
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    pke_sm9_copy(dst->re, a, SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           dst = 4x
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element to multiply.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul_4(mpc_t dst, const mpc_t x)
{
#if 1
#if 0
    (void)pke_sm9_simple_modadd(x->re, x->re, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->re);
    (void)pke_sm9_simple_modadd(x->im, x->im, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->im);
#else
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif
#else
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_set_operand_uint32_value((unsigned int *)(rPKE_B(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1, 4);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_set_operand_uint32_value((unsigned int *)(rPKE_B(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1, 4);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           dst = 8x
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element to multiply.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul_8(mpc_t dst, const mpc_t x)
{
#if 0
#if 0
    (void)pke_sm9_simple_modadd(x->re, x->re, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->re);
    (void)pke_sm9_simple_modadd(x->im, x->im, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->im);
#else
    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy((unsigned int *)(rPKE_B(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif
#else
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_set_operand_uint32_value((unsigned int *)(rPKE_B(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1, 8);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_set_operand_uint32_value((unsigned int *)(rPKE_B(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1, 8);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Subtracts one complex field element from another.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting difference will be stored.
 * @param[in]       x                    - Pointer to the first mpc_t structure containing the complex field element.
 * @param[in]       y                    - Pointer to the second mpc_t structure containing the complex field element to subtract.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_sub(mpc_t dst, const mpc_t x, const mpc_t y)
{
#if 0
    pke_sm9_simple_modsub(x->re, y->re, dst->re);
    pke_sm9_simple_modsub(x->im, y->im, dst->im);
#else
    unsigned int a[8];
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    pke_sm9_copy(dst->re, a, SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Subtracts two complex field elements from another.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting difference will be stored.
 * @param[in]       x                    - Pointer to the first mpc_t structure containing the complex field element.
 * @param[in]       y                    - Pointer to the second mpc_t structure containing the complex field element to subtract.
 * @param[in]       z                    - Pointer to the third mpc_t structure containing the complex field element to subtract.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_sub_sub(mpc_t dst, const mpc_t x, const mpc_t y, const mpc_t z)
{
    unsigned int a[8];
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), z->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), z->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    pke_sm9_copy(dst->re, a, SM9_BASE_WORD_LEN);

    return PKE_SUCCESS;
}

/*
static void mpc_fp2_sub_add(mpc_t dst, const mpc_t x, const mpc_t y, const mpc_t
z)
{
    unsigned int a[8];

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u,SM9_STEPS)),
(unsigned int *)(x->re), SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)),
(unsigned int *)(y->re), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)),
(unsigned int *)(z->re), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u,SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u,SM9_STEPS)),
(unsigned int *)(x->im), SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)),
(unsigned int *)(y->im), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)),
(unsigned int *)(z->im), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u,SM9_STEPS)),
SM9_BASE_WORD_LEN);

    pke_sm9_copy(dst->re, a, SM9_BASE_WORD_LEN);
}*/

#if 1
/**
 * @brief           dst = x*y
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the first mpc_t structure containing the complex field element.
 * @param[in]       y                    - Pointer to the second mpc_t structure containing the complex field element.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul(mpc_t dst, const mpc_t x, const mpc_t y)
{
    unsigned int a[8];
    unsigned int c[8];
    // unsigned int ret;

#if 0
    unsigned int b[8];
    (void)pke_modmul(sm9p256v1_p, x->re, y->re, a, 8u);
    (void)pke_modmul(sm9p256v1_p, x->im, y->im, b, 8u);
    (void)pke_modadd(sm9p256v1_p, b, b, b, 8u);
    (void)pke_modsub(sm9p256v1_p, a, b, c, 8u);

    (void)pke_modmul(sm9p256v1_p, x->re, y->im, a, 8u);
    (void)pke_modmul(sm9p256v1_p, x->im, y->re, b, 8u);
    (void)pke_modadd(sm9p256v1_p, a, b, dst->im, 8u);
    uint32_copy(dst->re, c, 8u);
#else
    //    pke_sm9_simple_modmul(x->re, y->re, a);
    //    pke_sm9_simple_modmul(x->im, y->im, (unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd((unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS))); pke_sm9_simple_modsub(a, (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS)), c);
    //
    //    pke_sm9_simple_modmul(x->re, y->im, a);
    //    pke_sm9_simple_modmul(x->im, y->re, (unsigned int *)(rPKE_B(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd(a, (unsigned int *)(rPKE_B(0u,
    //    SM9_STEPS)), dst->im); uint32_copy(dst->re, c, 8);

    //    pke_sm9_simple_modmul(x->re, y->im, c);
    //    pke_sm9_simple_modmul(x->im, (unsigned int *)(rPKE_B(1u, SM9_STEPS)),
    //    b); pke_sm9_simple_modmul(x->re, y->re, a); pke_sm9_simple_modmul(x->im,
    //    (unsigned int *)(rPKE_B(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u,
    //    SM9_STEPS))); pke_sm9_simple_modadd(c, (unsigned int *)(rPKE_B(1u,
    //    SM9_STEPS)), dst->im); pke_sm9_simple_modadd(b, b, (unsigned int
    //    *)(rPKE_B(1u, SM9_STEPS))); pke_sm9_simple_modsub(a, (unsigned int
    //    *)(rPKE_B(1u, SM9_STEPS)), dst->re);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(c, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), y->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy(dst->re, c, SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}
#else
static unsigned int mpc_fp2_mul(mpc_t dst, const mpc_t x, const mpc_t y)
{
    unsigned int A[8];
    unsigned int B[8];
    unsigned int C[8];

    (void)pke_sm9_simple_modadd(x->re, x->im, C);
    (void)pke_sm9_simple_modadd(y->re, y->im, B);

    (void)pke_sm9_simple_modmul(C, B, C);
    (void)pke_sm9_simple_modmul(x->re, y->re, A);
    (void)pke_sm9_simple_modmul(x->im, y->im, B);

    (void)pke_sm9_simple_modsub(C, A, C);
    (void)pke_sm9_simple_modsub(C, B, C);
    (void)pke_sm9_simple_modadd(B, B, B);
    (void)pke_sm9_simple_modsub(A, B, dst->re);

    uint32_copy(dst->im, C, 8u);

    return PKE_SUCCESS;
}
#endif

/**
 * @brief           Multiplies a complex field element by an array.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element.
 * @param[in]       a                    - Array of unsigned integers representing the multiplier.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul_a(mpc_t dst, const mpc_t x, unsigned int a[8])
{
#if 0
    (void)pke_sm9_simple_modmul(dst->im, x->im, a);
    (void)pke_sm9_simple_modmul(dst->re, x->re, a);
#else
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Multiplies a complex field element by a unit.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting product will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp2_mul_u(mpc_t dst, const mpc_t x)
{
#if 0
#if 0
    (void)pke_sm9_simple_modadd(x->im,x->im, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    uint32_copy(dst->im, x->re, 8u);
    (void)pke_sm9_prime_p_sub((unsigned int *)(rPKE_B(1u, SM9_STEPS)), dst->re);
#else
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u,SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u,SM9_STEPS)), (unsigned int *)(rPKE_A(0u,SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy(dst->im, x->re, SM9_BASE_WORD_LEN);

    pke_sm9_copy((unsigned int *)(rPKE_A(1u,SM9_STEPS)), (unsigned int *)(rPKE_A(0u,SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_INTSUB);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(1u,SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif
#else
    // unsigned int ret;

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy(dst->im, x->re, SM9_BASE_WORD_LEN);

    uint32_clear((unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Squares a complex field element.
 * @param[in,out]   x                    - Pointer to the mpc_t structure containing the complex field element, which will be overwritten with the squared result.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp2_square_v2(mpc_t x)
{
    unsigned int a[8];
    //    unsigned int b[8];
    unsigned int c[8];
#if 0
    (void)pke_modmul(sm9p256v1_p, x->re, x->re, a, 8u);
    (void)pke_modmul(sm9p256v1_p, x->im, x->im, b, 8u);
    (void)pke_modadd(sm9p256v1_p, b, b, b, 8u);
    (void)pke_modsub(sm9p256v1_p, a, b, c, 8u);

    (void)pke_modmul(sm9p256v1_p, x->im, x->re, b, 8u);
    (void)pke_modadd(sm9p256v1_p, b, b, x->im, 8u);
    uint32_copy((unsigned int *)x->re, c, 8u);
#else

#if 0
    (void)pke_sm9_simple_modmul(x->im, x->im, c);
    (void)pke_sm9_simple_modmul(x->re, x->re, a);
    (void)pke_sm9_simple_modmul(x->im, (unsigned int *)(rPKE_B(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modadd((unsigned int *)(rPKE_A(1u, SM9_STEPS)), (unsigned int *)(rPKE_B(1u, SM9_STEPS)), x->im);
    (void)pke_sm9_simple_modadd(c, c, (unsigned int *)(rPKE_B(1u, SM9_STEPS)));
    (void)pke_sm9_simple_modsub(a, (unsigned int *)(rPKE_B(1u, SM9_STEPS)), x->re);
#else
    //    pke_sm9_simple_modmul(x->re, x->re, a);
    //    pke_sm9_simple_modmul(x->im, x->im, (unsigned int *)(rPKE_B(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd((unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS)), (unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS))); pke_sm9_simple_modsub(a, (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS)), c);
    //
    //    pke_sm9_simple_modmul(x->im, x->re, (unsigned int *)(rPKE_B(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd((unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS)), (unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im);
    //    uint32_copy(x->re, c, 8u);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int *)(x->re), SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(x->re), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int *)(x->im), SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(x->im), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(c, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int *)(x->im), SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(x->re), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy((unsigned int *)(x->im), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy((unsigned int *)(x->re), c, 8u);
#endif

#endif

    return PKE_SUCCESS;
}

//
/**
 * @brief           Squares a complex field element.
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting square will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element to be squared.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 * @note            dst and x can not point the same
 */
unsigned int mpc_fp2_square_v1(mpc_t dst, const mpc_t x)
{
    unsigned int a[8];
// unsigned int ret;
#if 0
    unsigned int b[8];
    unsigned int c[8];

    (void)pke_modmul(sm9p256v1_p, x->re, x->re, a, 8u);
    (void)pke_modmul(sm9p256v1_p, x->im, x->im, b, 8u);
    (void)pke_modadd(sm9p256v1_p, b, b, b, 8u);
    (void)pke_modsub(sm9p256v1_p, a, b, c, 8u);

    (void)pke_modmul(sm9p256v1_p, x->im, x->re, b, 8u);
    (void)pke_modadd(sm9p256v1_p, b, b, dst->im, 8u);
#else
    //    pke_sm9_simple_modmul(x->re, x->re, a);
    //    pke_sm9_simple_modmul(x->im, x->im, (unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd((unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS))); pke_sm9_simple_modsub(a, (unsigned int
    //    *)(rPKE_B(0u, SM9_STEPS)), dst->re);
    //
    //    pke_sm9_simple_modmul(x->im, x->re, (unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS))); pke_sm9_simple_modadd((unsigned int *)(rPKE_A(0u,
    //    SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), dst->im);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);
    pke_sm9_copy(a, (unsigned int *)(rPKE_A(0, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), a, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_A(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODMUL);

    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODADD);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           dst = -x
 * @param[out]      dst                  - Pointer to the mpc_t structure where the resulting negative will be stored.
 * @param[in]       x                    - Pointer to the mpc_t structure containing the complex field element to be negated.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp2_negative(mpc_ptr dst, mpc_ptr x)
{
    // unsigned int ret;

    uint32_clear((unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1u);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->re, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->re, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    uint32_clear((unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN + 1u);
    pke_sm9_copy_with_clear_tail((unsigned int *)(rPKE_B(0u, SM9_STEPS)), x->im, SM9_BASE_WORD_LEN);
    (void)pke_set_micro_code_start_wait_return_code(MICROCODE_MODSUB);
    pke_sm9_copy(dst->im, (unsigned int *)(rPKE_A(0u, SM9_STEPS)), SM9_BASE_WORD_LEN);

    return PKE_SUCCESS;
}

/**
 * @brief           Verifies if a point lies on the sm9 curve.
 * @param[in]       x                    - Array of unsigned integers representing the x-coordinate of the point.
 * @param[in]       y                    - Array of unsigned integers representing the y-coordinate of the point.
 * @return          0:in the curve, others:not in the curve
 * @note
 *        1.actually the point S is GF(p) point.
 */
static unsigned int sm9_pointVerify(unsigned int x[8], unsigned int y[8])
{
    unsigned int A[8], B[8], C[8];

    pke_set_operand_uint32_value(C, SM9_BASE_WORD_LEN, 5);

    (void)pke_sm9_simple_modmul(x, x, A); // pke_modmul(sm9p256v1_p, S->x->re, S->x->re, A, 8u);
    (void)pke_sm9_simple_modmul(A, x, A); // pke_modmul(sm9p256v1_p, A, S->x->re, A, 8u);
    (void)pke_sm9_simple_modmul(y, y, B); // pke_modmul(sm9p256v1_p, S->y->re, S->y->re, B, 8u);
    (void)pke_sm9_simple_modadd(A, C, A); // pke_modadd(sm9p256v1_p, A, C, A, 8u);

    return uint32_cmp(A, B);
}

/**
 * @brief           E(Fp^2) jacobi point to affine point.
 * @param[in]       x_in                 - Pointer to the mpc_t structure containing the x-coordinate of the Jacobian point.
 * @param[in]       y_in                 - Pointer to the mpc_t structure containing the y-coordinate of the Jacobian point.
 * @param[in]       z_in                 - Pointer to the mpc_t structure containing the z-coordinate of the Jacobian point.
 * @param[out]      x_out                - Pointer to the mpc_t structure where the x-coordinate of the affine point will be stored.
 * @param[out]      y_out                - Pointer to the mpc_t structure where the y-coordinate of the affine point will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int coordinate_convert(mpc_t x_in, mpc_t y_in, mpc_t z_in, mpc_t x_out, mpc_t y_out)
{
    mpc_t z1, z2;
    unsigned int ret;

    ret = mpc_fp2_inv(z1, z_in);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)mpc_fp2_square_v1(z2, z1);
    (void)mpc_fp2_mul(z1, z2, z1);

    (void)mpc_fp2_mul(x_out, x_in, z2);
    (void)mpc_fp2_mul(y_out, y_in, z1);

    return PKE_SUCCESS;
}

/**
 * @brief           E(Fp^2) point double
 * @param[in]       x                    - Pointer to the mpc_t structure containing the x-coordinate of the input point.
 * @param[in]       y                    - Pointer to the mpc_t structure containing the y-coordinate of the input point.
 * @param[in]       z                    - Pointer to the mpc_t structure containing the z-coordinate of the input point.
 * @param[out]      xo                   - Pointer to the mpc_t structure where the x-coordinate of the doubled point will be stored.
 * @param[out]      yo                   - Pointer to the mpc_t structure where the y-coordinate of the doubled point will be stored.
 * @param[out]      zo                   - Pointer to the mpc_t structure where the z-coordinate of the doubled point will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int PD_fp2(mpc_t x, mpc_t y, mpc_t z, mpc_t xo, mpc_t yo, mpc_t zo)
{
    mpc_t x1, y1, z1, A, B, C;

    mpc_fp2_copy(x1, x);
    mpc_fp2_copy(y1, y);
    mpc_fp2_copy(z1, z);

    (void)mpc_fp2_mul(B, z1, y1);
    (void)mpc_fp2_add(z1, B, B); // z1 = 2*y1*z1

    (void)mpc_fp2_square_v1(A, x1);
    (void)mpc_fp2_mul_4(x1, x1); // x1 = 4*x1
    (void)mpc_fp2_mul_3(A, A);   // A  = 3*(x1^2)

    (void)mpc_fp2_square_v1(B, y1); // B  = y1^2
    (void)mpc_fp2_mul(y1, B, x1);   // y1 = 4*x1*y1^2
    (void)mpc_fp2_square_v1(C, A);  // C  = 9*(x1^4)

#if 1
    (void)mpc_fp2_add(x1, y1, y1);
    (void)mpc_fp2_sub(x1, C, x1); // x1 = 9*(x1^4) - 8*x1*y1^2
#else
    (void)pke_sm9_simple_multiple_modsub_v2(C->im, y1->im, x1->im);
    (void)pke_sm9_simple_multiple_modsub_v2(C->re, y1->re, x1->re);
#endif
    (void)mpc_fp2_square_v1(C, B); // C  = y1^4
    (void)mpc_fp2_mul_8(C, C);     // C  = 8*y1^4
    (void)mpc_fp2_sub(y1, y1, x1); // y1 = 4*x1*y1^2 - x1
    (void)mpc_fp2_mul(A, A, y1);   // A  = 3*(x1^2)(4*x1*y1^2 - x1)
    (void)mpc_fp2_sub(y1, A, C);   // y1 = 3*(x1^2)(4*x1*y1^2 - x1) - 8*y1^4

    mpc_fp2_copy(xo, x1);
    mpc_fp2_copy(yo, y1);
    mpc_fp2_copy(zo, z1);

    return PKE_SUCCESS;
}

/**
 * @brief           E(Fp^2) point addition
 *                  x:=(y2*z^3-y)^2-(x2*z^2-x)^2*(x+x2*z^2);
 *                  y:=(y2*z^3-y)*(x*(x2*z^2-x)^2-X)-y*(x2*z^2-x)^3;
 *                  z:=(x2*z^2-x)*z;
 * @param[in,out]   x                    - Pointer to the mpc_t structure containing the x-coordinate of the first point, which will be overwritten with the x-coordinate of the result.
 * @param[in,out]   y                    - Pointer to the mpc_t structure containing the y-coordinate of the first point, which will be overwritten with the y-coordinate of the result.
 * @param[in,out]   z                    - Pointer to the mpc_t structure containing the z-coordinate of the first point, which will be overwritten with the z-coordinate of the result.
 * @param[in]       x2                   - Pointer to the mpc_t structure containing the x-coordinate of the second point.
 * @param[in]       y2                   - Pointer to the mpc_t structure containing the y-coordinate of the second point.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int PA_fp2(mpc_t x, mpc_t y, mpc_t z, const mpc_t x2, const mpc_t y2)
{
    mpc_t A, B, C, D, E;
    // unsigned int ret;

    (void)mpc_fp2_square_v1(A, z);
    (void)mpc_fp2_mul(B, x2, A);   // B = x2*(z^2)
    (void)mpc_fp2_mul(A, A, z);    // A = z^3
    (void)mpc_fp2_sub(D, B, x);    // D = x2*z^2-x
    (void)mpc_fp2_mul(z, D, z);    // z = (x2*z^2-x)*z ------
    (void)mpc_fp2_add(E, B, x);    // E = x2*z^2+x
    (void)mpc_fp2_mul(B, A, y2);   // B = y2*z^3
    (void)mpc_fp2_square_v1(C, D); // C = (x2*z^2-x)^2
    (void)mpc_fp2_mul(A, C, E);    // A = (x2*z^2-x)^2*(x2*z^2+x)
    (void)mpc_fp2_mul(E, x, C);    // E = x*(x2*z^2-x)^2
    (void)mpc_fp2_mul(C, C, D);
    (void)mpc_fp2_mul(C, C, y);    // C = (x2*z^2-x)^3*y
    (void)mpc_fp2_sub(D, B, y);    // D = y2*z^3-y
    (void)mpc_fp2_square_v1(B, D); // B = (y2*z^3-y)^2
    (void)mpc_fp2_sub(x, B,
                      A);       // x = (y2*z^3-y)^2 - (x2*z^2-x)^2*(x2*z^2+x) ------
    (void)mpc_fp2_sub(E, E, x); // E = (x*(x2*z^2-x)^2-X)
    (void)mpc_fp2_mul(E, E, D); // E = (y2*z^3-y)(x*(x2*z^2-x)^2-X)
    (void)mpc_fp2_sub(y, E,
                      C); // y = (y2*z^3-y)(x*(x2*z^2-x)^2-X) - (x2*z^2-x)^3*y

    return PKE_SUCCESS;
}

/**
 * @brief           dst = -x
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the resulting negative will be stored.
 * @param[in]       x                    - Pointer to the mpc6_t structure containing the element to be negated.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp6_negative(mpc6_ptr dst, mpc6_ptr x)
{
    (void)mpc_fp2_negative(dst->f, x->f);
    (void)mpc_fp2_negative(dst->ft, x->ft);
    (void)mpc_fp2_negative(dst->ft2, x->ft2);

    return PKE_SUCCESS;
}

/**
 * @brief           g = f^(-1)
 * @param[out]      g                    - Pointer to the mpc6_t structure where the resulting inverse will be stored.
 * @param[in]       f                    - Pointer to the mpc6_t structure containing the element to be inverted.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp6_inv(mpc6_t g, mpc6_t f)
{
    mpc_t a0, a1, a2, A, B, C, D, E, F;
    unsigned int ret;

    mpc_fp2_copy(a0, f->f);
    mpc_fp2_copy(a1, f->ft);
    mpc_fp2_copy(a2, f->ft2);

    (void)mpc_fp2_square_v1(A, a0);
    (void)mpc_fp2_mul(B, a1, a2); // B = a1*a2
    (void)mpc_fp2_mul_u(B, B);    // B = a1*a2*u
    (void)mpc_fp2_sub(A, A, B);   // A = a0*a0 - a1*a2*u
    (void)mpc_fp2_square_v1(B, a2);
    (void)mpc_fp2_mul_u(B, B); // B = a2*a2*u
    (void)mpc_fp2_mul(C, a0, a1);
    (void)mpc_fp2_sub(B, B, C); // B = a2*a2*u - a0*a1
    (void)mpc_fp2_square_v1(C, a1);
    (void)mpc_fp2_mul(D, a0, a2);
    (void)mpc_fp2_sub(C, C, D); // C = a1*a1 - a0*a2
    (void)mpc_fp2_mul_u(F, a1);
    (void)mpc_fp2_mul(F, F, C); // F = a1*u(a1*a1 - a0*a2)
    (void)mpc_fp2_mul(E, a0, A);
    (void)mpc_fp2_add(F, F, E); // F = a1*u(a1*a1 - a0*a2) + a0*(a0*a0 - a1*a2*u)
                                // = (a1^3)*u+(a0^3)-2*a0*a1*a2*u
    (void)mpc_fp2_mul_u(E, a2);
    (void)mpc_fp2_mul(E, E,
                      B);       // E = (a2*a2*u - a0*a1)*a2*u = (a2^3)*u*u - a0*a1*a2*u
    (void)mpc_fp2_add(F, F, E); // F = (a2^3)*u*u+(a1^3)*u+(a0^3)-3*a0*a1*a2*u

    ret = mpc_fp2_inv(F, F);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)mpc_fp2_mul(A, A, F);
    (void)mpc_fp2_mul(B, B, F);
    (void)mpc_fp2_mul(C, C, F);

    mpc_fp2_copy(g->f, A);
    mpc_fp2_copy(g->ft, B);
    mpc_fp2_copy(g->ft2, C);

    return PKE_SUCCESS;
}

/**
 * @brief           Adds two elements in Fp^6.
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the resulting sum will be stored.
 * @param[in]       a                    - Pointer to the first mpc6_t structure containing one of the elements to be added.
 * @param[in]       b                    - Pointer to the second mpc6_t structure containing the other element to be added.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp6_add(mpc6_t dst, mpc6_t a, mpc6_t b)
{
    mpc_fp2_add(dst->f, a->f, b->f);
    mpc_fp2_add(dst->ft, a->ft, b->ft);
    mpc_fp2_add(dst->ft2, a->ft2, b->ft2);

    return PKE_SUCCESS;
}

/**
 * @brief           Subtracts two elements in Fp^6.
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the resulting difference will be stored.
 * @param[in]       a                    - Pointer to the first mpc6_t structure containing the element from which to subtract.
 * @param[in]       b                    - Pointer to the second mpc6_t structure containing the element to be subtracted.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp6_sub(mpc6_t dst, mpc6_t a, mpc6_t b)
{
    mpc_fp2_sub(dst->f, a->f, b->f);
    mpc_fp2_sub(dst->ft, a->ft, b->ft);
    mpc_fp2_sub(dst->ft2, a->ft2, b->ft2);

    return PKE_SUCCESS;
}

/**
 * @brief           Copies an element in Fp^6.
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the copied elements will be stored.
 * @param[in]       src                  - Pointer to the mpc6_t structure containing the element to be copied.
 */
static void mpc_fp6_copy(mpc6_t dst, mpc6_t src)
{
    mpc_fp2_copy(dst->f, src->f);
    mpc_fp2_copy(dst->ft, src->ft);
    mpc_fp2_copy(dst->ft2, src->ft2);
}

/**
 * @brief           Clears an element in Fp^6.
 * @param[in]       x                    - Pointer to the mpc6_t structure containing the element to be cleared.
 */
static void mpc_fp6_clears(mpc6_ptr x)
{
    mpc_fp2_clears(x->f);
    mpc_fp2_clears(x->ft);
    mpc_fp2_clears(x->ft2);
}

/**
 * @brief           Multiplies two elements in Fp^6.
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the resulting product will be stored.
 * @param[in]       a                    - Pointer to the first mpc6_t structure containing one of the elements to be multiplied.
 * @param[in]       b                    - Pointer to the second mpc6_t structure containing the other element to be multiplied.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp6_mul(mpc6_t dst, mpc6_t a, mpc6_t b)
{
    mpc_t a0, a1, a2, b0, b1, b2, A, B, C, D, E, F, G;

    mpc_fp2_copy(a0, a->f);
    mpc_fp2_copy(a1, a->ft);
    mpc_fp2_copy(a2, a->ft2);
    mpc_fp2_copy(b0, b->f);
    mpc_fp2_copy(b1, b->ft);
    mpc_fp2_copy(b2, b->ft2);

    (void)mpc_fp2_mul(A, a0, b0); // A = a0*b0 ---
    (void)mpc_fp2_mul(B, a1, b1); // B = a1*b1 ---
    (void)mpc_fp2_add(C, a0, a1); // C = a0+a1
    (void)mpc_fp2_add(D, b0, b1); // D = b0+b1
    (void)mpc_fp2_mul(E, C, D);   // E = (a0+a1)(b0+b1)
#if 0
    (void)mpc_fp2_sub(E,E,A);
    (void)mpc_fp2_sub(E,E,B);
#else
    (void)mpc_fp2_sub_sub(E, E, A, B); // E = a0*b1+a1*b0 ---
#endif
    (void)mpc_fp2_add(C, a0, a2); // C = a0+a2
    (void)mpc_fp2_add(D, b0, b2); // D = b0+b2
    (void)mpc_fp2_mul(F, a2, b2); // F = a2*b2 ---
    (void)mpc_fp2_mul(C, C, D);   // C = (a0+a2)(b0+b2)
#if 0
    (void)mpc_fp2_sub(C,C,A);
    (void)mpc_fp2_sub(C,C,F);
#else
    (void)mpc_fp2_sub_sub(C, C, A, F); // C = a0*b2+a2*b0 ---
#endif
    (void)mpc_fp2_add(D, B, C);   // D = a0*b2+a2*b0+a1*b1         dst->ft2
    (void)mpc_fp2_add(C, a1, a2); // C = a1+a2
    (void)mpc_fp2_add(G, b1, b2); // G = b1+b2
    (void)mpc_fp2_mul(C, C, G);   // C = (a1+a2)(b1+b2)
#if 0
    (void)mpc_fp2_sub(C,C,B);
    (void)mpc_fp2_sub(C,C,F);
#else
    (void)mpc_fp2_sub_sub(C, C, B, F); // C = a1*b2+a2*b1
#endif

    (void)mpc_fp2_mul_u(C, C);
    (void)mpc_fp2_add(C, C, A); // C = (a1*b2+a2*b1)*u+a0*b0     dst->f

    (void)mpc_fp2_mul_u(F, F);
    (void)mpc_fp2_add(F, F, E); // F = a2*b2*u+a0*b1+a1*b0       dst->ft

    mpc_fp2_copy(dst->f, C);
    mpc_fp2_copy(dst->ft, F);
    mpc_fp2_copy(dst->ft2, D);

    return PKE_SUCCESS;
}

/**
 * @brief           fp6 dst = a*a
 * @param[out]      dst                  - Pointer to the mpc6_t structure where the resulting square will be stored.
 * @param[in]       a                    - Pointer to the mpc6_t structure containing the element to be squared.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp6_square(mpc6_t dst, mpc6_t a)
{
    mpc_t a0, a1, a2, A, B, C;

    mpc_fp2_copy(a0, a->f);
    mpc_fp2_copy(a1, a->ft);
    mpc_fp2_copy(a2, a->ft2);

    (void)mpc_fp2_square_v1(A, a0); // A  = a0*a0
    (void)mpc_fp2_mul(B, a0, a1);
    (void)mpc_fp2_add(B, B, B); // B  = 2*a0*a1
#if 0
    (void)mpc_fp2_sub(a0,a0,a1);
    (void)mpc_fp2_add(a0,a0,a2);       //a0 = a0-a1+a2
#else
    (void)pke_sm9_simple_multiple_modsub_modadd(a0->re, a1->re, a2->re, a0->re);
    (void)pke_sm9_simple_multiple_modsub_modadd(a0->im, a1->im, a2->im, a0->im);
#endif
    (void)mpc_fp2_square_v2(a0); // a0 = a0*a0 - 2*a0*(a1-a2) + (a1-a2)*(a1-a2)
    (void)mpc_fp2_mul(C, a1, a2);
    (void)mpc_fp2_add(C, C, C);      // C  = 2*a1*a2
    (void)mpc_fp2_square_v1(a1, a2); // a1 = a2*a2
#if 0
    (void)mpc_fp2_sub(a0,a0,a1);
    (void)mpc_fp2_add(a0,a0,C);        //a0 = a0-a1+2*a1*a2 = a0*a0 - 2*a0*(a1-a2) + a1*a1
#else
    (void)pke_sm9_simple_multiple_modsub_modadd(a0->re, a1->re, C->re, a0->re);
    (void)pke_sm9_simple_multiple_modsub_modadd(a0->im, a1->im, C->im, a0->im);
#endif
    (void)mpc_fp2_mul_u(C, C);          // C  = 2*a1*a2*u
    (void)mpc_fp2_mul_u(a1, a1);        // a1 = a2*a2*u
    (void)mpc_fp2_sub(a0, a0, A);       // a0 = - 2*a0*(a1-a2) + a1*a1
    (void)mpc_fp2_add(dst->f, A, C);    // A  = a0*a0 + 2*a1*a2*u
    (void)mpc_fp2_add(dst->ft2, a0, B); // a0 = a1*a1 + 2*a0*a2
    (void)mpc_fp2_add(dst->ft, B, a1);  // B  = a2*a2*u + 2*a0*a1

    //    mpc_fp2_copy(dst->f,A);
    //    mpc_fp2_copy(dst->ft,B);
    //    mpc_fp2_copy(dst->ft2,a0);

    return PKE_SUCCESS;
}

/**
 * @brief           fp12 dst = a*a
 * @param[out]      dst                  - Pointer to the mpc12_t structure where the resulting square will be stored.
 * @param[in]       a                    - Pointer to the mpc12_t structure containing the element to be squared.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp12_square(mpc12_t dst, mpc12_t a)
{
#if 1
    mpc6_t a0, a1, c;

    mpc_fp6_copy(a0, a->f);
    mpc_fp6_copy(a1, a->fW);

    (void)mpc_fp6_add(c, a0, a1);
    (void)mpc_fp6_square(c, c);   // c  = (a0+a1)^2
    (void)mpc_fp6_square(a0, a0); // a0 = a0*a0
    (void)mpc_fp6_square(a1, a1); // a1 = a1*a1

// c = 2*a0*a1
#if 0
    (void)mpc_fp2_sub(c->f,c->f,a0->f);
    (void)mpc_fp2_sub(c->f,c->f,a1->f);
    (void)mpc_fp2_sub(c->ft,c->ft,a0->ft);
    (void)mpc_fp2_sub(c->ft,c->ft,a1->ft);
    (void)mpc_fp2_sub(c->ft2,c->ft2,a0->ft2);
    (void)mpc_fp2_sub(c->ft2,c->ft2,a1->ft2);
#else
    (void)mpc_fp2_sub_sub(c->f, c->f, a0->f, a1->f);
    (void)mpc_fp2_sub_sub(c->ft, c->ft, a0->ft, a1->ft);
    (void)mpc_fp2_sub_sub(c->ft2, c->ft2, a0->ft2, a1->ft2);
#endif

    // a0 = a0*a0 + a1*a1*v
    (void)mpc_fp2_add(a0->ft2, a0->ft2, a1->ft);
    (void)mpc_fp2_add(a0->ft, a0->ft, a1->f);
    (void)mpc_fp2_mul_u(a1->ft2, a1->ft2);
    (void)mpc_fp2_add(a0->f, a0->f, a1->ft2);

    mpc_fp6_copy(dst->f, a0);
    mpc_fp6_copy(dst->fW, c);

#else
    mpc6_t a0, a1, c;
    mpc_t m;

    mpc_fp6_copy(a0, a->f);
    mpc_fp6_copy(a1, a->fW);

    (void)mpc_fp6_mul(c, a0, a1);
    (void)mpc_fp6_add(c, c, c); // c = 2*a0*a1

    (void)mpc_fp6_square(a0, a0); // a0 = a0*a0
    (void)mpc_fp6_square(a1, a1); // a1 = a1*a1

    // a1 = a1*a1*v
    (void)mpc_fp2_mul_u(m, a1->ft2);
    (void)mpc_fp2_copy(a1->ft2, a1->ft);
    mpc_fp2_copy(a1->ft, a1->f);
    mpc_fp2_copy(a1->f, m);

    (void)mpc_fp6_add(a0, a1, a0); // a0 = a0*a0 + a1*a1*v

    mpc_fp6_copy(dst->f, a0);
    mpc_fp6_copy(dst->fW, c);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Fq^12  dst = a * b
 * @param[out]      dst                  - Pointer to the mpc12_t structure where the resulting product will be stored.
 * @param[in]       a                    - Pointer to the first mpc12_t structure containing one of the elements to be multiplied.
 * @param[in]       b                    - Pointer to the second mpc12_t structure containing the other element to be multiplied.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int mpc_fp12_mul(mpc12_t dst, mpc12_t a, mpc12_t b)
{
#if 1
    mpc6_t a0, a1, b0, b1, C, D;
    mpc_t m;

    mpc_fp6_copy(a0, a->f);
    mpc_fp6_copy(a1, a->fW);

    mpc_fp6_copy(b0, b->f);
    mpc_fp6_copy(b1, b->fW);

    (void)mpc_fp6_mul(C, a0, b0); // C = a0*b0
    (void)mpc_fp6_mul(D, a1, b1); // D = a1*b1

    (void)mpc_fp6_add(a0, a0, a1);
    (void)mpc_fp6_add(a1, b0, b1);
    (void)mpc_fp6_mul(a0, a0, a1); // a0 = (a0+a1)(b0+b1)

#if 0
    (void)mpc_fp6_sub(a0,a0,D);
    (void)mpc_fp6_sub(a0,a0,C);   //a0 = a0*b1+a1*b0
#else
    (void)mpc_fp2_sub_sub(a0->f, a0->f, D->f, C->f);
    (void)mpc_fp2_sub_sub(a0->ft, a0->ft, D->ft, C->ft);
    (void)mpc_fp2_sub_sub(a0->ft2, a0->ft2, D->ft2, C->ft2);
#endif

    // D = a1*b1*v+a0*b0
    (void)mpc_fp2_mul_u(m, D->ft2);
    mpc_fp2_copy(D->ft2, D->ft);
    mpc_fp2_copy(D->ft, D->f);
    mpc_fp2_copy(D->f, m);
    (void)mpc_fp6_add(D, D, C);

    mpc_fp6_copy(dst->f, D);
    mpc_fp6_copy(dst->fW, a0);
#else
    mpc6_t C1, C2, A1, A2;
    mpc_t m;

    (void)mpc_fp6_mul(A1, a->f, b->f);   // A1 = a0*b0
    (void)mpc_fp6_mul(A2, a->fW, b->fW); // A2 = a1*b1

    (void)mpc_fp6_add(C1, a->f, a->fW);
    (void)mpc_fp6_add(C2, b->f, b->fW);
    (void)mpc_fp6_mul(C1, C1, C2); // C1 = (a0+a1)(b0+b1)

#if 0
    (void)mpc_fp6_sub(C1,C1,A1);
    (void)mpc_fp6_sub(C1,C1,A2);         //a0 = a0*b1+a1*b0
#else
    (void)mpc_fp2_sub_sub(C1->f, C1->f, A1->f, A2->f);
    (void)mpc_fp2_sub_sub(C1->ft, C1->ft, A1->ft, A2->ft);
    (void)mpc_fp2_sub_sub(C1->ft2, C1->ft2, A1->ft2, A2->ft2);
#endif

    // A2 = a1*b1*v+a0*b0
    (void)mpc_fp2_mul_u(m, A2->ft2);
    mpc_fp2_copy(A2->ft2, A2->ft);
    mpc_fp2_copy(A2->ft, A2->f);
    mpc_fp2_copy(A2->f, m);
    (void)mpc_fp6_add(A2, A2, A1);

    mpc_fp6_copy(dst->f, A2);
    mpc_fp6_copy(dst->fW, C1);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Copies an element in Fp^12.
 * @param[out]      dst                  - Pointer to the mpc12_t structure where the copied elements will be stored.
 * @param[in]       src                  - Pointer to the mpc12_t structure containing the element to be copied.
 */
static void mpc_fp12_copy(mpc12_t dst, const mpc12_t src)
{
    mpc_fp2_copy(dst->f->f, src->f->f);
    mpc_fp2_copy(dst->f->ft, src->f->ft);
    mpc_fp2_copy(dst->f->ft2, src->f->ft2);
    mpc_fp2_copy(dst->fW->f, src->fW->f);
    mpc_fp2_copy(dst->fW->ft, src->fW->ft);
    mpc_fp2_copy(dst->fW->ft2, src->fW->ft2);
}

/**
 * @brief           Clears an element in Fp^12.
 * @param[in]       x                    - Pointer to the mpc12_t structure containing the element to be cleared.
 */
static void mpc_fp12_clears(mpc12_ptr x)
{
    mpc_fp2_clears(x->f->f);
    mpc_fp2_clears(x->f->ft);
    mpc_fp2_clears(x->f->ft2);
    mpc_fp2_clears(x->fW->f);
    mpc_fp2_clears(x->fW->ft);
    mpc_fp2_clears(x->fW->ft2);
}

/**
 * @brief           c = a^b
 *                  please make sure a != c, and b can not be zero.
 * @param[in]       a                    - Pointer to the mpc12_t structure containing the base element
 * @param[in]       b                    - Array representing the exponent, where each element is a word of the exponent.
 * @param[in]       b_wlen               - Length of the array b representing the number of words in the exponent.
 * @param[out]      c                    - Pointer to the mpc12_t structure where the resulting power will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp12_exp(mpc12_t a, const unsigned int b[], unsigned int b_wlen, mpc12_t c)
{
    unsigned int i;

    i = get_valid_bits(b, b_wlen);
    if (0U == i)
    {
        mpc_fp12_clears(c);
        c->f->f->re[0] = 1U;
        return PKE_SUCCESS;
    }
    else
    {
        ;
    }

    mpc_fp12_copy(c, a);
    i--;
    while (0u != (i--))
    {
        (void)mpc_fp12_square(c, c);
        if (1u == get_bit_value_by_index(b, i))
        {
            (void)mpc_fp12_mul(c, c, a);
        }
        else
        {
            ;
        }
    }

    return PKE_SUCCESS;
}

/**
 * @brief           c = g^b, here b is 256 bits
 *                  please make sure g != c, and b can not be zero.
 * @param[in]       g                    - Pointer to the mpc12_t structure containing the base generator element
 * @param[in]       b                    - Array representing the 256-bit exponent, where each element is a word of the exponent.
 * @param[out]      c                    - Pointer to the mpc12_t structure where the resulting power will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int mpc_fp12_g_exp(mpc12_t g, unsigned int b[], mpc12_t c)
{
#if (SM9_FP12_EXP_COMB_PARTS == 1U)
    return mpc_fp12_exp(g, b, 8u, c);
#elif (SM9_FP12_EXP_COMB_PARTS == 2U)
    unsigned int i, m;

    i = 128u;
    while (i--)
    {
        m = (get_bit_value_by_index(b + 4u, i)) << 1;
        m |= get_bit_value_by_index(b, i);
        if (0u != m)
        {
            break;
        }
        else
        {
            ;
        }
    }

    if (0U == m)
    {
        mpc_fp12_clears(c);
        c->f->f->re[0] = 1U;
        return PKE_SUCCESS;
    }
    else
    {
        ;
    }

    mpc_fp12_copy(c, g + m - 1u);
    while (i--)
    {
        (void)mpc_fp12_square(c, c);

        m = (get_bit_value_by_index(b + 4, i)) << 1;
        m |= get_bit_value_by_index(b, i);

        if (0u != m)
        {
            (void)mpc_fp12_mul(c, c, g + m - 1);
        }
        else
        {
            ;
        }
    }

    return PKE_SUCCESS;
#elif (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int i, m;

    b[8] = 0u; // to clear the high 32 bit, for b is just 8 words.

    i = 86u;
    while (i--)
    {
        m = (get_bit_value_by_index(b, 2u * 86u + i)) << 2;
        m |= get_bit_value_by_index(b, 86u + i) << 1;
        m |= get_bit_value_by_index(b, i);
        if (0u != m)
        {
            break;
        }
        else
        {
            ;
        }
    }

    if (0U == m)
    {
        mpc_fp12_clears(c);
        c->f->f->re[0] = 1U;
        return PKE_SUCCESS;
    }
    else
    {
        ;
    }

    mpc_fp12_copy(c, g + m - 1u);
    while (i--)
    {
        (void)mpc_fp12_square(c, c);

        m = (get_bit_value_by_index(b, 2u * 86u + i)) << 2;
        m |= get_bit_value_by_index(b, 86u + i) << 1;
        m |= get_bit_value_by_index(b, i);

        if (0U != m)
        {
            (void)mpc_fp12_mul(c, c, g + m - 1u);
        }
        else
        {
            ;
        }
    }

    return PKE_SUCCESS;
#endif
}

/**
 * @brief           gT,T(P)
 *                  only ret is output, others are inputs(Gz is ([2]T)z).
 * @param[in]       Tx                   - Pointer to the mpc_t structure containing the x-coordinate of point T.
 * @param[in]       Ty                   - Pointer to the mpc_t structure containing the y-coordinate of point T.
 * @param[in]       Tz                   - Pointer to the mpc_t structure containing the z-coordinate of point T.
 * @param[in]       x1                   - Pointer to the mpc_t structure containing the x-coordinate of another point.
 * @param[in]       y1                   - Pointer to the mpc_t structure containing the y-coordinate of another point.
 * @param[in]       Gz                   - Pointer to the mpc_t structure containing the element ([2]T)z.
 * @param[out]      ret                  - Pointer to the mpc12_t structure where the resulting value will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int line1(const mpc_t Tx, const mpc_t Ty, const mpc_t Tz, const mpc_t x1, const mpc_t y1, const mpc_t Gz, mpc12_t ret)
{
    mpc_t A, B, D, E, tmp;
    // unsigned int ret1;

    (void)mpc_fp2_square_v1(A, Tz);
    (void)mpc_fp2_mul(B, Gz, A);

    (void)mpc_fp2_mul(A, A, x1);    // A = x1*((Tz)^2)
    (void)mpc_fp2_mul(B, B, y1);    // B = y1*((Tz)^2)*Gz
    (void)mpc_fp2_square_v1(D, Ty); // D = (Ty)^2

    // E = 3(Tx^2)
    (void)mpc_fp2_square_v1(E, Tx);
    (void)mpc_fp2_add(tmp, E, E);
    (void)mpc_fp2_add(E, E, tmp);
    ////(void)mpc_fp2_mul_3(E, E);

    // A = 3(Tx^2)*x1*((Tz)^2)
    (void)mpc_fp2_mul(A, E, A);

    // D = 3(Tx^3) - 2(Ty^2)
    (void)mpc_fp2_mul(E, Tx, E);
    (void)mpc_fp2_add(D, D, D);
    (void)mpc_fp2_sub(D, E, D);
    //(void)pke_sm9_simple_multiple_modsub_modadd(D->re, E->re, D->re, D->re);
    //(void)pke_sm9_simple_multiple_modsub_modadd(D->im, E->im, D->im, D->im);
    ////(void)mpc_fp2_sub_add(D, D, E, D);

    (void)pke_sm9_prime_p_sub(A->im, A->im);
    (void)pke_sm9_prime_p_sub(A->re, A->re);

    mpc_fp2_copy(ret->f->ft2, B);
    mpc_fp2_copy(ret->fW->f, D);
    mpc_fp2_copy(ret->fW->ft, A);

    return PKE_SUCCESS;
}

/**
 * @brief           gT,Q(P)
 *                  only ret is output, others are inputs(Gz is (T+Q)z).
 * @param[in]       Tx                   - Pointer to the mpc_t structure containing the x-coordinate of point T.
 * @param[in]       Ty                   - Pointer to the mpc_t structure containing the y-coordinate of point T.
 * @param[in]       Tz                   - Pointer to the mpc_t structure containing the z-coordinate of point T.
 * @param[in]       Q1x                  - Pointer to the mpc_t structure containing the x-coordinate of point Q.
 * @param[in]       Q1y                  - Pointer to the mpc_t structure containing the y-coordinate of point Q.
 * @param[in]       x1                   - Pointer to the mpc_t structure containing the x-coordinate of another point.
 * @param[in]       y1                   - Pointer to the mpc_t structure containing the y-coordinate of another point.
 * @param[in]       Gz                   - Pointer to the mpc_t structure containing the element ((T+Q)z).
 * @param[out]      ret                  - Pointer to the mpc12_t structure where the resulting value will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int line2(const mpc_t Tx, const mpc_t Ty, const mpc_t Tz, const mpc_t Q1x, const mpc_t Q1y, const mpc_t x1, const mpc_t y1, const mpc_t Gz, mpc12_t ret)
{
    (void)Tx;
    mpc_t A, B, C, D, G;

    (void)mpc_fp2_mul(A, Gz, y1);

    // C = Q1y*Tz^3
    (void)mpc_fp2_square_v1(C, Tz);
    (void)mpc_fp2_mul(C, C, Tz);
    (void)mpc_fp2_mul(C, C, Q1y);

    (void)mpc_fp2_sub(D, Ty, C);  // D = Ty-Q1y*Tz^3
    (void)mpc_fp2_mul(G, D, Q1x); // G = (Ty-Q1y*Tz^3)*Q1x
    (void)mpc_fp2_mul(C, D, x1);  // C = (Ty-Q1y*Tz^3)*x1
    (void)mpc_fp2_mul(B, Q1y, Gz);
    (void)mpc_fp2_add(B, G, B); // B = (Ty-Q1y*Tz^3)*Q1x + Q1y*Gz

    (void)pke_sm9_prime_p_sub(B->im, B->im);
    (void)pke_sm9_prime_p_sub(B->re, B->re);

    mpc_fp2_copy(ret->f->ft2, A); // new combine 2015/11/6,fit M type
    mpc_fp2_copy(ret->fW->f, B);
    mpc_fp2_copy(ret->fW->ft, C);

    return PKE_SUCCESS;
}

/**
 * @brief           g = f^p
 * @param[in]       f                    - Pointer to the mpc12_t structure containing the source element to be transformed.
 * @param[out]      g                    - Pointer to the mpc12_t structure where the resulting transformed element will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int to_p(mpc12_t f, mpc12_t g)
{
    // k1=(u)^((p-1)/6), k2=k1^2, k3=k1^3, k4=k1^4, k5=k1^5
    unsigned int k1_re[8] = {
        0x377b698bu,
        0xa91d8354u,
        0x0ddd04edu,
        0x47c5c86eu,
        0x9c086749u,
        0x843c6cfau,
        0xe5720bdbu,
        0x3f23ea58u,
    };
    unsigned int k2_re[8] = {
        0x7be65334u,
        0xd5fc1196u,
        0x4f8b78f4u,
        0x78027235u,
        0x02a3a6f2u,
        0xf3000000u,
        0x00000000u,
        0x00000000u,
    };
    unsigned int k3_re[8] = {
        0xda24d011u,
        0xf5b21fd3u,
        0x06dc5177u,
        0x9f9d4118u,
        0xee0baf15u,
        0xf55acc93u,
        0xdc0a3f2cu,
        0x6c648de5u,
    };
    unsigned int k4_re[8] = {
        0x7be65333u,
        0xd5fc1196u,
        0x4f8b78f4u,
        0x78027235u,
        0x02a3a6f2u,
        0xf3000000u,
        0x00000000u,
        0x00000000u,
    };
    unsigned int k5_re[8] = {
        0xa2a96686u,
        0x4c949c7fu,
        0xf8ff4c8au,
        0x57d778a9u,
        0x520347ccu,
        0x711e5f99u,
        0xf6983351u,
        0x2d40a38cu,
    };

    mpc_t a1, a2, a3, a4, a5, a6;

    mpc_fp2_copy(a1, f->f->f);
    mpc_fp2_copy(a2, f->f->ft);
    mpc_fp2_copy(a3, f->f->ft2);
    mpc_fp2_copy(a4, f->fW->f);
    mpc_fp2_copy(a5, f->fW->ft);
    mpc_fp2_copy(a6, f->fW->ft2);

    (void)pke_sm9_prime_p_sub(a1->im, a1->im);
    (void)pke_sm9_prime_p_sub(a2->im, a2->im);
    (void)pke_sm9_prime_p_sub(a3->im, a3->im);
    (void)pke_sm9_prime_p_sub(a4->im, a4->im);
    (void)pke_sm9_prime_p_sub(a5->im, a5->im);
    (void)pke_sm9_prime_p_sub(a6->im, a6->im);

    (void)mpc_fp2_mul_a(a2, a2, k2_re);
    (void)mpc_fp2_mul_a(a3, a3, k4_re);
    (void)mpc_fp2_mul_a(a4, a4, k1_re);
    (void)mpc_fp2_mul_a(a5, a5, k3_re);
    (void)mpc_fp2_mul_a(a6, a6, k5_re);

    mpc_fp2_copy(g->f->f, a1);
    mpc_fp2_copy(g->f->ft, a2);
    mpc_fp2_copy(g->f->ft2, a3);
    mpc_fp2_copy(g->fW->f, a4);
    mpc_fp2_copy(g->fW->ft, a5);
    mpc_fp2_copy(g->fW->ft2, a6);

    return PKE_SUCCESS;
}

/**
 * @brief           g = f^p^p
 * @param[in]       f                    - Pointer to the mpc12_t structure containing the source element to be transformed.
 * @param[out]      g                    - Pointer to the mpc12_t structure where the resulting transformed element will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int to_p2(mpc12_t f, mpc12_t g)
{
    // g1=(KECI)^p_2_6, g2=g1^2, g3=g1^3, g4=g1^4, g5=g1^5    p_2_6 = (p^2-1)/6
    unsigned int g1_re[8] = {
        0x7be65334u,
        0xd5fc1196u,
        0x4f8b78f4u,
        0x78027235u,
        0x02a3a6f2u,
        0xf3000000u,
        0x00000000u,
        0x00000000u,
    };
    unsigned int g2_re[8] = {
        0x7be65333u,
        0xd5fc1196u,
        0x4f8b78f4u,
        0x78027235u,
        0x02a3a6f2u,
        0xf3000000u,
        0x00000000u,
        0x00000000u,
    };
    unsigned int g3_re[8] = {
        0xe351457cu,
        0xe56f9b27u,
        0x1a7aeedbu,
        0x21f2934bu,
        0xf58ec745u,
        0xd603ab4fu,
        0x02a3a6f1u,
        0xb6400000u,
    };
    unsigned int g4_re[8] = {
        0x676af249u,
        0x0f738991u,
        0xcaef75e7u,
        0xa9f02115u,
        0xf2eb2052u,
        0xe303ab4fu,
        0x02a3a6f0u,
        0xb6400000u,
    };
    unsigned int g5_re[8] = {
        0x676af24au,
        0x0f738991u,
        0xcaef75e7u,
        0xa9f02115u,
        0xf2eb2052u,
        0xe303ab4fu,
        0x02a3a6f0u,
        0xb6400000u,
    };

    mpc_t a1, a2, a3, a4, a5, a6;

    mpc_fp2_copy(a1, f->f->f);
    mpc_fp2_copy(a2, f->f->ft);
    mpc_fp2_copy(a3, f->f->ft2);
    mpc_fp2_copy(a4, f->fW->f);
    mpc_fp2_copy(a5, f->fW->ft);
    mpc_fp2_copy(a6, f->fW->ft2);

    (void)mpc_fp2_mul_a(a2, a2, g2_re);
    (void)mpc_fp2_mul_a(a3, a3, g4_re);
    (void)mpc_fp2_mul_a(a4, a4, g1_re);
    (void)mpc_fp2_mul_a(a5, a5, g3_re);
    (void)mpc_fp2_mul_a(a6, a6, g5_re);

    mpc_fp2_copy(g->f->f, a1);
    mpc_fp2_copy(g->f->ft, a2);
    mpc_fp2_copy(g->f->ft2, a3);
    mpc_fp2_copy(g->fW->f, a4);
    mpc_fp2_copy(g->fW->ft, a5);
    mpc_fp2_copy(g->fW->ft2, a6);

    return PKE_SUCCESS;
}

/**
 * @brief           g = f^p^p^p
 * @param[in]       f                    - Pointer to the mpc12_t structure containing the source element to be transformed.
 * @param[out]      g                    - Pointer to the mpc12_t structure where the resulting transformed element will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int to_p3(mpc12_t f, mpc12_t g)
{
    // e1=(k1)^p2_p_1, e2=e1^2, e3=e1^3, e4=e1^4, e5=e1^5   //p2_p_1 = (p^2+p+1)/6
    unsigned int e1_re[8] = {
        0xda24d011u,
        0xf5b21fd3u,
        0x06dc5177u,
        0x9f9d4118u,
        0xee0baf15u,
        0xf55acc93u,
        0xdc0a3f2cu,
        0x6c648de5u,
    };
    unsigned int e2_re[8] = {
        0xe351457cu,
        0xe56f9b27u,
        0x1a7aeedbu,
        0x21f2934bu,
        0xf58ec745u,
        0xd603ab4fu,
        0x02a3a6f1u,
        0xb6400000u,
    };
    unsigned int e3_re[8] = {
        0x092c756cu,
        0xefbd7b54u,
        0x139e9d63u,
        0x82555233u,
        0x0783182fu,
        0xe0a8debcu,
        0x269967c4u,
        0x49db721au,
    };
    //    unsigned int e4_re[8] =
    //    {0x00000001u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,0x00000000u,};
    unsigned int e5_re[8] = {
        0xda24d011u,
        0xf5b21fd3u,
        0x06dc5177u,
        0x9f9d4118u,
        0xee0baf15u,
        0xf55acc93u,
        0xdc0a3f2cu,
        0x6c648de5u,
    };

    mpc_t a1, a2, a3, a4, a5, a6;

#if 0
    mpc_fp2_copy(a1,f->f->f);
    mpc_fp2_copy(a2,f->f->ft);
    mpc_fp2_copy(a3,f->f->ft2);
    mpc_fp2_copy(a4,f->fW->f);
    mpc_fp2_copy(a5,f->fW->ft);
    mpc_fp2_copy(a6,f->fW->ft2);

    (void)pke_sm9_prime_p_sub(a1->im, a1->im);
    (void)pke_sm9_prime_p_sub(a2->im, a2->im);
    (void)pke_sm9_prime_p_sub(a3->im, a3->im);
    (void)pke_sm9_prime_p_sub(a4->im, a4->im);
    (void)pke_sm9_prime_p_sub(a5->im, a5->im);
    (void)pke_sm9_prime_p_sub(a6->im, a6->im);
#else
    uint32_copy(a1->re, f->f->f->re, 8u);
    uint32_copy(a2->re, f->f->ft->re, 8u);
    uint32_copy(a3->re, f->f->ft2->re, 8u);
    uint32_copy(a4->re, f->fW->f->re, 8u);
    uint32_copy(a5->re, f->fW->ft->re, 8u);
    uint32_copy(a6->re, f->fW->ft2->re, 8u);

    (void)pke_sm9_prime_p_sub(f->f->f->im, a1->im);
    (void)pke_sm9_prime_p_sub(f->f->ft->im, a2->im);
    (void)pke_sm9_prime_p_sub(f->f->ft2->im, a3->im);
    (void)pke_sm9_prime_p_sub(f->fW->f->im, a4->im);
    (void)pke_sm9_prime_p_sub(f->fW->ft->im, a5->im);
    (void)pke_sm9_prime_p_sub(f->fW->ft2->im, a6->im);
#endif

    (void)mpc_fp2_mul_a(a2, a2, e2_re);
    //    (void)mpc_fp2_mul_a(a3,a3,e4_re);  //since e4_re is 1
    (void)mpc_fp2_mul_a(a4, a4, e1_re);
    (void)mpc_fp2_mul_a(a5, a5, e3_re);
    (void)mpc_fp2_mul_a(a6, a6, e5_re);

    mpc_fp2_copy(g->f->f, a1);
    mpc_fp2_copy(g->f->ft, a2);
    mpc_fp2_copy(g->f->ft2, a3);
    mpc_fp2_copy(g->fW->f, a4);
    mpc_fp2_copy(g->fW->ft, a5);
    mpc_fp2_copy(g->fW->ft2, a6);

    return PKE_SUCCESS;
}

/**
 * @brief           f = f^((p^12-1)/r)
 * @param[in, out]  f                    - Pointer to the mpc12_t structure containing the source element to be transformed.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int final_exp(mpc12_t f)
{
#if 0
    const unsigned int t_6_5[3]  = {0x0215D941u,0x40000000u,0x00000002u};
    const unsigned int t2_6_1[4] = {0x0CB27659u,0x0000B98Bu,0x019062EDu,0xD8000000u};
    unsigned int i, ret;

    mpc12_t A,S,D,E,G,a,b,f1,f2,f3,f4,ff1,bf,ab,tmp12;
    mpc6_t c6,zero6,tmp1,tmp2,invc;
    mpc_t m;

    mpc_fp6_clears((mpc6_ptr)&zero6);
    mpc_fp12_clears((mpc12_ptr)&D);
    mpc_fp12_clears((mpc12_ptr)&E);

    /********** (p^2+1) **********/
    (void)to_p2(f,A);
    (void)mpc_fp12_mul(A,A,f);

    /********** (p^6-1) **********/
    mpc_fp6_copy(S->f,A->f);
    (void)mpc_fp6_sub(S->fW,zero6,A->fW);    //S:=B[1] - B[2]*W;

    (void)mpc_fp6_mul(tmp1,A->f,A->f);
    (void)mpc_fp6_mul(tmp2,A->fW,A->fW);
    (void)mpc_fp6_sub(tmp2,zero6,tmp2);      //-B[2]^2

    (void)mpc_fp2_mul_u(m,tmp2->ft2);
    mpc_fp2_copy(tmp2->ft2,tmp2->ft);
    mpc_fp2_copy(tmp2->ft,tmp2->f);
    mpc_fp2_copy(tmp2->f,m);                 //-B[2]^2*t
    (void)mpc_fp6_add(c6,tmp1,tmp2);         //C=B[1]^2 - t*B[2]^2;
    ret = mpc_fp6_inv(invc,c6);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
    mpc_fp6_copy(D->f,invc);

    (void)mpc_fp12_mul(D,D,S);
    (void)mpc_fp12_mul(D,D,S);  //D is output of step 2

    mpc_fp6_copy(E->f,invc);
    (void)mpc_fp12_mul(E,E,A);
    (void)mpc_fp12_mul(E,E,A);  //E = D^(-1)

    /********** (p^4-p^2+1)/r **********/
    (void)mpc_fp12_exp(E, t_6_5, 3, a);

    (void)to_p(a,b);
    (void)mpc_fp12_mul(b,a,b);

    (void)to_p(D,f1);
    (void)to_p2(D,f2);
    (void)to_p3(D,f3);

    (void)mpc_fp12_mul(E,D,D);
    (void)mpc_fp12_mul(f4,E,E);     //f4  = f^4
    (void)mpc_fp12_mul(f4,f4,f3);   //f4  = f3*f^4

    (void)mpc_fp12_mul(ff1,f1,D);   //ff1 = f*fp

    (void)mpc_fp12_mul(f1,f1,f1);   //f1  = f1^2
    (void)mpc_fp12_mul(bf,b,f1);
    (void)mpc_fp12_mul(bf,f2,bf);   //bf  = f2*b*f1^2

    (void)mpc_fp12_mul(ab,a,b);

    //ff1=ff1^9, tmp12=ff1
    mpc_fp6_copy(tmp12->f,ff1->f);
    mpc_fp6_copy(tmp12->fW,ff1->fW);
    for(i=0u; i<3u; i++)
    {
        (void)mpc_fp12_mul(ff1,ff1,ff1);
    }
    (void)mpc_fp12_mul(ff1,tmp12,ff1);

    (void)mpc_fp12_mul(ff1,ab,ff1);
    (void)mpc_fp12_mul(f4,ff1,f4);

    (void)mpc_fp12_exp(bf, (unsigned int *)t2_6_1, 4u, tmp12);

    (void)mpc_fp12_mul(f,f4,tmp12);
#else
    const unsigned int t_6_5[3] = {0x0215D941u, 0x40000000u, 0x00000002u};
    const unsigned int t2_6_1[4] = {0x0CB27659u, 0x0000B98Bu, 0x019062EDu, 0xD8000000u};
    unsigned int ret;

    mpc12_t A, B, C, D, E, F1;
    mpc6_ptr tmp1, tmp2;
    mpc_ptr m;

    tmp1 = C->fW;
    tmp2 = C->f;
    m = D->f->f;

    /********** (p^2+1) **********/
    (void)to_p2(f, A);
    (void)mpc_fp12_mul(A, A, f);

    /********** (p^6-1) **********/
    mpc_fp6_copy(B->f, A->f);
    (void)mpc_fp6_negative(B->fW, A->fW); // B = A[0] - A[1]*W;

    (void)mpc_fp6_square(tmp1, A->f);  // tmp1 = A[0]^2
    (void)mpc_fp6_square(tmp2, A->fW); // tmp2 = A[1]^2

    // tmp2 = (A[1]^2)*v, since (c2,c1,c0)*v = (c1,c0,c2*u)
    (void)mpc_fp2_mul_u(m, tmp2->ft2);
    mpc_fp2_copy(tmp2->ft2, tmp2->ft);
    mpc_fp2_copy(tmp2->ft, tmp2->f);
    mpc_fp2_copy(tmp2->f, m);

    // C->fW = 0, C->f = 1/(A[0]^2 - (A[1]^2)*v)
    (void)mpc_fp6_sub(tmp1, tmp1, tmp2);
    ret = mpc_fp6_inv(tmp2, tmp1);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }
    mpc_fp6_clears(tmp1);

    // B = A^(-1), A is output of step 2
    (void)mpc_fp12_square(D, B);
    (void)mpc_fp12_square(B, A);
    (void)mpc_fp12_mul(B, B, C);
    (void)mpc_fp12_mul(A, C, D);

    /********** (p^4-p^2+1)/r **********/
    (void)mpc_fp12_exp(B, t_6_5, 3u, C); // C = (f)^(-6t-5)

    (void)to_p(C, E);            // E = (fp)^(-6t-5)
    (void)mpc_fp12_mul(D, C, E); // D = (fp*f)^(-6t-5)
    (void)mpc_fp12_mul(C, C, D); // C receives;

    (void)mpc_fp12_square(E, A);
    (void)mpc_fp12_square(E, E); // E = f^4
    (void)mpc_fp12_mul(C, C, E); // C receives;

    (void)to_p3(A, E);
    (void)mpc_fp12_mul(C, C, E); // C receives;

    (void)to_p(A, F1);

    (void)mpc_fp12_mul(B, F1, A); // B = fp*f
    (void)mpc_fp12_square(E, B);
    (void)mpc_fp12_square(E, E);
    (void)mpc_fp12_square(E, E); // B = (fp*f)^8
    (void)mpc_fp12_mul(B, E, B); // B = (fp*f)^9
    (void)mpc_fp12_mul(C, C, B); // C receives;

    (void)to_p2(A, E);
    (void)mpc_fp12_square(F1, F1);
    (void)mpc_fp12_mul(F1, F1, E);
    (void)mpc_fp12_mul(F1, F1, D);

    (void)mpc_fp12_exp(F1, t2_6_1, 4u, E);

    (void)mpc_fp12_mul(f, C, E);
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Fx = (~qx)*u^(-(p-1)/3), Fy = (~qy)*u^(-(p-1)/2)
 * @param[in]       qx                   - Pointer to the mpc_t structure containing the x-coordinate of the input element q.
 * @param[in]       qy                   - Pointer to the mpc_t structure containing the y-coordinate of the input element q.
 * @param[out]      Fx                   - Pointer to the mpc_t structure where the resulting x-coordinate after Frobenius Twist will be stored.
 * @param[out]      Fy                   - Pointer to the mpc_t structure where the resulting y-coordinate after Frobenius Twist will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int FrobeniusTwist(const mpc_t qx, const mpc_t qy, mpc_t Fx, mpc_t Fy)
{
    // scal1=(sqrt(-2))^(-(p-1)/3), scal2=(sqrt(-2))^(-(p-1)/2)
    unsigned int scal1_re[8] = {
        0x676af24au,
        0x0f738991u,
        0xcaef75e7u,
        0xa9f02115u,
        0xf2eb2052u,
        0xe303ab4fu,
        0x02a3a6f0u,
        0xb6400000u,
    };
    unsigned int scal2_re[8] = {
        0x092c756cu,
        0xefbd7b54u,
        0x139e9d63u,
        0x82555233u,
        0x0783182fu,
        0xe0a8debcu,
        0x269967c4u,
        0x49db721au,
    };

    (void)pke_sm9_prime_p_sub(qx->im, Fx->im);
    (void)pke_sm9_prime_p_sub(qy->im, Fy->im);
    uint32_copy((unsigned int *)Fx->re, qx->re, 8u);
    uint32_copy((unsigned int *)Fy->re, qy->re, 8u);

    (void)mpc_fp2_mul_a(Fx, Fx, scal1_re);
    (void)mpc_fp2_mul_a(Fy, Fy, scal2_re);

    return PKE_SUCCESS;
}

/**
 * @brief           get f before finalexp
 * @param[out]      f                    - Pointer to the mpc12_t structure where the resulting element f will be stored.
 * @param[in]       Q1x                  - Pointer to the mpc_t structure containing the x-coordinate of point Q1.
 * @param[in]       Q1y                  - Pointer to the mpc_t structure containing the y-coordinate of point Q1.
 * @param[in]       Q1z                  - Pointer to the mpc_t structure containing the z-coordinate of point Q1.
 * @param[in]       x1                   - Pointer to the mpc_t structure containing the x-coordinate of another point.
 * @param[in]       y1                   - Pointer to the mpc_t structure containing the y-coordinate of another point.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int f_gen(mpc12_t f, const mpc_t Q1x, const mpc_t Q1y, const mpc_t Q1z, const mpc_t x1, const mpc_t y1)
{
    const unsigned int t_6_2[3] = {0x0215D93Eu, 0x40000000u, 0x00000002u};
    unsigned int i, ret;

    mpc_t Tx, Ty, Tz, Gx, Gy, Gz, QFrobx, QFroby;
    mpc12_t tmp12;

    mpc_fp2_copy(Tx, Q1x);
    mpc_fp2_copy(Ty, Q1y);
    mpc_fp2_copy(Tz, Q1z);

    mpc_fp12_clears(f);
    f->f->f->re[0] = 0x00000001u;

    mpc_fp12_clears(tmp12);

    i = 66u - 1u;
    while (0u != (i--))
    {
        ret = PD_fp2(Tx, Ty, Tz, Gx, Gy, Gz); // G = [2]T
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        (void)mpc_fp12_square(f, f);

        (void)line1(Tx, Ty, Tz, x1, y1, Gz, tmp12);

        (void)mpc_fp12_mul(f, f, tmp12);

        mpc_fp2_copy(Tx, Gx);
        mpc_fp2_copy(Ty, Gy);
        mpc_fp2_copy(Tz, Gz);
        if (1u == get_bit_value_by_index(t_6_2, i))
        {
            ret = PA_fp2(Tx, Ty, Tz, Q1x, Q1y); // T = T + Q
            if (PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {
                ;
            }

            (void)line2(Gx, Gy, Gz, Q1x, Q1y, x1, y1, Tz, tmp12);
            (void)mpc_fp12_mul(f, f, tmp12);
        }
    }

    mpc_fp2_copy(Gx, Tx);
    mpc_fp2_copy(Gy, Ty);
    mpc_fp2_copy(Gz, Tz);
    (void)FrobeniusTwist(Q1x, Q1y, QFrobx, QFroby);
    ret = PA_fp2(Tx, Ty, Tz, QFrobx, QFroby);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)line2(Gx, Gy, Gz, QFrobx, QFroby, x1, y1, Tz, tmp12);
    (void)mpc_fp12_mul(f, f, tmp12);

    mpc_fp2_copy(Gx, Tx);
    mpc_fp2_copy(Gy, Ty);
    mpc_fp2_copy(Gz, Tz);
    (void)FrobeniusTwist(QFrobx, QFroby, QFrobx, QFroby);
    (void)pke_sm9_prime_p_sub(QFroby->re, QFroby->re);
    (void)pke_sm9_prime_p_sub(QFroby->im, QFroby->im);
    ret = PA_fp2(Tx, Ty, Tz, QFrobx, QFroby);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)line2(Gx, Gy, Gz, QFrobx, QFroby, x1, y1, Tz, tmp12);
    (void)mpc_fp12_mul(f, f, tmp12);

    return PKE_SUCCESS;
}

/**
 * @brief           (x1, y1) is in G1, (Q1x, Q1y, Q1z) is in G2, f is output
 * @param[out]      f                    - Pointer to the mpc12_t structure where the resulting element f will be stored.
 * @param[in]       Q1x                  - Pointer to the mpc_t structure containing the x-coordinate of point Q1 in G2.
 * @param[in]       Q1y                  - Pointer to the mpc_t structure containing the y-coordinate of point Q1 in G2.
 * @param[in]       Q1z                  - Pointer to the mpc_t structure containing the z-coordinate of point Q1 in G2.
 * @param[in]       x1                   - Pointer to the mpc_t structure containing the x-coordinate of point (x1, y1) in G1.
 * @param[in]       y1                   - Pointer to the mpc_t structure containing the y-coordinate of point (x1, y1) in G1.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int pairing_raw(mpc12_t f, const mpc_t Q1x, const mpc_t Q1y, const mpc_t Q1z, const mpc_t x1, const mpc_t y1)
{
    unsigned int ret;

    ret = f_gen(f, Q1x, Q1y, Q1z, x1, y1);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return final_exp(f);
}

/**
 * @brief           Q = [k]P2
 *                  please make sure k is not 0
 * @param[in]       k                    - Pointer to an array of unsigned integers representing the scalar k.
 * @param[in]       k_wlen               - Length of the array k, indicating the number of words (unsigned integers) used to represent the scalar.
 * @param[out]      Q                    - Pointer to the fp2_pt_t structure where the resulting point after multiplication will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int G2_pointMul_P2(unsigned int *k, unsigned int k_wlen, fp2_pt_t Q)
{
    (void)k_wlen;

#if 0
    unsigned int i, ret;

    i = get_valid_bits(k, 8u);

    mpc_fp2_copy(Q->x,fp2ptP2->x);
    mpc_fp2_copy(Q->y,fp2ptP2->y);
    mpc_fp2_copy(Q->z,fp2ptP2->z);

    --i;
    while(0u != (i--))
    {
        ret = PD_fp2(Q->x,Q->y,Q->z,Q->x,Q->y,Q->z);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        if(1u == get_bit_value_by_index(k, i))
        {
            ret = PA_fp2(Q->x,Q->y,Q->z,fp2ptP2->x,fp2ptP2->y);
            if(PKE_SUCCESS != ret)
            {
                return ret;
            }
            else
            {;}
        }
    }
#else
    mpc_t P2_128_0_x = {{
        {
            0x86A0CE83u,
            0x4097DCBEu,
            0x60E1905Au,
            0xB378A947u,
            0xADF23461u,
            0xB7575B5Eu,
            0x334B6E71u,
            0x88A171A9u,
        },
        {
            0x3E962271u,
            0xD57FB53Fu,
            0xB12DB59Eu,
            0xA74ECCA2u,
            0xBEB3576Bu,
            0xBDC7B660u,
            0x3A6718D2u,
            0x7F4C4508u,
        },
    }};
    mpc_t P2_128_0_y = {{
        {
            0xB9DF876Au,
            0x9CC9FA50u,
            0xEADCF3CBu,
            0x484CBEA3u,
            0x306037B3u,
            0x66A8C5B9u,
            0x9DCB6CC8u,
            0x6D608E8Cu,
        },
        {
            0xB5FD529Eu,
            0xCE057F21u,
            0x200BFB50u,
            0xF255C11Du,
            0xB223D50Du,
            0x438A7EC5u,
            0x8578A96Cu,
            0x05DA221Fu,
        },
    }};
    mpc_t P2_128_1_x = {{
        {
            0x3215316Fu,
            0xC9BED5B9u,
            0x788D026Eu,
            0xF3F032C0u,
            0x4B9940C5u,
            0x8822B60Cu,
            0x7B0047ABu,
            0x4CC71ABBu,
        },
        {
            0xC14B65ADu,
            0xA43D5932u,
            0xABB1AEA9u,
            0xF5205B30u,
            0x8584A5FBu,
            0xD377F24Eu,
            0x6ACEAD8Au,
            0xA383210Cu,
        },
    }};
    mpc_t P2_128_1_y = {{
        {
            0xA59303BFu,
            0xC6DE9F05u,
            0xF6A85E3Au,
            0x745EC2CCu,
            0x057BB605u,
            0x0065ED3Eu,
            0x063A10D2u,
            0x3BCA62E6u,
        },
        {
            0xB0E4C4A0u,
            0xB9C7CDD8u,
            0x96AF7A35u,
            0x0C18C5EFu,
            0xB931F333u,
            0xA32FBFB6u,
            0xC33E5F4Du,
            0x78060342u,
        },
    }};

    unsigned int i, m, ret;

    i = 128u;
    while (i--)
    {
        m = (get_bit_value_by_index(k + 4u, i)) << 1;
        m |= get_bit_value_by_index(k, i);
        if (0u != m)
        {
            break;
        }
        else
        {
            ;
        }
    }

    mpc_fp2_copy(Q->z, fp2ptP2->z);
    if (1u == m)
    {
        mpc_fp2_copy(Q->x, fp2ptP2->x);
        mpc_fp2_copy(Q->y, fp2ptP2->y);
    }
    else if (2u == m)
    {
        mpc_fp2_copy(Q->x, P2_128_0_x);
        mpc_fp2_copy(Q->y, P2_128_0_y);
    }
    else if (3u == m)
    {
        mpc_fp2_copy(Q->x, P2_128_1_x);
        mpc_fp2_copy(Q->y, P2_128_1_y);
    }
    else
    {
        ;
    }

    while (i--)
    {
        ret = PD_fp2(Q->x, Q->y, Q->z, Q->x, Q->y, Q->z);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        m = (get_bit_value_by_index(k + 4u, i)) << 1;
        m |= get_bit_value_by_index(k, i);

        if (1u == m)
        {
            ret = PA_fp2(Q->x, Q->y, Q->z, fp2ptP2->x, fp2ptP2->y);
        }
        else if (2u == m)
        {
            ret = PA_fp2(Q->x, Q->y, Q->z, P2_128_0_x, P2_128_0_y);
        }
        else if (3u == m)
        {
            ret = PA_fp2(Q->x, Q->y, Q->z, P2_128_1_x, P2_128_1_y);
        }
        else
        {
            ;
        }

        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }
    }
#endif

    return PKE_SUCCESS;
}

/**
 * @brief           Converts a G1 point buffer to a G2 point.
 * @param[in]       g1_point_buffer      - Pointer to a 64-byte array containing the G1 point.
 * @param[out]      g2_point             - Pointer to the fp2_pt_t structure where the resulting G2 point will be stored.
 */
void set_G1_point_buffer_2_G2_point(const unsigned char g1_point_buffer[64], fp2_pt_t g2_point)
{
    u8big_to_u32little_8(g2_point->x->re, (const unsigned char *)(g1_point_buffer));
    u8big_to_u32little_8(g2_point->y->re, (const unsigned char *)(g1_point_buffer + 32));
    uint32_clear(g2_point->x->im, 8u);
    uint32_clear(g2_point->y->im, 8u);
    mpc_fp2_clears(g2_point->z);
    g2_point->z->re[0] = 0x00000001u;
}

/**
 * @brief           Converts a G2 point buffer to a G2 point.
 * @param[in]       g1_point_buffer      - Pointer to a 128-byte array containing the G2 point.
 * @param[out]      g2_point             - Pointer to the fp2_pt_t structure where the resulting G2 point will be stored.
 */
void set_G2_point_buffer_2_G2_point(const unsigned char g1_point_buffer[128], fp2_pt_t g2_point)
{
    u8big_to_u32little_8(g2_point->x->im, (const unsigned char *)g1_point_buffer);
    u8big_to_u32little_8(g2_point->x->re, (const unsigned char *)(g1_point_buffer + 32u));
    u8big_to_u32little_8(g2_point->y->im, (const unsigned char *)(g1_point_buffer + 64u));
    u8big_to_u32little_8(g2_point->y->re, (const unsigned char *)(g1_point_buffer + 96u));
    mpc_fp2_clears(g2_point->z);
    g2_point->z->re[0] = 0x00000001u;
}

/**
 * @brief           g = e(P1, P2)
 * @param[in]       P1                   - Pointer to a 64-byte array containing the G1 point P1.
 * @param[in]       P2                   - Pointer to a 128-byte array containing the G2 point P2.
 * @param[out]      g                    - Pointer to a 384-byte array where the resulting pairing value will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_pairing_calc(const unsigned char P1[64], const unsigned char P2[128], unsigned char g[32 * 12])
{
    mpc12_t f;
    fp2_pt_t p1_, p2_;
    const FP2_POINT *p1, *p2;
    uint32_t ret;

    if (NULL == g)
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    if (NULL == P1)
    {
        p1 = fp2ptP1;
    }
    else
    {
        p1 = p1_;
        set_G1_point_buffer_2_G2_point(P1, p1_); // in out
    }

    if (NULL == P2)
    {
        p2 = fp2ptP2;
    }
    else
    {
        p2 = p2_;
        set_G2_point_buffer_2_G2_point(P2, p2_);
    }

    (void)pke_sm9_set_p_and_pre_mont();

    ret = pairing_raw(f, p2->x, p2->y, p2->z, p1->x, p1->y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    FE2OSP(f, g);

    return SM9_SUCCESS;
}

/**
 * @brief           out = g^r
 *                  r can not be zero.
 * @param[in]       g                    - Pointer to a 384-byte array containing the FP12 element g.
 * @param[in]       r                    - Pointer to a 32-byte array containing the scalar r.
 * @param[out]      out                  - Pointer to a 384-byte array where the resulting FP12 element will be stored.
 * @return          PKE_SUCCESS if the operation is successful, SM9_BUFFER_NULL if any input buffer is null, other values indicate an error.
 */
unsigned int sm9_fp12_exp(unsigned char g[32 * 12], unsigned char r[32], unsigned char out[32 * 12])
{
    mpc12_t a, c;
    unsigned int b[8];

    if ((NULL == g) || (NULL == r) || (NULL == out))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    OS2FEP(g, a);
    u8big_to_u32little_8(b, r);

    (void)mpc_fp12_exp(a, b, 8u, c);
    FE2OSP(c, out);

    return SM9_SUCCESS;
}

/**
 * @brief           Generates the sign master public key from the master private key.
 * @param[in]       ks                   - Pointer to a 32-byte array containing the master private key ks.
 * @param[out]      Ppub_s               - Pointer to a 128-byte array where the resulting master public key Ppub_s will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_sign_gen_mastPubKey_from_mastPriKey(const unsigned char ks[32], unsigned char Ppub_s[128])
{
    unsigned int tmp_ks[8];
    unsigned int ret;

    fp2_pt_t MastPubKey;

    if ((NULL == ks) || (NULL == Ppub_s))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_ks, ks);

    // make sure sysPriKey in [1, n-1]
    ret = uint32_integer_check(tmp_ks, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    ret = G2_pointMul_P2(tmp_ks, 8u, MastPubKey);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ret = coordinate_convert(MastPubKey->x, MastPubKey->y, MastPubKey->z, MastPubKey->x, MastPubKey->y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    u32little_to_u8big(Ppub_s, MastPubKey->x->im);
    u32little_to_u8big((unsigned char *)(Ppub_s + 32u), MastPubKey->x->re);
    u32little_to_u8big((unsigned char *)(Ppub_s + 64u), MastPubKey->y->im);
    u32little_to_u8big((unsigned char *)(Ppub_s + 96u), MastPubKey->y->re);

    return SM9_SUCCESS;
}

/**
 * @brief           Generates a sign master key pair.
 * @param[out]      ks                   - Pointer to a 32-byte array where the generated master private key ks will be stored.
 * @param[out]      Ppub_s               - Pointer to a 128-byte array where the generated master public key Ppub_s will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_sign_gen_mastKeyPair(unsigned char ks[32], unsigned char Ppub_s[128])
{
    unsigned int ret;

    if ((NULL == ks) || (NULL == Ppub_s))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    do
    {
        ret = get_rand(ks, 32u);
        if (TRNG_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // make sure priKey in [1, n-1]
        ret = sm9_sign_gen_mastPubKey_from_mastPriKey(ks, Ppub_s);
    } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));

    return ret;
}

/**
 * @brief           Generates a user sign private key.
 * @param[in]       IDA                  - Pointer to an array containing the user identifier IDA.
 * @param[in]       IDA_bytes            - Length of the user identifier IDA in bytes.
 * @param[in]       hid                  - Hash value used in the generation process.
 * @param[in]       ks                   - Pointer to a 32-byte array containing the master private key ks.
 * @param[out]      dsA                  - Pointer to a 64-byte array where the generated user private key dsA will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_sign_gen_userPriKey(const unsigned char *IDA, unsigned int IDA_bytes, unsigned char hid, const unsigned char ks[32], unsigned char dsA[64])
{
    unsigned int tmp_ks[8];
    unsigned int tmp[8];
    unsigned int ret;

    if ((NULL == IDA) || (NULL == ks) || (NULL == dsA))
    {
        return SM9_BUFFER_NULL;
    }
    else if (0u == IDA_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_ks, ks);

    // make sure sysPriKey in [1, n-1]
    ret = uint32_integer_check(tmp_ks, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    ret = sm9_h1_h2((unsigned char)1, IDA, IDA_bytes, (unsigned char *)(&hid), 1u, tmp);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)pke_modadd(sm9_curve->eccp_n, tmp, tmp_ks, tmp, 8u);

    if (0u != uint32_bignum_check_zero(tmp, 8u))
    {
        return SM9_ZERO_ALL;
    }
    else
    {
        ;
    }

    ret = pke_modinv(sm9_curve->eccp_n, tmp, tmp, 8u, 8u);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    (void)pke_modmul(sm9_curve->eccp_n, tmp, tmp_ks, tmp, 8u);

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_base(sm9_curve, tmp, tmp_ks, tmp);
#else
    ret = eccp_pointmul(sm9_curve, tmp, fp2ptP1->x->re, fp2ptP1->y->re, tmp_ks, tmp);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    u32little_to_u8big(dsA, tmp_ks);
    u32little_to_u8big((unsigned char *)(dsA + 32u), tmp);

    return SM9_SUCCESS;
}

/**
 * @brief           Generates a signature using a random number r.
 *                  r is 256-bit random number, only tmp and h are output, tmp is l
 * @param[in]       fp12g                - Pointer to an array containing the FP12 element g in byte form.
 * @param[in]       g                    - Pointer to the group element g of type __mpc12_struct.
 * @param[out]      r                    - Pointer to a 32-byte array containing the 256-bit random number r.
 * @param[in]       msg                  - Pointer to an array containing the message to be signed.
 * @param[in]       msg_len              - Length of the message in bytes.
 * @param[out]      tmp                  - Pointer to an array of 8 unsigned integers where the resulting signature component l (tmp) will be stored.
 * @param[out]      h                    - Pointer to a 32-byte array where the hash value h will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 */
static unsigned int sm9_sign_with_r(const unsigned char *fp12g, __mpc12_struct *g, unsigned int *r, const unsigned char *msg, unsigned int msg_len, unsigned int tmp[8],
                                    unsigned char h[32])
{
    unsigned char h2rf_para[32u * 12u];
    mpc12_t omega;
    unsigned int ret;

    // make sure r in [1, n-1]
    ret = uint32_integer_check(r, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    if (NULL != fp12g)
    {
        (void)mpc_fp12_g_exp(g, r, omega);
    }
    else
    {
        (void)mpc_fp12_exp(g, r, 8, omega);
    }

    FE2OSP(omega, h2rf_para);

    ret = sm9_h1_h2((unsigned char)2, msg, msg_len, h2rf_para, 32u * 12u, tmp);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    u32little_to_u8big(h, tmp);

    (void)pke_modsub(sm9_curve->eccp_n, r, tmp, tmp, 8u); // tmp = (r - h) mod n

    if (0u != uint32_bignum_check_zero(tmp, 8u))
    {
        return SM9_ZERO_ALL;
    }
    else
    {
        return SM9_SUCCESS;
    }
}

/**
 * @brief           Generates a signature for a given message.
 * @param[in]       msg                  - Pointer to an array containing the message to be signed.
 * @param[in]       msg_len              - Length of the message in bytes.
 * @param[in]       fp12g                - Pointer to an array containing the FP12 element g in byte form.
 * @param[in]       Ppub_s               - Pointer to a 128-byte array containing the master public key Ppub_s. Ensure that Ppub_s is valid.
 * @param[in]       dsA                  - Pointer to a 64-byte array containing the user private key dsA.
 * @param[in]       r                    - Pointer to a 32-byte array containing the 256-bit random number r.
 * @param[out]      h                    - Pointer to a 32-byte array where the hash value h of the message will be stored.
 * @param[out]      S                    - Pointer to a 65-byte array where the resulting signature S will be stored.
 * @return          PKE_SUCCESS if the operation is successful, other values indicate an error.
 * @note
 *        1.please make sure Ppub_s is valid
 */
unsigned int sm9_sign(const unsigned char *msg, unsigned int msg_len, const unsigned char *fp12g, const unsigned char Ppub_s[128], const unsigned char dsA[64],
                      const unsigned char r[32], unsigned char h[32], unsigned char S[65])
{
#if (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int tmp_r[9];
#else
    unsigned int tmp_r[8];
#endif
    unsigned int tmp[8];
    unsigned int px[8], Py[8];
    __mpc12_struct g[(1u << SM9_FP12_EXP_COMB_PARTS) - 1u];
    fp2_pt_t MastPubKey;
    unsigned int i, ret;

    if ((NULL == msg) || (NULL == dsA) || (NULL == h) || (NULL == S))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    if (NULL != fp12g)
    {
        ret = (1u << SM9_FP12_EXP_COMB_PARTS) - 1u;
        for (i = 0u; i < ret; i++)
        {
            OS2FEP(fp12g + 32u * 12u * i, g + i);
        }
    }
    else if (NULL != Ppub_s)
    {
        set_G2_point_buffer_2_G2_point(Ppub_s, MastPubKey);
        ret = pairing_raw(g, MastPubKey->x, MastPubKey->y, MastPubKey->z, fp2ptP1->x, fp2ptP1->y);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ret = SM9_BUFFER_NULL;
        goto END;
    }

    // check dsA
    u8big_to_u32little_8(px, (const unsigned char *)(dsA));
    u8big_to_u32little_8(Py, (const unsigned char *)(dsA + 32u));
    if (0u != sm9_pointVerify(px, Py))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    if (NULL == r)
    {
        do
        {
            ret = get_rand((unsigned char *)tmp_r, 32u);
            if (TRNG_SUCCESS != ret)
            {
                break;
            }
            else
            {
                ;
            }

            ret = sm9_sign_with_r(fp12g, g, tmp_r, msg, msg_len, tmp, h);
        } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));
    }
    else
    {
        u8big_to_u32little_8(tmp_r, r);

        ret = sm9_sign_with_r(fp12g, g, tmp_r, msg, msg_len, tmp, h);
    }

    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = eccp_pointmul(sm9_curve, tmp, px, Py, tmp, tmp_r);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    S[0] = POINT_UNCOMPRESSED;
    u32little_to_u8big((unsigned char *)(S + 1u), tmp);
    u32little_to_u8big((unsigned char *)(S + 33u), tmp_r);

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           SM9 verify the signature.
 * @param[in]       msg                  - Pointer to an array containing the message to be verified.
 * @param[in]       msg_len              - Length of the message in bytes.
 * @param[in]       IDA                  - Pointer to an array containing the user identifier IDA (user A is the signer).
 * @param[in]       IDA_bytes            - Length of the user identifier IDA in bytes.
 * @param[in]       hid                  - User private key generation function identity, published by KGC. Default value is 0x01 (one byte).
 * @param[in]       fp12g                - Pointer to an array containing the value of e(P1, Ppub_s) as a U8 big-endian. If set to NULL, it will be calculated within the function.
 * @param[in]       Ppub_s               - Pointer to a 128-byte array containing KGC's master public key Ppub_s.
 * @param[in]       h                    - Pointer to a 32-byte array containing the partial signature result h.
 * @param[in]       S                    - Pointer to a 65-byte array containing the partial signature result S.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 * @note
 *        1. IDA represents only the signer.
 */
unsigned int sm9_verify(const unsigned char *msg, unsigned int msg_len, const unsigned char *IDA, unsigned int IDA_bytes, unsigned char hid, const unsigned char *fp12g,
                        const unsigned char Ppub_s[128], const unsigned char h[32], const unsigned char S[65])
{
#if (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int SigH[9];
#else
    unsigned int SigH[8];
#endif
    unsigned int tmp[8];
    fp2_pt_t MastPubKey, SigS, TP2;
    mpc12_t omega;
    __mpc12_struct g[(1u << SM9_FP12_EXP_COMB_PARTS) - 1u];
    unsigned char *h2rf_para = (unsigned char *)g; // unsigned char h2rf_para[32*12];
    unsigned int i, ret;

    if ((NULL == msg) || (NULL == IDA) || (NULL == Ppub_s) || (NULL == h) || (NULL == S))
    {
        return SM9_BUFFER_NULL;
    }
    else if ((0u == IDA_bytes) || (POINT_UNCOMPRESSED != S[0]))
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    set_G2_point_buffer_2_G2_point(Ppub_s, MastPubKey);

    if (NULL != fp12g)
    {
        ret = (1u << SM9_FP12_EXP_COMB_PARTS) - 1u;
        for (i = 0u; i < ret; i++)
        {
            OS2FEP(fp12g + 32u * 12u * i, g + i);
        }
    }
    else
    {
        ret = pairing_raw(g, MastPubKey->x, MastPubKey->y, MastPubKey->z, fp2ptP1->x, fp2ptP1->y); // g = e(P1, Ppub_s)
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

    // check h in [1, n-1]
    u8big_to_u32little_8(SigH, h);
    ret = uint32_integer_check(SigH, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // check S part
    set_G1_point_buffer_2_G2_point((S + 1u), SigS);
    if (0u != sm9_pointVerify(SigS->x->re, SigS->y->re))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    // omega = g^h
    if (NULL != fp12g)
    {
        (void)mpc_fp12_g_exp(g, SigH, omega);
    }
    else
    {
        (void)mpc_fp12_exp(g, SigH, 8u, omega);
    }

    ret = sm9_h1_h2((unsigned char)1, IDA, IDA_bytes, (unsigned char *)(&hid), 1u, tmp);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    ret = G2_pointMul_P2(tmp, 8u, TP2); // TP2 = [h1]P2
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = PA_fp2(TP2->x, TP2->y, TP2->z, MastPubKey->x,
                 MastPubKey->y); // TP2 = [h1]P2 + Ppub_s
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = coordinate_convert(TP2->x, TP2->y, TP2->z, TP2->x, TP2->y);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    mpc_fp2_clears(TP2->z);
    TP2->z->re[0] = 0x00000001u;
    ret = pairing_raw(g, TP2->x, TP2->y, TP2->z, SigS->x, SigS->y); // g = e(S, TP2)
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    (void)mpc_fp12_mul(omega, omega, g);

    FE2OSP(omega, h2rf_para);

    ret = sm9_h1_h2((unsigned char)2, msg, msg_len, h2rf_para, 32u * 12u, tmp);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if (0u != uint32_cmp(tmp, SigH))
    {
        ret = SM9_VERIFY_FAILED;
    }
    else
    {
        ret = SM9_SUCCESS;
    }

END:

    return ret;
}

/**
 * @brief           Generate KGC's master public key from master private key for SM9 encryption system.
 * @param[in]       ke                   - Pointer to a 32-byte array containing the KGC's master private key, in big-endian format.
 * @param[out]      Ppub_e               - Pointer to a 64-byte array where the KGC's master public key (x||y) will be stored, in big-endian format.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_enc_gen_mastPubKey_from_mastPriKey(const unsigned char ke[32], unsigned char Ppub_e[64])
{
    unsigned int tmp_ke[8];
    unsigned int tmp1[8];
    unsigned int ret;

    if ((NULL == ke) || (NULL == Ppub_e))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_ke, ke);

    // make sure priKey in [1, n-1]
    ret = uint32_integer_check(tmp_ke, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_base(sm9_curve, tmp_ke, tmp_ke, tmp1);
#else
    ret = eccp_pointmul(sm9_curve, tmp_ke, fp2ptP1->x->re, fp2ptP1->y->re, tmp_ke, tmp1);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    u32little_to_u8big(Ppub_e, tmp_ke);
    u32little_to_u8big((unsigned char *)(Ppub_e + 32u), tmp1);

    return SM9_SUCCESS;
}

/**
 * @brief           Generate random KGC's master key pair for SM9 encryption system.
 * @param[out]      ke                   - Pointer to a 32-byte array where the KGC's master private key will be stored, in big-endian format.
 * @param[out]      Ppub_e               - Pointer to a 64-byte array where the KGC's master public key (x||y) will be stored, in big-endian format.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_enc_gen_mastKeyPair(unsigned char ke[32], unsigned char Ppub_e[64])
{
    unsigned int ret;

    if ((NULL == ke) || (NULL == Ppub_e))
    {
        return SM9_BUFFER_NULL;
    }
    else
    {
        ;
    }

    do
    {
        ret = get_rand(ke, 32u);
        if (TRNG_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        // make sure priKey in [1, n-1]
        ret = sm9_enc_gen_mastPubKey_from_mastPriKey(ke, Ppub_e);
    } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));

    return ret;
}

/**
 * @brief           Generate user's private key for SM9 encryption system.
 * @param[in]       IDB                  - Pointer to an array containing the user identifier IDB (user B).
 * @param[in]       IDB_bytes            - Length of the user identifier IDB in bytes.
 * @param[in]       hid                  - User private generation function identity, published by KGC. Default value is 0x03 (one byte).
 * @param[in]       ke                   - Pointer to a 32-byte array containing the KGC's master private key, in big-endian format.
 * @param[out]      deB                  - Pointer to a 128-byte array where the user B's private key will be stored, in big-endian format.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_enc_gen_userPriKey(const unsigned char *IDB, unsigned int IDB_bytes, unsigned char hid, const unsigned char ke[32], unsigned char deB[128])
{
    unsigned int tmp_ke[8];
    unsigned int tmp1[8];
    unsigned int tmp2[8];
    fp2_pt_t UserPrivKey;
    unsigned int ret;

    if ((NULL == IDB) || (NULL == ke) || (NULL == deB))
    {
        return SM9_BUFFER_NULL;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_ke, ke);

    // make sure sysPriKey in [1, n-1]
    ret = uint32_integer_check(tmp_ke, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = sm9_h1_h2((unsigned char)1, IDB, IDB_bytes, (unsigned char *)(&hid), 1u, tmp1);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    (void)pke_modadd(sm9_curve->eccp_n, tmp1, tmp_ke, tmp2, 8u);

    if (0u != uint32_bignum_check_zero(tmp2, 8u))
    {
        ret = SM9_ZERO_ALL;
        goto END;
    }
    else
    {
        ;
    }

    ret = pke_modinv(sm9_curve->eccp_n, tmp2, tmp1, 8u, 8u);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    (void)pke_modmul(sm9_curve->eccp_n, tmp1, tmp_ke, tmp2, 8u);

    (void)pke_sm9_set_p_and_pre_mont();

    ret = G2_pointMul_P2(tmp2, 8u, UserPrivKey);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = coordinate_convert(UserPrivKey->x, UserPrivKey->y, UserPrivKey->z, UserPrivKey->x, UserPrivKey->y);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u32little_to_u8big(deB, UserPrivKey->x->im);
    u32little_to_u8big((unsigned char *)(deB + 32u), UserPrivKey->x->re);
    u32little_to_u8big((unsigned char *)(deB + 64u), UserPrivKey->y->im);
    u32little_to_u8big((unsigned char *)(deB + 96u), UserPrivKey->y->re);

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           SM9 key encapsulation with r, get cipher C and output key
 * @param[in]       IDB                  - Pointer to user identifier IDB (receiver of cipher C)
 * @param[in]       IDB_bytes            - Length of IDB in bytes
 * @param[in]       QBx                  - x coordinate of QB = [H1(IDB||hid, N)]P1 + Ppub_e (32 bytes)
 * @param[in]       QBy                  - y coordinate of QB = [H1(IDB||hid, N)]P1 + Ppub_e (32 bytes)
 * @param[in]       fp12g                - Value of e(P1, Ppub_e) (precomputed pairing)
 * @param[in]       g                    - Pointer to __mpc12_struct for pairing computation
 * @param[in]       r                    - Random integer r for encapsulation
 * @param[out]      C                    - Output encapsulated cipher C (64 bytes)
 * @param[in]       k_bytes              - Length of output key in bytes
 * @param[out]      k                    - Output plaintext key
 * @return          SM9_SUCCESS on success, error code otherwise
 * @note
 *        1. fp12g, QBx, and QBy should not be modified during execution
 *        2. r must be a valid random integer in the appropriate range
 *        3. C buffer must be at least 64 bytes
 *        4. k buffer must be at least k_bytes in size
 */
unsigned int sm9_wrap_key_with_r(const unsigned char *IDB, unsigned int IDB_bytes, unsigned int *QBx, unsigned int *QBy, const unsigned char *fp12g, __mpc12_struct *g,
                                 unsigned int *r, unsigned char C[64], unsigned int k_bytes, unsigned char *k)
{
    unsigned char counter[4] = {0, 0, 0, 1};
    mpc12_t omega;
    unsigned char w[32u * 12u];
    hash_node_t hash_node[4] = {
        {C, 64u},
        {(unsigned char *)w, 32u * 12u},
        {IDB, IDB_bytes},
        {counter, 4u},
    };
    unsigned int ret;

    // make sure r in [1, n-1]
    ret = uint32_integer_check(r, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = eccp_pointmul(sm9_curve, r, QBx, QBy, (unsigned int *)omega, ((unsigned int *)omega) + 8u);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u32little_to_u8big(C, (unsigned int *)omega);
    u32little_to_u8big(C + 32u, ((unsigned int *)omega) + 8u);

#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    if (NULL != fp12g)
    {
        (void)mpc_fp12_g_exp(g, r, omega);
    }
    else
    {
        (void)mpc_fp12_exp(g, r, 8u, omega);
    }

    FE2OSP(omega, w);

    // key = KDF(C||w||IDB, key_bytes)
    ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, k, k_bytes, NULL, 0u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if (0u != uint8_bignum_check_zero(k, k_bytes))
    {
        ret = SM9_ZERO_ALL;
        goto END;
    }
    else
    {
        ;
    }

END:

    return ret;
}

/**
 * @brief           SM9 key encapsulation, generates key and its encapsulated cipher C.
 * @param[in]       IDB                  - Pointer to an array containing the user identifier IDB (user B is the receiver of the cipher C).
 * @param[in]       IDB_bytes            - Length of the user identifier IDB in bytes.
 * @param[in]       hid                  - User private generation function identity, published by KGC. Default value is 0x03 (one byte).
 * @param[in]       fp12g                - Pointer to the value of e(Ppub_e, P2). If set to NULL, it will be calculated within the function.
 * @param[in]       Ppub_e               - Pointer to a 64-byte array containing KGC's system encryption master public key (x||y), in big-endian format.
 * @param[in,out]   r                    - Pointer to a 32-byte array containing the random big integer r in wrapping, in big-endian format. If set to NULL, it will be generated inside the function.
 * @param[out]      C                    - Pointer to a 64-byte array where the encapsulated cipher C will be stored.
 * @param[in]       k_bytes              - Length of the output key in bytes.
 * @param[out]      k                    - Pointer to an array where the plaintext output key will be stored.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_wrap_key(const unsigned char *IDB, unsigned int IDB_bytes, unsigned char hid, const unsigned char *fp12g, const unsigned char Ppub_e[64],
                          const unsigned char r[32], unsigned char C[64], unsigned int k_bytes, unsigned char *k)
{
#if (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int tmp_r[9];
#else
    unsigned int tmp_r[8];
#endif
    unsigned int tmp[8], Ax[8], Ay[8];
    __mpc12_struct g[(1u << SM9_FP12_EXP_COMB_PARTS) - 1u]; //    mpc12_t g;
    fp2_pt_t MastPubKey;
    unsigned int i, ret;

    if ((NULL == IDB) || (NULL == Ppub_e) || (NULL == C) || (NULL == k))
    {
        return SM9_BUFFER_NULL;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    // check Ppub_e
    set_G1_point_buffer_2_G2_point(Ppub_e, MastPubKey);
    if (0u != sm9_pointVerify(MastPubKey->x->re, MastPubKey->y->re))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    if (NULL != fp12g)
    {
        // OS2FEP(fp12g, g);
        ret = (1u << SM9_FP12_EXP_COMB_PARTS) - 1u;
        for (i = 0u; i < ret; i++)
        {
            OS2FEP(fp12g + 32u * 12u * i, g + i);
        }
    }
    else
    {
        ret = pairing_raw(g, fp2ptP2->x, fp2ptP2->y, fp2ptP2->z, MastPubKey->x,
                          MastPubKey->y); // g = e(Ppub_e, P2)
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

    ret = sm9_h1_h2((unsigned char)1, IDB, IDB_bytes, (unsigned char *)(&hid), 1u, tmp);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_base(sm9_curve, tmp, Ax, Ay);
#else
    ret = eccp_pointmul(sm9_curve, tmp, fp2ptP1->x->re, fp2ptP1->y->re, Ax, Ay);
#endif
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = eccp_pointadd_safe(sm9_curve, Ax, Ay, MastPubKey->x->re, MastPubKey->y->re, Ax, Ay);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if (NULL == r)
    {
        do
        {
            ret = get_rand((unsigned char *)tmp_r, 32u);
            if (TRNG_SUCCESS != ret)
            {
                goto END;
            }
            else
            {
                ;
            }

            ret = sm9_wrap_key_with_r(IDB, IDB_bytes, Ax, Ay, fp12g, g, tmp_r, C, k_bytes, k);
        } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));
    }
    else
    {
        u8big_to_u32little_8(tmp_r, r);

        ret = sm9_wrap_key_with_r(IDB, IDB_bytes, Ax, Ay, fp12g, g, tmp_r, C, k_bytes, k);
    }

END:

    return ret;
}

/**
 * @brief           SM9 key decapsulation, generates key from encapsulated cipher C.
 * @param[in]       IDB                  - Pointer to an array containing the user identifier IDB.
 * @param[in]       IDB_bytes            - Length of the user identifier IDB in bytes.
 * @param[in]       deB                  - Pointer to a 128-byte array containing the private key of user B, in big-endian format.
 * @param[in]       C                    - Pointer to a 64-byte array containing the encapsulated cipher C.
 * @param[in]       k_bytes              - Length of the output key in bytes.
 * @param[out]      k                    - Pointer to an array where the plaintext output key will be stored.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_unwrap_key(const unsigned char *IDB, unsigned int IDB_bytes, const unsigned char deB[128], const unsigned char C[64], unsigned int k_bytes, unsigned char *k)
{
    unsigned char counter[4] = {0, 0, 0, 1};
    unsigned char h2rf_para[32u * 12u];
    mpc12_t fp12g;
    fp2_pt_t DecPriKey, C1;
    unsigned int ret;

    hash_node_t hash_node[4] = {
        {C, 64u},
        {h2rf_para, 32u * 12u},
        {IDB, IDB_bytes},
        {counter, 4u},
    };

    if ((NULL == IDB) || (NULL == deB) || (NULL == C) || (NULL == k))
    {
        return SM9_BUFFER_NULL;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    set_G1_point_buffer_2_G2_point(C, C1);

    (void)pke_sm9_set_p_and_pre_mont();

    if (sm9_pointVerify(C1->x->re, C1->y->re))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    set_G2_point_buffer_2_G2_point(deB, DecPriKey);

    ret = pairing_raw(fp12g, DecPriKey->x, DecPriKey->y, DecPriKey->z, C1->x, C1->y);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    FE2OSP(fp12g, h2rf_para);

    // key = KDF(C||w||IDB, key_bytes)
    ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, k, k_bytes, NULL, 0u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    if (0u != uint8_bignum_check_zero(k, k_bytes))
    {
        ret = SM9_VERIFY_FAILED;
    }
    else
    {
        ret = SM9_SUCCESS;
    }

END:

    return ret;
}

/**
 * @brief           Perform SM9 encryption using a specific random value r
 * @param[in]       hash_node            - Pointer to hash node structure for KDF computation
 * @param[in]       IDB                  - Receiver's identifier (user B)
 * @param[in]       IDB_bytes            - Length of IDB in bytes
 * @param[in]       r                    - Random integer r for encryption (32 bytes, big-endian)
 * @param[in]       QBx                  - x-coordinate of QB = [H1(IDB||hid, N)]P1 + Ppub_e
 * @param[in]       QBy                  - y-coordinate of QB = [H1(IDB||hid, N)]P1 + Ppub_e
 * @param[in]       fp12g                - Precomputed value of e(Ppub_e, P2) (optional)
 * @param[in]       g                    - MPC12 structure for pairing computation
 * @param[out]      C1                   - Output cipher component C1 (64 bytes)
 * @param[out]      K1                   - Output encryption key K1
 * @param[in]       K1_bytes             - Length of K1 in bytes
 * @param[out]      K2                   - Output MAC key K2
 * @param[in]       K2_bytes             - Length of K2 in bytes
 * @return          PKE_SUCCESS on success, error code otherwise
 * @note
 *        1. r must be in the range [1, n-1]
 *        2. K2_bytes must be less than SM9_MAX_ENC_K2_BYTE_LEN
 *        3. If fp12g is NULL, it will be computed internally
 */
unsigned int sm9_enc_with_r(hash_node_t *hash_node, const unsigned char *IDB, unsigned int IDB_bytes, unsigned int *r, unsigned int *QBx, unsigned int *QBy,
                            const unsigned char *fp12g, __mpc12_struct *g, unsigned char *C1, unsigned char *K1, unsigned int K1_bytes, unsigned char *K2, unsigned int K2_bytes)
{
    unsigned char counter[4] = {0, 0, 0, 1};
    mpc12_t omega;
    unsigned char w[32u * 12u];
    unsigned int ret;

    // make sure r in [1, n-1]
    ret = uint32_integer_check(r, sm9_curve->eccp_n, SM9_BASE_WORD_LEN, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // get C1 = [r]QB
    ret = eccp_pointmul(sm9_curve, r, QBx, QBy, (unsigned int *)omega, (unsigned int *)(omega) + 8u);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u32little_to_u8big(C1, (unsigned int *)omega);
    u32little_to_u8big(C1 + 32u, ((unsigned int *)omega) + 8u);

#if (defined(PKE_LP) || defined(PKE_SECURE))
    pke_set_exe_cfg(PKE_EXE_CFG_ALL_NON_MONT);
#endif
    if (fp12g)
    {
        (void)mpc_fp12_g_exp(g, r, omega);
    }
    else
    {
        (void)mpc_fp12_exp(g, r, 8u, omega);
    }

    FE2OSP(omega, w);

    // get k1||K2 = KDF(C1||w||IDB, K1_bytes+K2_bytes)
    hash_node[0].msg_addr = C1;
    hash_node[0].msg_len = 64u;
    hash_node[1].msg_addr = w;
    hash_node[1].msg_len = 32u * 12u;
    hash_node[2].msg_addr = IDB;
    hash_node[2].msg_len = IDB_bytes;
    hash_node[3].msg_addr = counter;
    hash_node[3].msg_len = 4u;
    ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, K1, K1_bytes, K2, K2_bytes);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // K1 can not be zero
    if (0u != uint8_bignum_check_zero(K1, K1_bytes))
    {
        ret = SM9_ZERO_ALL;
        goto END;
    }
    else
    {
        ;
    }

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           SM9 encrypt.
 * @param[in]       IDB                  - Pointer to an array containing the user identifier IDB (user B is the cipher receiver).
 * @param[in]       IDB_bytes            - Length of the user identifier IDB in bytes.
 * @param[in]       hid                  - User private generation function identity, published by KGC. Default value is 0x03, one byte.
 * @param[in]       M                    - Pointer to an array containing the message to be encrypted.
 * @param[in]       M_bytes              - Length of the message to be encrypted in bytes.
 * @param[in]       fp12g                - Pointer to the value of e(Ppub_e, P2). If set to NULL, it will be calculated within the function.
 * @param[in]       Ppub_e               - KGC's system encryption master public key.
 * @param[in]       r                    - Pointer to a 32-byte array containing the random big integer r in wrapping, in big-endian format. If you do not have this integer, please set this parameter to NULL, and it will be generated inside.
 * @param[in]       enc_type             - Type of encryption (SM9_ENC_KDF_STREAM_CIPHER or SM9_ENC_KDF_BLOCK_CIPHER).
 * @param[in]       padding_type         - Type of padding (SKE_NO_PADDING or SKE_PKCS_5_7_PADDING).
 * @param[in]       K2_bytes             - Length of the key K2 in MAC function in bytes.
 * @param[out]      C                    - Pointer to an array where the cipher will be stored.
 * @param[out]      C_bytes              - Length of the cipher in bytes.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 * @note
 *        1. IDB only represents the decryptor.
 *        2. K2_bytes should be less than SM9_MAX_ENC_K2_BYTE_LEN.
 */
unsigned int sm9_enc(const unsigned char *IDB, unsigned int IDB_bytes, unsigned char hid, const unsigned char *M, unsigned int M_bytes, const unsigned char *fp12g,
                     const unsigned char Ppub_e[64], const unsigned char r[32], sm9_enc_type_e enc_type, sm9_enc_padding_e padding_type, unsigned int K2_bytes, unsigned char *C,
                     unsigned int *C_bytes)
{
#if (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int tmp_r[9];
#else
    unsigned int tmp_r[8];
#endif
    unsigned int Ax[8], Ay[8];
    __mpc12_struct g[(1u << SM9_FP12_EXP_COMB_PARTS) - 1u]; // mpc12_t g;
    fp2_pt_t MastPubKey;
    unsigned char *K2 = (unsigned char *)MastPubKey; // unsigned char K2[SM9_MAX_ENC_K2_BYTE_LEN];
#if 0
#if (defined(SUPPORT_HASH_SHA3_224) || defined(SUPPORT_HASH_SHA3_256) || defined(SUPPORT_HASH_SHA3_384) || defined(SUPPORT_HASH_SHA3_512)) && \
    defined(CONFIG_HASH_SUPPORT_MUL_THREAD)
    hash_ctx_t ctx[1];
#else
    hash_ctx_t *ctx = (hash_ctx_t *)(K2+SM9_MAX_ENC_K2_BYTE_LEN);   //hash_ctx_t ctx[1];
#endif
#endif
    unsigned int i;
    unsigned int K1_bytes;
    unsigned char *C2 = C + 96u;
    unsigned int ret;

    hash_node_t hash_node[4];

    if (enc_type > SM9_ENC_KDF_BLOCK_CIPHER)
    {
        return SM9_INPUT_INVALID;
    }
    else if (SM9_ENC_KDF_BLOCK_CIPHER == enc_type)
    {
        if (padding_type > SKE_PKCS_5_7_PADDING)
        {
            return SM9_INPUT_INVALID;
        }
        else if ((0u != (M_bytes & 0x0Fu)) && (SKE_NO_PADDING == padding_type))
        {
            return SM9_INPUT_INVALID;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    if ((NULL == IDB) || (NULL == M) || (NULL == Ppub_e) || (NULL == C))
    {
        return SM9_BUFFER_NULL;
    }
    else if (M == C)
    {
        return SM9_IN_OUT_SAME_BUFFER;
    }
    else if ((0u == M_bytes) || (M_bytes >= SM9_MAX_MSG_BYTE_LEN))
    {
        return SM9_INPUT_INVALID;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else if (K2_bytes > SM9_MAX_ENC_K2_BYTE_LEN)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    set_G1_point_buffer_2_G2_point(Ppub_e, MastPubKey);

    if (NULL != fp12g)
    {
        ret = (1u << SM9_FP12_EXP_COMB_PARTS) - 1u;
        for (i = 0u; i < ret; i++)
        {
            OS2FEP(fp12g + 32u * 12u * i, g + i);
        }
    }
    else
    {
        (void)pke_sm9_set_p_and_pre_mont();
        ret = pairing_raw(g, fp2ptP2->x, fp2ptP2->y, fp2ptP2->z, MastPubKey->x,
                          MastPubKey->y); // g = e(Ppub_e, P2)
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }

    ret = sm9_h1_h2((unsigned char)1, IDB, IDB_bytes, (unsigned char *)(&hid), 1u, tmp_r);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_base(sm9_curve, tmp_r, Ax, Ay);
#else
    ret = eccp_pointmul(sm9_curve, tmp_r, fp2ptP1->x->re, fp2ptP1->y->re, Ax, Ay);
#endif
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = eccp_pointadd_safe(sm9_curve, Ax, Ay, MastPubKey->x->re, MastPubKey->y->re, Ax, Ay);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // get K1 length
    if (SM9_ENC_KDF_STREAM_CIPHER == enc_type) // KDF
    {
        K1_bytes = M_bytes;
    }
    else if (SM9_ENC_KDF_BLOCK_CIPHER == enc_type) // SM4
    {
        K1_bytes = 16u;
    }
    else
    {
        ;
    }

    if (NULL == r)
    {
        do
        {
            ret = get_rand((unsigned char *)tmp_r, 32u);
            if (TRNG_SUCCESS != ret)
            {
                break;
            }
            else
            {
                ;
            }

            ret = sm9_enc_with_r(hash_node, IDB, IDB_bytes, tmp_r, Ax, Ay, fp12g, g, C, C2, K1_bytes, K2, K2_bytes);
        } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));
    }
    else
    {
        u8big_to_u32little_8(tmp_r, r);

        ret = sm9_enc_with_r(hash_node, IDB, IDB_bytes, tmp_r, Ax, Ay, fp12g, g, C, C2, K1_bytes, K2, K2_bytes);
    }

    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    *C_bytes = M_bytes;

    if (SM9_ENC_KDF_STREAM_CIPHER == enc_type) // KDF
    {
        (void)uint8_xor(C2, M, C2, M_bytes);
    }
    else if (SM9_ENC_KDF_BLOCK_CIPHER == enc_type) // SM4
    {
#if defined(SKE_HP)
        ret = ske_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, C2, 0, NULL, padding_type, M, C2, M_bytes, C_bytes);
#elif defined(SKE_LP)
        ret = ske_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, C2, 0, NULL, padding_type, M, C2, M_bytes, C_bytes);
#elif defined(SKE_SECURE)
        ret = ske_sec_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_ENCRYPT, C2, 0, NULL, padding_type, M, C2, M_bytes, C_bytes);
#else
        ret = SM9_INPUT_INVALID;
#endif
        if (SKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    // get C3 = MAC(K2, C2) = sm3(C2||K2)
    hash_node[0].msg_addr = C2;
    hash_node[0].msg_len = *C_bytes;
    hash_node[1].msg_addr = K2;
    hash_node[1].msg_len = K2_bytes;
    ret = hash_node_steps(HASH_SM3, hash_node, 2u, C + 64u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    *C_bytes += 96u;

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           SM9 decrypt.
 * @param[in]       IDB                  - Pointer to an array containing the user identifier IDB (user B is the cipher receiver).
 * @param[in]       IDB_bytes            - Length of the user identifier IDB in bytes.
 * @param[in]       C                    - Pointer to an array containing the cipher.
 * @param[in]       C_bytes              - Length of the cipher in bytes.
 * @param[in]       deB                  - User B's private key.
 * @param[in]       enc_type             - Type of encryption (SM9_ENC_KDF_STREAM_CIPHER or SM9_ENC_KDF_BLOCK_CIPHER).
 * @param[in]       padding_type         - Type of padding (SKE_NO_PADDING or SKE_PKCS_5_7_PADDING).
 * @param[in]       K2_bytes             - Length of the key in MAC function in bytes.
 * @param[out]      M                    - Pointer to an array where the plaintext message will be stored.
 * @param[out]      M_bytes              - Length of the plaintext message in bytes.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 * @note
 *        1. Ensure that all input parameters are correctly provided and valid.
 */
unsigned int sm9_dec(const unsigned char *IDB, unsigned int IDB_bytes, const unsigned char *C, unsigned int C_bytes, const unsigned char deB[128], sm9_enc_type_e enc_type,
                     sm9_enc_padding_e padding_type, unsigned int K2_bytes, unsigned char *M, unsigned int *M_bytes)
{
    unsigned char counter[4] = {0, 0, 0, 1};
    unsigned char K1[16];
    unsigned char K2[SM9_MAX_ENC_K2_BYTE_LEN];
    unsigned char h2rf_para[32u * 12u];
    const unsigned char *C2 = (C) + 96u;
    unsigned int C2_bytes = C_bytes - 96u;
    mpc12_t w;
    fp2_pt_t DecPriKey, C1;
    unsigned int ret;

    hash_node_t hash_node[4] = {
        {C, 64u},
        {h2rf_para, 32u * 12u},
        {IDB, IDB_bytes},
        {counter, 4u},
    };

    if (enc_type > SM9_ENC_KDF_BLOCK_CIPHER)
    {
        return SM9_INPUT_INVALID;
    }
    else if (SM9_ENC_KDF_BLOCK_CIPHER == enc_type)
    {
        if (padding_type > SKE_PKCS_5_7_PADDING)
        {
            return SM9_INPUT_INVALID;
        }
        else if ((C_bytes - 96u) & 0x0Fu)
        {
            return SM9_INPUT_INVALID;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    if ((NULL == IDB) || (NULL == deB) || (NULL == M) || (NULL == C))
    {
        return SM9_BUFFER_NULL;
    }
    else if (M == C)
    {
        return SM9_IN_OUT_SAME_BUFFER;
    }
    else if (C_bytes < (96u + 1u))
    {
        return SM9_INPUT_INVALID;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else if (K2_bytes > SM9_MAX_ENC_K2_BYTE_LEN)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    set_G1_point_buffer_2_G2_point(C, C1);

    (void)pke_sm9_set_p_and_pre_mont();

    if (0u != sm9_pointVerify(C1->x->re, C1->y->re))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    set_G2_point_buffer_2_G2_point(deB, DecPriKey);

    ret = pairing_raw(w, DecPriKey->x, DecPriKey->y, DecPriKey->z, C1->x, C1->y);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    FE2OSP(w, h2rf_para);

    if (SM9_ENC_KDF_STREAM_CIPHER == enc_type) // KDF
    {
        ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, M, C2_bytes, K2, K2_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        if (0u != uint8_bignum_check_zero(M, C2_bytes))
        {
            ret = SM9_ZERO_ALL;
            goto END;
        }
        else
        {
            ;
        }

        uint8_xor(M, C2, M, C2_bytes);

        *M_bytes = C2_bytes;
    }
    else if (SM9_ENC_KDF_BLOCK_CIPHER == enc_type) // SM4
    {
        if (0u != (C_bytes & 0x0Fu))
        {
            ret = SM9_INPUT_INVALID;
            goto END;
        }
        else
        {
            ;
        }

        ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 4u, counter, K1, 16u, K2, K2_bytes);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        if (0u != uint8_bignum_check_zero(K1, 16u))
        {
            ret = SM9_ZERO_ALL;
            goto END;
        }
        else
        {
            ;
        }

#if defined(SKE_HP)
        ret = ske_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_DECRYPT, K1, 0, NULL, padding_type, C2, M, C2_bytes, M_bytes);
#elif defined(SKE_LP)
        ret = ske_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_DECRYPT, K1, 0, NULL, padding_type, C2, M, C2_bytes, M_bytes);
#elif defined(SKE_SECURE)
        ret = ske_sec_crypto(SKE_ALG_SM4, SKE_MODE_ECB, SKE_CRYPTO_DECRYPT, K1, 0, NULL, padding_type, C2, M, C2_bytes, M_bytes);
#else
        ret = SM9_INPUT_INVALID;
#endif
        if (SKE_SUCCESS != ret)
        {
            ret = SM9_DECRY_VERIFY_FAILED;
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    // get u = MAC(K2, C2) = sm3(C2||K2)
    hash_node[0].msg_addr = C2;
    hash_node[0].msg_len = C2_bytes;
    hash_node[1].msg_addr = K2;
    hash_node[1].msg_len = K2_bytes;
    ret = hash_node_steps(HASH_SM3, hash_node, 2u, (unsigned char *)h2rf_para);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // check u = C3 ?
    if (((unsigned char)0) != memcmp_(C + 64u, h2rf_para, 32u))
    {
        ret = SM9_DECRY_VERIFY_FAILED;
        goto END;
    }
    else
    {
        ;
    }

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           Generate KGC's master public key from master private key for SM9 key-exchange system.
 * @param[in]       ke                   - Pointer to an array containing the KGC's master private key, 32 bytes, big-endian.
 * @param[out]      Ppub_e               - Pointer to an array where the KGC's master public key (x||y) will be stored, 64 bytes, big-endian.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_exckey_gen_mastPubKey_from_mastPriKey(const unsigned char ke[32], unsigned char Ppub_e[64])
{
    return sm9_enc_gen_mastPubKey_from_mastPriKey(ke, Ppub_e);
}

/**
 * @brief           Generate random KGC's master key pair for SM9 key-exchange system.
 * @param[out]      ke                   - Pointer to an array where the KGC's master private key will be stored, 32 bytes, big-endian.
 * @param[out]      Ppub_e               - Pointer to an array where the KGC's master public key (x||y) will be stored, 64 bytes, big-endian.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_exckey_gen_mastKeyPair(unsigned char ke[32], unsigned char Ppub_e[64])
{
    return sm9_enc_gen_mastKeyPair(ke, Ppub_e);
}

/**
 * @brief           Generate user's private key for SM9 key-exchange system.
 * @param[in]       IDA                  - Pointer to an array containing the identify of user A.
 * @param[in]       IDA_bytes            - Length of the user identifier IDA in bytes.
 * @param[in]       hid                  - User private key generation function identity, published by KGC. Default value is 0x02, one byte.
 * @param[in]       ke                   - Pointer to an array containing the KGC's master private key, 32 bytes, big-endian.
 * @param[out]      deA                  - Pointer to an array where the user A's private key will be stored, 128 bytes, big-endian.
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error.
 */
unsigned int sm9_exckey_gen_userPriKey(const unsigned char *IDA, unsigned int IDA_bytes, unsigned char hid, const unsigned char ke[32], unsigned char deA[128])
{
    return sm9_enc_gen_userPriKey(IDA, IDA_bytes, hid, ke, deA);
}

/**
 * @brief           Generate user's temporary public key from private key for SM9 key-exchange system.
 * @param[in]       IDB                  - Pointer to an array containing the peer's identifier IDB
 * @param[in]       IDB_bytes            - Length of the peer identifier IDB in bytes
 * @param[in]       hid                  - User private key generation function identity, published by KGC. Default value is 0x02, one byte
 * @param[in]       Ppub_e               - Pointer to an array containing KGC's public key
 * @param[in]       rA                   - Pointer to an array containing local's temporary private key, 32 bytes, big-endian
 * @param[out]      RA                   - Pointer to an array where local's temporary public key (x||y) will be stored, 64 bytes, big-endian
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error
 * @note
 *        1. Ensure all input parameters are correctly provided and valid
 *        2. rA must be in the range [1, n-1] where n is the curve order
 */
unsigned int sm9_exckey_gen_tmpPubKey_from_tmpPriKey(const unsigned char *IDB, unsigned int IDB_bytes, unsigned char hid, const unsigned char Ppub_e[64],
                                                     const unsigned char rA[32], unsigned char RA[64])
{
    unsigned int tmp_r[8];
    unsigned int tmp1[8];
    unsigned int tmp2[8];
    unsigned int tmp3[8];
    unsigned int ret;

    if ((NULL == IDB) || (NULL == Ppub_e) || (NULL == rA) || (NULL == RA))
    {
        return SM9_BUFFER_NULL;
    }
    else if (0u == IDB_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    ret = sm9_h1_h2((unsigned char)1, IDB, IDB_bytes, (unsigned char *)(&hid), 1u, tmp1);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

#if (defined(PKE_HP) || defined(PKE_UHP))
    ret = eccp_pointMul_base(sm9_curve, tmp1, tmp1, tmp2);
#else
    ret = eccp_pointmul(sm9_curve, tmp1, fp2ptP1->x->re, fp2ptP1->y->re, tmp1, tmp2);
#endif
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_r, (const unsigned char *)(Ppub_e));
    u8big_to_u32little_8(tmp3, (const unsigned char *)(Ppub_e + 32u));

    ret = eccp_pointadd_safe(sm9_curve, tmp1, tmp2, tmp_r, tmp3, tmp1, tmp2);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u8big_to_u32little_8(tmp_r, rA);

    // make sure priKey in [1, n-1]
    ret = uint32_integer_check(tmp_r, sm9_curve->eccp_n, 8u, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    ret = eccp_pointmul(sm9_curve, tmp_r, tmp1, tmp2, tmp1, tmp2);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    u32little_to_u8big(RA, tmp1);
    u32little_to_u8big((unsigned char *)(RA + 32u), tmp2);

    ret = SM9_SUCCESS;

END:

    return ret;
}

/**
 * @brief           Generate user's temporary random key pair for SM9 key-exchange system.
 * @param[in]       IDB                  - Pointer to an array containing the peer's identifier IDB
 * @param[in]       IDB_bytes            - Length of the peer identifier IDB in bytes
 * @param[in]       hid                  - User private key generation function identity, published by KGC. Default value is 0x02, one byte
 * @param[in]       Ppub_e               - Pointer to an array containing KGC's public key
 * @param[out]      rA                   - Pointer to an array where local's temporary private key will be stored, 32 bytes, big-endian
 * @param[out]      RA                   - Pointer to an array where local's temporary public key (x||y) will be stored, 64 bytes, big-endian
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error
 * @note
 *        1. Ensure all input parameters are correctly provided and valid
 *        2. The generated private key rA will be in the range [1, n-1] where n is the curve order
 */
unsigned int sm9_exckey_gen_tmpKeyPair(const unsigned char *IDB, unsigned int IDB_bytes, unsigned char hid, const unsigned char Ppub_e[64], unsigned char rA[32],
                                       unsigned char RA[64])
{
    unsigned int ret;

    do
    {
        ret = get_rand(rA, 32u);
        if (TRNG_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
            ;
        }

        ret = sm9_exckey_gen_tmpPubKey_from_tmpPriKey(IDB, IDB_bytes, hid, Ppub_e, rA, RA);
    } while ((SM9_ZERO_ALL == ret) || (SM9_INTEGER_TOO_BIG == ret));

    return ret;
}

/**
 * @brief           SM9 Key exchange
 * @param[in]       role                 - Local user's role (SM9_Role_Sponsor or SM9_Role_Responsor)
 * @param[in]       IDA                  - Pointer to an array containing local user's identity
 * @param[in]       IDA_bytes            - Length of local user's identity in bytes
 * @param[in]       IDB                  - Pointer to an array containing peer user's identity
 * @param[in]       IDB_bytes            - Length of peer user's identity in bytes
 * @param[in]       fp12g                - Pointer to the value of e(Ppub_e, P2). If set to NULL, it will be calculated within the function
 * @param[in]       Ppub_e               - Pointer to an array containing KGC's system encryption master public key
 * @param[in]       deA                  - Pointer to an array containing local user's private key
 * @param[in]       rA                   - Pointer to an array containing local user's temporary private key
 * @param[in]       RA                   - Pointer to an array containing local user's temporary public key
 * @param[in]       RB                   - Pointer to an array containing peer user's temporary public key
 * @param[in]       k_bytes              - Length of the output key in bytes
 * @param[out]      k                    - Pointer to an array where the output key will be stored
 * @param[out]      S1                   - Pointer to an array where sponsor's S1 or responsor's S2 will be stored
 * @param[out]      SA                   - Pointer to an array where sponsor's SA or responsor's SB will be stored
 * @return          SM9_SUCCESS if the operation is successful, other values indicate an error
 * @note
 *        1. fp12g and Ppub_e cannot be NULL at the same time
 *        2. Ensure all input parameters are correctly provided and valid
 */
unsigned int sm9_exchangekey(sm9_exchange_role_e role, const unsigned char *IDA, unsigned int IDA_bytes, const unsigned char *IDB, unsigned int IDB_bytes,
                             const unsigned char *fp12g, const unsigned char Ppub_e[64], const unsigned char deA[128], const unsigned char rA[32], const unsigned char RA[64],
                             const unsigned char RB[64], unsigned int k_bytes, unsigned char *k, unsigned char S1[32], unsigned char SA[32])
{
    unsigned char counter[4] = {0, 0, 0, 1};
#if (SM9_FP12_EXP_COMB_PARTS == 3U)
    unsigned int tmp[9];
#else
    unsigned int tmp[8];
#endif
    unsigned char digest[32];

    FP2_POINT fp2_pt[2]; // fp2_pt_t MastPubKey, RRB;
    __mpc12_struct fp12g2[(1u << SM9_FP12_EXP_COMB_PARTS) - 1u];
    mpc12_t fp12g1, fp12g3;
    unsigned char *g1 = (unsigned char *)fp12g2; // unsigned char h2rf_para[32*12*3];
    unsigned char *g2 = (unsigned char *)fp12g3;
    unsigned char *g3 = (unsigned char *)fp2_pt;
    unsigned char *p;

    hash_node_t hash_node[8];
    unsigned int i, ret;
    unsigned char tag;

    if ((NULL == IDA) || (NULL == IDB) || (NULL == deA) || (NULL == rA))
    {
        return SM9_BUFFER_NULL;
    }
    else if ((NULL == RA) || (NULL == RB) || (NULL == k))
    {
        return SM9_BUFFER_NULL;
    }
    else if (role > SM9_Role_Responsor)
    {
        return SM9_EXCHANGE_ROLE_INVALID;
    }
    else if ((0u == IDA_bytes) || (0u == IDB_bytes))
    {
        return SM9_INPUT_INVALID;
    }
    else if (0u == k_bytes)
    {
        return SM9_INPUT_INVALID;
    }
    else
    {
        ;
    }

    (void)pke_sm9_set_p_and_pre_mont();

    // g2 = e(Ppub_e, P2)
    if (NULL != fp12g)
    {
        ret = (1u << SM9_FP12_EXP_COMB_PARTS) - 1u;
        for (i = 0u; i < ret; i++)
        {
            OS2FEP(fp12g + 32u * 12u * i, fp12g2 + i);
        }
    }
    else if (NULL != Ppub_e)
    {
        set_G1_point_buffer_2_G2_point(Ppub_e, &fp2_pt[0]);
        ret = pairing_raw(fp12g2, fp2ptP2->x, fp2ptP2->y, fp2ptP2->z, fp2_pt[0].x, fp2_pt[0].y);
        if (PKE_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ret = SM9_BUFFER_NULL;
        goto END;
    }

    set_G1_point_buffer_2_G2_point(RB, &fp2_pt[0]);
    if (0u != sm9_pointVerify(fp2_pt[0].x->re, fp2_pt[0].y->re))
    {
        ret = SM9_NOT_ON_CURVE;
        goto END;
    }
    else
    {
        ;
    }

    // check rA
    u8big_to_u32little_8(tmp, rA);
    ret = uint32_integer_check(tmp, sm9_curve->eccp_n, SM9_BASE_WORD_LEN, SM9_ZERO_ALL, SM9_INTEGER_TOO_BIG, SM9_SUCCESS);
    if (SM9_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // g1 = (e(Ppub_e, P2))^rA
    u8big_to_u32little_8(tmp, rA);
    if (NULL != fp12g)
    {
        (void)mpc_fp12_g_exp(fp12g2, tmp, fp12g1);
    }
    else
    {
        (void)mpc_fp12_exp(fp12g2, tmp, 8u, fp12g1);
    }

    // g2 = e(RB, deA)
    set_G2_point_buffer_2_G2_point(deA, &fp2_pt[1]);
    ret = pairing_raw(fp12g2, fp2_pt[1].x, fp2_pt[1].y, fp2_pt[1].z, fp2_pt[0].x, fp2_pt[0].y);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // g3 = g2^rA
    (void)mpc_fp12_exp(fp12g2, tmp, 8u, fp12g3);

    FE2OSP(fp12g3, g3);
    FE2OSP(fp12g2, g2);
    FE2OSP(fp12g1, g1);

    if (SM9_Role_Sponsor == role)
    {
        hash_node[0].msg_addr = IDA;
        hash_node[0].msg_len = IDA_bytes;
        hash_node[1].msg_addr = IDB;
        hash_node[1].msg_len = IDB_bytes;
        hash_node[2].msg_addr = RA;
        hash_node[3].msg_addr = RB;
    }
    else if (SM9_Role_Responsor == role)
    {
        p = g1;
        g1 = g2;
        g2 = p;

        hash_node[0].msg_addr = IDB;
        hash_node[0].msg_len = IDB_bytes;
        hash_node[1].msg_addr = IDA;
        hash_node[1].msg_len = IDA_bytes;
        hash_node[2].msg_addr = RB;
        hash_node[3].msg_addr = RA;
    }
    else
    {
        ;
    }

    hash_node[2].msg_len = 64u;
    hash_node[3].msg_len = 64u;
    hash_node[4].msg_addr = (unsigned char *)g1;
    hash_node[4].msg_len = 32u * 12u;
    hash_node[5].msg_addr = (unsigned char *)g2;
    hash_node[5].msg_len = 32u * 12u;
    hash_node[6].msg_addr = (unsigned char *)g3;
    hash_node[6].msg_len = 32u * 12u;
    hash_node[7].msg_addr = counter;
    hash_node[7].msg_len = 4u;

    // key = kdf(IDA||IDB||RA||RB||g1||g2||g3, key_bytes)
    ret = ansi_x9_63_kdf_node(HASH_SM3, hash_node, 8u, counter, k, k_bytes, NULL, 0u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
        ;
    }

    // check value is optional
    if ((NULL != S1) && (NULL != SA))
    {
        hash_node[0].msg_len = 32u * 12u;
        hash_node[0].msg_addr = (unsigned char *)g2;
        hash_node[1].msg_len = 32u * 12u;
        hash_node[1].msg_addr = (unsigned char *)g3;

        if (SM9_Role_Sponsor == role)
        {
            // tmp = Hash(g2||g3||IDA||IDB||RA||RB))
            hash_node[2].msg_addr = IDA;
            hash_node[2].msg_len = IDA_bytes;
            hash_node[3].msg_addr = IDB;
            hash_node[3].msg_len = IDB_bytes;
            hash_node[4].msg_addr = RA;
            hash_node[5].msg_addr = RB;
        }
        else if (SM9_Role_Responsor == role)
        {
            // tmp = Hash(g1||g3||IDB||IDA||RB||RA)), here g1 is responsor's g1, it is
            // expected to be sponsor's g2
            hash_node[2].msg_addr = IDB;
            hash_node[2].msg_len = IDB_bytes;
            hash_node[3].msg_addr = IDA;
            hash_node[3].msg_len = IDA_bytes;
            hash_node[4].msg_addr = RB;
            hash_node[5].msg_addr = RA;
        }
        else
        {
            ;
        }

        hash_node[4].msg_len = 64u;
        hash_node[5].msg_len = 64u;

        ret = hash_node_steps(HASH_SM3, hash_node, 6u, (unsigned char *)digest);
        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        hash_node[0].msg_addr = &tag;
        hash_node[0].msg_len = 1u;
        hash_node[1].msg_addr = (unsigned char *)g1;
        hash_node[1].msg_len = 32u * 12u;
        hash_node[2].msg_addr = (unsigned char *)digest;
        hash_node[2].msg_len = 32u;

        tag = (unsigned char)0x82;
        if (SM9_Role_Sponsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 3u, S1);
        }
        else if (SM9_Role_Responsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 3u, SA);
        }
        else
        {
            ;
        }

        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }

        tag = (unsigned char)0x83;
        if (SM9_Role_Sponsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 3u, SA);
        }
        else if (SM9_Role_Responsor == role)
        {
            ret = hash_node_steps(HASH_SM3, hash_node, 3u, S1);
        }
        else
        {
            ;
        }

        if (HASH_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    ret = SM9_SUCCESS;

END:

    return ret;
}

#endif
