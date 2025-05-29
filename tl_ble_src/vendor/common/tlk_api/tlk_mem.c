/********************************************************************************************************
 * @file    tlk_mem.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tlk_mem.h"
#include "tlk_heap.h"
#include "types.h"
#include "utility.h"
#include "assert.h"
#include "common/compiler.h"
#include "driver.h"

/**************************************************************************************************
  Global Variables
**************************************************************************************************/


/*! \brief  Critical section nesting level. */
_attribute_data_retention_ static u8 tlk_cs_nesting = 0;

/*************************************************************************************************/
/*!
 *  \brief  Enter a critical section.
 */
/*************************************************************************************************/
_attribute_ram_code_ void tlk_cs_enter(void)
{
    if (tlk_cs_nesting == 0) {
        core_interrupt_disable();
    }
    tlk_cs_nesting++;
}

/*************************************************************************************************/
/*!
 *  \brief  Exit a critical section.
 */
/*************************************************************************************************/
_attribute_ram_code_ void tlk_cs_exit(void)
{
    assert(tlk_cs_nesting != 0);

    tlk_cs_nesting--;
    if (tlk_cs_nesting == 0) {
        core_interrupt_enable();
    }
}

/**************************************************************************************************
  Data Types
**************************************************************************************************/

/* Unit of memory storage-- a structure containing a pointer. */
typedef struct tlk_mem_buff_tag
{
    struct tlk_mem_buff_tag *pNext;
} __attribute__((aligned(4))) tlk_mem_buff_t;

/* Internal buffer pool. */
typedef struct
{
    tlk_mem_pool_desc_t desc;   /* Number of buffers and length. */
    tlk_mem_buff_t     *pStart; /* Start of pool. */
    tlk_mem_buff_t     *pFree;  /* First free buffer in pool. */
#if TLK_MEM_STATS == TRUE
    u8  numAlloc;               /* Number of buffers currently allocated from pool. */
    u8  maxAlloc;               /* Maximum buffers ever allocated from pool. */
    u16 maxReqLen;              /* Maximum request length from pool. */
#endif
} __attribute__((aligned(4))) tlk_mem_pool_t;

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/* Number of pools. */
_attribute_data_retention_ u8 tlk_mem_pools_num;

/* Memory used for pools. */
_attribute_data_retention_ tlk_mem_buff_t *tlkMemBuf = NULL;

/* Currently use for debugging only. */
_attribute_data_retention_ u32 tlkMemBufLen;

#if TLK_MEM_STATS_HIST == TRUE
/* Buffer allocation counter. */
_attribute_data_retention_ u8 tlk_mem_mallocCount[TLK_MEM_STATS_MAX_LEN];

/* Pool Overflow counter. */
_attribute_data_retention_ u8 tlk_mempool_overflow_count[TLK_MEM_STATS_MAX_POOL];
#endif


/*************************************************************************************************/
/**
 * @brief       Calculate size required by the memory pools.
 * @param[in]   numPools - Number of buffer pools.
 * @param[in]   pDesc    - Array of buffer pool descriptors, one for each pool.
 * @return      Memory size of all memory pool used.
 */
u32 tlk_mem_cal_size(u8 numPools, tlk_mem_pool_desc_t *pDesc)
{
    u32             len;
    u32             descLen;
    tlk_mem_pool_t *pPool;
    tlk_mem_buff_t *pStart;
    u8              i;

    tlkMemBuf = (tlk_mem_buff_t *)0;
    pPool     = (tlk_mem_pool_t *)tlkMemBuf;

    /* Buffer storage starts after the pool structs. */
    pStart = (tlk_mem_buff_t *)(pPool + numPools);

    /* Create each pool; see loop exit condition below. */
    while (TRUE) {
        /* Exit loop after verification check. */
        if (numPools-- == 0) {
            break;
        }

        /* Adjust pool lengths for minimum size and alignment. */
        if (pDesc->len < sizeof(tlk_mem_buff_t)) {
            descLen = sizeof(tlk_mem_buff_t);
        } else if ((pDesc->len % sizeof(tlk_mem_buff_t)) != 0) {
            descLen = pDesc->len + sizeof(tlk_mem_buff_t) - (pDesc->len % sizeof(tlk_mem_buff_t));
        } else {
            descLen = pDesc->len;
        }

        len = descLen / sizeof(tlk_mem_buff_t);
        for (i = pDesc->num; i > 0; i--) {
            /* Pointer to the next free buffer is stored in the buffer itself. */
            pStart += len;
        }
        pDesc++;
    }

    return (u8 *)pStart - (u8 *)tlkMemBuf;
}

/**
 * @brief       This function serves to allocate memory in global heap for the memory pools.
 *              It should be called when you want to allocate a new amount of memory pools.
 * @param[in]   numPools - Number of memory pools.
 * @param[in]   pDesc    - Array of memory pool descriptors, one for each pool.
 * @return      0        - Memory allocate success .
 *              others   - Memory allocate success,search for TLK_MEM_STATE_T.
 */
u8 tlk_mempool_init(u8 numPools, tlk_mem_pool_desc_t *pDesc)
{
    tlk_mem_pool_t *pPool;
    tlk_mem_buff_t *pStart;
    u16             len;
    u8              i;

    tlkMemBuf = (tlk_mem_buff_t *)tlk_get_heap_free_startAddr(); //get free start address of the heap.
    pPool     = (tlk_mem_pool_t *)tlkMemBuf;

    /* Buffer storage starts after the pool tags. */
    pStart = (tlk_mem_buff_t *)(pPool + numPools);

    tlk_mem_pools_num = numPools;

    /* Create each pool,string each block in memory pool*/
    while (TRUE) {
        /* Verify we didn't overrun memory*/
        if (pStart > &tlkMemBuf[tlk_get_available_heap_size() / sizeof(tlk_mem_buff_t)]) {
            return TLK_MEM_OUT_OF_MEM;
        }
        /* Exit loop after verification check. */
        if (numPools-- == 0) {
            break;
        }
        /* Adjust pool lengths for minimum size and alignment. */
        if (pDesc->len < sizeof(tlk_mem_buff_t))                                                           //block length < sizeof(tlk_mem_buff_t)
        {
            pPool->desc.len = sizeof(tlk_mem_buff_t);                                                      //block length = sizeof(tlk_mem_buff_t)
        } else if ((pDesc->len % sizeof(tlk_mem_buff_t)) != 0)                                             //n*sizeof(tlk_mem_buff_t)< block length < (n+1)*sizeof(tlk_mem_buff_t)
        {
            pPool->desc.len = pDesc->len + sizeof(tlk_mem_buff_t) - (pDesc->len % sizeof(tlk_mem_buff_t)); //block length = (n+1)*sizeof(tlk_mem_buff_t)
        } else                                                                                             //block length = n*sizeof(tlk_mem_buff_t)
        {
            pPool->desc.len = pDesc->len;
        }

        pPool->desc.num = pDesc->num;
        pDesc++;

        pPool->pStart = pStart;
        pPool->pFree  = pStart; // initial state,free_block_list = block_list.
#if TLK_MEM_STATS == TRUE
        pPool->numAlloc  = 0;
        pPool->maxAlloc  = 0;
        pPool->maxReqLen = 0;
#endif


        /* Initialize free block list. */
        len = pPool->desc.len / sizeof(tlk_mem_buff_t); //size of each block,counting by sizeof(tlk_mem_buff_t).
        for (i = pPool->desc.num; i > 1; i--) {
            /* Verify we didn't overrun memory; if we did, abort. */
            if (pStart > &tlkMemBuf[tlk_get_available_heap_size() / sizeof(tlk_mem_buff_t)]) {
                return TLK_MEM_OUT_OF_MEM;
            }
            /* Pointer to the next free buffer is stored in the buffer itself. */
            pStart->pNext = pStart + len; //assign pStart->next.
            pStart += len;                //pStart++
        }

        /* Verify we didn't overrun memory; if we did, abort. */
        if (pStart > &tlkMemBuf[tlk_get_available_heap_size() / sizeof(tlk_mem_buff_t)]) {
            return TLK_MEM_OUT_OF_MEM;
        }
        /* Last one in list points to NULL. */
        pStart->pNext = NULL; //Last one in memory pool points to NULL
        pStart += len;        //pStart++

        /* Next pool. */
        pPool++;
    }
    tlkMemBufLen = (u8 *)pStart - (u8 *)tlkMemBuf;
    tlk_heap_alloc(tlkMemBufLen);

    return TLK_MEM_SUCCESS;
}

/**
 * @brief       This function serves to malloc a buffer in all memory pools.
 * @param[in]   len      - Length need to alloc.
 * @return      0        - Allocate failed .
 *              others   - Pointer to the allocated buffer.
 */
_attribute_ram_code_ void *tlk_mem_malloc(u16 len)
{
    tlk_mem_pool_t *pPool;
    tlk_mem_buff_t *pBuf;
    u8              i;

    pPool = (tlk_mem_pool_t *)tlkMemBuf;

    for (i = tlk_mem_pools_num; i > 0; i--, pPool++) //Check each memory pools if its length is available.
    {
        /* Check if buffer is big enough. */
        if (len <= pPool->desc.len) {
            /* Enter critical section. */
            tlk_cs_enter();

            /* Check if buffers are available. */
            if (pPool->pFree != NULL) {
                /* Allocation succeeded. */
                pBuf = pPool->pFree;

                /* free buffer list update. */
                pPool->pFree = pBuf->pNext;
#if TLK_MEM_STATS_HIST == TRUE
                /* Increment count for buffers of this length. */
                if (len < TLK_MEM_STATS_MAX_LEN) {
                    tlk_mem_mallocCount[len]++;
                } else {
                    tlk_mem_mallocCount[0]++;
                }
#endif
#if TLK_MEM_STATS == TRUE
                if (++pPool->numAlloc > pPool->maxAlloc) {
                    pPool->maxAlloc = pPool->numAlloc;
                }
                pPool->maxReqLen = max2(pPool->maxReqLen, len);
#endif
                /* Exit critical section. */
                tlk_cs_exit();
                ;
                return pBuf;
            }
#if TLK_MEM_STATS_HIST == TRUE
            else {
                /* Pool overflow: increment count of overflow for current pool. */
                tlk_mempool_overflow_count[tlk_mem_pools_num - i]++;
            }
#endif
            /* Exit critical section. */
            tlk_cs_exit();
        }
    }
    return NULL;
}

/**
 * @brief       This function serves to free a buffer in the specific memory pool identified by the buffer_address.
 * @param[in]   pBuf  - Pointer to the buffer need to free.
 * @return      none.
 */
_attribute_ram_code_ u8 tlk_mem_free(void *pBuf)
{
    tlk_mem_pool_t *pPool;
    tlk_mem_buff_t *p = pBuf;

    /* Verify pointer is within range. */
    if (p < ((tlk_mem_pool_t *)tlkMemBuf)->pStart || p >= (tlk_mem_buff_t *)(((u8 *)tlkMemBuf) + tlkMemBufLen)) {
        return TLK_MEM_INVALID_PARAMETER;
    }
    /* Iterate over pools starting from last pool. */
    pPool = (tlk_mem_pool_t *)tlkMemBuf + (tlk_mem_pools_num - 1);
    while (pPool >= (tlk_mem_pool_t *)tlkMemBuf) {
        /* Check if the buffer memory is located inside this pool. */
        if (p >= pPool->pStart) {
            /* Enter critical section. */
            tlk_cs_enter();

#if TLK_MEM_STATS == TRUE
            pPool->numAlloc--;
#endif

            /* Pool found; put buffer back in free list. */
            p->pNext     = pPool->pFree;
            pPool->pFree = p;
            /* Exit critical section. */
            tlk_cs_exit();
            ;

            return TLK_MEM_SUCCESS;
        }
        /* Next pool. */
        pPool--;
    }
    return TLK_MEM_ALLOC_FAIL;
}

/**
 * @brief       This function serves to get the buffer allocate statistics.
 * @param[in]   none.
 * @return      buffer allocation statistics array - tlk_mem_mallocCount[TLK_MEM_STATS_MAX_LEN].
 */
u8 *tlk_mem_get_alloc_sts(void)
{
#if TLK_MEM_STATS_HIST == TRUE
    return tlk_mem_mallocCount;
#else
    return NULL;
#endif
}

/**
 * @brief       This function serves to get the number of overflow times for each pool.
 * @param[in]   none.
 * @return      Overflow times statistics array - tlk_mempool_overflow_count[TLK_MEM_STATS_MAX_POOL].
 */
u8 *tlk_mem_get_pool_overflow_sts(void)
{
#if TLK_MEM_STATS_HIST == TRUE
    return tlk_mempool_overflow_count;
#else
    return NULL;
#endif
}

/**
 * @brief       This function serves to get the number of pools.
 * @param[in]   none.
 * @return      Number of pools.
 */
u8 tlk_mem_get_pool_num(void)
{
    return tlk_mem_pools_num;
}

/**
 * @brief       This function serves to get statistics for each pool.
 * @param[in]   pStat   - Buffer to store the statistics.
 * @param[in]   numPool - Number of pool elements.
 * @return      Number of pools.
 */
void tlk_mem_get_pools_sts(tlk_mem_pool_state_t *pStat, u8 poolId)
{
    tlk_mem_pool_t *pPool;

    if (poolId >= tlk_mem_pools_num) {
        pStat->bufSize = 0;
        return;
    }

    //MY_CS_INIT(cs);
    tlk_cs_enter();

    pPool = (tlk_mem_pool_t *)tlkMemBuf;

    pStat->bufSize = pPool[poolId].desc.len;
    pStat->numBuf  = pPool[poolId].desc.num;
#if TLK_MEM_STATS == TRUE
    pStat->numAlloc  = pPool[poolId].numAlloc;
    pStat->maxAlloc  = pPool[poolId].maxAlloc;
    pStat->maxReqLen = pPool[poolId].maxReqLen;
#else
    pStat->numAlloc  = 0;
    pStat->maxAlloc  = 0;
    pStat->maxReqLen = 0;
#endif

    /* Exit critical section. */
    tlk_cs_exit();
    ;
}


#if 0 //demo test
    void user_init(void)
    {
        tlk_mem_pool_desc_t poolDesc[] =
        {
            { 16,            8 },
            { 32,            4 },
            { 64,            8 },
            { 128,           4 },
            { 256,           4 },
            { 512,           2 },
        };
        const u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
        //blocks malloc
        if(tlk_mempool_init(numPools, poolDesc)!=0)
        {
            //my_dump_str_data(APP_DUMP_EN, "mempool init failed!", NULL, 0);
            while(1);
        }
        for(u8 i=0;i<30;i++)
        {
            u8 *p = (u8*)tlk_mem_malloc(7);
            if(p == NULL)
            {
                //my_dump_str_data(APP_DUMP_EN, "mempool malloc failed!", NULL, 0);
            }
            //my_dump_str_data(APP_DUMP_EN, "address of p", &p, 4);
            if(i%3 == 0)
            {
                u8 index = tlk_mem_free(p);
                if(index !=0 )
                {
                    //my_dump_str_data(APP_DUMP_EN, "free mem fail", &index, 1);
                    //my_dump_str_data(APP_DUMP_EN, "free mem fail", &index, 1);
                }
                else
                {
                    //my_dump_str_data(APP_DUMP_EN, "free mem success", &p, 4);
                }
            }
        }

        //global memory malloc
        u8 *p_global = tlk_get_heap_free_startAddr();
        if(tlk_heap_alloc(1024)!=0)
        {
            //my_dump_str_data(APP_DUMP_EN, "global memory alloc fail", &p_global, 4);
        }
        else
        {
            //my_dump_str_data(APP_DUMP_EN, "global memory alloc success", &p_global, 4);
        }

        u8 *p_global1 = tlk_get_heap_free_startAddr();
        if(tlk_heap_alloc(512)!=0)
        {
            //my_dump_str_data(APP_DUMP_EN, "global memory alloc fail", &p_global1, 4);
        }
        else
        {
            //my_dump_str_data(APP_DUMP_EN, "global memory alloc success", &p_global1, 4);
        }
    }
#endif
