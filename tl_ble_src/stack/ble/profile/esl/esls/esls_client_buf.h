/********************************************************************************************************
 * @file    esls_client_buf.h
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

#define ESLS_CLIENT_LED_INFORMATION_MAX_SIZE     128
#define ESLS_CLIENT_SENSOR_INFORMATION_MAX_SIZE  128
#define ESLS_CLIENT_DISPLAY_INFORMATION_MAX_SIZE 128

/* ESLS characteristics read */
typedef enum
{
    BLT_ESLSC_READ_DISPLAY_INFORMATION,
    BLT_ESLSC_READ_SENSOR_INFORMATION,
    BLT_ESLSC_READ_IMAGE_INFORMATION,
    BLT_ESLSC_READ_LED_INFORMATION,
    BLT_ESLSC_READ_MAX,
} blt_eslsc_read_t;

/* ESLS characteristics write */
typedef enum
{
    BLT_ESLSC_WRITE_ESL_ADDRESS,
    BLT_ESLSC_WRITE_ESL_RESPONSE_KEY_MATERIAL,
    BLT_ESLSC_WRITE_AP_SYNC_KEY_MATERIAL,
    BLT_ESLSC_WRITE_CURRENT_ABSOLUTE_TIME,
    BLT_ESLSC_WRITE_CONTROL_POINT,
    BLT_ESLSC_WRITE_MAX,
} blt_eslsc_write_t;

typedef struct
{
    u16 len;
    u8  val[ESLS_CLIENT_LED_INFORMATION_MAX_SIZE];
} eslClientLEDInformation_t;

typedef struct
{
    u16 len;
    u8  val[ESLS_CLIENT_SENSOR_INFORMATION_MAX_SIZE];
} eslClientSensorInformation_t;

typedef struct
{
    u16 len;
    u8  val[ESLS_CLIENT_DISPLAY_INFORMATION_MAX_SIZE];
} eslClientDisplayInformation_t;

typedef struct
{
    u16 len;
    u8  val[];
} eslClientCharValue_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;
    u16                 eslAddressHdl;
    u16                 apSyncKetMaterialHdl;
    u16                 eslResponseKeyMaterialHdl;
    u16                 eslCurrentAbsoluteTimeHdl;
    u16                 eslDisplayInformationHdl;
    u16                 eslImageInformationHdl;
    u16                 eslSensorInformationHdl;
    u16                 eslLedInformationHdl;
    u16                 eslControlPointHdl;
    u16                 eslControlPointCccHdl;

    /* Service handle range */
    u16                   connHandle;
    u8                    eslImageInformation;
    eslClientCharValue_t *eslDisplayInformation;
    eslClientCharValue_t *eslSensorInformation;
    eslClientCharValue_t *eslLEDInformation;
    u8                    eslResponseKeyMaterialBuf[24];
    u8                    apSyncKeyMaterialBuf[24];
    u8                    res[3];
} blc_esls_client_t;

typedef struct
{
    blc_prf_proc_t     process;
    blc_esls_client_t *pEslsClient[STACK_PRF_ACL_CONN_MAX_NUM];
} blc_esls_client_ctrl_t;

typedef struct
{
} blc_eslsc_regParam_t;

/**
 * @brief      Get the client buffer for a given index in the ESL client structure.
 * @param[in]  index - The index of the client buffer to retrieve.
 * @return     blc_esls_client_t* - Pointer to the ESL client buffer at the specified index, or NULL if the index is invalid.
 */
blc_esls_client_t *blc_eslsc_getClientBuf(u8 index);

/**
 * @brief      Get the LED information buffer for a given ACL index.
 * @param[in]  aclIdx - The ACL index of the connection.
 * @return     eslClientCharValue_t* - Pointer to the LED information buffer, or NULL if the index is invalid.
 */
eslClientCharValue_t *blc_eslsc_getLedInformationBuf(u8 aclIdx);

/**
 * @brief      Get the sensor information buffer for a given ACL index.
 * @param[in]  aclIdx - The ACL index of the connection.
 * @return     eslClientCharValue_t* - Pointer to the sensor information buffer, or NULL if the index is invalid.
 */
eslClientCharValue_t *blc_eslsc_getSensorInformationBuf(u8 aclIdx);

/**
 * @brief      Get the display information buffer for a given ACL index.
 * @param[in]  aclIdx - The ACL index of the connection.
 * @return     eslClientCharValue_t* - Pointer to the display information buffer, or NULL if the index is invalid.
 */
eslClientCharValue_t *blc_eslsc_getDisplayInformationBuf(u8 aclIdx);
