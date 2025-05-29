/********************************************************************************************************
 * @file    tlk_mem.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef TLK_MEM_H
#define TLK_MEM_H


#include "common/types.h"
#include "common/compiler.h"


/**************************************************************************************************
  Configuration
**************************************************************************************************/
/*! \brief Buffer histogram stats */
#ifndef TLK_MEM_STATS_HIST
    #define TLK_MEM_STATS_HIST FALSE //TRUE//
#endif

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! \brief Length of the buffer statistics array */
#define TLK_MEM_STATS_MAX_LEN 512

/*! \brief Max number of pools can allocate */
#define TLK_MEM_STATS_MAX_POOL 32

#ifndef TLK_MEM_STATS
    /*! \brief Enable buffer allocation statistics. */
    #define TLK_MEM_STATS FALSE //TRUE//
#endif

/**************************************************************************************************
  Data Types
**************************************************************************************************/
/*! \brief Mem pool descriptor structure */
typedef struct
{
    u16 len; /*!< \brief Length of buffers in pool */
    u8  num; /*!< \brief Number of buffers in pool */
} __attribute__((aligned(4))) tlk_mem_pool_desc_t;

/*! \brief Pool statistics */
typedef struct
{
    u16 bufSize;   /*!< \brief Pool buffer size. */
    u8  numBuf;    /*!< \brief Total number of buffers. */
    u8  numAlloc;  /*!< \brief Number of outstanding allocations. */
    u8  maxAlloc;  /*!< \brief High allocation watermark. */
    u16 maxReqLen; /*!< \brief Maximum requested buffer length. */
} __attribute__((aligned(4))) tlk_mem_pool_state_t;

/**
 *  @brief Definition error code of memory operation
 */
typedef enum
{
    TLK_MEM_SUCCESS = 0,
    TLK_MEM_NOT_ALIGNED,
    TLK_MEM_INVALID_PARAMETER,
    TLK_MEM_ALREADY_FREE,
    TLK_MEM_ALLOC_FAIL,
    TLK_MEM_OUT_OF_MEM, //The size of HEAP_MEM_SIZE_CFG is not enough.
} TLK_MEM_STATE_T;

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/
/**
 * @brief       Calculate size required by the memory pool.
 * @param[in]   numPools - Number of buffer pools.
 * @param[in]   pDesc    - Array of buffer pool descriptors, one for each pool.
 * @return      Memory size of all memory pool used.
 */
u32 tlk_mem_cal_size(u8 numPools, tlk_mem_pool_desc_t *pDesc);

/**
 * @brief       This function serves to allocate memory in global heap for the memory pools.
 *              It should be called when you want to allocate a new amount of memory pools.
 * @param[in]   numPools - Number of memory pools.
 * @param[in]   pDesc    - Array of memory pool descriptors, one for each pool.
 * @return      0        - Memory allocate failed .
 *              others   - The size used by all memory pools.
 */
u8 tlk_mempool_init(u8 numPools, tlk_mem_pool_desc_t *pDesc);

/**
 * @brief       This function serves to malloc a buffer in all memory pools.
 * @param[in]   len      - Length need to alloc.
 * @return      0        - Allocate failed .
 *              others   - Pointer to the allocated buffer.
 */
void *tlk_mem_malloc(u16 len);

/**
 * @brief       This function serves to free a buffer in the specific memory pool identified by the buffer_address.
 * @param[in]   pBuf  - Pointer to the buffer need to free.
 * @return      none.
 */
u8 tlk_mem_free(void *pBuf);

/**
 * @brief       This function serves to get the buffer allocate statistics.
 * @param[in]   none.
 * @return      buffer allocation statistics array - tlk_mem_mallocCount[TLK_MEM_STATS_MAX_LEN].
 */
u8 *tlk_mem_get_alloc_sts(void);

/**
 * @brief       This function serves to get the number of overflow times for each pool.
 * @param[in]   none.
 * @return      Overflow times statistics array - tlk_mempool_overflow_count[TLK_MEM_STATS_MAX_POOL].
 */
u8 *tlk_mem_get_pool_overflow_sts(void);

/**
 * @brief       This function serves to get the number of pools.
 * @param[in]   none.
 * @return      Number of pools.
 */
u8 tlk_mem_get_pool_num(void);

/**
 * @brief       This function serves to get statistics for each pool.
 * @param[in]   pStat   - Buffer to store the statistics.
 * @param[in]   numPool - Number of pool elements.
 * @return      Number of pools.
 */
void tlk_mem_get_pools_sts(tlk_mem_pool_state_t *pStat, u8 numPool);


#endif /* TLK_MEM_H */
