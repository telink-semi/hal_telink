/*! @file lib_extension.c */
#include "lib/include/crypto_common/lib_extension.h"

#define __weak __attribute__((weak))

/**
 * @brief           Address remap.
 * @param[in]       addr_h               - address high 32-bit (unsigned int)
 * @param[in]       addr_l               - address low 32-bit (unsigned int)
 * @param[in]       is_dma_read_addr     - whether the address is DMA input or output (0 for output, non-zero for input)
 * @return          None
 * @note            
 */
__weak unsigned char *lib_addr_arch32_lock_remap(unsigned int addr_h, unsigned int addr_l, unsigned int is_dma_read_addr)
{
    (void)addr_h;
    (void)is_dma_read_addr;

    return (unsigned char *)addr_l;
}

/**
 * @brief           Address unremap
 * @return          None
 * @note            
 */
__weak void lib_addr_arch32_unlock_remap(void)
{
    return;
}

/**
 * @brief           Configure secure port for SKE (Symmetric Key Encryption).
 * @param[in]       alg                  - SKE algorithm
 * @param[in]       sp_key_idx           - secure port key index
 * @return          None
 * @note            
 */
__weak unsigned int lib_ske_secure_port_config(unsigned int alg, unsigned short sp_key_idx)
{
    (void)alg;
    (void)sp_key_idx;

    return 0U;
}

/**
 * @brief           Configure secure port for Lib Hash
 * @param[in]       sp_key_idx           - secure port key ID
 * @return          None
 * @note            
 */
__weak unsigned int lib_hash_secure_port_config(unsigned short sp_key_idx)
{
    (void)sp_key_idx;

    return 0U;
}
