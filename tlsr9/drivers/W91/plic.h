/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

/********************************************************************************************************
 * @file	plic.h
 *
 * @brief	This is the header file for W91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/

#ifndef  INTERRUPT_H
#define  INTERRUPT_H

#include <stdint.h>
#include "core.h"

#define NDS_PLIC_BASE                                      0xe4000000
#define NDS_PLIC_SW_BASE                                   0xe6400000

#define PLIC_THRESHOLD_OFFSET                              0x00200000
#define PLIC_THRESHOLD_SHIFT_PER_TARGET                    12
#define PLIC_PRIORITY_OFFSET                               0x00000000
#define PLIC_PRIORITY_SHIFT_PER_SOURCE                     2
#define PLIC_ENABLE_OFFSET                                 0x00002000
#define PLIC_ENABLE_SHIFT_PER_TARGET                       7
#define PLIC_PENDING_OFFSET                                0x00001000
#define PLIC_CLAIM_OFFSET                                  0x00200004
#define PLIC_CLAIM_SHIFT_PER_TARGET                        12

static inline void plic_set_feature(uint32_t feature)
{
	volatile uint32_t *feature_ptr = (volatile uint32_t *)NDS_PLIC_BASE;
	*feature_ptr = feature;
}

static inline void plic_set_threshold(uint32_t threshold)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *threshold_ptr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_THRESHOLD_OFFSET + (hart_id << PLIC_THRESHOLD_SHIFT_PER_TARGET));
	*threshold_ptr = threshold;
}

static inline void plic_set_priority(uint32_t source, uint32_t priority)
{
	volatile uint32_t *priority_ptr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_PRIORITY_OFFSET + (source << PLIC_PRIORITY_SHIFT_PER_SOURCE));
	*priority_ptr = priority;
}

static inline void plic_set_pending(uint32_t source)
{
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_PENDING_OFFSET + ((source >> 5) << 2));
	*current_ptr = (1 << (source & 0x1F));
}

static inline void plic_interrupt_enable(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_ENABLE_OFFSET + (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
	uint32_t current = *current_ptr;
	current = current | (1 << (source & 0x1F));
	*current_ptr = current;
}

static inline void plic_interrupt_disable(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_ENABLE_OFFSET + (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
	uint32_t current = *current_ptr;
	current = current & ~((1 << (source & 0x1F)));
	*current_ptr = current;
}

static inline uint32_t plic_interrupt_claim(void)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *claim_addr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_CLAIM_OFFSET + (hart_id << PLIC_CLAIM_SHIFT_PER_TARGET));
	return  *claim_addr;
}

static inline void plic_interrupt_complete(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *claim_addr = (volatile uint32_t *)(NDS_PLIC_BASE +
			PLIC_CLAIM_OFFSET + (hart_id << PLIC_CLAIM_SHIFT_PER_TARGET));
	*claim_addr = source;
}

static inline void plic_sw_set_threshold(uint32_t threshold)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *threshold_ptr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_THRESHOLD_OFFSET + (hart_id << PLIC_THRESHOLD_SHIFT_PER_TARGET));
	*threshold_ptr = threshold;
}

static inline void plic_sw_set_priority(uint32_t source, uint32_t priority)
{
	volatile uint32_t *priority_ptr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_PRIORITY_OFFSET + (source << PLIC_PRIORITY_SHIFT_PER_SOURCE));
	*priority_ptr = priority;
}

static inline void plic_sw_interrupt_enable(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_ENABLE_OFFSET + (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
	uint32_t current = *current_ptr;
	current = current | (1 << (source & 0x1F));
	*current_ptr = current;
}

static inline void plic_sw_interrupt_disable(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_ENABLE_OFFSET + (hart_id << PLIC_ENABLE_SHIFT_PER_TARGET) + ((source >> 5) << 2));
	uint32_t current = *current_ptr;
	current = current & ~((1 << (source & 0x1F)));
	*current_ptr = current;
}

static inline void plic_sw_set_pending(uint32_t source)
{
	volatile uint32_t *current_ptr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_PENDING_OFFSET + ((source >> 5) << 2));
	*current_ptr = (1 << (source & 0x1F));
}

static inline uint32_t plic_sw_claim_interrupt(void)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *claim_addr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_CLAIM_OFFSET + (hart_id << PLIC_CLAIM_SHIFT_PER_TARGET));
	return  *claim_addr;
}

static inline void plic_sw_complete_interrupt(uint32_t source)
{
	uint32_t hart_id;

	read_csr(hart_id, NDS_MHARTID);
	volatile uint32_t *claim_addr = (volatile uint32_t *)(NDS_PLIC_SW_BASE +
			PLIC_CLAIM_OFFSET + (hart_id << PLIC_CLAIM_SHIFT_PER_TARGET));
	*claim_addr = source;
}

#endif /* INTERRUPT_H */
