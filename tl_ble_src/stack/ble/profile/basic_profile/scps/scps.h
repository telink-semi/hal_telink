/********************************************************************************************************
 * @file    scps.h
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

// ScPS: Scan Parameters Service
// ScPSC: Scan Parameters Service Client.
// ScPSS: Scan Parameters Service

/******************************* ScPS Common Start **********************************************************************/
//The format and values of these fields are the same as the HCI LE Create Connection Command.
//LE_Scan_Interval and LE_Scan_Window range 0x0004 to 0x4000, time range 2.5ms to 10.24s
struct scan_interval_window
{
    u16 leScanInterval;
    u16 leScanWindow;
};

/******************************* ScPS Common End **********************************************************************/


/******************************* ScPS Client Start **********************************************************************/
//ScPS Client Event ID
enum
{
    BASIC_EVT_SCPSC_START = BASIC_EVT_TYPE_SCPS_CLIENT,
    SCPSC_EVT_RECV_SERVER_REQUIRES_REFRESH, //refer to 'NULL'
};

struct blc_scpsc_regParam
{
};

/**
 * @brief       for user to register scan parameters service control client module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 */
void blc_basic_registerSCPSControlClient(const struct blc_scpsc_regParam *param);

int blc_scpsc_writeScanIntervalWindow(u16 connHandle, struct scan_interval_window *scanIntervalWindow);

/******************************* ScPS Client End **********************************************************************/


/******************************* ScPS Server Start **********************************************************************/
//ScPS Server Event ID
enum
{
    BASIC_EVT_SCPSS_START = BASIC_EVT_TYPE_SCPS_SERVER,
    SCPSS_EVT_SCAN_INTERVAL_WINDOW_CHANGE, //refer to 'struct blc_scpsc_scanIntervalWindowChangeEvt'
};

struct blc_scpss_regParam
{
};

struct blc_scpsc_scanIntervalWindowChangeEvt
{ //Event ID: SCPSS_EVT_SCAN_INTERVAL_WINDOW_CHANGE
    struct scan_interval_window param;
};

/**
 * @brief       for user to register scan parameters service control server module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 */
void blc_basic_registerSCPSControlServer(const struct blc_scpss_regParam *param);

int blc_scpss_updateServerRequiresRefresh(u16 connHandle);

/******************************* ScPS Server End **********************************************************************/
