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
#include "plic.h"

#if CONFIG_SOC_RISCV_TELINK_B91
#include "stack/ble/B91/controller/ble_controller.h"
#include "stack/ble/B91/controller/os_sup.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "stack/ble/B92/controller/ble_controller.h"
#include "stack/ble/B92/controller/os_sup.h"
#elif CONFIG_SOC_RISCV_TELINK_B95
#include "stack/ble/B95/controller/ble_controller.h"
#include "stack/ble/B95/controller/os_sup.h"
#elif CONFIG_SOC_RISCV_TELINK_TL321X
#include "stack/ble/TL321X/controller/ble_controller.h"
#include "stack/ble/TL321X/controller/os_sup.h"
#endif

/* Module defines */
#define BLE_THREAD_STACK_SIZE CONFIG_B9X_BLE_CTRL_THREAD_STACK_SIZE
#define BLE_THREAD_PRIORITY CONFIG_B9X_BLE_CTRL_THREAD_PRIORITY
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

static volatile enum b9x_bt_controller_state b9x_bt_state = B9X_BT_CONTROLLER_STATE_STOPPED;
static void b9x_bt_controller_thread();
K_THREAD_STACK_DEFINE(b9x_bt_controller_thread_stack, BLE_THREAD_STACK_SIZE);
static struct k_thread b9x_bt_controller_thread_data;

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

static struct b9x_ctrl_t {
	b9x_bt_host_callback_t callbacks;
} b9x_ctrl;

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
static int b9x_bt_hci_tx_handler(void)
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

			if (b9x_bt_state == B9X_BT_CONTROLLER_STATE_ACTIVE) {
				/* Send data to the host */
				if (b9x_ctrl.callbacks.host_read_packet) {
					b9x_ctrl.callbacks.host_read_packet(p, len);
				}
			} else if (b9x_bt_state == B9X_BT_CONTROLLER_STATE_STOPPING) {
				/* In this state HCI reset is sent - waiting for command complete */
				static const uint8_t hci_reset_cmd_complette[] = {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};

				if (len == sizeof(hci_reset_cmd_complette) && !memcmp(p, hci_reset_cmd_complette, len)) {
					b9x_bt_state = B9X_BT_CONTROLLER_STATE_STOPPED;
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
static int b9x_bt_hci_rx_handler(void)
{
	/* Check for data available */
	if (bltHci_rxfifo.wptr == bltHci_rxfifo.rptr) {
		/* No data to process, send host_send_available message to the host */
		if (b9x_ctrl.callbacks.host_send_available) {
			b9x_ctrl.callbacks.host_send_available();
		}

		return 0;
	}

	/* Get HCI data */
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if (p) {
		/* Send data to the controller */
		blc_hci_handler(&p[0], 0);
#if CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95 || CONFIG_SOC_RISCV_TELINK_TL321X
		if (p[0] == HCI_TYPE_ACL_DATA) {
			k_sem_give(&controller_sem);
		}
#endif
		bltHci_rxfifo.rptr++;
	}

	return 0;
}

/**
 * @brief    Telink B9X BLE Controller thread
 */
static void b9x_bt_controller_thread()
{
	while (b9x_bt_state == B9X_BT_CONTROLLER_STATE_ACTIVE ||
		b9x_bt_state == B9X_BT_CONTROLLER_STATE_STOPPING) {
		k_sem_take(&controller_sem, K_FOREVER);
		blc_sdk_main_loop();
	}
}

/**
 * @brief    BLE Controller IRQs initialization
 */
static void b9x_bt_irq_init()
{
#if CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95 || CONFIG_SOC_RISCV_TELINK_TL321X
	plic_preempt_feature_dis();
	flash_plic_preempt_config(0,1);
#endif

#if CONFIG_SOC_RISCV_TELINK_B91
#define IRQ_SYSTIMER IRQ1_SYSTIMER
#define IRQ_ZB_RT IRQ15_ZB_RT
#endif

	/* Init STimer IRQ */
	IRQ_CONNECT(IRQ_SYSTIMER + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, stimer_irq_handler, 0, 0);
	/* Init RF IRQ */
#if CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#else
	IRQ_CONNECT(IRQ_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#endif
	riscv_plic_set_priority(IRQ_SYSTIMER, 2);
	riscv_plic_set_priority(IRQ_ZB_RT, 2);
}

/**
 * @brief    Telink B9X BLE Controller initialization
 * @return   Status - 0: command succeeded; -1: command failed
 */
int b9x_bt_controller_init()
{
	int status;

#if CONFIG_PM && (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \
CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif /* CONFIG_PM && (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \ */
/* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION) */

	/* Reset Radio */
	rf_radio_reset();
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_TL321X
	rf_reset_dma();
	rf_baseband_reset();
#endif

	/* Init RF driver */
	rf_drv_ble_init();

	/* Init BLE Controller stack */
	status = b9x_bt_blc_init(b9x_bt_hci_rx_handler, b9x_bt_hci_tx_handler);
	if (status != INIT_OK) {
		return status;
	}

	/* Init IRQs */
	b9x_bt_irq_init();

	/* Register callback to controller. */
#if CONFIG_SOC_RISCV_TELINK_B91
	blc_ll_registerGiveSemCb(os_give_sem_cb);
#elif CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_B95 || CONFIG_SOC_RISCV_TELINK_TL321X
	blc_ll_registerGiveSemCb(os_give_sem_cb, os_give_sem_cb);
	blc_setOsSupEnable(true);
	blc_ll_enOsPowerManagement_module();
#endif
	/* init semaphore */
	k_sem_reset(&controller_sem);
	k_sem_give(&controller_sem);

	/* Create BLE main thread */
	(void)k_thread_create(&b9x_bt_controller_thread_data,
		b9x_bt_controller_thread_stack, K_THREAD_STACK_SIZEOF(b9x_bt_controller_thread_stack),
		b9x_bt_controller_thread, NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, K_NO_WAIT);
#if CONFIG_SOC_RISCV_TELINK_B91
		(void)k_thread_name_set(&b9x_bt_controller_thread_data, "B91_BT");
#elif CONFIG_SOC_RISCV_TELINK_B92
		(void)k_thread_name_set(&b9x_bt_controller_thread_data, "B92_BT");
#elif CONFIG_SOC_RISCV_TELINK_B95
		(void)k_thread_name_set(&b9x_bt_controller_thread_data, "B95_BT");
#elif CONFIG_SOC_RISCV_TELINK_TL321X
		(void)k_thread_name_set(&b9x_bt_controller_thread_data, "TL321X_BT");
#endif

	/* Start thread */
	b9x_bt_state = B9X_BT_CONTROLLER_STATE_ACTIVE;
	k_thread_start(&b9x_bt_controller_thread_data);

	return status;
}

/**
 * @brief    Telink B9X BLE Controller deinitialization
 */
void b9x_bt_controller_deinit()
{
	/* start BLE stopping procedure */
	b9x_bt_state = B9X_BT_CONTROLLER_STATE_STOPPING;

	/* reset controller */
	static const uint8_t hci_reset_cmd[] = {0x03, 0x0c, 0x00};
	b9x_bt_host_send_packet(0x01, hci_reset_cmd, sizeof(hci_reset_cmd));

	/* wait thread finish */
	(void)k_thread_join(&b9x_bt_controller_thread_data, K_FOREVER);

	/* disable interrupts */
	plic_interrupt_disable(IRQ_SYSTIMER);
	plic_interrupt_disable(IRQ_ZB_RT);

	/* Reset Radio */
	rf_radio_reset();
#if CONFIG_SOC_RISCV_TELINK_B91 || CONFIG_SOC_RISCV_TELINK_B92 || CONFIG_SOC_RISCV_TELINK_TL321X
	rf_reset_dma();
	rf_baseband_reset();
#endif

#if CONFIG_PM && (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \
CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif /* CONFIG_PM && (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \ */
/* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION) */
}

/**
 * @brief      Host send HCI packet to controller
 * @param      data the packet point
 * @param      len the packet length
 */
void b9x_bt_host_send_packet(uint8_t type, const uint8_t *data, uint16_t len)
{
	if (b9x_bt_state == B9X_BT_CONTROLLER_STATE_STOPPED) {
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
void b9x_bt_host_callback_register(const b9x_bt_host_callback_t *pcb)
{
	b9x_ctrl.callbacks.host_read_packet = pcb->host_read_packet;
	b9x_ctrl.callbacks.host_send_available = pcb->host_send_available;
}

/**
 * @brief     Get state of Telink B9X BLE Controller
 */
enum b9x_bt_controller_state b9x_bt_controller_state(void) {

	return b9x_bt_state;
}
