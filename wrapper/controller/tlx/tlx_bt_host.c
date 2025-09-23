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

#include "tlx_bt_host.h"
#include "tlx_bt_init.h"

#if TLK_ONLY_BLE_HOST
#include "stack/multiCoreComm/drv/shareMemory.h"
#include "stack/multiCoreComm/service/service_shareMemory.h"
# if CONFIG_PM
#include "stack/pm/pm_sys.h"
# endif
#else
#include "stack/ble/controller/ble_controller.h"
#endif

#define BLE_THREAD_STACK_SIZE CONFIG_TL_BLE_CTRL_THREAD_STACK_SIZE
#define BLE_THREAD_PRIORITY CONFIG_TL_BLE_CTRL_THREAD_PRIORITY
#define BLE_CONTROLLER_SEMAPHORE_MAX 50

static volatile enum tl_bt_controller_state tl_bt_state = TL_BT_CONTROLLER_STATE_STOPPED;
K_THREAD_STACK_DEFINE(tlx_bt_controller_thread_stack, BLE_THREAD_STACK_SIZE);
static struct k_thread tlx_bt_controller_thread_data;
K_SEM_DEFINE(controller_sem, 0, BLE_CONTROLLER_SEMAPHORE_MAX);

/**
 * @brief    Telink TLX BLE Controller thread
 */
static void tlx_bt_controller_thread()
{
	while (tl_bt_state == TL_BT_CONTROLLER_STATE_ACTIVE ||
		tl_bt_state == TL_BT_CONTROLLER_STATE_STOPPING) {
		k_sem_take(&controller_sem, K_FOREVER);	//Mailbox irq can also trigger controller_sem in time.
		tlk_multi_core_communication_loop();
	}
}

static tlx_bt_host_callback_t tlx_bt_cb = {0};

extern void mailbox_n22_to_d25_irq_handler(void);
/**
 * @brief    Telink TLX BLE Controller initialization
 * @return   Status - 0: command succeeded; -1: command failed
 */
int tlx_bt_controller_init()
{
    int status = INIT_OK;

    IRQ_CONNECT(IRQ_MAILBOX_N22_TO_D25 + CONFIG_2ND_LVL_ISR_TBL_OFFSET, 2, mailbox_n22_to_d25_irq_handler, 0, 0);
#ifdef TLK_ONLY_BLE_HOST
	volatile uint32_t key = arch_irq_lock();
	sys_n22_start();
	tlk_mailbox_service_init();
	tlk_share_memory_service_init();
	arch_irq_unlock(key);

# if CONFIG_PM
	/* Enable PM for BLE stack */
	blc_ll_initPowerManagement_module();
	blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_PERIPHR);
# endif /* CONFIG_PM */
#endif /* TLK_ONLY_BLE_HOST */

    /* init semaphore */
	k_sem_reset(&controller_sem);
	k_sem_give(&controller_sem);

    (void)k_thread_create(&tlx_bt_controller_thread_data,
		tlx_bt_controller_thread_stack, K_THREAD_STACK_SIZEOF(tlx_bt_controller_thread_stack),
		tlx_bt_controller_thread, NULL, NULL, NULL, BLE_THREAD_PRIORITY, 0, K_NO_WAIT);

    (void)k_thread_name_set(&tlx_bt_controller_thread_data, "TL322X_BT");

    tl_bt_state = TL_BT_CONTROLLER_STATE_ACTIVE;
	k_thread_start(&tlx_bt_controller_thread_data);

	return status;
}

_attribute_ram_code_ void tlk_mailbox_d25f_sm_data_ready_process(u8* data)
{
    (void) data;

    k_sem_give(&controller_sem);
}

/**
 * @brief    Telink TLX BLE Controller deinitialization
 */
void tlx_bt_controller_deinit(void)
{
	/* start BLE stopping procedure */
	tl_bt_state = TL_BT_CONTROLLER_STATE_STOPPING;

    	/* reset controller */
	static const uint8_t hci_reset_cmd[] = {0x03, 0x0c, 0x00};
	tlx_bt_host_send_packet(0x01, hci_reset_cmd, sizeof(hci_reset_cmd));
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

#if TLK_ONLY_BLE_HOST
    u8 hci_cmd[255+3] = {0}; 			//Controllers shall be able to accept HCI Command packets with up to 255 bytes of data excluding the HCI Command packet header.
	hci_cmd[0] = type;
	memcpy(hci_cmd+1, data, len);
	tlk_sm_ret_e ret = tlk_d25f_hci_send_message(0, hci_cmd, len+1);
	if (ret == TLK_SHARE_MEMORY_SUCCESS) {
		/* No data to process, send host_send_available message to the host */
		if (tlx_bt_cb.host_send_available) {
			tlx_bt_cb.host_send_available();
		}
	}
#endif
}

/**
 * @brief Register the vhci reference callback
 */
void tlx_bt_host_callback_register(const tlx_bt_host_callback_t *pcb)
{
	tlx_bt_cb.host_read_packet = pcb->host_read_packet;		//hci_tlx_host_rcv_pkt
	tlx_bt_cb.host_send_available = pcb->host_send_available;	//hci_tlx_controller_rcv_pkt_ready
#if TLK_ONLY_BLE_HOST
	tlk_d25f_register_hci_receive_cb(TLK_SHARE_MEMORY_MESSAGE_TYPE_BLE, (void (*)(unsigned char *, unsigned int))tlx_bt_cb.host_read_packet);
#endif
}

/**
 * @brief     Get state of Telink TLX BLE Controller
 */
enum tl_bt_controller_state tl_bt_controller_state(void) {

	return tl_bt_state;
}
