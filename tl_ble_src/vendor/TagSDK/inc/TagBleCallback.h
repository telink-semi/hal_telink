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
 ****************************************************************************/

#ifndef TAGSDK_INC_TAGBLECALLBACK_H_
#define TAGSDK_INC_TAGBLECALLBACK_H_

#include "TagCore.h"
#include "PortBleDataType.h"

typedef enum
{
    BleConnected,
    BleDisconnected,
    BleConnectionParameterUpdated,
    BleHandleValueConfirmation,
    BleAttributeWritten,
    BleAttributeRead,
    BleConfirmationError,
    BleBondingStatus,
#ifdef TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
    BleNotificationSent,
#endif
    EndOfBleEvent,
} BleEventType;

typedef enum
{
    BONDING_STATUS_REQUEST,
    BONDING_STATUS_SUCCESS_BOND,
    BONDING_STATUS_FAIL_BOND,
} BondingStatus;

typedef enum
{
    CON_PARAMS_UPDATE_SUCCESS,
    CON_PARAMS_UPDATE_FAIL,
} ConParamsUpdateStatus;

typedef struct
{
    PortBleConnInfo portConnHandle;
    BondingStatus bondingStatus;
} BleBondingStatusData;

typedef struct
{
    PortBleConnInfo portConnHandle;
    uint8_t isBond;
} BleConnectionData;

typedef struct
{
    PortBleConnInfo portConnHandle;
    uint16_t connInterval;
    uint16_t connLatency;
    uint16_t supervisionTimeout;
    ConParamsUpdateStatus status;
} BleConnectionParamsData;

typedef struct
{
    PortBleConnInfo portConnHandle;
    PortBleAttrInfo portAttrInfo;
    uint8_t charIndex;
    bool needResponse;
    void *value;
    size_t valueLength;
} BleGattData;

typedef struct
{
    BleEventType eventType;
    union
    {
        BleConnectionData connectionData;
        BleBondingStatusData bondingStatusData;
        BleConnectionData disconnectionData;
        BleConnectionParamsData paramsData;
        BleGattData gattData;
    } eventData;
} BleEvent;

TagBleError_t TagBleCallback(BleEvent *event);

#endif /* TAGSDK_INC_TAGBLECALLBACK_H_ */
