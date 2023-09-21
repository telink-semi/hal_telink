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

#include "zephyr/bluetooth/buf.h"
#include <zephyr/storage/flash_map.h>
#if CONFIG_SOC_RISCV_TELINK_B91
#include "stack/ble/B91/ble.h"
#include "stack/ble/B91/ble_format.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "stack/ble/B92/ble.h"
#include "stack/ble/B92/ble_format.h"
#endif
#include "b9x_bt_buffer.h"
#include "b9x_bt_init.h"
#include "b9x_bt_flash.h"
#include "b9x_rf_power.h"

#ifdef CONFIG_PM
#include "ext_driver/ext_pm.h"
#endif /* CONFIG_PM */

#if CONFIG_B9X_BLE_CTRL_EXT_ADV

/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define APP_ADV_SETS_NUMBER CONFIG_B9X_BLE_CTRL_EXT_ADV_SETS_NUM

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA CONFIG_B9X_BLE_CTRL_EXT_ADV_DATA_LEN_MAX

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA CONFIG_B9X_BLE_CTRL_EXT_ADV_SCAN_DATA_LEN_MAX

_attribute_ble_data_retention_ u8 app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8 app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

#endif /* CONFIG_B9X_BLE_CTRL_EXT_ADV */

#if CONFIG_B9X_BLE_CTRL_PER_ADV

/** Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX" */
#define APP_PER_ADV_SETS_NUMBER CONFIG_B9X_BLE_CTRL_PER_ADV_SETS_NUM

/** Maximum Periodic Advertising Data Length */
#define APP_MAX_LENGTH_PER_ADV_DATA CONFIG_B9X_BLE_CTRL_PER_ADV_DATA_LEN_MAX

_attribute_ble_data_retention_ u8
	app_perdAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PER_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_perdAdvData_buffer[APP_MAX_LENGTH_PER_ADV_DATA * APP_PER_ADV_SETS_NUMBER];

#endif /* CONFIG_B9X_BLE_CTRL_PER_ADV */

/**
 * @brief       Telink B9X BLE Controller initialization
 * @param[in]   prx - HCI RX callback
 * @param[in]   ptx -HCI TX callback
 * @return      Status - 0: command succeeded; -1: command failed
 */
int b9x_bt_blc_init(void *prx, void *ptx)
{
	/* random number generator must be initiated here(in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();

	/* for 512K Flash, mac_address equals to 0x76000
	 * for 1M   Flash, mac_address equals to 0xFF000 */
	u8 ble_mac[BLE_ADDR_LEN];

	b9x_bt_blc_mac_init(ble_mac);

	blc_ll_initBasicMCU();

	blc_ll_initStandby_module(ble_mac);

	blc_ll_initLegacyAdvertising_module(); // adv module: 		 mandatory for BLE slave,

	blc_ll_initLegacyScanning_module(); // scan module: 		 mandatory for BLE master

	blc_ll_initInitiating_module(); // initiate module: 	 mandatory for BLE master

	blc_ll_initAclConnection_module();
#ifdef CONFIG_BT_CENTRAL
	blc_ll_initAclMasterRole_module();
#endif /* CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_PERIPHERAL
	blc_ll_initAclSlaveRole_module();
#endif /* CONFIG_BT_PERIPHERAL */

#if CONFIG_B9X_BLE_CTRL_EXT_ADV
	blc_ll_initExtendedAdvertising_module();
	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);
	blc_ll_initExtAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
#endif /* CONFIG_B9X_BLE_CTRL_EXT_ADV */

#if CONFIG_B9X_BLE_CTRL_PER_ADV
	blc_ll_initPeriodicAdvertising_module();
	blc_ll_initPeriodicAdvParamBuffer(app_perdAdvSet_buffer, APP_PER_ADV_SETS_NUMBER);
	blc_ll_initPeriodicAdvDataBuffer(app_perdAdvData_buffer, APP_MAX_LENGTH_PER_ADV_DATA);
#endif /* CONFIG_B9X_BLE_CTRL_PER_ADV */

#if CONFIG_B9X_BLE_CTRL_EXT_SCAN
	blc_ll_initExtendedScanning_module();
#endif /* CONFIG_B9X_BLE_CTRL_EXT_SCAN */

#if CONFIG_B9X_BLE_CTRL_PER_ADV_SYNC
	blc_ll_initPeriodicAdvertisingSynchronization_module();
#endif /* CONFIG_B9X_BLE_CTRL_PER_ADV_SYNC */

	blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_MASTER_MAX_TX_OCTETS,
					 ACL_SLAVE_MAX_TX_OCTETS);

	/* all ACL connection share same RX FIFO */
	if (blc_ll_initAclConnRxFifo(app_acl_rxfifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM) !=
	    BLE_SUCCESS) {
		return INIT_FAILED;
	}

#ifdef CONFIG_BT_CENTRAL
	/* ACL Master TX FIFO */
	if (blc_ll_initAclConnMasterTxFifo(app_acl_mstTxfifo, ACL_MASTER_TX_FIFO_SIZE,
					   ACL_MASTER_TX_FIFO_NUM,
					   CONFIG_B9X_BLE_CTRL_MASTER_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_CENTRAL */

#ifdef CONFIG_BT_PERIPHERAL
	/* ACL Slave TX FIFO */
	if (blc_ll_initAclConnSlaveTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE,
					  ACL_SLAVE_TX_FIFO_NUM,
					  CONFIG_B9X_BLE_CTRL_SLAVE_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if CONFIG_SOC_RISCV_TELINK_B92
	blc_ll_configLegacyAdvEnableStrategy(LEG_ADV_EN_STRATEGY_3);
	blc_ll_configScanEnableStrategy(SCAN_STRATEGY_1);
#endif

	blc_ll_setMaxConnectionNumber(CONFIG_B9X_BLE_CTRL_MASTER_MAX_NUM,
				      CONFIG_B9X_BLE_CTRL_SLAVE_MAX_NUM);
	blc_ll_setAclMasterConnectionInterval(CONFIG_B9X_BLE_CTRL_CONNECTION_INTERVAL_IDX);
	blc_ll_setCreateConnectionTimeout(CONFIG_B9X_BLE_CTRL_CONNECTION_TIMEOUT_MS);
	rf_set_power_level(b9x_tx_pwr_lt[CONFIG_B9X_BLE_CTRL_RF_POWER - B9X_TX_POWER_MIN]);

	blc_ll_initChannelSelectionAlgorithm_2_feature();
	blc_ll_init2MPhyCodedPhy_feature();

	/* HCI RX FIFO */
	if (blc_ll_initHciRxFifo(app_hci_rxfifo, HCI_RX_FIFO_SIZE, HCI_RX_FIFO_NUM) !=
	    BLE_SUCCESS) {
		return INIT_FAILED;
	}

	/* HCI TX FIFO */
	if (blc_ll_initHciTxFifo(app_hci_txfifo, HCI_TX_FIFO_SIZE, HCI_TX_FIFO_NUM) !=
	    BLE_SUCCESS) {
		return INIT_FAILED;
	}

	/* HCI RX ACL FIFO */
	if (blc_ll_initHciAclDataFifo(app_hci_rxAclfifo, HCI_RX_ACL_FIFO_SIZE,
				      HCI_RX_ACL_FIFO_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}

	/* HCI Data && Event */
	blc_hci_registerControllerDataHandler(blc_hci_sendACLData2Host);
	blc_hci_registerControllerEventHandler(
		blc_hci_send_data); // controller hci event to host all processed in this func

	/* bluetooth event */
	blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

	/* bluetooth low energy(LE) event, all enable */
	blc_hci_le_setEventMask_cmd(0xFFFFFFFF);
	blc_hci_le_setEventMask_2_cmd(0x7FFFFFFF);

	u8 check_status = blc_controller_check_appBufferInitialization();
	if (check_status != BLE_SUCCESS) {
		return INIT_FAILED;
	}

	/* HCI configuration */
	blc_register_hci_handler(prx, ptx);

#ifdef CONFIG_PM
	/* Enable PM for BLE stack */
	blc_ll_initPowerManagement_module();

	/* Enable the sleep masks for BLE stack thread */
	blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_SLAVE |
			    PM_SLEEP_ACL_MASTER);
#endif /* CONFIG_PM */

	return INIT_OK;
}
