/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#ifndef B91_BT_BUFFER_H_
#define B91_BT_BUFFER_H_

/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin *******************************/
#define ACL_CONN_MAX_RX_OCTETS			251
#define ACL_SLAVE_MAX_TX_OCTETS			251
#define ACL_MASTER_MAX_TX_OCTETS		251

/**
 * @brief	LE_ACL_Data_Packet_Length, refer to BLE SPEC "7.8.2 LE Read Buffer Size command"
 * usage limitation:
 * 1. only used for BLE controller project
 * 2. must be an integer multiple of 4, such as 32,80,200...
 * 3. must greater than maximum of ACL_SLAVE_MAX_TX_OCTETS, ACL_MASTER_MAX_TX_OCTETS
 * 4. can not exceed 252(for maximum tx_octets when DLE used)
 */
#define LE_ACL_DATA_PACKET_LENGTH		(252)

#define ACL_RX_FIFO_SIZE		    CAL_LL_ACL_RX_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#define ACL_RX_FIFO_NUM			    8

#define ACL_SLAVE_TX_FIFO_SIZE		CAL_LL_ACL_TX_FIFO_SIZE(ACL_SLAVE_MAX_TX_OCTETS)
#define ACL_SLAVE_TX_FIFO_NUM		9

#define ACL_MASTER_TX_FIFO_SIZE		CAL_LL_ACL_TX_FIFO_SIZE(ACL_MASTER_MAX_TX_OCTETS)
#define ACL_MASTER_TX_FIFO_NUM		9

extern	u8	app_acl_rxfifo[];
extern	u8	app_acl_mstTxfifo[];
extern	u8	app_acl_slvTxfifo[];

/***************************** HCI TX & RX data FIFO allocation, Begin *********************************************/
#define HCI_MAX_TX_SIZE				251

#define HCI_TX_FIFO_SIZE			HCI_FIFO_SIZE(HCI_MAX_TX_SIZE)
#define HCI_TX_FIFO_NUM				8

#define HCI_RX_FIFO_SIZE			HCI_FIFO_SIZE(ACL_CONN_MAX_RX_OCTETS)
#define HCI_RX_FIFO_NUM				8

#define HCI_RX_ACL_FIFO_SIZE		CALCULATE_HCI_ACL_DATA_FIFO_SIZE(LE_ACL_DATA_PACKET_LENGTH)
#define HCI_RX_ACL_FIFO_NUM			8

extern	u8	app_hci_rxfifo[];
extern	u8	app_hci_txfifo[];
extern	u8	app_hci_rxAclfifo[];

#endif /* B91_BT_BUFFER_H_ */
