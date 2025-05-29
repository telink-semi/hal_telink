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

#ifndef TAGSDK_INC_TAGVERSION_H_
#define TAGSDK_INC_TAGVERSION_H_

//#include "sec_version.h"
#include "TagFwVersion.h"

#define _STAG_STR(s) #s
#define _STAG_VERSION_STR(a, b, c) _STAG_STR(a) "." _STAG_STR(b) "." _STAG_STR(c)
#define _STAG_CONFIGURATION_VERSION_STR(a, b) _STAG_STR(a) "." _STAG_STR(b)

/**
 * Version of SmartThings Find Device Specification supported
 * DO NOT modify arbitrarily.
 */
#define SPEC_VERSION_STRING _STAG_VERSION_STR(1, 1, 1)

/**
 * Version of SmartThings Find Device SDK
 * DO NOT modify arbitrarily.
 */
#define TAGSDK_VERSION_STRING _STAG_VERSION_STR(2, 3, 0)

/**
 * Version of BLE configuration
 * It is specified in the SmartThings Find Device Specification
 * DO NOT modify arbitrarily.
 */
#define CONFIGURATION_VERSION_STRING _STAG_CONFIGURATION_VERSION_STR(2, 0)

#ifdef TAG_SW_VER
#define DEVICE_FW_VERSION_STRING TAG_SW_VER

#define DEVICE_QB_CL_STRING TAG_QB_CL_NUM
#define DEVICE_QB_ID_STRING TAG_QB_ID_NUM
#else
/**
 * Version of your product firmware
 * It will be used for firmware update logic.
 * You can modify version at conf/TagFwVersion.h
 */
#define DEVICE_FW_VERSION_STRING _STAG_VERSION_STR(TAG_FWVER_MAJOR, TAG_FWVER_MINOR, TAG_FWVER_PATCH)
#endif

#endif /* TAGSDK_INC_TAGVERSION_H_ */
