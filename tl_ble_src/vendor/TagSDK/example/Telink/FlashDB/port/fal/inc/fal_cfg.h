/*
 * 
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#define NOR_FLASH_DEV_NAME             "telink_onchip"
#define FAL_PART_HAS_TABLE_CFG
/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev telink_onchip_flash;
//extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &telink_onchip_flash,                                                     \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                                               \
{                                                                                    \
    {FAL_PART_MAGIC_WORD, "smarttag", NOR_FLASH_DEV_NAME,         0, 8*1024, 0},  \
}
#endif /* FAL_PART_HAS_TABLE_CFG */



void * pvPortMalloc( unsigned int  xWantedSize );
void vPortFree( void * pv );

#define FAL_MALLOC                     pvPortMalloc
//#define FAL_REALLOC                    realloc_nonreten
#define FAL_FREE                       vPortFree


#endif /* _FAL_CFG_H_ */
