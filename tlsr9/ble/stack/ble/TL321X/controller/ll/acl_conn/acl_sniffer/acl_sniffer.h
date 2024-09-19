/********************************************************************************************************
 * @file    acl_sniffer.h
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
#ifndef ACL_SNIFFER_H_
#define ACL_SNIFFER_H_

#include "tl_common.h"
#include "stack/ble/controller/ll/ll.h"


/**
 * @brief   acl sniffer RSSI Report Type
 */
typedef enum {
    //0x00
    RSSI_TYPE_INVALID,
    RSSI_TYPE_SLAVE,
    RSSI_TYPE_MASTER,
    RSSI_TYPE_ALL,
}acl_sniffer_rssi_report_type_t;


#include "acl_sniffer_mst.h"
#include "acl_sniffer_slv.h"


/**
 * @brief   update acl sniffer sync result
 */
enum{
    //0x00
    SNIFFER_SYNC_CREATE             = 0,            //
    SNIFFER_CURRENT_STATE_NOT_SUPPORTED_THIS_CMD,   //
    SNIFFER_PARAMETER_CHECKSUM_ERR,                 //
    SNIFFER_PARAMETER_INVALID,                      //

    //0x04
    SNIFFER_ESTABLISH_FAIL,                         //
    SNIFFER_TIMEOUT,                                //
    SNIFFER_LOSS,                                   //
    SNIFFER_SEEK_FAIL,                              //

    //0x08
    SNIFFER_SEEK_IN_PROGRESS,                       //
    SNIFFER_UNKNOWN_SNIFHANDLE,                     //
    SNIFFER_USER_STOP_EFFECTIVE,                    //
    SNIFFER_USER_STOP_NOT_SUPPORTED,                //

    //0x0C
    SNIFFER_UNSUPPORTED_FEATURE,                    //
};


/**
 * @brief   acl sniffer sync status
 */
enum{
    //0x00
    SNIFFER_STATUS_STANDBY          = 0,            //
    SNIFFER_STATUS_CREATING,                        //
    SNIFFER_STATUS_ESTABLISH,                       //
    SNIFFER_STATUS_SEEK,                            //
};


/**
 * @brief   Telink defined LinkLayer Event Callback for Sniffer
 */
typedef enum {
    BLT_EV_FLAG_CHANNEL_MAP_UPDATE = BLT_EV_MAX_NUM,
    BLT_EV_FLAG_SNIFFER_RSSI_REPORT,
    BLT_EV_FLAG_SNIFFER_SYNC_STATUS,
    BLT_EV_FLAG_ACL_EVERY_CONN_EVENT,

    BLT_SNIFFER_EV_MAX_NUM,
}blt_ext_ev_flag_4_sniffer_t;

/**
 *  @brief  Event Parameters for "BLT_EV_FLAG_SNIFFER_RSSI_REPORT"
 */
typedef struct {
    u8  snifChannel :6;
    u8  type        :2;     //peer device RSSI mark, 2:master RSSI, 1:slave RSSI, 0:invalid RSSI
    u8  snifHandle;
    u8  rssi;
}acl_sniffer_rssi_reportEvt_t;


/**
 *  @brief  Event Parameters for "BLT_EV_FLAG_SNIFFER_SYNC_STATUS"
 */
typedef struct {
    u8  snifHandle;
    u8  status;
}acl_sniffer_sync_statusEvt_t;


/**
 *  @brief  Event Parameters for "BLT_EV_FLAG_CHANNEL_MAP_UPDATE"
 */
typedef struct {
    u16 connHandle;
}acl_channel_map_updateEvt_t;


/**
 *  @brief  Event Parameters for "BLT_EV_FLAG_ACL_EVERY_CONN_EVENT"
 */
typedef struct {
    u16 connHandle;
    u16 connEventCounter;
    u32 connExpectTime;
}acl_every_conn_eventEvt_t;


/**
 * @brief      for user to get Legacy Adv status.
 * @param[in]  none.
 * @return     status, 0x00:  Adv enabled
 *                     other: Adv disabled
 */
int         blc_ll_getLegacyAdvStatus(void);


/**
 * @brief      for user to update ACL sniffer sync information.
 * @param[in]  cmd - ACL sniffer sync command.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
int         blc_ll_updateAclSnifferSync(u8 *cmd);


/**
 * @brief      for user to set ACL sniffer max Rx buffer len.
 * @param[in]  len - ACL sniffer max Rx buffer len.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_setAclSnifferMaxRxBufferLen(u8 len);

#endif /* ACL_SNIFFER_C_H_ */
