/********************************************************************************************************
 * @file    ble.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "ble_common.h"
#include "ble.h"
#include "tl_common.h"
#include "drivers.h"





#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) void tlk_mem_set(void *dest, int val, unsigned int len)
{
    unsigned char *ptr = dest;
    while (len-- > 0) {
        *ptr++ = val;
    }
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) void tlk_mem_cpy(void *pd, const void *ps, unsigned int len)
{
    const char *pi = ps;
    char *po = pd;
    while (len-- > 0) {
        *po++ = *pi++;
    }

}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) int tlk_mem_cmp(const void *m1, const void *m2, unsigned int len)
{
    const char *st1 = m1;
    const char *st2 = m2;

    while(len--){
        if(*st1 != *st2){
            return 1; //return (*st1 - *st2)
        }
        st1++;
        st2++;
    }
    return 0;
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) void tlk_mem_set4(void *dest, int val, unsigned int len)
{
    unsigned int *ptr = dest;
    len >>= 2;
    while(len--){
        *(ptr++) = val;
    }
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) void tlk_mem_cpy4(void *pd, const void *ps, unsigned int len)
{
    const int *pi = ps;
    int *po = pd;
    len >>= 2;
    while (len--) {
        *po++ = *pi++;
    }
}

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
__attribute__((noinline)) int tlk_mem_cmp4(const void *m1, const void *m2, unsigned int len)
{
    const int *st1 = m1;
    const int *st2 = m2;
    len >>= 2;
    while(len--){
        if(*st1 != *st2){
            return 1; //return (*st1 - *st2)
        }
        st1++;
        st2++;
    }
    return 0;
}

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2)
_attribute_ram_code_sec_
#endif
int tlk_strlen(const char *str)
{
    int count = 0;
    while(*str != '\0'){
        count ++;
        str ++;
        //if(count > 0x3FF) break;
    }

    return count;
}




#ifndef BLC_ZEPHYR_BLE_INTEGRATION /* 1ULL<<n  used RAMCODE Function version */
//https://elixir.bootlin.com/linux/v5.7-rc4/source/lib
_attribute_ram_code_ long long __lshrdi3(long long u, int b)
{
    union {
        struct {
            int low, high; //little-endian, TLK used,  int high, low; //big-endian
        }s;
        long long ll;
    } uu, w;
    int bm;

    if (b == 0)
        return u;

    uu.ll = u;
    bm = 32 - b;

    if (bm <= 0) {
        w.s.high = 0;
        w.s.low = (unsigned int) uu.s.high >> -bm;
    } else {
        unsigned int carries = (unsigned int) uu.s.high << bm;

        w.s.high = (unsigned int) uu.s.high >> b;
        w.s.low = ((unsigned int) uu.s.low >> b) | carries;
    }

    return w.ll;
}

_attribute_ram_code_ long long __ashldi3(long long u, int b)
{
    union {
        struct {
            int low, high; //little-endian, TLK used,  int high, low; //big-endian
        }s;
        long long ll;
    } uu, w;

    int bm;

    if(b == 0){
        return u;
    }

    uu.ll = u;
    bm = 32 - b;

    if(bm <= 0){
        w.s.low = 0;
        w.s.high = (unsigned int) uu.s.low << -bm;
    }
    else{
        unsigned int carries = (unsigned int) uu.s.low >> bm;

        w.s.low = (unsigned int) uu.s.low << b;
        w.s.high = ((unsigned int) uu.s.high << b) | carries;
    }

    return w.ll;
}
#endif




_attribute_ble_data_retention_  unsigned short      crc16_poly[2] = {0, 0xa001}; //0x8005 <==> 0xa001

_attribute_no_inline_  //for big OTA PDU, CRC calculate should be quick
unsigned short blt_Crc16ComputeInternal(unsigned char *pD, int len)
{
    unsigned short crc = 0xffff;
    //unsigned char ds;
    int i,j;

    for(j=len; j>0; j--)
    {
        unsigned char ds = *pD++;
        for(i=0; i<8; i++)
        {
            crc = (crc >> 1) ^ crc16_poly[(crc ^ ds ) & 1];
            ds = ds >> 1;
        }
    }

     return crc;
}


unsigned char blc_get_sdk_version(unsigned char *pbuf, unsigned char number)
{
    //The parameter "number" is not used now, it is left here because there was it and has been released.
    //In future, this "number" can be used for a specific purpose, such as if (number == 12) {do something, or return a vendor version}
    (void)number;   //clean warning

    //  struct version_format{
    //      char    CERTIFICATION_MARK;         //BLE 5.<certification_mark>
    //      char    SOFT_STRUCTURE;
    //      char    MAJOR_VERSION;
    //      char    MINOR_VERSION;
    //      char    PATCH;
    //      char    month[3];                   //In Eng, like Dec
    //
    //      char    date;                       //Showed in DEC
    //      char    year;
    //      char    hour;
    //      char    minute;
    //  };
    u8  version[] = {CERTIFICATION_MARK, SOFT_STRUCTURE, MAJOR_VERSION, MINOR_VERSION, PATCH_NUM, CUSTOM_MAJOR_VERSION, CUSTOM_MINOR_VERSION,
            __DATE__[0], __DATE__[1], __DATE__[2],                      //Month
            (( __DATE__[4] - 0x30 ) << 4) | (__DATE__[5] - 0x30),       //Date
            (( __DATE__[9] - 0x30 ) << 4) | (__DATE__[10] - 0x30),      //Year
            (( __TIME__[0] - 0x30 ) << 4) | (__TIME__[1] - 0x30),       //Hour
            (( __TIME__[3] - 0x30 ) << 4) | (__TIME__[4] - 0x30),       //Minute
            };

    memcpy(pbuf,version,sizeof(version));

    return sizeof(version);
}

