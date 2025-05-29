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

#ifndef TAGSDK_INC_TAGCONSTANT_H_
#define TAGSDK_INC_TAGCONSTANT_H_

#include "TagConfig.h"

/*
 * Value defines
 */
#define UNKNOWN_BLE_CONNECTION_TIMEOUT (30) /* 30 secondes disconnect unknown ble connection */
#define PARING_REQUEST_TIMEOUT (5)          /* 5 secondes for accepting end device paring request */

#define AGING_COUNTER_NORM_UTC 1593648000 /* 00:00:00 2nd. July. 2020 (UTC) */
#define AGING_COUNTER_INTERVAL 900        /* 15-min Aging Counter interval */
#define PREMATURE_TIMEOUT_PERIOD 900      /* 900(sec) premature to offline timeout */
#define OFFLINE_TIMEOUT_PERIOD 28800      /* 28,800(sec) offline to overmature timeout */

#define T_OOB_COMPLETED_TIMEOUT_SEC 30                /* 30(sec) OOB Completed timeout */
#define T_OOB_ADVERTISE_FAST_INTERVAL_TIMEOUT_SEC 300 /* 5(min) T_OOB_AdvIntervalFastTimeout */
#define T_BOOT_ADVERTISE_FAST_INTERVAL_TIMEOUT_SEC 30 /* 30(sec) Spec. TBD */
#define T_REQUEST_ADVERTISE_TIMEOUT_SEC 10            /* 10(s) T_Request_AdvTimeout */
#define T_OOB_ADVERTISE_INTERVAL_NORMAL 1000          /* 1000(msec) T_OOB_AdvIntervalNormal */
#define T_OOB_ADVERTISE_INTERVAL_FAST 50              /* 50(msec) T_OOB_AdvIntervalFast */
#define T_NORMAL_ADVERTISE_INTERVAL 1970              /* 1970(msec) T_Normal_AdvInterval */
#define T_POWER_SAVE_ADVERTISE_INTERVAL 3000          /* 3000(msec) T_PowerSaveMode_AdvInterval */
#define T_REQUEST_ADVERTISE_INTERVAL 20               /* 20(msec) T_Request_AdvTimeout */
#define CONVERT_BLE_ADV_INTERVAL(x) (x * 1.6)         /* convert ble advertising interval time to unit */

#define T_CONNECTION_PARAM_UPDATE_TIMEOUT (3)           /* 3(sec) T_ConnParamUpdateTimeout*/
#define T_CONNECTION_PARAM_UPDATE_RECOVERY_TIMEOUT (10) /* 10(sec) T_ConnParamUpdateRecoveryTimeout*/
#define T_CONNECTION_PARAM_UPDATE_RETRY_TIMEOUT (10) /* 10(sec) T_ConnParamUpdateRetryTimeout*/
#define CONNECTION_PARAM_UPDATE_RETRY_COUNT_MAX (10)     /* Max connection parameters update retry count */

#define DEFAULT_CONNECTION_INTERVAL_MIN 360                  /* msec */
#define DEFAULT_CONNECTION_INTERVAL_MAX 380                  /* msec */
#define CONVERT_CONNECTION_INTERVAL_TO_API_UNIT(x) ((x)*0.8) /* convert connection interval time to ble api unit */
#define DEFAULT_CONNECTION_LATENCY 4
#define DEFAULT_CONNECTION_SUPERVISION_TIMEOUT 20000            /* msec */
#define CONVERT_CONNECTION_SUPERVISION_TO_API_UNIT(x) ((x)*0.1) /* convert connection supervision time to ble api unit */

#define MAX_ALLOWED_BLE_CONNECTIONS (2) /* Max allowed ble connections */

#define ONBOARDING_READINESS_FLAG_ON 1

#define MAX_END_USER_DEVICES 2

#define TAG_ADVERTISING_STRUCT_NUM 3 /* number of BLE advertising struct */

#define AUTH_GATT_SERVICE_UUID                                                                             \
    {                                                                                                      \
        {                                                                                                  \
            0x7C, 0xA8, 0x9D, 0x48, 0x8A, 0x39, 0x19, 0x82, 0x73, 0x46, 0xA8, 0x6A, 0x73, 0x5E, 0xDD, 0xEE \
        }                                                                                                  \
    }
#define CONTROL_GATT_SERVICE_UUID 0xFD5A
#define ONBOARDING_GATT_SERVICE_UUID 0xFD59

#define TAG_DEVICE_NAME "Tag"

#define TAG_ADVERTISING_FLAG_STRUCT_LENGTH 1
#define TAG_ADVERTISING_SERVICE_UUID_STRUCT_LENGTH 2
#define TAG_ADVERTISING_SERVICE_DATA_STRUCT_LENGTH 22
/* In case of OOB*/
#define TAG_ADV_OOB_SERVICE_DATA_UUID_OFFSET 0
#define TAG_ADV_OOB_SERVICE_DATA_UUID_LENGTH 2
#define TAG_ADV_OOB_SERVICE_DATA_VERSION_OFFSET 2
#define TAG_ADV_OOB_SERVICE_DATA_VERSION_LENGTH 1
#define TAG_ADV_OOB_SERVICE_DATA_MNID_OFFSET 3
#define TAG_ADV_OOB_SERVICE_DATA_MNID_LENGTH 4
#define TAG_ADV_OOB_SERVICE_DATA_SETUPID_OFFSET 7
#define TAG_ADV_OOB_SERVICE_DATA_SETUPID_LENGTH 3
#define TAG_ADV_OOB_SERVICE_DATA_READNESS_OFFSET 10
#define TAG_ADV_OOB_SERVICE_DATA_READNESS_LENGTH 1
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_OFFSET 11
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_LENGTH 1
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH_VALUE 5
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_OFFSET 12
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_LENGTH 1
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_TYPE_VALUE 1
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_OFFSET 13
#define TAG_ADV_OOB_SERVICE_DATA_SERIAL_DATA_LENGTH 4
/* In case of Other States*/
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_UUID_OFFSET 0
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_UUID_LENGTH 2
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_OFFSET 2
#define TAG_SERVICE_DATA_VERSION_BIT_OFFSET 4
#define TAG_ADV_TYPE_BIT_OFFSET 3
#define CONNECTED_BIT_OFFSET 2
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_VERSION_LENGTH 1
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_AGING_COUNTER_OFFSET 3
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_AGING_COUNTER_LENGTH 3
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_PRIVACYID_OFFSET 6
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_PRIVACYID_LENGTH 8
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_REGIONID_OFFSET 14
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_REGIONID_LENGTH 1
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_SIGNATURE_OFFSET 18
#define TAG_ADV_OTHER_STATES_SERVICE_DATA_SIGNATURE_LENGTH 4

#define TAG_SERVICE_DATA_VERSION 1

#ifdef TAG_CONFIG_USE_UWB_CHARACTERISTICS
#define TAG_UWB_SUPPORT 0x01
#else
#define TAG_UWB_SUPPORT 0x00
#endif

#define BATTERY_LEVEL_UNAVAILABLE (0x04) /* battery level unavailable */
#define BATTERY_LEVEL_FULL (0x03)        /*        91 % ~ 100 %       */
#define BATTERY_LEVEL_MEDIUM (0x02)      /*        16 % ~  90 %       */
#define BATTERY_LEVEL_LOW (0x01)         /*         6 % ~  15 %       */
#define BATTERY_LEVEL_VERY_LOW (0x00)    /*         0 % ~   5 %       */
#define BATTERY_LEVEL_FULL_PERCENT   100
#define BATTERY_LEVEL_MEDIUM_PERCENT 90
#define BATTERY_LEVEL_LOW_PERCENT 15
#define BATTERY_LEVEL_VERY_LOW_PERCENT 5

#endif /* TAGSDK_INC_TAGCONSTANT_H_ */
