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

#ifndef STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_
#define STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_


#define  ISO_TESt_SUPPORT_MAX_NUM		(2)

typedef struct{
  	u32	successCnt;
  	u32 missedCnt;
  	u32	failedCnt;
  	u32 lastPkt;
}iso_test_receive_infor_t;

typedef struct{
  	u32	send_pkt_cnt;
  	u32 isoTestSendTick;
}iso_test_trasmit_infor_t;

typedef struct{

	u8  occupy;
  	u8 	isoTestMode;  // 0: test mode disable, 1: transmit  2: receive
  	u8  isoTest_payload_type;
  	u8  rsvd1;

	union{
			iso_test_receive_infor_t recMode;
			iso_test_trasmit_infor_t tranMode;
	};

}iso_test_param_t;

typedef enum{
	ISO_TEST_ZERO,
	ISO_TEST_VARIABLE,
	ISO_TEST_MAXIMUM,
}iso_test_payload_type_t;

/**
 * @brief      This function is used to initialize LE ISO test mode.
 * @param	   none
 * @return     none
 */
void  blc_initIsoTestMode(void);


/**
 * @brief      This function is used to enter ISO test mode, only for testing purposes.
 * @param	   *pCmdParam
 * @return     ble_sts_t
 */
ble_sts_t blc_hci_le_iso_transmit_test_cmd(hci_le_isoTransmitTestCmdParams_t *pCmdParam);


ble_sts_t blc_hci_le_iso_receive_test_cmd(hci_le_isoReceiveTestCmdParams_t *pCmdParam);

#endif /* STACK_BLE_CONTROLLER_LL_ISO_ISO_TEST_H_ */
