/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef LL_H_
#define LL_H_



/**
 * @brief	Telink defined LinkLayer Event Callback
 */
typedef void (*blt_event_callback_t)(u8 e, u8 *p, int n);
typedef void (*user_irq_handler_cb_t)(void);
extern  user_irq_handler_cb_t  usr_irq_handler_cb;


typedef enum{
	BLT_EV_FLAG_RX_DATA_ABANDON,
	BLT_EV_FLAG_GPIO_EARLY_WAKEUP,
	BLT_EV_FLAG_SLEEP_ENTER,
	BLT_EV_FLAG_SUSPEND_EXIT,
	BLT_EV_FLAG_KEY_MISSING,
	BLT_EV_MAX_NUM,
}blt_ev_flag_t;







typedef enum{
	LL_FEATURE_ENABLE	= 1,
	LL_FEATURE_DISABLE  = 0,
}ll_feature_value_t;



/**
 * @brief	Telink defined LinkLayer Event callBack
 * @param[in]	e - event number, must use element of "blt_ev_flag_t"
 * @param[in]	p - callBack function
 * @return	none
 */
void 		blc_ll_registerTelinkControllerEventCallback (u8 e, blt_event_callback_t p);

/**
 * @brief	irq_handler for BLE stack, process system tick interrupt and RF interrupt
 * @param	none
 * @return	none
 */
void 		blc_sdk_irq_handler(void);

/**
 * @brief   main_loop for BLE stack, process data and event
 * @param	none
 * @return	none
 */
void 		blc_sdk_main_loop (void);



/**
 * @brief      for user to initialize MCU
 * @param	   none
 * @return     none
 */
void 		blc_ll_initBasicMCU (void);


/**
 * @brief      for user to initialize default RF TX power level index
 * @param      rfPwrLvlIdx - refer to 'rf_power_level_index_e'
 * @return     none
 */
void        blc_ll_setDefaultTxPowerLevel (rf_power_level_index_e rfPwrLvlIdx);


/**
 * @brief      for user to initialize link layer Standby state
 * @param	   none
 * @return     none
 */
void 		blc_ll_initStandby_module (u8 *public_adr);


/**
 * @brief      this function is used to read MAC address
 * @param[in]  *addr -  The address where the read value(MAC address) prepare to write.
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t   blc_ll_readBDAddr(u8 *addr);


/**
 * @brief      this function is used to set the LE Random Device Address in the Controller
 * @param[in]  *randomAddr -  Random Device Address
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t 	blc_ll_setRandomAddr(u8 *randomAddr);


/**
 * @brief      This function is used to check if the address's type is public
 * @param[in]  *addr -  The address need to check.
 * @return     bool, 0x00: no public, 0x01: Public
 */
bool 		blc_ll_isValidPublicAddr(u8* addr);


/**
 * @brief      This function is used to check if the address's type is random
 * @param[in]  *addr -  The address need to check.
 * @return     bool, 0x00: no random, 0x01: random
 */
bool 		blc_ll_isValidRandomAddr(u8* addr);


/**
 * @brief      This function is used to check if owner's address type is valid
 * @param[in]  ownAddrType -  Owner address type.
 * @param[in]  randomAddr -  If Owner's address type is Random, input Random address.
 * @return     bool, 0x00: invalid, 0x01: valid
 */
bool 		blc_ll_isValidOwnAddrByAddrType(u8 ownAddrType, u8* randomAddr);


/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the master role is valid.
 * @param[in]  bit_number - Bit position in the FeatureSet.
 * @param[in]  bit_value - refer to the struct "ll_feature_value_t".
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_ll_setHostFeature(u8 bit_number, ll_feature_value_t bit_value);


/**
 * @brief      this function is used check if any controller buffer initialized by application incorrect.
 * 			   attention: this function must be called at the end of BLE LinkLayer Initialization.
 * @param	   none
 * @return     status, 0x00:  succeed, no buffer error
 * 					   other: buffer error code
 */
init_err_t	blc_contr_checkControllerInitialization(void);

#define	blc_controller_check_appBufferInitialization		blc_contr_checkControllerInitialization

/**
 * @brief      this function is used by the Host to specify a channel classification based on its local information,
 *             only the master role is valid.
 * @param[in]  *map - channel map
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t 	blc_ll_setHostChannel(u8 * chnMap);


/**
 * @brief      this function is used to read the minimum and maximum transmit powers supported by the Controller
 *             across all supported PHYs.
 * @param[in]  *pMinTxPwr - Min_TX_Power
 * @param[in]  *pMaxTxPwr - Max_TX_Power
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_ll_readSuppTxPower (s8 *pOutMinTxPwr, s8 *pOutMaxTxPwr);


/**
 * @brief      this function is used to read the RF Path Compensation Values parameter used in the Tx Power Level
 *             and RSSI calculation.
 * @param[in]  *pTxPathComp - RF_TX_Path_Compensation_Value
 * @param[in]  *pRxPathComp - RF_RX_Path_Compensation_Value
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_ll_readRfPathComp(s16 *pOutTxPathComp, s16 *pOutRxPathComp);


/**
 * @brief      this function is used to indicate the RF path gain or loss between the RF transceiver and the
 *             antenna contributed by intermediate components.
 * @param[in]  txPathComp - RF_TX_Path_Compensation_Value
 * @param[in]  rxPathComp - RF_RX_Path_Compensation_Value
 * @return     status, 0x00:  succeed
 * 			           other: failed
 */
ble_sts_t	blc_ll_writeRfPathComp(s16 txPathComp, s16 rxPathComp);


/**
 * @brief      this function is used to read the minimum and maximum transmit powers supported by the Controller
 *             across all supported PHYs.
 * @param[in]  refer to 'hci_le_rdSuppTxPwrRetParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_readSuppTxPower(hci_le_rdSuppTxPwrRetParams_t *retPara);


/**
 * @brief      this function is used to read the RF Path Compensation Values parameter used in the Tx Power Level
 *             and RSSI calculation.
 * @param[in]  refer to 'hci_le_rdRfPathCompRetParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_readRfPathComp(hci_le_rdRfPathCompRetParams_t *retPara);


/**
 * @brief      this function is used to indicate the RF path gain or loss between the RF transceiver and the
 *             antenna contributed by intermediate components.
 * @param[in]  refer to 'hci_le_writeRfPathCompCmdParams_t'
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_writeRfPathComp(hci_le_writeRfPathCompCmdParams_t *cmdPara);


/**
 * @brief      this function is used to reset module of all.
 * @param	   none
 * @return     status, 0x00:  succeed, no buffer error
 * 					   other: buffer error code
 */
ble_sts_t  	blc_hci_reset(void);


/**
 * @brief      this function checks whether the Bluetooth stack task is IDLE
 * @param	   none
 * @return     status, 0:  idle
 *                     1:  task
 */
u32 blc_ll_checkBleTaskIsIdle(void);


#endif /* LL_H_ */
