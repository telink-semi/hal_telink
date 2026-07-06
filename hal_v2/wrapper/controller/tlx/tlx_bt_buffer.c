/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "tlx_bt_buffer.h"
#include "tlx_bt_cs.h"

/******* ACL connection LinkLayer TX & RX data FIFO allocation, Begin ********/
_attribute_data_sec_ u8* app_acl_rxfifo;
#ifdef CONFIG_BT_CENTRAL
_attribute_data_sec_ u8* app_acl_mstTxfifo;
#endif /* CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_PERIPHERAL
_attribute_data_sec_ u8* app_acl_slvTxfifo;
#endif /* CONFIG_BT_PERIPHERAL */
/******** HCI TX & RX data FIFO allocation, Begin  ***************************/
_attribute_data_sec_ u8* app_hci_rxfifo;
_attribute_data_sec_ u8* app_hci_txfifo;
_attribute_data_sec_ u8* app_hci_rxAclfifo;
/******** CS data allocation, Begin  ******************************************/
u8 app_CsConfigParam[CS_PARAM_LENGTH * APP_CS_CONFIG_NUM];
u8 app_cs_rx_buf[CS_RX_FIFO_SIZE * CS_RX_FIFO_NUM];
u8 hciCsSubeventRxFifo[CS_SUBEVENT_BUFF_LEN_MAX * HCI_RX_FIFO_NUM];
u8 pct_raw_data[CS_PCT_DATA_SIZE];
/******** CS data allocation, End  ********************************************/
u8 app_cs_transport_rx_buf[CS_TRANSPORT_RX_FIFO_NUM * CS_TRANSPORT_RX_FIFO_SIZE];
u8 app_cs_stepDRBGInfoBuffer[CS_STEP_DRBG_INFO_SIZE * STEP_NUM_PER_SUBEVENT];
