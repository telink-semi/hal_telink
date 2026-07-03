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
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tlx_bt_init, LOG_LEVEL_INF);

#include "zephyr/bluetooth/buf.h"
#include <zephyr/storage/flash_map.h>
#include "stack/ble/ble.h"
#include "stack/ble/ble_format.h"
#include "tlx_bt_buffer.h"
#include "tlx_bt_init.h"
#include "tlx_bt_flash.h"
#include "tlx_bt_cs.h"
#include "tl_rf_power.h"

#ifdef CONFIG_PM
#include "ext_driver/ext_pm.h"
#endif /* CONFIG_PM */

#if CONFIG_TL_BLE_CTRL_EXT_ADV

/** Number of Supported Advertising Sets, no exceed "ADV_SETS_NUMBER_MAX" */
#define APP_ADV_SETS_NUMBER CONFIG_TL_BLE_CTRL_EXT_ADV_SETS_NUM

/** Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_ADV_DATA CONFIG_TL_BLE_CTRL_EXT_ADV_DATA_LEN_MAX

/** Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough) */
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA CONFIG_TL_BLE_CTRL_EXT_ADV_SCAN_DATA_LEN_MAX

_attribute_ble_data_retention_ u8 app_advSet_buffer[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8 app_advData_buffer[APP_MAX_LENGTH_ADV_DATA * APP_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_scanRspData_buffer[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

#endif /* CONFIG_TL_BLE_CTRL_EXT_ADV */

#if CONFIG_TL_BLE_CTRL_PER_ADV

/** Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX" */
#define APP_PER_ADV_SETS_NUMBER CONFIG_TL_BLE_CTRL_PER_ADV_SETS_NUM

/** Maximum Periodic Advertising Data Length */
#define APP_MAX_LENGTH_PER_ADV_DATA CONFIG_TL_BLE_CTRL_PER_ADV_DATA_LEN_MAX

_attribute_ble_data_retention_ u8
	app_perdAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PER_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8
	app_perdAdvData_buffer[APP_MAX_LENGTH_PER_ADV_DATA * APP_PER_ADV_SETS_NUMBER];

#endif /* CONFIG_TL_BLE_CTRL_PER_ADV */

/** a temporary method to set exit latency which can be set in header files */
#if CONFIG_SOC_RISCV_TELINK_TL321X
	#define SUSPEND_EXIT_LATENCY_US		(320U)
	#define DEEPRETN_EXIT_LATENCY_US	(1000U)		/*!< Not used for now */
#elif CONFIG_SOC_RISCV_TELINK_TL323X
	/* for tl323x, if in pm mode the cclk is 48M, the early wakeup should be 500us.
	 * In non-pm mode ,the cclk is 96M, the early wakeup should be 400us, will be
	 * more stable
	 */
	#if CONFIG_PM
	/*!< No Fast Settle Used For Now, Update Later */
	#define SUSPEND_EXIT_LATENCY_US		(500U)
	#else
	/*!< No Fast Settle Used For Now, Update Later */
	#define SUSPEND_EXIT_LATENCY_US		(400U)
	#endif
	/*!< Not used for now */
	#define DEEPRETN_EXIT_LATENCY_US	(1000U)
#elif CONFIG_SOC_RISCV_TELINK_TL521X
	/* for tl521x, if in pm mode the cclk is 48M, the early wakeup should be 500us.
	 * In non-pm mode ,the cclk is 96M, the early wakeup should be 400us, will be
	 * more stable
	 */
	#if CONFIG_PM
	/*!< No Fast Settle Used For Now, Update Later */
	#define SUSPEND_EXIT_LATENCY_US		(260U)
	#else
	/*!< No Fast Settle Used For Now, Update Later */
	#define SUSPEND_EXIT_LATENCY_US		(300U)
	#endif
	/*!< Not used for now */
	#define DEEPRETN_EXIT_LATENCY_US	(0U)
#elif CONFIG_SOC_RISCV_TELINK_TL721X
	#define SUSPEND_EXIT_LATENCY_US		(250U)
	/*!< Not used for now */
	#define DEEPRETN_EXIT_LATENCY_US	(0U)
#endif

/**
 * @brief       Telink TLX BLE Controller initialization
 * @param[in]   prx - HCI RX callback
 * @param[in]   ptx -HCI TX callback
 * @return      Status - 0: command succeeded; -1: command failed
 */
#ifndef TLK_ONLY_BLE_HOST
int tlx_bt_blc_init(void *prx, void *ptx)
{
	/* random number generator must be initiated here(in the beginning of user_init_nromal).
	 * When deepSleep retention wakeUp, no need initialize again */
	random_generator_init();

	/* for 512K Flash, mac_address equals to 0x76000
	 * for 1M   Flash, mac_address equals to 0xFF000 */
	u8 ble_mac[BLE_ADDR_LEN];

	tlx_bt_blc_mac_init(ble_mac);

	blc_ll_initBasicMCU();

	blc_ll_initStandby_module(ble_mac);

	blc_ll_initLegacyAdvertising_module(); // adv module: 		 mandatory for BLE slave,

	blc_ll_initInitiating_module(); // initiate module: 	 mandatory for BLE master

	blc_ll_initAclConnection_module();
#ifdef CONFIG_BT_CENTRAL
	blc_ll_initAclMasterRole_module();
#endif /* CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_PERIPHERAL
	blc_ll_initAclSlaveRole_module();
#endif /* CONFIG_BT_PERIPHERAL */

#if CONFIG_TL_BLE_CTRL_EXT_ADV
	blc_ll_initExtendedAdvertising_module();
	blc_ll_initExtendedAdvSetBuffer(app_advSet_buffer, APP_ADV_SETS_NUMBER);
	blc_ll_initExtAdvDataBuffer(app_advData_buffer, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData_buffer, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
#endif /* CONFIG_TL_BLE_CTRL_EXT_ADV */

#if CONFIG_TL_BLE_CTRL_PER_ADV
	blc_ll_initPeriodicAdvertising_module();
	blc_ll_initPeriodicAdvParamBuffer(app_perdAdvSet_buffer, APP_PER_ADV_SETS_NUMBER);
	blc_ll_initPeriodicAdvDataBuffer(app_perdAdvData_buffer, APP_MAX_LENGTH_PER_ADV_DATA);
#endif /* CONFIG_TL_BLE_CTRL_PER_ADV */

#if CONFIG_TL_BLE_CTRL_EXT_SCAN
	blc_ll_initExtendedScanning_module();
#endif /* CONFIG_TL_BLE_CTRL_EXT_SCAN */

#if CONFIG_TL_BLE_CTRL_PER_ADV_SYNC
	blc_ll_initPeriodicAdvertisingSynchronization_module();
#endif /* CONFIG_TL_BLE_CTRL_PER_ADV_SYNC */

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
					   CONFIG_TL_BLE_CTRL_MASTER_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_CENTRAL */

#ifdef CONFIG_BT_PERIPHERAL
	/* ACL Slave TX FIFO */
	if (blc_ll_initAclConnSlaveTxFifo(app_acl_slvTxfifo, ACL_SLAVE_TX_FIFO_SIZE,
					  ACL_SLAVE_TX_FIFO_NUM,
					  CONFIG_TL_BLE_CTRL_SLAVE_MAX_NUM) != BLE_SUCCESS) {
		return INIT_FAILED;
	}
#endif /* CONFIG_BT_PERIPHERAL */

#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL322X || CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X
	blc_ll_configLegacyAdvEnableStrategy(LEG_ADV_EN_STRATEGY_3);
#endif

	blc_ll_setMaxConnectionNumber(CONFIG_TL_BLE_CTRL_MASTER_MAX_NUM,
				      CONFIG_TL_BLE_CTRL_SLAVE_MAX_NUM);
	blc_ll_setAclMasterConnectionInterval(CONFIG_TL_BLE_CTRL_CONNECTION_INTERVAL_IDX);
	blc_ll_setCreateConnectionTimeout(CONFIG_TL_BLE_CTRL_CONNECTION_TIMEOUT_MS);
	rf_set_power_level(tl_tx_pwr_lt[CONFIG_TL_BLE_CTRL_RF_POWER - TL_TX_POWER_MIN]);

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

#ifdef CONFIG_BT_CHANNEL_SOUNDING
	app_channel_sounding_init();
#endif


#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
blc_smp_configPairingSecurityInfoStorageAddressAndSize(0x1EC000, 2 * 4096);
blc_smp_smpParamInit();
#endif

	u8 check_status = blc_controller_check_appBufferInitialization();
	if (check_status != BLE_SUCCESS) {
		return INIT_FAILED;
	}
	/* HCI configuration */
	blc_register_hci_handler(prx, ptx);
#ifdef CONFIG_PM
	/* Enable PM for BLE stack */
	blc_ll_initPowerManagement_module();
	blc_ll_enOsPowerManagement_module();
	/* Enable the sleep masks for BLE stack thread */
	blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_SLAVE |
			    PM_SLEEP_ACL_MASTER);
	#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X || CONFIG_SOC_RISCV_TELINK_TL323X || CONFIG_SOC_RISCV_TELINK_TL521X
	extern void blc_ll_setOsLowPowerExitLatencyUs(uint32_t suspendUs, uint32_t deepretUs);
	blc_ll_setOsLowPowerExitLatencyUs(SUSPEND_EXIT_LATENCY_US, DEEPRETN_EXIT_LATENCY_US);
	#endif
#endif /* CONFIG_PM */

	return INIT_OK;
}

#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST
#include <zephyr/device.h>
#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/net/ieee802154.h>
#include "ieee802154_tlx.h"
#include "thd_task.h"
#include "debug_gpio.h"

extern void tlx_init_ble_rf_hw(void);
extern void tlx_init_802154_rf_hw(void);
extern void tlx_rf_tx_is_sending(void);
extern void tlx_rf_isr(const void *parameter);
extern bool blc_ll_isOnly802p15p4ScanTaskBusy(void);

extern void openthread_suspend(void);
extern void openthread_resume(void);
extern void net_if_thread_suspend(void);
extern void net_if_thread_resume(void);
extern void net_tc_threads_suspend(void);
extern void net_tc_threads_resume(void);
extern void ot_radio_workq_thread_suspend(void);
extern void ot_radio_workq_thread_resume(void);
extern volatile bool tlx_rf_zigbee_250K_mode;

volatile bool tlx_rf_802154_mode;
volatile bool tlx_openthread_threads_suspend;
extern struct k_sem ieee802154_task_ready_sem;

/* Suspend OpenThread and 802.15.4 threads at the same time */
_attribute_ram_code_
static void tlx_suspend_openthread_threads(void)
{
	const struct device *const radio_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

	struct tlx_data *tlx = radio_dev->data;

	if(k_sem_count_get(&tlx->ack_wait) == 0) {
		LOG_ERR("ack_wait taking\n");
		/* release ack wait semaphore */
		k_sem_give(&tlx->ack_wait);
		/* ack wait failure, disable ack handler */
		tlx->ack_handler_en = false;
	}

	k_sched_lock();
	ot_radio_workq_thread_suspend();
	openthread_suspend();
	net_if_thread_suspend();
	net_tc_threads_suspend();
	k_sched_unlock();
	tlx_openthread_threads_suspend = true;
}

/* Resume OpenThread and 802.15.4 threads at the same time */
_attribute_ram_code_
static void tlx_resume_openthread_threads(void)
{
	k_sched_lock();
	net_if_thread_resume();
	net_tc_threads_resume();
	openthread_resume();
	ot_radio_workq_thread_resume();
	k_sched_unlock();
	tlx_openthread_threads_suspend = false;
}

_attribute_ram_code_
void tlx_switch_to_802154_mode(void)
{
	DBG_OT_BLE_CHN2_HIGH;
	// tlx_init_802154_rf_hw();
	tlx_resume_openthread_threads();
	k_sem_give(&ieee802154_task_ready_sem);
	tlx_rf_zigbee_250K_mode = false;
	tlx_rf_802154_mode = true;
}

_attribute_ram_code_
void tlx_switch_to_ble_mode(void)
{
	DBG_OT_BLE_CHN2_LOW;
	tlx_rf_tx_is_sending();
	tlx_suspend_openthread_threads();
	k_sem_reset(&ieee802154_task_ready_sem);
	// tlx_rf_zigbee_250K_mode = true;
	tlx_rf_802154_mode = false;
}

void tlx_switch_to_802154_rf_irq_routine(void)
{
#ifdef CONFIG_IEEE802154_TLX_BLE_COEXIST
#if CONFIG_DYNAMIC_INTERRUPTS
		/* lock interrupts */
		unsigned int key = irq_lock();
		irq_connect_dynamic(15, 2,
				(void (*)(const void *))tlx_rf_isr, 0, 0);
		/* unlock interrupts */
		irq_unlock(key);
#else
	#error "error occurred, BLE + 802154 dual-mode need to enable the dynamic interrupts"
#endif
#endif
}

/**
 * @brief   Enable BLE and 802.15.4 coexistence mode
 * @param   None.
 * @return  None.
 */
void tlx_bt_802154_dual_mode_enable(void)
{
	// tlksdk_thd_initFlexibleTask_module();// add by junwei todo
    tlksdk_thd_initInsertTask1_module();
	tlksdk_thd_registerSwitchTo802154RfCb(tlx_init_802154_rf_hw, tlx_init_ble_rf_hw);
	tlksdk_thd_registerModeChangeCb(tlx_switch_to_802154_mode, tlx_switch_to_ble_mode);

	// // tlksdk_thd_enableFlexibleTask(THD_TASK_ENABLE);
	// tlksdk_thd_enableInsertTask1(THD_TASK_ENABLE);
	gpio_function_en(GPIO_CHN0);
    gpio_output_en(GPIO_CHN0);
	gpio_function_en(GPIO_CHN1);
    gpio_output_en(GPIO_CHN1);
	gpio_function_en(GPIO_CHN2);
    gpio_output_en(GPIO_CHN2);
	gpio_function_en(GPIO_CHN3);
    gpio_output_en(GPIO_CHN3);
	DBG_OT_BLE_CHN0_HIGH;


}

/**
 * @brief   Disable BLE and 802.15.4 coexistence mode
 * @param   None.
 * @return  None.
 */
void tlx_bt_802154_dual_mode_disable(void)
{
	// tlksdk_thd_enableFlexibleTask(THD_TASK_DISABLE);
    tlksdk_thd_enableInsertTask1(THD_TASK_DISABLE);
	DBG_OT_BLE_CHN0_LOW;
}

#endif /* IEEE802154_TLX_BLE_COEXIST */
#endif /* TLK_ONLY_BLE_HOST */