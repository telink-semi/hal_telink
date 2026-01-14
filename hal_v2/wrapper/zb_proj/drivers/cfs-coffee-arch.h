/********************************************************************************************************
 * @file    cfs-coffee-arch.h
 *
 * @brief   This is the header file for cfs-coffee-arch
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
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
#ifndef CFS_COFFEE_ARCH_H_
#define CFS_COFFEE_ARCH_H_


#include "cfs-coffee.h"
#include "drv_flash.h"


/** Logical sector size */
#define COFFEE_SECTOR_SIZE      4096

/** Logical page size */
#define COFFEE_PAGE_SIZE        256

/** Start offset of the file system */
#define COFFEE_START            0x30000

/** Total size in bytes of the file system */
#define COFFEE_SIZE             (1024 * 64)

/** Maximal filename length */
#define COFFEE_NAME_LENGTH      40

/** Number of file cache entries */
#define COFFEE_MAX_OPEN_FILES   5

/** Number of file descriptor entries */
#define COFFEE_FD_SET_SIZE      5

/** Maximal amount of log table entries read in one batch */
#define COFFEE_LOG_TABLE_LIMIT  16

/** Default reserved file size */
#define COFFEE_DYN_SIZE         (COFFEE_SECTOR_SIZE - 50)

/** Default micro-log size */
#define COFFEE_LOG_SIZE         (4 * COFFEE_PAGE_SIZE)

/** Whether Coffee will use micro logs */
#define COFFEE_MICRO_LOGS       1

/** Whether files are expected to be appended to only */
#define COFFEE_APPEND_ONLY      0


/** Erase */
#define COFFEE_ERASE(sector) \
    flash_erase(COFFEE_START + (sector) * COFFEE_SECTOR_SIZE)
/** Write */
#define COFFEE_WRITE(buf, size, offset) \
    cfs_flash_write(COFFEE_START + (offset), (size), (buf))
/** Read */
#define COFFEE_READ(buf, size, offset) \
    cfs_flash_read(COFFEE_START + (offset), (size), (buf))

#endif
