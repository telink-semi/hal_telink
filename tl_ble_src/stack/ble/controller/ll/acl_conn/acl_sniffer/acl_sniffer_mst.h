/********************************************************************************************************
 * @file    acl_sniffer_mst.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020.06
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef ACL_SNIFFER_MST_H_
#define ACL_SNIFFER_MST_H_


#include "acl_sniffer.h"


/**
 * @brief   acl master status for compatible mode.
 */
enum{
    //0x00
    MASTER_STATUS_STANDBY           = 0,            //
    MASTER_STATUS_CONNECTION_SETUP,                 //
    MASTER_STATUS_CONNECTION_GENERAL,               //
    MASTER_STATUS_CONNECTION_UPDATE,                //
    MASTER_STATUS_CONNECTION_CHANNEL_MAP,           //
};


/**
 * @brief      for user to initialize ACL sniffer for monitor peer-slave.
 * @param      none
 * @return     none
 */
void        blc_ll_initAclSnifferMst_module(void);


/**
 * @brief       for user to add ACL sniffer sync time
 * @param[in]   earlyTime_us
 * @return      none.
 */
void        blc_ll_addAclSnifferMstSyncEarlyTime(u32 earlyTime_us);


/**
 * @brief       for user to set ACL sniffer report RSSI type
 * @param[in]   rssi_type
 * @return      status, 0x00:  succeed
 *                      other: failed
 */
ble_sts_t   blc_ll_setAclSnifferMstReportRssiType(acl_sniffer_rssi_report_type_t rssi_type);


/**
 * @brief      for user to set ACL sniffer first sync window maximum enable.
 * @param[in]  enable - 1: enable
 *                      0: disable
 * @return     none
 */
void        blc_ll_setAclSnifferMst1stSyncWinMaxEnable(u8 enable);


/**
 * @brief      for user to obtain the sync number of ACL sniffer.
 * @param[in]  none.
 * @return     The number of all ACL sniffer sync.
 */
int         blc_ll_getAclSnifferMstSyncNumber(void);


/**
 * @brief      for user to obtain the sync status of ACL sniffer.
 * @param[in]  snifHandle - ACL sniffer handle.
 * @return     The status of currently ACL sniffer sync.
 */
int         blc_ll_getAclSnifferMstSyncStatus(u16 snifHandle);


/**
 * @brief      for user to set ACL sniffer terminate sync.
 * @param[in]  snifHandle - ACL sniffer handle.
 * @return     status, 0x00:   succeed
 *                      other: failed
 */
int         blc_ll_setAclSnifferMstTerminateSync(u16 snifHandle);


/**
 * @brief      for user to get ACL Master connection timing parameter for sniffer.
 * @param[in]  connHandle - ACL connection handle.
 * @param[out] ACL Master timing parameter.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_getAclMasterConnectionTimingParameter(u16 connHandle, u8* aclMasterParam);


/**
 * @brief      for user to set ACL Master connection parameter update response latency for sniffer.
 * @param[in]  updateRsp_latency.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_setAclMasterConnParamUpdateRspLatency(u16 updateRsp_latency);


#endif /* ACL_SNIFFER_MST_H_ */
