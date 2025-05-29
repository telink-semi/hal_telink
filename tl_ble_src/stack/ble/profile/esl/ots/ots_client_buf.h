/********************************************************************************************************
 * @file    ots_client_buf.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    10,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#pragma once

#define OTS_CLIENT_OBJECT_NAME_MAX_SIZE   64
#define OTS_CLIENT_OBJECT_TYPE_MAX_SIZE   16
#define OTS_CLIENT_OBJECT_FILTER_MAX_SIZE 64
#define OTS_OBJECT_FILTER_CHAR_NUM        3
#define OTS_CLIENT_UTC_MAX_SIZE           7

/* OTS characteristics read */
typedef enum
{
    BLT_OTSC_READ_OTS_FEATURE,
    BLT_OTSC_READ_OBJECT_NAME,
    BLT_OTSC_READ_OBJECT_TYPE,
    BLT_OTSC_READ_OBJECT_SIZE,
    BLT_OTSC_READ_OBJECT_FIRST_CREATED,
    BLT_OTSC_READ_OBJECT_LAST_MODIFIED,
    BLT_OTSC_READ_OBJECT_ID,
    BLT_OTSC_READ_OBJECT_PROPERTIES,
    BLT_OTSC_READ_OBJECT_LIST_FILTER_0,
    BLT_OTSC_READ_OBJECT_LIST_FILTER_1,
    BLT_OTSC_READ_OBJECT_LIST_FILTER_2,
    BLT_OTSC_READ_MAX,
} blt_otsc_read_t;

/* OTS characteristics write */
typedef enum
{
    BLT_OTSC_WRITE_OBJECT_NAME,
    BLT_OTSC_WRITE_OBJECT_FIRST_CREATED,
    BLT_OTSC_WRITE_OBJECT_LAST_MODIFIED,
    BLT_OTSC_WRITE_OBJECT_PROPERTIES,
    BLT_OTSC_WRITE_OBJECT_ACTION_CONTROL_POINT,
    BLT_OTSC_WRITE_OBJECT_LIST_CONTROL_POINT,
    BLT_OTSC_WRITE_OBJECT_LIST_FILTER_0,
    BLT_OTSC_WRITE_OBJECT_LIST_FILTER_1,
    BLT_OTSC_WRITE_OBJECT_LIST_FILTER_2,
    BLT_OTSC_WRITE_MAX,
} blt_otsc_write_t;

typedef enum
{
    BLT_OTSC_L2CAP_STATE_IDLE = 0,
    BLT_OTSC_L2CAP_STATE_CONNECTING,
    BLT_OTSC_L2CAP_STATE_CONNECTED,
} blt_otsc_l2cap_state_t;

typedef struct
{
    u16 len;
    u8  val[];
} otsClientCharValue_t;

typedef struct
{
    u16 len;
    u8  val[OTS_CLIENT_OBJECT_NAME_MAX_SIZE];
} otsClientObjectName_t;

typedef struct
{
    u16 len;
    u8  val[OTS_CLIENT_OBJECT_TYPE_MAX_SIZE];
} otsClientObjectType_t;

typedef struct
{
    u16 len;
    u8  val[OTS_CLIENT_OBJECT_FILTER_MAX_SIZE];
} otsClientObjectFilter_t;

typedef void (*blt_otsc_timer_cb_t)(void *data);

typedef struct
{
    u32                 tick;
    u8                  secRemaining;
    bool                active;
    void               *data;
    blt_otsc_timer_cb_t cb;
} blt_otsc_timer_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;

    u16 otsFeatureHdl;
    u16 otsObjectNameHdl;
    u16 otsObjectTypeHdl;
    u16 otsObjectSizeHdl;
    u16 otsObjectFirstCreatedHdl;
    u16 otsObjectLastModifiedHdl;
    u16 otsObjectIdHdl;
    u16 otsObjectPropertiesHdl;
    u16 otsObjectActionControlPointHdl;
    u16 otsObjectActionControlPointCccHdl;
    u16 otsObjectListControlPointHdl;
    u16 otsObjectListControlPointCccHdl;
    u16 otsObjectListFilterHdl[3];
    u16 otsObjectChangedHdl;
    u16 otsObjectChangedCccHdl;

    u8 otsObjectNameProperties;
    u8 otsObjectFirstCreatedProperties;
    u8 otsObjectLastModifiedProperties;
    u8 otsObjectPropertiesProperties;

    u8  otsFeature[8];
    u16 otsFeatureSize;
    u8  objectSize[8];
    u16 objectSizeLen;
    u8  objectFirstCreated[OTS_CLIENT_UTC_MAX_SIZE];
    u16 objectFirstCreatedLen;
    u8  objectLastModified[OTS_CLIENT_UTC_MAX_SIZE];
    u16 objectLastModifiedLen;
    u8  objectId[6];
    u16 objectIdLen;
    u8  objectProperties[4];
    u16 objectPropertiesLen;
    u8  objectListFilterCnt;
    u8  objectListFilterIdx;

    blt_otsc_timer_t oacpTimer;
    blt_otsc_timer_t olcpTimer;

    bool writeInProgress   : 1;
    bool oacpIndInProgress : 1;
    bool olcpIndInProgress : 1;

    blt_otsc_l2cap_state_t l2capState;
    u16                    scid;
    u16                    dcid;
    u16                    mtu;

    otsClientCharValue_t *objectName;
    otsClientCharValue_t *objectNameWrBuf;
    otsClientCharValue_t *objectType;
    otsClientCharValue_t *objectFilter[3];
    otsClientCharValue_t *objectFilterWrBuf[3];
    u8                    res[3];
} blc_otsc_t;

typedef struct
{
} blc_otsc_regParam_t;

typedef struct
{
    blc_prf_proc_t process;
    blc_otsc_t    *pOtsClient[STACK_PRF_ACL_CONN_MAX_NUM];
} blc_ots_client_ctrl_t;

/**
 * @brief           This function retrieves the client buffer for the OTS client.
 * @param[in]       index - The index of the client buffer.
 * @return          blc_otsc_t* - Pointer to the OTS client buffer.
 */
blc_otsc_t *blt_otsc_getClientBuf(u8 index);

/**
 * @brief           This function cleans the OTS client buffer.
 * @return          void
 */
void blt_otsc_cleanBuf(void);

/**
 * @brief           This function retrieves the Object Name buffer for the OTS client.
 * @param[in]       aclIdx - The ACL index.
 * @return          otsClientCharValue_t* - Pointer to the Object Name buffer.
 */
otsClientCharValue_t *blc_otsc_getObjectNameBuf(u8 aclIdx);

/**
 * @brief           This function retrieves the Object Type buffer for the OTS client.
 * @param[in]       aclIdx - The ACL index.
 * @return          otsClientCharValue_t* - Pointer to the Object Type buffer.
 */
otsClientCharValue_t *blc_otsc_getObjectTypeBuf(u8 aclIdx);

/**
 * @brief           This function retrieves the Object Filter buffer for the OTS client.
 * @param[in]       aclIdx - The ACL index.
 * @param[in]       idx - The filter index.
 * @return          otsClientCharValue_t* - Pointer to the Object Filter buffer.
 */
otsClientCharValue_t *blc_otsc_getObjectFilterBuf(u8 aclIdx, u8 idx);

/**
 * @brief           This function retrieves the Object Filter Write buffer for the OTS client.
 * @param[in]       aclIdx - The ACL index.
 * @param[in]       idx - The filter index.
 * @return          otsClientCharValue_t* - Pointer to the Object Filter Write buffer.
 */
otsClientCharValue_t *blc_otsc_getObjectFilterWrBuf(u8 aclIdx, u8 idx);

/**
 * @brief           This function retrieves the Object Name Write buffer for the OTS client.
 * @param[in]       aclIdx - The ACL index.
 * @return          otsClientCharValue_t* - Pointer to the Object Name Write buffer.
 */
otsClientCharValue_t *blc_otsc_getObjectNameWrBuf(u8 aclIdx);
