/********************************************************************************************************
 * @file    app_nv.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2025
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include <flashdb.h>


//static uint32_t boot_count = 0;
//static time_t boot_time[10] = {0, 1, 2, 3};
/* default KV nodes */
//static struct fdb_default_kv_node default_kv_table[] = {
//
//    {"boot_count", &boot_count, sizeof(boot_count)}, /* int type KV */
//    {"boot_time", &boot_time, sizeof(boot_time)},    /* int array type KV */
//};
/* KVDB object */
_attribute_ble_data_retention_ static struct fdb_kvdb kvdb = {0};


void app_tag_stoage_init(void)
{
#ifdef FDB_USING_KVDB
    { /* KVDB Sample */

//        struct fdb_default_kv default_kv;
//        default_kv.kvs = default_kv_table;
//        default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);

        /* set the lock and unlock function if you want */
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, NULL);
        fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, NULL);
        /* Key-Value database initialization
         *
         *       &kvdb: database object
         *       "env": database name
         * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
         *              Please change to YOUR partition name.
         * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
         *        NULL: The user data if you need, now is empty.
         */
//        fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "smarttag", &default_kv, NULL);
        fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "smarttag", NULL, NULL);
        if (result != FDB_NO_ERR)
        {
            tlk_printf("app_tag_stoage_init %d",result);
            return;
        }
//        void fal_show_part_table(void);
//        fal_show_part_table();
    }
#endif /* FDB_USING_KVDB */
}

typedef int idx_type;

static void int2str(idx_type idx, u8 * idx_str)
{
       idx_str[0] = (u8)(idx & 0xf) | 0x30;
       idx_str[1] = (u8)((idx>>4)&0xff) | 0x80;
       idx_str[2] = (u8)((idx>>4)&0xff) ;
       idx_str[3] = (u8)((idx>>8)&0xff) ;
       idx_str[4] = '\0';
}

int app_tag_stoage_set_data(int idx, u8* data, size_t len)
{
   fdb_err_t rt ;
   char idx_str[5] = {0};
   struct fdb_blob blob;
   int2str(idx,idx_str);
   rt = fdb_kv_set_blob(&kvdb, idx_str, fdb_blob_make(&blob, data, len));
   return rt;
}

int app_tag_stoage_get_data(int idx, u8* data, size_t len)
{
    char idx_str[5] = {0};
    struct fdb_blob blob;
    int2str(idx,idx_str);
    size_t read_len = fdb_kv_get_blob(&kvdb, idx_str, fdb_blob_make(&blob, data, len));
    return read_len;
}


int app_tag_stoage_del_data(int idx)
{
       char idx_str[5] = {0};
       int2str(idx,idx_str);
       return fdb_kv_del(&kvdb, idx_str);
}

void test_storage_fun2(void)
{
    u8 test_buf[256] = {0};
    int len = 0;
    if((len = app_tag_stoage_get_data(0,test_buf,256)) == 0)
    {
        tlk_printf("test:get item 0 of len 0 \r\n");
    }
    else tlk_printf("test:get item 0 of len %d \r\n",len);

    if((len = app_tag_stoage_get_data(1,test_buf,256)) == 0)
    {
        tlk_printf("test:get item 1 of len 0 \r\n");
    }
    else tlk_printf("test:get item 1 of len %d \r\n",len);
}

void test_storage_fun(void)
{
    u8 test_buf[256] = {0};
    int len = 0;
    if((len = app_tag_stoage_get_data(0,test_buf,256)) == 0)
    app_tag_stoage_set_data(0,test_buf,1);

    if((len = app_tag_stoage_get_data(1,test_buf,256)) == 0)
    app_tag_stoage_set_data(1,test_buf,1);

    if((len = app_tag_stoage_get_data(2,test_buf,256)) == 0)
    app_tag_stoage_set_data(2,test_buf,1);
    if((len = app_tag_stoage_get_data(6,test_buf,256)) == 0)
    app_tag_stoage_set_data(6,test_buf,1);
    if((len = app_tag_stoage_get_data(7,test_buf,256)) == 0)
    app_tag_stoage_set_data(7,test_buf,1);
    if((len = app_tag_stoage_get_data(9,test_buf,256)) == 0)
    app_tag_stoage_set_data(9,test_buf,1);


    if((len = app_tag_stoage_get_data(10,test_buf,256)) == 0)
    {
        tlk_printf("test:get item 10 of len 0 \r\n");
        test_buf[1]  = 1;
        test_buf[12]  = 33;
        test_buf[28]  = 77;
        if(app_tag_stoage_set_data(10,test_buf,1))
        {
            tlk_printf("test:set item 10 fail \r\n");
        }
        else
        {
            tlk_printf("test:set item 10 success\r\n");
        }
    }

    if(app_tag_stoage_get_data(10,test_buf,256) == 0)
    {
        tlk_printf("test:2get item 10 of len 0 \r\n");

    }
    else
    {
        tlkapi_send_string_data(APP_LOG_EN, " test:get ", test_buf, 128);
    }

    if(app_tag_stoage_get_data(11,test_buf,256) == 0)
    {
        tlk_printf("test:2get item 11 of len 0 \r\n");

        test_buf[1]  = 2;
        test_buf[12]  = 0x55;
        test_buf[32]  = 0x66;
        tlkapi_send_string_data(APP_LOG_EN, " pre:set11 ", test_buf, 128);
        if(app_tag_stoage_set_data(11,test_buf,1))
        {
            tlk_printf("test:set item 11 fail \r\n");
        }
        else
        {
            tlk_printf("test:set item 11 success\r\n");
        }


    }
    else
    {
        tlkapi_send_string_data(APP_LOG_EN, " test:get11 ", test_buf, 128);
    }

    len = app_tag_stoage_get_data(13,test_buf,256);
    if(len == 0)
    {
        tlk_printf("test:3get item 13 of len 0 \r\n");
         test_buf[1]  = 3;
         test_buf[32]  = 0x77;
         test_buf[63]  = 0x55;
         tlkapi_send_string_data(APP_LOG_EN, " pre:set13 ", test_buf, 128);
         if(app_tag_stoage_set_data(13,test_buf,2))
         {
             tlk_printf("test:set item 13 fail \r\n");
         }
         else
         {
             tlk_printf("test:set item 13 success\r\n");
         }
    }
    else
    {
        tlkapi_send_string_data(APP_LOG_EN, " test:get13", test_buf, len);
    }
}
