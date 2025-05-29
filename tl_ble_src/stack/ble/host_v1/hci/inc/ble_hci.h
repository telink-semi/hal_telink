/********************************************************************************************************
 * @file    ble_hci.h
 *
 * @brief   This is the header file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef STACK_BLE_HOST_V1_BLE_HCI_H_
#define STACK_BLE_HOST_V1_BLE_HCI_H_


#define BLE_HCI_OPCODE_NOP (0)

enum ble_host_hci_malloc_type {
    BLE_HOST_HCI_MALLOC_TYPE = 0x100,
    BLE_HOST_HCI_MALLOC_RX_FIFO,
    BLE_HOST_HCI_MALLOC_ACL_DATA_PACKET,
    BLE_HOST_HCI_MALLOC_ISO_DATA_PACKET,
};

/* Definition of HCI H4 packet types */
enum {
    BLE_HCI_H4_NONE = 0x00,
    BLE_HCI_H4_CMD = 0x01,
    BLE_HCI_H4_ACL = 0x02,
    BLE_HCI_H4_SCO = 0x03,
    BLE_HCI_H4_EVT = 0x04,
    BLE_HCI_H4_ISO = 0x05
};

enum ble_host_hci_error_code {
    BLE_HOST_HCI_ERR_SUCCESS = 0,
    BLE_HOST_HCI_ERR_WAIT_TIMEOUT,
    BLE_HOST_ERR_CONTROLLER,        /** Controller error */
    BLE_HOST_HCI_ERR_NULL_PIONTER,  /** Null pointer */
    BLE_HOST_HCI_ERR_INVALID_PARAM, /** Invalid parameter */
};

#define BLE_HOST_HCI_ASSERT(cond)                                                                      \
    do {                                                                                               \
        if (!(cond)) {                                                                                 \
            BLE_HOST_HCI_LE_CMD_ERROR("ble host hci assert func:%s, line:%d", __FUNCTION__, __LINE__); \
        }                                                                                              \
    } while (0)


/**
 *   @brief BLE Host HCI RX event callback type.
 *
 *   @param[in] p_evt - Pointer to the HCI event packet.
 *
 *   @return None.
 */
typedef int (*ble_host_hci_rx_event_callback_t)(void *p_evt);

/**
 *   @brief BLE Host HCI RX ACL data callback type.
 *
 *   @param[in] acl_data - Pointer to the ACL data packet.
 *
 *   @return None.
 */
typedef void (*ble_host_hci_rx_acl_data_callback_t)(void *acl_data);

/**
 *   @brief BLE Host HCI RX ISO data callback type.
 *
 *   @param[in] iso_data - Pointer to the ISO data packet.
 *
 *   @return None.
 */
typedef void (*ble_host_hci_rx_iso_data_callback_t)(void *iso_data);

/******************************************************************************
 * Functions
 ******************************************************************************/
/**
 *   @brief BLE Host HCI initialization.
 *
 *   @param[in] p_hci_memory - Pointer to the HCI memory.
 *   @param[in] size        - Size of the HCI memory.
 *
 *   @return None.
 */
void ble_host_hci_init(uint8_t *p_hci_memory, uint32_t size);

/**
 *   @brief BLE Host HCI Receive HCI packet.
 *
 *   @param[in] data        - Pointer to the HCI packet data.
 *   @param[in] len         - Length of the HCI packet data.
 *
 *   @return None.
 *
 *   @note This function is Host HCI data input API, called by the Application Layer.
 */
void ble_host_hci_rx_packet(uint8_t *data, unsigned int len);

/**
 *   @brief BLE Host HCI memory pool malloc.
 *
 *   @param[in] size        - Size of the memory to allocate.
 *   @param[in] type_id     - Type ID of the memory to allocate.
 *
 *   @return Pointer to the allocated memory, or NULL if allocation failed.
 */
void *ble_host_hci_malloc(uint32_t size, uint16_t type_id);

/**
 *   @brief BLE Host HCI memory pool free.
 *
 *   @param[in] ptr         - Pointer to the memory to free.
 *
 *   @return None.
 */
void ble_host_hci_free(void *ptr);

/**
 *   @brief BLE Host HCI register RX event callback.
 *
 *   @param[in] callback - Pointer to the RX event callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_event_callback(ble_host_hci_rx_event_callback_t callback);

/**
 *   @brief BLE Host HCI register RX ACL data callback.
 *
 *   @param[in] callback - Pointer to the RX ACL data callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_acl_data_callback(ble_host_hci_rx_acl_data_callback_t callback);

/**
 *   @brief BLE Host HCI register RX ISO data callback.
 *
 *   @param[in] callback - Pointer to the RX ISO data callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_iso_data_callback(ble_host_hci_rx_iso_data_callback_t callback);

/* OS systerm used only, here empty function */
/**
 *   @brief BLE Host HCI lock task.
 *
 *   @return None.
 */
void ble_host_hci_lock(void);

/**
 *   @brief BLE Host HCI unlock task.
 *
 *   @return None.
 */
void ble_host_hci_unlock(void);

int ble_host_hci_vs_cmd_tx(uint16_t ocf, const void *cmdbuf, uint8_t cmdlen, void *rspbuf, uint8_t rsplen);

void ble_host_hci_rx_task(void);


void ble_host_hci_send_packet(const uint8_t *data, uint16_t len);

#endif /* STACK_BLE_HOST_V1_BLE_HCI_H_ */
