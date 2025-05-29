/********************************************************************************************************
 * @file    tlk_malloc.c
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

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "tlk_malloc_stack.h"

static void *_sbrk(struct mem_arena_header *memHead, int incr)
{
    unsigned char *prev_brk;
    unsigned char *sbrkBase  = memHead->sbrkBase;
    unsigned char *sbrkLimit = memHead->sbrkLimit;

    if (incr < 0) {
        /* Returning memory to the heap. */
        incr = -incr;
        if (memHead->brk - incr < sbrkBase) {
            prev_brk = (unsigned char *)-1;
        } else {
            prev_brk = memHead->brk;
            memHead->brk -= incr;
        }
    } else {
        /* Allocating memory from the heap. */
        if (sbrkLimit - memHead->brk >= incr) {
            prev_brk = memHead->brk;
            memHead->brk += incr;
        } else {
            prev_brk = (unsigned char *)-1;
        }
    }

    return prev_brk;
}

static inline void mark_block_dead(struct free_arena_header *ah)
{
#ifdef DEBUG_MALLOC
    ah->a.type = ARENA_TYPE_DEAD;
#else
    (void)ah;
#endif
}

static inline void remove_from_main_chain(struct free_arena_header *ah)
{
    struct free_arena_header *ap, *an;

    mark_block_dead(ah);

    ap         = ah->a.prev;
    an         = ah->a.next;
    ap->a.next = an;
    an->a.prev = ap;
}

static inline void remove_from_free_chain(struct free_arena_header *ah)
{
    struct free_arena_header *ap, *an;

    ap            = ah->prev_free;
    an            = ah->next_free;
    ap->next_free = an;
    an->prev_free = ap;
}

static inline void remove_from_chains(struct free_arena_header *ah)
{
    remove_from_free_chain(ah);
    remove_from_main_chain(ah);
}

static void *__malloc_from_block(struct free_arena_header *fp, size_t size)
{
    size_t                    fsize;
    struct free_arena_header *nfp, *na, *fpn, *fpp;

    fsize = fp->a.size;

    /* We need the 2* to account for the larger requirements of a
       free block */
    if (fsize >= size + 2 * sizeof(struct arena_header)) {
        /* Bigger block than required -- split block */
        nfp = (struct free_arena_header *)((char *)fp + size);
        na  = fp->a.next;

        nfp->a.type = ARENA_TYPE_FREE;
        nfp->a.size = fsize - size;
        fp->a.type  = ARENA_TYPE_USED;
        fp->a.size  = size;

        /* Insert into all-block chain */
        nfp->a.prev = fp;
        nfp->a.next = na;
        na->a.prev  = nfp;
        fp->a.next  = nfp;

        /* Replace current block on free chain */
        nfp->next_free = fpn = fp->next_free;
        nfp->prev_free = fpp = fp->prev_free;
        fpn->prev_free       = nfp;
        fpp->next_free       = nfp;
    } else {
        fp->a.type = ARENA_TYPE_USED; /* Allocate the whole block */
        remove_from_free_chain(fp);
    }

    return (void *)(&fp->a + 1);
}

static struct free_arena_header *__free_block(struct free_arena_header *head, struct free_arena_header *ah)
{
    struct free_arena_header *pah, *nah;

    pah = ah->a.prev;
    nah = ah->a.next;
    if (pah->a.type == ARENA_TYPE_FREE &&
        (char *)pah + pah->a.size == (char *)ah) {
        /* Coalesce into the previous block */
        pah->a.size += ah->a.size;
        pah->a.next = nah;
        nah->a.prev = pah;
        mark_block_dead(ah);

        ah  = pah;
        pah = ah->a.prev;
    } else {
        /* Need to add this block to the free chain */
        ah->a.type               = ARENA_TYPE_FREE;
        ah->next_free            = head->next_free;
        ah->prev_free            = head;
        head->next_free          = ah;
        ah->next_free->prev_free = ah;
    }

    /* In either of the previous cases, we might be able to merge
       with the subsequent block... */
    if (nah->a.type == ARENA_TYPE_FREE &&
        (char *)ah + ah->a.size == (char *)nah) {
        ah->a.size += nah->a.size;

        /* Remove the old block from the chains */
        remove_from_chains(nah);
    }

    /* Return the block that contains the called block */
    return ah;
}

/* Call this to give malloc some memory to allocate from */
static void add_malloc_block(struct free_arena_header *head, void *buf, size_t size)
{
    struct free_arena_header *fp = buf;
    struct free_arena_header *pah;

    if (size < sizeof(struct free_arena_header)) {
        return; // Too small.
    }

    /* Insert the block into the management chains.  We need to set
       up the size and the main block list pointer, the rest of
       the work is logically identical to free(). */
    fp->a.type = ARENA_TYPE_FREE;
    fp->a.size = size;

    /* We need to insert this into the main block list in the proper
       place -- this list is required to be sorted.  Since we most likely
       get memory assignments in ascending order, search backwards for
       the proper place. */
    for (pah = head->a.prev; pah->a.type != ARENA_TYPE_HEAD;
         pah = pah->a.prev) {
        if (pah < fp) {
            break;
        }
    }

    /* Now pah points to the node that should be the predecessor of
       the new node */
    fp->a.next         = pah->a.next;
    fp->a.prev         = pah;
    pah->a.next        = fp;
    fp->a.next->a.prev = fp;

    /* Insert into the free chain and coalesce with adjacent blocks */
    fp = __free_block(head, fp);
}

static void *tlk_malloc(struct mem_arena_header *memHead, size_t size)
{
    struct free_arena_header *fp;
    void                     *more_mem;

    if (size == 0 || size > (SIZE_MAX - sizeof(struct arena_header))) {
        //printf("malloc failed 0:%d\n", size);
        return NULL;
    }

    struct free_arena_header *head = &memHead->a;

    /* Add the obligatory arena header, and round up */
    //    size = (size + 2 * sizeof(struct arena_header) - 1) & ARENA_SIZE_MASK;
    size = (size + sizeof(struct arena_header) + 3) & (~3);
    size = size > sizeof(struct free_arena_header) ? size : sizeof(struct free_arena_header); //must need

    void *result = NULL;
retry_alloc:
    for (fp = head->next_free; fp->a.type != ARENA_TYPE_HEAD;
         fp = fp->next_free) {
        if (fp->a.size >= size) {
            /* Found fit -- allocate out of this block */
            result = __malloc_from_block(fp, size);
            break;
        }
    }

    if (result == NULL) {
        more_mem = _sbrk(memHead, size);
        if (more_mem != (void *)-1) {
            add_malloc_block(head, more_mem, size);
            ////printf("malloc failed: retry_alloc\n");
            goto retry_alloc;
        } else {
            //printf("malloc failed 2:%d\n", size);
        }
    } else {
        //printf("malloc[0x%p]:%d\n", result, (int)size);
    }

    return result;
}

static void tlk_free(struct mem_arena_header *memHead, void *ptr)
{
    struct free_arena_header *ah;

    if (!ptr) {
        //printf("free failed 0\n");
        return;
    }

    ah = (struct free_arena_header *)((struct arena_header *)ptr - 1);

#ifdef DEBUG_MALLOC
    assert(ah->a.type == ARENA_TYPE_USED);
#endif

    struct free_arena_header *head = &memHead->a;

    //printf("free[0x%p]\n", ptr);

    /* Merge into adjacent free blocks */
    ah = __free_block(head, ah);
}

static void *tlk_realloc(struct mem_arena_header *memHead, void *ptr, size_t size)
{
    struct free_arena_header *ah;
    void                     *newptr;
    size_t                    oldsize;

    if (!ptr) {
        return tlk_malloc(memHead, size);
    }

    if (size == 0 || size > (SIZE_MAX - sizeof(struct arena_header))) {
        tlk_free(memHead, ptr);
        return NULL;
    }

    /* Add the obligatory arena header, and round up */
    //  size = (size + 2 * sizeof(struct arena_header) - 1) & ARENA_SIZE_MASK;
    size = (size + sizeof(struct arena_header) + 3) & (~3);
    size = size > sizeof(struct free_arena_header) ? size : sizeof(struct free_arena_header); //must need

    ah = (struct free_arena_header *)((struct arena_header *)ptr - 1);

    if (ah->a.size >= size && size >= (ah->a.size >> 2)) {
        /* This field is a good size already. */
        return ptr;
    } else {
        /* Make me a new block.  This is kind of bogus; we should
        be checking the following block to see if we can do an
        in-place adjustment... fix that later. */

        oldsize = ah->a.size - sizeof(struct arena_header);

        newptr = tlk_malloc(memHead, size);

        if (newptr) {
            memcpy(newptr, ptr, (size < oldsize) ? size : oldsize);
        }

        tlk_free(memHead, ptr);

        return newptr;
    }
}


__attribute__((section(".retention_data"))) static struct mem_arena_header __retentionHeader = {
    .a = {
          {
            ARENA_TYPE_HEAD,
            0,
            &__retentionHeader.a,
            &__retentionHeader.a,
        },
          &__retentionHeader.a,
          &__retentionHeader.a,
          },
};

__attribute__((section(".retention_data"))) static struct mem_arena_header __nonRetentionHeader = {
    .a = {
          {
            ARENA_TYPE_HEAD,
            0,
            &__nonRetentionHeader.a,
            &__nonRetentionHeader.a,
        },
          &__nonRetentionHeader.a,
          &__nonRetentionHeader.a,
          },
};

#ifdef MCU_CORE_D25F_ENABLE
void tlk_malloc_init(void *base, size_t size)
{
    struct mem_arena_header *memHead = base;
    memHead->a.a.type                = ARENA_TYPE_HEAD;
    memHead->a.a.size                = 0;
    memHead->a.a.prev = memHead->a.a.next = (struct free_arena_header *)&memHead->a.a;
    memHead->a.next_free                  = &memHead->a;
    memHead->a.prev_free                  = &memHead->a;
    memHead->sbrkBase                     = (unsigned char *)base + sizeof(struct mem_arena_header);
    memHead->sbrkLimit                    = (unsigned char *)base + size - 1;
    memHead->brk                          = (unsigned char *)memHead->sbrkBase;
}

void tlk_malloc_deinit(void *base)
{
    (void)base;
}

void *tlk_malloc_buffer(void *base, size_t size, unsigned short usedType)
{
    (void)usedType;
    return tlk_malloc(base, size); //todo: xh
}

void tlk_free_buffer(void *base, void *ptr)
{
    return tlk_free(base, ptr);
}
#endif

void tlk_initialRetentionBuffer(void *base, size_t size)
{
    __retentionHeader.sbrkBase  = (unsigned char *)base;
    __retentionHeader.sbrkLimit = (unsigned char *)base + size - 1;
    __retentionHeader.brk       = (unsigned char *)base;
}

void tlk_initialNonRetentionBuffer(void *base, size_t size)
{
    __nonRetentionHeader.sbrkBase  = (unsigned char *)base;
    __nonRetentionHeader.sbrkLimit = (unsigned char *)base + size - 1;
    __nonRetentionHeader.brk       = (unsigned char *)base;
}

void *malloc_reten(size_t size)
{
    return tlk_malloc(&__retentionHeader, size);
}

void free_reten(void *ptr)
{
    return tlk_free(&__retentionHeader, ptr);
}

void *realloc_reten(void *ptr, size_t size)
{
    return tlk_realloc(&__retentionHeader, ptr, size);
}

void *malloc_nonreten(size_t size)
{
    return tlk_malloc(&__nonRetentionHeader, size);
}

void free_nonreten(void *ptr)
{
    return tlk_free(&__nonRetentionHeader, ptr);
}

void *realloc_nonreten(void *ptr, size_t size)
{
    return tlk_realloc(&__nonRetentionHeader, ptr, size);
}
