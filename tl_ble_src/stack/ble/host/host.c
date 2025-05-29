/********************************************************************************************************
 * @file    host.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"
#include "stack/ble/host/gatt/tlk_list_stack.h"

_attribute_ble_data_retention_ _attribute_aligned_(4) host_acl_ms_t blhAclms[LL_MAX_ACL_CONN_NUM]; //blt_host_acl_ms
_attribute_ble_data_retention_ _attribute_aligned_(4) host_acl_m_t blhAclm[LL_MAX_ACL_CEN_NUM];    //blt_host_acl_m
_attribute_ble_data_retention_ _attribute_aligned_(4) host_acl_s_t blhAcls[LL_MAX_ACL_PER_NUM];    //blt_host_acl_s


_attribute_ble_data_retention_ _attribute_aligned_(4) host_param_t blhPara = {
    .host_init_err = 0,
};

#define HOST_MALLOC(size) malloc_nonreten((size) + sizeof(hostAclConnCommonHeader_t))
#define HOST_FREE(ptr)    free_nonreten(ptr)


void blt_host_init(void *base, u32 size)
{
    tlk_initialNonRetentionBuffer(base, size);
}

typedef struct hostAclConnCommonHeader
{
    struct hostAclConnCommonHeader *pNext;
    u16                             connHandle;
    u16                             scid;
} hostAclConnCommonHeader_t;

void *blt_host_mallocAclConn(void *head, u16 connHandle, u16 scid, u16 len)
{
    if (blt_ll_isAclhdlInvalid(connHandle) || head == NULL) {
        return NULL;
    }

    struct single_list_node *cur = NULL;
    SLIST_FOREACH(cur, ((struct single_list *)head), next)
    {
        hostAclConnCommonHeader_t *node = (hostAclConnCommonHeader_t *)cur;
        if (node->connHandle == connHandle && node->scid == scid) {
            return node + 1;
        }
    }

    hostAclConnCommonHeader_t *newNode = HOST_MALLOC(len);

    if (newNode == NULL) {
        return NULL;
    }

    newNode->connHandle = connHandle;
    newNode->scid       = scid;

    SLIST_INSERT_NODE_HEAD(head, newNode);

    return newNode + 1;
}

void *blt_host_getAclConn(void *head, u16 connHandle, u16 scid)
{
    if (head == NULL) {
        return NULL;
    }

    struct single_list_node *cur = NULL;
    SLIST_FOREACH(cur, ((struct single_list *)head), next)
    {
        hostAclConnCommonHeader_t *node = (hostAclConnCommonHeader_t *)cur;
        if (node->connHandle == connHandle && node->scid == scid) {
            return node + 1;
        }
    }

    return NULL;
}

//void blt_host_freeAclConn(void* head, u16 connHandle, u16 scid)
//{
//  struct single_list_node *cur = NULL;
//  hostAclConnCommonHeader_t* node = NULL;
//  SLIST_FOREACH(cur, ((struct single_list*)head), next) {
//      node = (hostAclConnCommonHeader_t*)cur;
//      if(node->connHandle == connHandle && node->scid == scid)
//      {
//          SLIST_DELETE_NODE(head, cur);
//          HOST_FREE(node);
//          break;
//      }
//  }
//}

void blt_host_freeAclConn(void *head, void *node)
{
    if (head == NULL || node == NULL) {
        return;
    }

    hostAclConnCommonHeader_t *cur = (hostAclConnCommonHeader_t *)node;

    cur -= 1;
    SLIST_DELETE_NODE(head, cur);
    HOST_FREE(cur);
}

init_err_t blc_host_checkHostInitialization(void)
{
    return blhPara.host_init_err;
}
