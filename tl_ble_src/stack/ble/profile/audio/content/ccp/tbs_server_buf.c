/********************************************************************************************************
 * @file    tbs_server_buf.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


const u8                    defaultGtbsBearerProviderName[] = {'D', 'e', 'f', 'a', 'u', 'l', 't', ' ', 'p', 'r', 'o', 'v', 'i', 'd', 'e', 'r'};
const u8                    defaultGtbsBearerUci[]          = {'u', 'n', '0', '0', '0'};
const u8                    defaultGtbsUriScheme[]          = {'t', 'e', 'l'};
const blc_tbss_uri_scheme_t defaultGtbsURISchemes[]         = {
    {
     .uri    = defaultGtbsUriScheme,
     .uriLen = sizeof(defaultGtbsUriScheme),
     },
};

const blc_ccps_regParam_t defaultCppsParam = {
    .gtbsParam = {
                  .bearerProviderName      = defaultGtbsBearerProviderName,
                  .bearerProviderNameLen   = sizeof(defaultGtbsBearerProviderName),
                  .bearerUci               = defaultGtbsBearerUci,
                  .bearerUciLen            = sizeof(defaultGtbsBearerUci),
                  .bearerTechnology        = GTBS_TECHNOLOGY_3G,
                  .bearerUriSchemeList     = defaultGtbsURISchemes,
                  .bearerUriSchemeListLen  = ARRAY_SIZE(defaultGtbsURISchemes),
                  .signalStrength          = GTBS_SIGNAL_STRENGTH_UNAVAILABLE,
                  .CCID                    = 0,
                  .statusFlags.statusFlags = 0,
                  }
};
