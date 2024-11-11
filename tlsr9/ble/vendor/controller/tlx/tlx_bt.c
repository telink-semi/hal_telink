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
#include <zephyr/kernel.h>
#ifdef CONFIG_PM
#include <zephyr/pm/policy.h>
#endif /* CONFIG_PM */

#undef irq_enable
#undef irq_disable
#undef ARRAY_SIZE

#include "tlx_bt.h"
#include "tlx_bt_init.h"
#include "compiler.h"
#include "drivers.h"
#include <stdlib.h>
#include "tlx_bt_buffer.h"
#if CONFIG_SOC_RISCV_TELINK_TL321X
#include "stack/ble/TL321X/controller/ble_controller.h"
#include "stack/ble/TL321X/controller/os_sup.h"
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#include "stack/ble/TL721X/controller/ble_controller.h"
#include "stack/ble/TL721X/os_sup.h"
#endif

/* Module defines */
#define BLE_THREAD_STACK_SIZE CONFIG_TL_BLE_CTRL_THREAD_STACK_SIZE
#define BLE_THREAD_PRIORITY CONFIG_TL_BLE_CTRL_THREAD_PRIORITY
#define BLE_CONTROLLER_SEMAPHORE_MAX 50

#define BYTES_TO_UINT16(n, p)                                                                      \
	{                                                                                          \
		n = ((u16)(p)[0] + ((u16)(p)[1] << 8));                                            \
	}
#define BSTREAM_TO_UINT16(n, p)                                                                    \
	{                                                                                          \
		BYTES_TO_UINT16(n, p);                                                             \
		p += 2;                                                                            \
	}

static volatile enum tl_bt_controller_state tl_bt_state = TL_BT_CONTROLLER_STATE_STOPPED;
static void tlx_bt_controller_thread();
K_THREAD_STACK_DEFINE(tlx_bt_controller_thread_stack, BLE_THREAD_STACK_SIZE);
static struct k_thread tlx_bt_controller_thread_data;

/**
 * @brief    Semaphore define for controller.
 */
K_SEM_DEFINE(controller_sem, 0, BLE_CONTROLLER_SEMAPHORE_MAX);

/**
 * @brief    BLE semaphore callback.
 */
_attribute_ram_code_ void os_give_sem_cb(void)
{
	k_sem_give(&controller_sem);
}

static struct tlx_ctrl_t {
	tlx_bt_host_callback_t callbacks;
} tlx_ctrl;

/**
 * @brief    RF driver interrupt handler
 */
_attribute_ram_code_ void rf_irq_handler(const void *param)
{
	(void)param;

	blc_sdk_irq_handler();
}

/**
 * @brief    System Timer interrupt handler
 */
_attribute_ram_code_ void stimer_irq_handler(const void *param)
{
	(void)param;

	blc_sdk_irq_handler();
}

/**
 * @brief    BLE Controller HCI Tx callback implementation
 */
static int tlx_bt_hci_tx_handler(void)
{
	/* check for data available */
	while(bltHci_txfifo.wptr != bltHci_txfifo.rptr)
	{
		/* Get HCI data */
		u8 *p = bltHci_txfifo.p + (bltHci_txfifo.rptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
		if (p) {
			u32 len;
			BSTREAM_TO_UINT16(len, p);
			bltHci_txfifo.rptr++;

			if (tl_bt_state == TL_BT_CONTROLLER_STATE_ACTIVE) {
				/* Send data to the host */
				if (tlx_ctrl.callbacks.host_read_packet) {
					tlx_ctrl.callbacks.host_read_packet(p, len);
				}
			} else if (tl_bt_state == TL_BT_CONTROLLER_STATE_STOPPING) {
				/* In this state HCI reset is sent - waiting for command complete */
				static const uint8_t hci_reset_cmd_complette[] = {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};

				if (len == sizeof(hci_reset_cmd_complette) && !memcmp(p, hci_reset_cmd_complette, len)) {
					tl_bt_state = TL_BT_CONTROLLER_STATE_STOPPED;
					k_sem_give(&controller_sem);
				}
			}
		}
	}

	return 0;
}

/**
 * @brief    BLE Controller HCI Rx callback implementation
 */
static int tlx_bt_hci_rx_handler(void)
{
	/* Check for data available */
	if (bltHci_rxfifo.wptr == bltHci_rxfifo.rptr) {
		/* No data to process, send host_send_available message to the host */
		if (tlx_ctrl.callbacks.host_send_available) {
			tlx_ctrl.callbacks.host_send_available();
		}

		return 0;
	}

	/* Get HCI data */
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if (p) {
		/* Send data to the controller */
		blc_hci_handler(&p[0], 0);
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
		if (p[0] == HCI_TYPE_ACL_DATA) {
			k_sem_give(&controller_sem);
		}
#endif
		bltHci_rxfifo.rptr++;
	}

	return 0;
}

/**
 * @brief    Telink TLX BLE Controller thread
 */
static void tlx_bt_controller_thread()
{
	while (tl_bt_state == TL_BT_CONTROLLER_STATE_ACTIVE ||
		tl_bt_state == TL_BT_CONTROLLER_STATE_STOPPING) {
		k_sem_take(&controller_sem, K_FOREVER);
		blc_sdk_main_loop();
	}
}

/**
 * @brief    BLE Controller IRQs initialization
 */
static void tlx_bt_irq_init()
{
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
	plic_preempt_feature_dis();
	flash_plic_preempt_config(0,1);
#endif

	/* Init STimer IRQ */
	IRQ_CONNECT(IRQ_SYSTIMER + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, stimer_irq_handler, 0, 0);
	/* Init RF IRQ */
#if CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#else
	IRQ_CONNECT(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#endif
	plic_set_priority(IRQ_SYSTIMER, 2);
	plic_set_priority(IRQ_ZB_RT, 2);
}

/**
 * @brief    Telink TLX BLE Controller initialization
 * @return   Status - 0: command succeeded; -1: command failed
 */
int tlx_bt_controller_init()
{
	int status;

#if CONFIG_PM && CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif /* CONFIG_PM && CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */

	/* Reset Radio */
	rf_radio_reset();

	/* Init RF driver */
	rf_drv_ble_init();

#ifdef CONFIG_BT_CENTRAL
	app_acl_mstTxfifo = (u8 *)calloc(ACL_MASTER_TX_FIFO_SIZE * ACL_MASTER_TX_FIFO_NUM * CONFIG_TL_BLE_CTRL_MASTER_MAX_NUM,1);
#endif /* CONFIG_BT_CENTRAL */

#ifdef CONFIG_BT_PERIPHERAL
	app_acl_slvTxfifo = (u8 *)calloc(ACL_SLAVE_TX_FIFO_SIZE * ACL_SLAVE_TX_FIFO_NUM * CONFIG_TL_BLE_CTRL_SLAVE_MAX_NUM,1);
#endif /* CONFIG_BT_PERIPHERAL */
	
	app_acl_rxfifo = (u8 *)calloc(ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM,1);
	app_hci_rxfifo = (u8 *)calloc(HCI_RX_FIFO_SIZE * HCI_RX_FIFO_NUM,1);
	app_hci_txfifo = (u8 *)calloc(HCI_TX_FIFO_SIZE * HCI_TX_FIFO_NUM,1);
	app_hci_rxAclfifo = (u8 *)calloc(HCI_RX_ACL_FIFO_SIZE * HCI_RX_ACL_FIFO_NUM,1);

	/* Init BLE Controller stack */
	status = tlx_bt_blc_init(tlx_bt_hci_rx_handler, tlx_bt_hci_tx_handler);
	if (status != INIT_OK) {
		return status;
	}

	/* Init IRQs */
	tlx_bt_irq_init();

	/* Register callback to controller. */
#if CONFIG_SOC_RISCV_TELINK_TL321X || CONFIG_SOC_RISCV_TELINK_TL721X
	blc_ll_registerGiveSemCb(os_give_sem_cb, os_give_sem_cb);
	blc_setOsSupEnable(true);
#endif
	/* init semaphore */
	k_sem_reset(&controller_sem);
	k_sem_give(&controller_sem);

	/* Create BLE main thread */
	(void)k_thread_create(&tlx_bt_controller_thread_data,
		tlx_bt_controller_thread_stack, K_THREAD_STACK_SIZEOF(tlx_bt_controller_thread_stack),
		tlx_bt_controller_thread, NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, K_NO_WAIT);
#if CONFIG_SOC_RISCV_TELINK_TL321X
		(void)k_thread_name_set(&tlx_bt_controller_thread_data, "TL321X_BT");
#elif CONFIG_SOC_RISCV_TELINK_TL721X
		(void)k_thread_name_set(&tlx_bt_controller_thread_data, "TL721X_BT");
#endif

	/* Start thread */
	tl_bt_state = TL_BT_CONTROLLER_STATE_ACTIVE;
	k_thread_start(&tlx_bt_controller_thread_data);

	return status;
}

/**
 * @brief    Telink TLX BLE Controller deinitialization
 */
void tlx_bt_controller_deinit()
{
	/* start BLE stopping procedure */
	tl_bt_state = TL_BT_CONTROLLER_STATE_STOPPING;

	/* reset controller */
	static const uint8_t hci_reset_cmd[] = {0x03, 0x0c, 0x00};
	tlx_bt_host_send_packet(0x01, hci_reset_cmd, sizeof(hci_reset_cmd));

	/* wait thread finish */
	(void)k_thread_join(&tlx_bt_controller_thread_data, K_FOREVER);

	/* disable interrupts */
	plic_interrupt_disable(IRQ_SYSTIMER);
	plic_interrupt_disable(IRQ_ZB_RT);

	/* Reset Radio */
	rf_radio_reset();

#ifdef CONFIG_BT_CENTRAL
	free(app_acl_mstTxfifo);
#endif /* CONFIG_BT_CENTRAL */
#ifdef CONFIG_BT_PERIPHERAL
	free(app_acl_slvTxfifo);
#endif /* CONFIG_BT_PERIPHERAL */
	free(app_acl_rxfifo);
	free(app_hci_rxfifo);
	free(app_hci_txfifo);
	free(app_hci_rxAclfifo);


#if CONFIG_PM && CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif /* CONFIG_PM && CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION */
}

/**
 * @brief      Host send HCI packet to controller
 * @param      data the packet point
 * @param      len the packet length
 */
void tlx_bt_host_send_packet(uint8_t type, const uint8_t *data, uint16_t len)
{
	if (tl_bt_state == TL_BT_CONTROLLER_STATE_STOPPED) {
		return;
	}

	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.wptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	*p++ = type;
	memcpy(p, data, len);
	bltHci_rxfifo.wptr++;

	k_sem_give(&controller_sem);
}

/**
 * @brief Register the vhci reference callback
 */
void tlx_bt_host_callback_register(const tlx_bt_host_callback_t *pcb)
{
	tlx_ctrl.callbacks.host_read_packet = pcb->host_read_packet;
	tlx_ctrl.callbacks.host_send_available = pcb->host_send_available;
}

/**
 * @brief     Get state of Telink TLX BLE Controller
 */
enum tl_bt_controller_state tl_bt_controller_state(void) {

	return tl_bt_state;
}
