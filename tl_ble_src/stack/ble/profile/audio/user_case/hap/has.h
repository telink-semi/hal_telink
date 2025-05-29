/********************************************************************************************************
 * @file    has.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once

#include "has_client_buf.h"
#include "has_server_buf.h"

/******************************* HAS Common Start **********************************************************************/

/******************************* HAS Common End **********************************************************************/


/******************************* HAS Client Start **********************************************************************/

//HAS Client Event ID
typedef enum
{
    AUDIO_EVT_HSC_START = AUDIO_EVT_TYPE_HASC,
    //NONE:
} audio_hasc_evt_enum;

/******************************* HAS Client End **********************************************************************/


/******************************* HAS Server Start **********************************************************************/

//HAS Server Event ID
typedef enum
{
    AUDIO_EVT_HASC_START = AUDIO_EVT_TYPE_HASC,
    //NONE:
} audio_hass_evt_enum;

/******************************* HAS Server End **********************************************************************/
