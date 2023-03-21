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
#undef irq_enable
#undef irq_disable
#undef ARRAY_SIZE

#include "b91_bt.h"
#include "b91_bt_init.h"
#include "compiler.h"
#include "plic.h"
#include "stack/ble/controller/ble_controller.h"
#include "stack/ble/controller/os_sup.h"

/* Module defines */
#define BLE_THREAD_STACK_SIZE CONFIG_B91_BLE_CTRL_THREAD_STACK_SIZE
#define BLE_THREAD_PRIORITY CONFIG_B91_BLE_CTRL_THREAD_PRIORITY
#define BLE_THREAD_PERIOD_MS CONFIG_B91_BLE_CTRL_THREAD_PERIOD_MS
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

static volatile enum b91_bt_controller_state b91_bt_state = B91_BT_CONTROLLER_STATE_STOPPED;
static void b91_bt_controller_thread();
K_THREAD_STACK_DEFINE(b91_bt_controller_thread_stack, BLE_THREAD_STACK_SIZE);
static struct k_thread b91_bt_controller_thread_data;

/**
 * @brief    Semaphore define for controller.
 */
K_SEM_DEFINE(controller_sem, 0, BLE_CONTROLLER_SEMAPHORE_MAX);

/**
 * @brief    BLE semaphore callback.
 */
static void os_give_sem_cb(void)
{
	k_sem_give(&controller_sem);
}

static struct b91_ctrl_t {
	b91_bt_host_callback_t callbacks;
} b91_ctrl;

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
static int b91_bt_hci_tx_handler(void)
{
	/* check for data available */
	if (bltHci_txfifo.wptr == bltHci_txfifo.rptr) {
		return 0;
	}

	while(bltHci_txfifo.wptr != bltHci_txfifo.rptr)
	{
		/* Get HCI data */
		u8 *p = bltHci_txfifo.p + (bltHci_txfifo.rptr & bltHci_txfifo.mask) * bltHci_txfifo.size;
		if (p) {
			u32 len;
			BSTREAM_TO_UINT16(len, p);
			bltHci_txfifo.rptr++;

			if (b91_bt_state == B91_BT_CONTROLLER_STATE_ACTIVE) {
				/* Send data to the host */
				if (b91_ctrl.callbacks.host_read_packet) {
					b91_ctrl.callbacks.host_read_packet(p, len);
				}
			} else if (b91_bt_state == B91_BT_CONTROLLER_STATE_STOPPING) {
				/* In this state HCI reset is sent - waiting for command complete */
				static const uint8_t hci_reset_cmd_complette[] = {0x04, 0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00};

				if (len == sizeof(hci_reset_cmd_complette) && !memcmp(p, hci_reset_cmd_complette, len)) {
					b91_bt_state = B91_BT_CONTROLLER_STATE_STOPPED;
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
static int b91_bt_hci_rx_handler(void)
{
	/* Check for data available */
	if (bltHci_rxfifo.wptr == bltHci_rxfifo.rptr) {
		/* No data to process, send host_send_available message to the host */
		if (b91_ctrl.callbacks.host_send_available) {
			b91_ctrl.callbacks.host_send_available();
		}

		return 0;
	}

	/* Get HCI data */
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.rptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	if (p) {
		/* Send data to the controller */
		blc_hci_handler(&p[0], 0);
		bltHci_rxfifo.rptr++;
	}

	return 0;
}

/**
 * @brief    Telink B91 BLE Controller thread
 */
static void b91_bt_controller_thread()
{
	while (b91_bt_state == B91_BT_CONTROLLER_STATE_ACTIVE ||
		b91_bt_state == B91_BT_CONTROLLER_STATE_STOPPING) {
		k_sem_take(&controller_sem, K_FOREVER);
		blc_sdk_main_loop();
	}
}

/**
 * @brief    BLE Controller IRQs initialization
 */
static void b91_bt_irq_init()
{
	/* Init STimer IRQ */
	IRQ_CONNECT(IRQ1_SYSTIMER + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, stimer_irq_handler, 0, 0);
	/* Init RF IRQ */
#if CONFIG_DYNAMIC_INTERRUPTS
	irq_connect_dynamic(IRQ15_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#else
	IRQ_CONNECT(IRQ15_ZB_RT + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, rf_irq_handler, 0, 0);
#endif
	riscv_plic_set_priority(IRQ1_SYSTIMER, 2);
	riscv_plic_set_priority(IRQ15_ZB_RT, 2);
}

/**
 * @brief    Telink B91 BLE Controller initialization
 * @return   Status - 0: command succeeded; -1: command failed
 */
int b91_bt_controller_init()
{
	int status;

	/* init semaphore */
	k_sem_reset(&controller_sem);
	k_sem_give(&controller_sem);

	/* Init IRQs */
	b91_bt_irq_init();

	/* Init RF driver */
	rf_drv_ble_init();

	/* Init BLE Controller stack */
	status = b91_bt_blc_init(b91_bt_hci_rx_handler, b91_bt_hci_tx_handler);
	if (status != INIT_OK) {
		return status;
	}

	/* Register callback to controller. */
	blc_ll_registerGiveSemCb(os_give_sem_cb);

	/* Create BLE main thread */
	(void)k_thread_create(&b91_bt_controller_thread_data,
		b91_bt_controller_thread_stack, K_THREAD_STACK_SIZEOF(b91_bt_controller_thread_stack),
		b91_bt_controller_thread, NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, K_NO_WAIT);

	/* Start thread */
	b91_bt_state = B91_BT_CONTROLLER_STATE_ACTIVE;
	k_thread_start(&b91_bt_controller_thread_data);

	return status;
}

/**
 * @brief    Telink B91 BLE Controller deinitialization
 */
void b91_bt_controller_deinit()
{
	/* start BLE stopping procedure */
	b91_bt_state = B91_BT_CONTROLLER_STATE_STOPPING;

	/* reset controller */
	static const uint8_t hci_reset_cmd[] = {0x03, 0x0c, 0x00};
	b91_bt_host_send_packet(0x01, hci_reset_cmd, sizeof(hci_reset_cmd));

	/* wait thread finish */
	(void)k_thread_join(&b91_bt_controller_thread_data, K_FOREVER);

	/* disable interrupts */
	plic_interrupt_disable(IRQ1_SYSTIMER);
	plic_interrupt_disable(IRQ15_ZB_RT);

	/* Reset Radio */
	rf_radio_reset();
	rf_reset_dma();
	rf_baseband_reset();
}

/**
 * @brief      Host send HCI packet to controller
 * @param      data the packet point
 * @param      len the packet length
 */
void b91_bt_host_send_packet(uint8_t type, const uint8_t *data, uint16_t len)
{
	u8 *p = bltHci_rxfifo.p + (bltHci_rxfifo.wptr & bltHci_rxfifo.mask) * bltHci_rxfifo.size;
	*p++ = type;
	memcpy(p, data, len);
	bltHci_rxfifo.wptr++;

	k_sem_give(&controller_sem);
}

/**
 * @brief Register the vhci reference callback
 */
void b91_bt_host_callback_register(const b91_bt_host_callback_t *pcb)
{
	b91_ctrl.callbacks.host_read_packet = pcb->host_read_packet;
	b91_ctrl.callbacks.host_send_available = pcb->host_send_available;
}

/**
 * @brief     Get state of Telink B91 BLE Controller
 */
enum b91_bt_controller_state b91_bt_controller_state(void) {

	return b91_bt_state;
}
