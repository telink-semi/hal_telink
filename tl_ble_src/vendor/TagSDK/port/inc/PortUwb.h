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

#ifndef TAGSDK_PORT_INC_PORTUWB_H_
#define TAGSDK_PORT_INC_PORTUWB_H_

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
//#include "arch.h"
#include "TagErrorType.h"

typedef enum {
    UWB_POWER_MODE_DEACTIVATE = 0,
    UWB_POWER_MODE_ACTIVATE = 1
} UwbPowerMode_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t version;
    uint8_t requestType : 1;
    uint8_t result :1;
    uint8_t deviceRole :1;
    uint8_t deviceType :1;
    uint8_t rangingMethod :2;
    uint8_t multiNodeMode :2;
    uint8_t macFcsType :2;
    uint8_t macConfig :2;
    uint8_t prfMode :2;
    uint8_t stsConfig :1;
    uint8_t keyRotation :1;
    uint8_t macAddressMode :2;
    uint8_t ppduConfig :2;
    uint8_t preambleDuration :1;
    uint8_t psduDataRate :1;
    uint8_t sfdId :2;
    uint8_t noOfStsSeg :2;
    uint8_t rangingRoundHopping :1;
    uint8_t reserved1 :5;
    uint32_t sessionId;
    uint16_t initiatorMac;
    uint16_t responderMac;
    uint8_t channelId;
    uint16_t slotLen;
    uint16_t rangingInterval;
    uint8_t rangingRoundPhaseControl;
    uint8_t preambleId;
    uint8_t responderSlotIndex;
    uint8_t keyRotationRate;
    uint16_t vendorId;
    uint8_t staticStsIv[6];
    uint8_t slotsPerRangingRound;
    uint32_t rangingStartOffset;
} UwbParam_t;
#pragma pack(pop)

/**
 * @brief       Set UWB power
 *
 * @details     This function sets UWB power on/off
 *
 * @param[in]   UWB power mode
 * @retval      TAG_ERROR_NONE for success
 * @retval      TAG_ERROR_NOT_SUPPORTED if this device do not support UWB feature
 * @retval      TAG_ERROR_INVALID_ARG if not supported argument passed
 *
 */
TagError_t PortUwbSetPower(UwbPowerMode_t mode, uint32_t uwbSessionId);

/**
 * @brief       Get UWB power
 *
 * @details     This function returns UWB power on/off status
 *
 * @param[out]  UWB power mode
 * @retval      TAG_ERROR_NONE for success
 * @retval      TAG_ERROR_NOT_SUPPORTED if this device do not support UWB feature
 * @retval      TAG_ERROR_INVALID_ARG if mode is NULL
 *
 */
TagError_t PortUwbGetPower(UwbPowerMode_t* mode);

/**
 * @brief       Set UWB parameter
 *
 * @details     This function sets UWB parameter from end user device
 *
 * @param[in]   UWB parameter from end user device
 * @retval      TAG_ERROR_NONE for success
 * @retval      TAG_ERROR_NOT_SUPPORTED if this device do not support UWB feature
 * @retval      TAG_ERROR_INVALID_ARG if argument is wrong
 *
 */
TagError_t PortUwbSetParam(UwbParam_t* param);

/**
 * @brief       Init UWB
 *
 * @details     This function initializes UWB.
 *
 * @retval      TAG_ERROR_NONE for success, others for errors
 *
 */
TagError_t PortUwbInit(void);

/**
 * @brief       Set UWB power
 *
 * @details     This function sets UWB power off due to ring
 *
 * @retval      TAG_ERROR_NONE for success
 * @retval      Others for failure
 *
 */
TagError_t PortUwbSetPowerOffDueToRing(void);

/**
 * @brief       Check whether the UWB SW (FW+DSP) needs to be updated. If needed then execute the update.
 *
 * @details     This checks whether the UWB SW (FW+DSP) needs to be updated. If needed then execute the update.
 *
 */
void PortUwbSwup(void);

/**
 * @brief       Create the UWB Manager task to process its internal events.
 *
 * @details     This function creates the UWB Manager task to process its internal events
 *
 * @retval      TAG_ERROR_NONE for success
 * @retval      Others for failure
 *
 */
TagError_t PortUwbStartMgr(void);

#endif
#endif /* TAGSDK_PORT_INC_PORTUWB_H_ */
