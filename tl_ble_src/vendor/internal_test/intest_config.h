/********************************************************************************************************
 * @file    intest_config.h
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
#ifndef INTEST_CONFIG_H_
#define INTEST_CONFIG_H_





#define TEST_ADV_SCAN_ONOFF                         1

#define TEST_DIRECT_INITIATE                        2

#define TEST_MASTER_UPDATE                          3   //test initiate timing and conn_update and map_update

#define TEST_HCI_ACL_MORE_DATA                      4

#define TEST_HCI_ACL_MORE_DATA_UPTSTER              5 //UART upper tester for TEST_HCI_ACL_MORE_DATA only

#define TEST_HOST_TRX_DATA                          6

#define TEST_LOW_POWER                              7

#define TEST_ISO_TEST_BIS_TRANSMIT                  8

#define TEST_ISO_TEST_BIS_RECEIVE                   9

#define TEST_CONTROLLER_BQB                         10

#define TEST_CONTROLLER_BIS                         11

#define TEST_DBG_CIS_MASTER                         12  //CIS Master debug

#define TEST_DBG_CIS_SLAVE                          13  //CIS Slave debug

#define TEST_HOST_BQB                               14

#define TEST_ADV_PM_MANAGE                          15

#define TEST_DIFF_CON_DIFF_SMP_LEVEL                16

#define TEST_INT_MISC                               100

#define TEST_BIS_AUDIO_SENDER                       160
#define TEST_BIS_AUDIO_RECEIVER                     161

#define TEST_CIS_AUDIO_CLIENT                       170
#define TEST_CIS_AUDIO_SERVER                       171

#define TEST_ULL_HID_DEVICE                         172     //HID Device shall be a GATT server and ACL/CIS peripheral
#define TEST_ULL_HID_HOST                           173     //Boot/Report Host shall be a GATT client and ACL/CIS central

#define TEST_USB_PPM_ASRC                           180

#define TEST_BACKUP                                 200

#define TEST_PPM_ASRC_WITH_IIS_LINEIN               300

#define TEST_CS_SUBEVENT                            400
#define TEST_CS_ACL_CENTRAL                         401
#define TEST_CS_ACL_PERIPHERAL                      402
#define TEST_CS_DRBG                                403

#define TEST_LE_AUDIO_SWTICH_CLIENT                 404
#define TEST_LE_AUDIO_SWTICH_SERVER                 405


#define TEST_TERCEL                                 500
#define TEST_ONCA                                   501

#define TEST_LIVE_MIC_PROJECT                       502
#define TEST_TERCEL_ZIGBEE                          503

#define INTER_TEST_MODE                             TEST_LIVE_MIC_PROJECT






#endif /* INTEST_CONFIG_H_ */
