/*
 * Copyright (c) 2020, Armink, <armink.ztl@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <fal.h>
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#define TAG_DATA_FLASH_SIZE 1024*8
static int init(void)
{
    /* do nothing now */
    return 1;
}


//static int ef_err_port_cnt = 0;
int on_ic_read_cnt  = 0;
int on_ic_write_cnt = 0;

static  u32 data_flash_addr = CFG_ADR_TAG_2M_FLASH;
void feed_dog(void)
{

}



static int read(long offset, uint8_t *buf, size_t size)
{
// tlk_printf("read 0x%4x,%d\r\n",offset,size);
flash_read_page(data_flash_addr + offset, size, buf);
// tlkapi_send_string_data(APP_LOG_EN, " read flash ", buf, size);
    return size;
}

uint8_t test_buf_0[256];

static int write(long offset, const uint8_t *buf, size_t size)
{
// tlk_printf("write 0x%4x,%d\r\n",offset,size);
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_APP_TAG_INFO_BEGIN, data_flash_addr, data_flash_addr + TAG_DATA_FLASH_SIZE );
    }
#endif
//    if(size == 1 && buf[0] == 3)
//    {
//        tlk_printf("write crc addr1 \r\n",offset);
//    }
//      tlkapi_send_string_data(APP_LOG_EN, " write flash ", buf, size);
     flash_write_page(data_flash_addr + offset, size, buf);
    if(size == 1 && buf[0] == 3)
    {
        tlk_printf("write crc addr \r\n",offset);
    }
     if(size <= 256 )
     {
         flash_read_page(data_flash_addr + offset, size, test_buf_0);

     if(memcmp(test_buf_0,buf,size))
     {
         tlk_printf("write fail\r\n",offset,size);
         tlkapi_send_string_data(APP_LOG_EN, " src ", buf, size);
         tlkapi_send_string_data(APP_LOG_EN, " dst ", test_buf_0, size);
     }
      }
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb)  {
        flash_prot_op_cb(FLASH_OP_EVT_APP_TAG_INFO_END, 0, 0);
    }
#endif
    on_ic_write_cnt++;
    return size;
}


static int erase(long offset, size_t size)
{
    uint32_t addr = data_flash_addr + offset;
    tlk_printf("erase 0x%4x + 0x%4x,%d\r\n",data_flash_addr ,offset,size);
    int sec_num = (size + ((1L<<12) -1)) >> 12;
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb) {
        flash_prot_op_cb(FLASH_OP_EVT_APP_TAG_INFO_BEGIN, data_flash_addr, data_flash_addr + TAG_DATA_FLASH_SIZE);
    }
#endif
    for(int i = 0; i < sec_num ;i++)
    {
        flash_erase_sector(addr + i * 0x1000);
    }
#if (STACK_SUPPORT_FLASH_PROTECTION_ENABLE)
    if (flash_prot_op_cb ) {
        flash_prot_op_cb(FLASH_OP_EVT_APP_TAG_INFO_END, 0, 0);
    }
#endif


    return size;
}

const struct fal_flash_dev telink_onchip_flash =
{
    .name       = "telink_onchip",
    .addr       = CFG_ADR_TAG_2M_FLASH,
    .len        = TAG_DATA_FLASH_SIZE,
    .blk_size   = 4*1024,
    .ops        = {init, read, write, erase},
    .write_gran = 1
};



