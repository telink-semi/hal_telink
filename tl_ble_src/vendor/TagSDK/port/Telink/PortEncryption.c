/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 * This file uses some APIs that were provided by mbed TLS (https://tls.mbed.org)
 *
 ****************************************************************************/

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_mem.h"
#include "TagConfig.h"
#include "TagErrorType.h"
#include "PortSleep.h"
//#include "mbedtls/platform.h"

#ifdef FILE_LOG_LEVEL
#undef FILE_LOG_LEVEL
#endif
#define FILE_LOG_LEVEL FILE_LOG_LEVEL_DEBUG
#define  UNUSEDARG(x)  ((void )x);
#define PORT_AES128_BLOCK_LEN 16
void* TagMalloc(size_t size);
void TagFree(void *ptr);
int mbedtls_platform_set_calloc_free( void * (*calloc_func)( size_t, size_t ),
                              void (*free_func)( void * ) );



static void *PortMbedtlsCalloc(size_t n, size_t size)
{

    //DBG_CHN7_HIGH;

   #if 0
    return pvPortMalloc(n * size);
   #else
    void *ptr = NULL;
    u32 r = irq_disable();
    ptr = app_malloc_nonreten(n * size);
    if(ptr == NULL){
        DBG_CHN7_TOGGLE;
        DBG_CHN7_TOGGLE;
    }
    irq_restore(r);
    if (ptr) {
       memset(ptr, 0, n * size);
    }
//    DBG_CHN7_LOW;
    return ptr;
   #endif
}

static void PortMbedtlsFree(void *ptr)
{
//    DBG_CHN8_HIGH;

#if 0
    vPortFree(ptr);
#else
    u32 r = irq_disable();
    app_free_nonreten(ptr);
    irq_restore(r);
//    DBG_CHN8_LOW;
#endif
}

TagError_t PortKeyEncrypt(unsigned char *inputBuf, size_t inputLen, unsigned char **outputBuf, size_t *outputLen)
{
    // TODO
    // This is simple memory copy to show how the CONFIG_USE_KEY_NV_ITEM_ENCRYPTION option works.
    // This function should be implemented for each devices to perform encryption.
    if (!inputBuf || !outputBuf)
    {
        return TAG_ERROR_INVALID_ARG;
    }
    *outputBuf = PortMbedtlsCalloc(1,inputLen);
    if (!*outputBuf)
    {
        return TAG_ERROR_MEM_ALLOC;
    }
    memcpy(*outputBuf, inputBuf, inputLen);
    *outputLen = inputLen;

    return TAG_ERROR_NONE;
}

TagError_t PortKeyDecrypt(unsigned char *inputBuf, size_t inputLen, unsigned char **outputBuf, size_t *outputLen)
{
    // TODO
    // This is simple memory copy to show how the CONFIG_USE_KEY_NV_ITEM_ENCRYPTION option works.
    // This function should be implemented for each devices to perform decryption.
    if (!inputBuf || !outputBuf) {
        return TAG_ERROR_INVALID_ARG;
    }
    *outputBuf = PortMbedtlsCalloc(1,inputLen);
    if (!*outputBuf) {
        return TAG_ERROR_MEM_ALLOC;
    }
    memcpy(*outputBuf, inputBuf, inputLen);
    *outputLen = inputLen;

    return TAG_ERROR_NONE;
}

TagError_t PortEncryptionInit(void)
{
    mbedtls_platform_set_calloc_free(&PortMbedtlsCalloc, &PortMbedtlsFree);
    return TAG_ERROR_NONE;
}
#include <mbedtls/entropy.h>

int mbedtls_hardware_poll(void *data,
                          unsigned char *output,
                          size_t len,
                          size_t *olen)
{
    (void)data;

    if (output == NULL)
    {
        return -1;
    }

    if (olen == NULL)
    {
        return -1;
    }

    if (len == 0)
    {
        return -1;
    }
    generateRandomNum(len,output);
    *olen = len;
    return 0;

}
