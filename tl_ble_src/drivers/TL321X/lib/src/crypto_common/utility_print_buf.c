/********************************************************************************************************
 * @file    utility_print_buf.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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

#include "lib/include/crypto_common/utility.h"


#ifdef UTILITY_PRINT_BUF
//#include "xil_printf.h"


void print_buf_U8(const unsigned char *buf, unsigned int byteLen, char *name)
{
    unsigned int i;

    if(NULL != buf)
    {
        (void)printf("\r\n %s: %08x\r\n  ",name, (unsigned int)buf); //fflush(stdout);
        for(i=0U; i<byteLen; i++)
        {
            //if(i%16 ==0 && i>0)
            //    (void)printf("\r\n");
            //(void)printf("%02x", buf[byteLen-1-i]);
            (void)printf("%02x", buf[i]);
        }

        (void)printf("\r\n");
    }
}

void print_buf_U32(const unsigned int *buf, unsigned int wordLen, char *name)
{
    unsigned int i;

    if(NULL != buf)
    {
        (void)printf("\r\n %s: %08x\r\n",name, (unsigned int)buf);//fflush(stdout);
        for(i=0U; i<wordLen; i++)
        {
            //if(i%16 ==0 && i>0)
            //    (void)printf("\r\n");
            //(void)printf("%08x", buf[wordLen-1-i]);
            (void)printf("%08x", buf[i]);//fflush(stdout);
        }

        (void)printf("\r\n");//fflush(stdout);
    }
}

void print_BN_buf_U32(const unsigned int *buf, unsigned int wordLen, char *name)
{
    unsigned int i;

    if(NULL != buf)
    {
        (void)printf("\r\n %08x %s: ", (unsigned int)buf, name);//fflush(stdout);
        for(i=0U; i<wordLen; i++)
        {
            //if(i%16 ==0 && i>0)
            //    (void)printf("\r\n");
            (void)printf("%08x", buf[wordLen-1U-i]);
        }
        (void)printf("\r\n");//fflush(stdout);
    }
}
#endif

