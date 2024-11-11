/********************************************************************************************************
 * @file    acl_sniffer_slv.h
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
#ifndef ACL_SNIFFER_SLV_H_
#define ACL_SNIFFER_SLV_H_

#include "acl_sniffer.h"

/**
 * @brief   acl slave status for compatible mode.
 */
enum{
    //0x00
    SLAVE_STATUS_STANDBY            = 0,            //
    SLAVE_STATUS_CONNECTION_SETUP,                  //
    SLAVE_STATUS_CONNECTION_GENERAL,                //
    SLAVE_STATUS_CONNECTION_UPDATE,                 //
    SLAVE_STATUS_CONNECTION_CHANNEL_MAP,            //
};


/**
 * @brief   acl slave compatible mode parameter, 24Bytes
 */
typedef struct {
    u8      slave_index;        //0~3
    u8      slave_status;
    u8      slave_accessAddress[4]; //Little-Endian
    u8      slave_crcInit[3];   //Little-Endian
    u16     slave_winOffset;    //unit: 1.25 ms
    u16     slave_connInterval; //unit: 1.25 ms
    u16     slave_connTimeout;  //unit: 10 ms
    u8      slave_connChannelMap[5];//Little-Endian
    u8      slave_connHop;      //5~16
    u16     slave_instant;
    u8      slave_CI_ChSel_PHY; //Bit7~Bit4: coding_ind 2 or 8, Bit3: ChSel 0 or 1, Bit1~Bit0: le_phy_type_t 1~3
} acl_slave_compatible_param_t;


/**
 * @brief   acl slave interoperable mode parameter, 28Bytes
 */
typedef struct {
    u8      slave_index;        //0~3
    u32     slave_connExpectTime;
    u16     slave_connEventCounter;
    u16     slave_connInterval; //unit: 1.25 ms
    u16     slave_connTimeout;  //unit: 10 ms
    u8      slave_accessAddress[4];//Little-Endian
    u8      slave_crcInit[3];   //Little-Endian
    u8      slave_connChannelMap[5];//Little-Endian
    u8      slave_connHop;      //5~16
    u8      slave_connChannelIndex;//0~36, each interval is incremented by 1
    u8      slave_CI_ChSel_PHY; //Bit7~Bit4: coding_ind 2 or 8, Bit3: ChSel 0 or 1, Bit1~Bit0: le_phy_type_t 1~3
    u16     u16_rsvd1;
} acl_slave_interoperable_param_t;


/**
 * @brief      for user to initialize ACL sniffer for monitor peer-master.
 * @param      none
 * @return     none
 */
void        blc_ll_initAclSnifferSlv_module(void);


/**
 * @brief       for user to add ACL sniffer sync time
 * @param[in]   earlyTime_us
 * @return      none.
 */
void        blc_ll_addAclSnifferSlvSyncEarlyTime(u32 earlyTime_us);


/**
 * @brief       for user to set ACL sniffer report RSSI type
 * @param[in]   rssi_type
 * @return      status, 0x00:  succeed
 *                      other: failed
 */
ble_sts_t   blc_ll_setAclSnifferSlvReportRssiType(acl_sniffer_rssi_report_type_t rssi_type);


/**
 * @brief      for user to set ACL sniffer first sync window maximum enable.
 * @param[in]  enable - 1: enable
 *                      0: disable
 * @return     none
 */
void        blc_ll_setAclSnifferSlv1stSyncWinMaxEnable(u8 enable);


/**
 * @brief      for user to obtain the sync number of ACL sniffer.
 * @param[in]  none.
 * @return     The number of all ACL sniffer sync.
 */
int         blc_ll_getAclSnifferSlvSyncNumber(void);


/**
 * @brief      for user to obtain the sync status of ACL sniffer.
 * @param[in]  snifHandle - ACL sniffer handle.
 * @return     The status of currently ACL sniffer sync.
 */
int         blc_ll_getAclSnifferSlvSyncStatus(u16 snifHandle);


/**
 * @brief      for user to set ACL sniffer terminate sync.
 * @param[in]  snifHandle - ACL sniffer handle.
 * @return     status, 0x00:   succeed
 *                      other: failed
 */
int         blc_ll_setAclSnifferSlvTerminateSync(u16 snifHandle);


/**
 * @brief      for user to update ACL sniffer sync information for compatible mode.
 * @param[in]  cmd - ACL sniffer sync command, format reference structure acl_slave_compatible_param_t.
 * @param[in]  latencyTime - cmd transport latency time for CONNECTION_SETUP, unit is 0.0625us
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
int         blc_ll_updateAclSnifferSlvSync_v2(u8 *cmd, u32 latencyTime);


/**
 * @brief      for user to obtain the sniffer handle for compatible mode or interoperable mode.
 * @param[in]  slave_index - 0 ~ 3.
 * @return     sniffer handle
 *                  0x00:  input parameter is incorrect
 *                  other: correct sniffer handle
 */
u16         blc_ll_getAclSnifferSlvHandle_v2(u8 slave_index);


/**
 * @brief      for user to update ACL sniffer sync information for interoperable mode.
 * @param[in]  cmd - ACL sniffer sync command, format reference structure acl_slave_interoperable_param_t.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
int         blc_ll_updateAclSnifferSlvSync_v3(u8 *cmd);


/**
 * @brief      for user to get ACL Slave connection timing parameter for sniffer.
 * @param[in]  connHandle - ACL connection handle.
 * @param[out] ACL Slave timing parameter.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_getAclSlaveConnectionTimingParameter(u16 connHandle, u8* aclSlaveParam);


/**
 * @brief      for user to get ACL Slave connection setup parameter for compatible mode.
 * @param[in]  connHandle - ACL connection handle.
 * @param[out] ACL Slave connection setup parameter.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_getAclSlaveConnectionSetupParameter(u16 connHandle, acl_slave_compatible_param_t *aclSlaveConnSetupParam);


/**
 * @brief   Compatible with old API names
 */
#define     blc_ll_initAclSniffer_module                    blc_ll_initAclSnifferSlv_module
#define     blc_ll_addAclSnifferSyncEarlyTime               blc_ll_addAclSnifferSlvSyncEarlyTime
#define     blc_ll_setAclSnifferReportRssiType              blc_ll_setAclSnifferSlvReportRssiType
#define     blc_ll_setAclSnifferFirstSyncWindowMaxEnable    blc_ll_setAclSnifferSlv1stSyncWinMaxEnable
#define     blc_ll_getAclSnifferSyncNumber                  blc_ll_getAclSnifferSlvSyncNumber
#define     blc_ll_getAclSnifferSyncStatus                  blc_ll_getAclSnifferSlvSyncStatus
#define     blc_ll_setAclSnifferTerminateSync               blc_ll_setAclSnifferSlvTerminateSync
#define     blc_ll_updateAclSnifferSync_v2                  blc_ll_updateAclSnifferSlvSync_v2
#define     blc_ll_getAclSnifferHandle_v2                   blc_ll_getAclSnifferSlvHandle_v2


#endif /* ACL_SNIFFER_SLV_H_ */
