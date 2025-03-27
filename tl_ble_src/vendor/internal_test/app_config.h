/********************************************************************************************************
 * @file    app_config.h
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


#include "intest_config.h"






#if (INTER_TEST_MODE == TEST_ADV_SCAN_ONOFF)
    #include "adv_scan_onoff/app_config.h"
#elif (INTER_TEST_MODE == TEST_HCI_ACL_MORE_DATA)
    #include "hci_acl_more_data/app_config.h"
#elif (INTER_TEST_MODE == TEST_HCI_ACL_MORE_DATA_UPTSTER)
    #include "hci_acl_more_data_uptster/app_config.h"
#elif (INTER_TEST_MODE == TEST_DIRECT_INITIATE)
    #include "direct_initiate/app_config.h"
#elif (INTER_TEST_MODE == TEST_MASTER_UPDATE)
    #include "master_update/app_config.h"
#elif (INTER_TEST_MODE == TEST_HOST_TRX_DATA)
    #include "host_trx_data/app_config.h"
#elif (INTER_TEST_MODE == TEST_LOW_POWER)
    #include "low_power/app_config.h"
#elif (INTER_TEST_MODE == TEST_ISO_TEST_BIS_TRANSMIT)
    #include "iso_test_bis_transmit/app_config.h"
#elif (INTER_TEST_MODE == TEST_ISO_TEST_BIS_RECEIVE)
    #include "iso_test_bis_receive/app_config.h"
#elif (INTER_TEST_MODE == TEST_INT_MISC)
    #include "intest_misc/app_config.h"
#elif (INTER_TEST_MODE == TEST_CONTROLLER_BQB)
    #include "B91_controller_bqb/app_config.h"
#elif (INTER_TEST_MODE == TEST_CONTROLLER_BIS)
    #include "B91_controller_bis/app_config.h"
#elif (INTER_TEST_MODE == TEST_DBG_CIS_MASTER)
    #include "dbg_cis_master/app_config.h"
#elif (INTER_TEST_MODE == TEST_DBG_CIS_SLAVE)
    #include "dbg_cis_slave/app_config.h"
#elif (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)
    #include "broadcast_sender/app_config.h"
#elif (INTER_TEST_MODE == TEST_BIS_AUDIO_RECEIVER)
    #include "broadcast_receiver/app_config.h"
#elif (INTER_TEST_MODE == TEST_HOST_BQB)
    #include "host_bqb/app_config.h"
#elif (INTER_TEST_MODE == TEST_CIS_AUDIO_CLIENT)
    #include "unicast_client/app_config.h"
#elif (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)
    #include "unicast_server/app_config.h"
#elif (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)
    #include "ull_hid_device/app_config.h"
#elif (INTER_TEST_MODE == TEST_ULL_HID_HOST)
    #include "ull_hid_host/app_config.h"
#elif (INTER_TEST_MODE == TEST_USB_PPM_ASRC)
    #include "usb_ppm_asrc/app_config.h"
#elif (INTER_TEST_MODE == TEST_BACKUP)
    #include "intest_backup/app_config.h"
#elif (INTER_TEST_MODE == TEST_PPM_ASRC_WITH_IIS_LINEIN)
    #include "iis_line_ppm_asrc/app_config.h"
#elif (INTER_TEST_MODE == TEST_CS_SUBEVENT)
    #include "chn_sound_subevent/app_config.h"
#elif (INTER_TEST_MODE == TEST_CS_ACL_CENTRAL)
    #include "cs_acl_central_demo/app_config.h"
#elif (INTER_TEST_MODE == TEST_CS_ACL_PERIPHERAL)
    #include "cs_acl_peripheral_demo/app_config.h"
#elif (INTER_TEST_MODE == TEST_CS_DRBG)
    #include "cs_drbg_demo/app_config.h"
#elif (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)
    #include "cis_bis_switch_client/app_config.h"
#elif (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)
    #include "cis_bis_switch_server/app_config.h"
#elif (INTER_TEST_MODE == TEST_TERCEL)
    #include "tercel_test/app_config.h"
#elif (INTER_TEST_MODE == TEST_ONCA)
    #include "onca_test/app_config.h"
#elif (INTER_TEST_MODE == TEST_LIVE_MIC_PROJECT)
    #include "live_mic_project_test/app_config.h"
#elif (INTER_TEST_MODE == TEST_TERCEL_ZIGBEE)
    #include "tercel_zigbee_test/app_config.h"
#elif (INTER_TEST_MODE == TEST_DIFF_CON_DIFF_SMP_LEVEL)
    #include "Diff_connect_Diff_smp_level/app_config.h"
#else
    #error "need include one app_config.h at least"
#endif



