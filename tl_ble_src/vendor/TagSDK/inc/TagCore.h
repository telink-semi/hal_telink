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

#ifndef TAGSDK_INC_TAGCORE_H_

#define TAGSDK_INC_TAGCORE_H_

#include <stdbool.h>

#include "PortOs.h"
//#include "PortTime.h"
#include "PortBleDataType.h"

#include "TagConfig.h"
#include "TagSdk.h"
#include "TagErrorType.h"
#include "TagBleCharacteristic.h"
#include "TagDebug.h"
#include "TagConstant.h"
#include "TagSecurityTypes.h"
#include "TagOnboardingService.h"

#ifdef TAG_CONFIG_TASK_PRIORITY
#define TAG_MAIN_TASK_PRIORITY   TAG_CONFIG_TASK_PRIORITY
#else
#define TAG_MAIN_TASK_PRIORITY   4
#endif
#define TAG_MAIN_TASK_STACK_SIZE 1024
#define TAG_MAIN_TASK_NAME       "TagMain"
#define TAG_TASK_EVENT_SIGNAL (1 << 0)
#define MAIN_TASK_QUEUE_SIZE    10

typedef void* TagTaskWorkParam;
typedef void (*TagTaskWorkHandler)(TagTaskWorkParam param);
typedef void (*TagBatteryUpdateCb)(void);

typedef struct _tagTaskWork {
    TagTaskWorkParam param;
    TagTaskWorkHandler handler;
    struct _tagTaskWork *next;
} TagTaskWork;

typedef enum {
    TAG_STATE_OUT_OF_BOX = 1,
    TAG_STATE_OOB_DEEP_SLEEP,
    TAG_STATE_CONNECTED,
    TAG_STATE_PREMATURE_OFFLINE,
    TAG_STATE_OFFLINE,
    TAG_STATE_OVERMATURE_OFFLINE,
} TagState;

typedef enum
{
    TAG_PSM_NORMAL,
    TAG_PSM_POWER_SAVE
} TagPowerSavingMode;

#define LIMIT_DEVICE_ID_VALUE            100000
typedef uint32_t TagBleDeviceId;

typedef enum {
    END_USER_DEVICE_UNKNOWN,
    END_USER_DEVICE_OWNER,
    END_USER_DEVICE_NON_OWNER,
} EndUserDeviceType;

typedef enum {
    TAG_BOOT_REASON_INIT,
    TAG_BOOT_REASON_NORMAL,
    TAG_BOOT_REASON_FACTORY_RESET,
    TAG_BOOT_REASON_SILENT_RESET,
    TAG_BOOT_REASON_FIRMWARE_UPDATE,
    TAG_BOOT_REASON_VENDOR_SPECIPIC_FIRMWARE_UPDATE,
    TAG_BOOT_REASON_ENTER_VENDOR_SPECIPIC_FIRMWARE_UPDATE_MODE,
    TAG_BOOT_REASON_RETURN_TO_NORMAL,
    TAG_BOOT_REASON_MAX,
} TagBootReason;

typedef struct _endUserDevice {
    TagBleDeviceId deviceId;
    PortBleConnInfo portConnHandle;
    PortTimerHandle_t connectionParmRecoveryTimer;
    PortTimerHandle_t initConnectionParmRecoveryTimer;
    PortTimerHandle_t connectionParmRetryTimer;
    PortTimerHandle_t paringRequestTimer;
    uint8_t isParing;
    uint8_t isPaired;
    uint16_t connectionParamIntervalMin;
    uint16_t connectionParamIntervalMax;
    uint16_t connectionParamTimeoutMultiplier;
    uint8_t connectionParamSlaveLatency;
    uint8_t connectionParamUpdateRetryCount;
    uint16_t lastConnectionParamIntervalMin;
    uint16_t lastConnectionParamIntervalMax;
    uint16_t lastConnectionParamTimeoutMultiplier;
    uint8_t lastConnectionParamSlaveLatency;
    PortTimerHandle_t unknownBleConnectionTimer;
    EndUserDeviceType deviceType;
    TagSecurityTagKeyType_t commandKeyType;
    TagSecurityAuthParams_t *authParam;
    uint8_t controlServiceIndicationProgress;
#ifdef TAG_CONFIG_USE_NOTIFICATION_FOR_DEBUG
    uint8_t controlServiceNotificationProgress;
#endif
    uint8_t sentConnectionReason;
    PortQueueHandle_t indicationQueue;
    uint8_t logging;
    TagLogParam_t *logParam;
    unsigned int seqNoE;
    unsigned int seqNoT;
    struct _endUserDevice *next;
#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
    uint32_t uwbSessionId;
#endif
} EndUserDevice;

#define TAG_ONBOARDED_VALUE_ONBOARDED       (0x01)
#define TAG_ONBOARDED_VALUE_NOT_ONBOARDED   (0x00)

typedef struct {
    uint8_t initialized;
    PortTaskHandle_t mainTaskHandle;
    uint8_t mainTaskSoftKillSignal;
    PortEventGroupHandle_t eventGroup;
    TagTaskWork *taskWorkHead, *taskWorkTail;

    /* State Management */
    TagState state;
    PortTimerHandle_t prematureTimer;
    PortTimerHandle_t offlineTimer;

    unsigned int agingCounter;
    uint8_t regionID;
    uint8_t E2EEFlag;
    uint8_t batteryLevel;
    uint32_t numberOfPrivacyId;
    size_t privacyIdLength;
    uint8_t privacyId[8];
    uint32_t privacyIdLastUpdateCounter;
    uint8_t maxAllowedConnections;
    uint8_t currentConnections;
    uint8_t nonOwnerConnections;
    uint8_t pairedConnection;
    uint8_t pushButtonEnabled;
    uint8_t holdButtonEnabled;
    uint8_t overmaturePrivacyIdInterval;
    uint32_t prematureOfflineTimeout;
    uint32_t offlineTimeout;
    uint8_t needAdvAddrChanged;
    uint8_t advParamUpdated;
    EndUserDevice *endUserDevices;
    PortTimerHandle_t agingCounterTimer;
    PortTimerHandle_t OOBCompletedTimer;
    uint8_t OOBCompletedActiveFlag;
    PortTimerHandle_t OOBAdvFastIntervalTimer;
    PortTimerHandle_t bootAdvFastIntervalTimer;
    PortTimerHandle_t requestAdvIntervalTimer;
    PortTimerHandle_t BatteryCheckTimer;
    uint8_t requestAdvIntervalActiveFlag;
    uint8_t factoryResetFlag;
    TagPowerSavingMode tagPSM;
    int8_t txPower;

    unsigned int MNID;
    unsigned int setupID;
    unsigned int serialNumber;
    OnboardingStatus onBoardingStatus;

    uint8_t bootReason;

    uint16_t svcHandleAuth;
    uint16_t svcHandleOnboarding;
    uint16_t svcHandleTag;

#if defined(__APP_BLE_INDICATION_SINGLE_QUEUE__)
    PortQueueHandle_t indicationQueue;
#endif
} TagContext;

extern TagContext *gTagContext;

/*! *********************************************************************************
* \brief  Refresh BLE advertising
*
* \return  TODO
*
********************************************************************************** */
TagError_t TagRefreshAdv(void);

/*! *********************************************************************************
* \brief  Transfer tag state
*
********************************************************************************** */
void TagTransferState(TagState newState);

/*! *********************************************************************************
* \brief  Update tag battery level
*
* \return  TODO
*
********************************************************************************** */


void TagCreateBatteryCheckTimer(void);

bool TagUpdateBatteryLevel(void);

void TagUpdateAgingCounter(void);

TagError_t LoadTagContextFromNV(TagContext *tagContext);

TagError_t ResetOOBFastAdvertisingTimer(void);

TagError_t CreateBootFastAdvertisingOneShotTimer(void);

TagError_t TagFactoryReset(uint32_t delay_ms);

bool IsTagStateOOB(TagContext *tagContext);

EndUserDevice *TagFindEndUserDevice(TagBleDeviceId deviceId);

EndUserDevice *TagFindEndUserDeviceFromPortHandle(PortBleConnInfo *portConnHandle);

TagError_t UpdateConnectionParameters(EndUserDevice *endUserDevice,
        uint16_t intervalMin, uint16_t intervalMax, uint16_t slaveLatency, uint16_t timeoutMultiplier);

TagError_t PrepareFwUpdate(TagBleDeviceId peerDeviceId);

TagError_t TagPutPostWork(TagTaskWorkHandler handler, TagTaskWorkParam param);

#endif /* TAGSDK_INC_TAGCORE_H_ */
