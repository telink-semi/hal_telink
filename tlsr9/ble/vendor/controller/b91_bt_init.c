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

#include "../../stack/ble/ble.h"
#include "../../stack/ble/ble_format.h"
#include "b91_bt_buffer.h"
#include "b91_bt_init.h"

#define B91_BT_SLEEP_AJUSTMENT (2)

#ifdef CONFIG_PM
#include "ext_driver/ext_pm.h"
#endif /* CONFIG_PM */

#if CONFIG_B91_BLE_CTRL_EXT_ADV

/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define APP_ADV_SETS_NUMBER CONFIG_B91_BLE_CTRL_EXT_ADV_SETS_NUM

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA CONFIG_B91_BLE_CTRL_EXT_ADV_DATA_LEN_MAX

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA CONFIG_B91_BLE_CTRL_EXT_ADV_SCAN_DATA_LEN_MAX

_attribute_ble_data_retention_ u8 app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8 app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

#endif /* CONFIG_B91_BLE_CTRL_EXT_ADV */

#if CONFIG_B91_BLE_CTRL_PER_ADV

/** Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX" */
#define APP_PER_ADV_SETS_NUMBER CONFIG_B91_BLE_CTRL_PER_ADV_SETS_NUM

/** Maximum Periodic Advertising Data Length */
#define APP_MAX_LENGTH_PER_ADV_DATA CONFIG_B91_BLE_CTRL_PER_ADV_DATA_LEN_MAX

_attribute_ble_data_retention_ u8
	app_perdAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PER_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_perdAdvData_buffer[APP_MAX_LENGTH_PER_ADV_DATA * APP_PER_ADV_SETS_NUMBER];

#endif /* CONFIG_B91_BLE_CTRL_PER_ADV */

/**
 * @brief		This function is used to initialize the MAC address
 * @param[in]	flash_addr - flash address for MAC address
 * @param[in]	mac_public - public address
 * @param[in]	mac_random_static - random static MAC address
 * @return      none
 */
_attribute_no_inline_ static void b91_bt_blc_mac_init(int flash_addr, u8 *mac_public,
						      u8 *mac_random_static)
{
	if (flash_addr == 0) {
		return;
	}

	u8 mac_read[8];
	flash_read_page(flash_addr, 8, mac_read);

	u8 value_rand[5];
	generateRandomNum(5, value_rand);

	u8 ff_six_byte[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	if (memcmp(mac_read, ff_six_byte, 6)) {
		memcpy(mac_public, mac_read, 6); /* copy public address from flash */
	} else {				 /* no public address in flash */
		mac_public[0] = value_rand[0];
		mac_public[1] = value_rand[1];
		mac_public[2] = value_rand[2];
		mac_public[3] = 0x38; /* company id: 0xA4C138 */
		mac_public[4] = 0xC1;
		mac_public[5] = 0xA4;

		flash_write_page(flash_addr, 6, mac_public);
	}

	mac_random_static[0] = mac_public[0];
	mac_random_static[1] = mac_public[1];
	mac_random_static[2] = mac_public[2];
	mac_random_static[5] = 0xC0; /* for random static */

	u16 high_2_byte = (mac_read[6] | mac_read[7] << 8);
	if (high_2_byte != 0xFFFF) {
		memcpy((u8 *)(mac_random_static + 3), (u8 *)(mac_read + 6), 2);
	} else {
		mac_random_static[3] = value_rand[3];
		mac_random_static[4] = value_rand[4];

		flash_write_page(flash_addr + 6, 2, (u8 *)(mac_random_static + 3));
	}
}

/**
 * @brief       Telink B91 BLE Controller initialization
 * @param[in]   prx - HCI RX callback
 * @param[in]   ptx -HCI TX callback
 * @return      Status - 0: command succeeded; -1: command failed
 */
int b91_bt_blc_init(void *prx, void *ptx)
{
	/* random number generator must be initiated here(in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();

	/* for 512K Flash, mac_address equals to 0x76000
	 * for 1M   Flash, mac_address equals to 0xFF000 */
	u8 mac_public[BLE_ADDR_LEN];
	u8 mac_random_static[BLE_ADDR_LEN];
	b91_bt_blc_mac_init(CONFIG_B91_BLE_CTRL_MAC_FLASH_ADDR, mac_public, mac_random_static);

	blc_ll_initBasicMCU();

#ifdef CONFIG_B91_BLE_CTRL_MAC_PUBLIC
	blc_ll_initStandby_module(mac_public);
#else
	blc_ll_initStandby_module(mac_random_static);
#endif /* CONFIG_B91_BLE_CTRL_MAC_PUBLIC */

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

#if CONFIG_B91_BLE_CTRL_EXT_ADV
	blc_ll_initExtendedAdvertising_module();
	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);
	blc_ll_initExtAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
#endif /* CONFIG_B91_BLE_CTRL_EXT_ADV */

#if CONFIG_B91_BLE_CTRL_PER_ADV
	blc_ll_initPeriodicAdvertising_module();
	blc_ll_initPeriodicAdvParamBuffer(app_perdAdvSet_buffer, APP_PER_ADV_SETS_NUMBER);
	blc_ll_initPeriodicAdvDataBuffer(app_perdAdvData_buffer, APP_MAX_LENGTH_PER_ADV_DATA);
#endif /* CONFIG_B91_BLE_CTRL_PER_ADV */

#if CONFIG_B91_BLE_CTRL_EXT_SCAN
	blc_ll_initExtendedScanning_module();
#endif /* CONFIG_B91_BLE_CTRL_EXT_SCAN */

#if CONFIG_B91_BLE_CTRL_PER_ADV_SYNC
	blc_ll_initPeriodicAdvertisingSynchronization_module();
#endif /* CONFIG_B91_BLE_CTRL_PER_ADV_SYNC */

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
					   CONFIG_B91_BLE_CTRL_MASTER_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_CENTRAL */

#ifdef CONFIG_BT_PERIPHERAL
	/* ACL Slave TX FIFO */
	if (blc_ll_initAclConnSlaveTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE,
					  ACL_SLAVE_TX_FIFO_NUM,
					  CONFIG_B91_BLE_CTRL_SLAVE_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_PERIPHERAL */

	blc_ll_ConfigLegacyAdvEnable_by_API_only(LEG_ADV_STRATEGY_2);
	blc_ll_ConfigLegacyScanEnable_by_API_only(LEG_SCAN_STRATEGY_2);

	blc_ll_setMaxConnectionNumber(CONFIG_B91_BLE_CTRL_MASTER_MAX_NUM,
				      CONFIG_B91_BLE_CTRL_SLAVE_MAX_NUM);
	blc_ll_setAclMasterConnectionInterval(CONFIG_B91_BLE_CTRL_CONNECTION_INTERVAL_IDX);
	blc_ll_setCreateConnectionTimeout(CONFIG_B91_BLE_CTRL_CONNECTION_TIMEOUT_MS);

	rf_set_power_level_index(CONFIG_B91_BLE_CTRL_RF_POWER);

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
