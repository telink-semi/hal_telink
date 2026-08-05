/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TELINK_SOC_MBEDTLS_USER_CONFIG_H
#define TELINK_SOC_MBEDTLS_USER_CONFIG_H

#ifdef MBEDTLS_PLATFORM_MEMORY
#undef MBEDTLS_PLATFORM_MEMORY
#endif

#ifdef MBEDTLS_MEMORY_BUFFER_ALLOC_C
#undef MBEDTLS_MEMORY_BUFFER_ALLOC_C
#endif

#if CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL721X

#define big_integer_compare      uint32_BigNumCmp
#define pke_clr_irq_status       pke_clear_interrupt
// #define pke_get_irq_status       pke_wait_till_done
#define pke_opr_start            pke_start
#define pke_mod_add              pke_modadd
#define pke_mod_sub              pke_modsub
#define pke_mod_mul              pke_modmul
#define pke_mod_inv              pke_modinv
#define div2n_u32                Big_Div2n
#define sub_u32                  pke_sub
#define pke_eccp_point_mul       eccp_pointMul
#define pke_eccp_point_add       eccp_pointAdd
#define pke_eccp_point_verify    eccp_pointVerify
#if defined(SUPPORT_C25519) || defined(CONFIG_SOC_RISCV_TELINK_TL721X)
#define pke_x25519_point_mul     x25519_pointMul
#define pke_ed25519_point_mul    ed25519_pointMul
#define pke_ed25519_point_add    ed25519_pointAdd
# else
// #define pke_x25519_point_mul
// #define pke_ed25519_point_mul
// #define pke_ed25519_point_add
# endif /* SUPPORT_C25519 */

#else

#define big_integer_compare      uint32_BigNumCmp
#define pke_clr_irq_status       pke_clear_interrupt
#define pke_get_irq_status       pke_wait_till_done
#define pke_opr_start            pke_start
#define pke_mod_add              pke_modadd
#define pke_mod_sub              pke_modsub
#define pke_mod_mul              pke_modmul
#define pke_mod_inv              pke_modinv
#define div2n_u32                Big_Div2n
#define sub_u32                  pke_sub
#define pke_eccp_point_mul       eccp_pointMul
#define pke_eccp_point_add       eccp_pointAdd
#define pke_eccp_point_verify    eccp_pointVerify
# ifdef SUPPORT_C25519
#define pke_x25519_point_mul     x25519_pointMul
#define pke_ed25519_point_mul    ed25519_pointMul
#define pke_ed25519_point_add    ed25519_pointAdd
# else
#define pke_x25519_point_mul
#define pke_ed25519_point_mul
#define pke_ed25519_point_add
# endif /* SUPPORT_C25519 */

#endif /* CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL721X */

#endif /* TELINK_SOC_MBEDTLS_USER_CONFIG_H */
