#ifndef HASH_CONFIG_H
#define HASH_CONFIG_H

#include "lib/include/crypto_common/common_config.h"

/************************************************************************************
 *******************************    HASH config    **********************************
 ************************************************************************************/

/*
 *function: hash_lp IP base address
 *caution:
 */
// #define HASH_BASE_ADDR (0x81400000U) //(0x43C00000U)  //HASH register base address

/*
 *function: supported hash algorithms
 *caution:
 */
// #define SUPPORT_HASH_SM3
// #define SUPPORT_HASH_MD5
#define SUPPORT_HASH_SHA256
// #define SUPPORT_HASH_SHA384
// #define SUPPORT_HASH_SHA512
#define SUPPORT_HASH_SHA1
#define SUPPORT_HASH_SHA224
// #define SUPPORT_HASH_SHA512_224
// #define SUPPORT_HASH_SHA512_256
// #define SUPPORT_HASH_SHA3_224 //hw not support
// #define SUPPORT_HASH_SHA3_256 //hw not support
// #define SUPPORT_HASH_SHA3_384 //hw not support
// #define SUPPORT_HASH_SHA3_512 //hw not support

/*
 *function: support hash dma style
 *caution:
 */
#define HASH_DMA_FUNCTION

#ifdef HASH_DMA_FUNCTION
/*
 *function: ram for hash dma
 *caution:
 *    1.just for temporary use
 */
// #define HASH_DMA_RAM_BASE (0x60000000U) //(0x80000000U)
#endif

/*
 *function: support multiple thread
 *caution:
 */
// #define CONFIG_HASH_SUPPORT_MUL_THREAD

/*
 *function: whether for standalone ip or firmware
 *caution:
 *   1. different choices affect some structure members
 *   2. if open the macro, will reduces stack cost
 *   3. if use for firmware, should closed this macro
 */
#ifndef CONFIG_HASH_SUPPORT_MUL_THREAD
#define CONFIG_SUPPORT_STRUCTURE_OPTIMIZATION
#endif

// support node style
#define SUPPORT_HASH_NODE

#ifdef HASH_DMA_FUNCTION
/*
 *function: support dma node style
 *caution:
 */
#define SUPPORT_HASH_DMA_NODE
#endif

/*
 *function: support PBKDF2
 *caution:
 */
// #define SUPPORT_PBKDF2

#ifdef SUPPORT_PBKDF2
/*
 *function: support PBKDF2 high speed
 *caution:
 */
#define PBKDF2_HIGH_SPEED
#endif

/*
 *function: support ANSI_x9.36_KDF
 *caution:
 */
#define SUPPORT_ANSI_X9_63_KDF

/*
 *function: support hash endian choice
 *caution:
 *    1.default close
 */
// #define HASH_CPU_BIG_ENDIAN

/*
 *function: support hash secure port
 *caution:
 *    1.if key is from secure port, HMAC_MAX_K_IDX is the max key index(or the number of keys),
 *      and HMAC_MAX_SP_KEY_SIZE is the max key byte length
 *    2.for hash lp, it is not support secure port
 *    3.close default
 */

#ifdef HASH_DMA_FUNCTION
/*
 *function: support DMA address with high 32bit and low 32bit and address remap
 *caution:
 *    1.please make sure dependency function rewrite on lib extension model
 */
// #define CONFIG_HASH_SUPPORT_DMA_HIGH_LOW_ADDRESS
#endif

#endif
