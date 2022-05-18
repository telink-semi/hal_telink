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

#include "stack/ble/ble.h"
#include "b91_bt_buffer.h"


/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin *******************************/
u8	app_acl_rxfifo[ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM] = {0};
u8	app_acl_mstTxfifo[ACL_MASTER_TX_FIFO_SIZE * ACL_MASTER_TX_FIFO_NUM * CONFIG_B91_BLE_CTRL_MASTER_MAX_NUM] = {0};
u8	app_acl_slvTxfifo[ACL_SLAVE_TX_FIFO_SIZE * ACL_SLAVE_TX_FIFO_NUM * CONFIG_B91_BLE_CTRL_SLAVE_MAX_NUM] = {0};

/***************************** HCI TX & RX data FIFO allocation, Begin *********************************************/
u8	app_hci_rxfifo[HCI_RX_FIFO_SIZE * HCI_RX_FIFO_NUM] = {0};
u8	app_hci_txfifo[HCI_TX_FIFO_SIZE * HCI_TX_FIFO_NUM] = {0};
u8	app_hci_rxAclfifo[HCI_RX_ACL_FIFO_SIZE * HCI_RX_ACL_FIFO_NUM] = {0};
