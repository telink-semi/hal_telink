/********************************************************************************************************
 * @file    ble_hci.c
 *
 * @brief   This is the source file for TLSR/TL
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
#include <stddef.h>
#include <string.h>
#include <sys/queue.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host_sal.h"
#include "../inc/ble_host.h"

#include "inc/ble_hci.h"
#include "inc/ble_hci_log.h"
#include "inc/ble_acl_data.h"
#include "inc/ble_iso_data.h"
#include "inc/ble_hci_cmd.h"
#include "inc/ble_hci_evt.h"
#include "inc/ble_btsnoop.h"

#define BLE_HCI_RX_MALLOC(size) ble_host_hci_malloc(size, BLE_HOST_HCI_MALLOC_RX_FIFO)
#define BLE_HCI_RX_FREE(ptr)    ble_host_hci_free(ptr)

struct ble_host_hci_rx_fifo {
    STAILQ_ENTRY(ble_host_hci_rx_fifo) next; // next element in the list
    uint8_t hci_packet[0]; // pointer to the HCI event packet
};

struct ble_host_hci_info {
    uint8_t *p_hci_memory;
    ble_host_hci_rx_event_callback_t    hci_rx_event_callback;
    ble_host_hci_rx_acl_data_callback_t hci_rx_acl_data_callback;
    ble_host_hci_rx_iso_data_callback_t hci_rx_iso_data_callback;
    STAILQ_HEAD(, ble_host_hci_rx_fifo)
        hci_rx_fifo;
};

static struct ble_host_hci_info s_ble_host_hci_info = {
    .p_hci_memory = NULL,
    .hci_rx_fifo = STAILQ_HEAD_INITIALIZER(s_ble_host_hci_info.hci_rx_fifo),
    .hci_rx_event_callback = NULL,
    .hci_rx_acl_data_callback = NULL,
    .hci_rx_iso_data_callback = NULL,
};

/**
 *   @brief BLE Host HCI initialization.
 *
 *   @param[in] p_hci_memory - Pointer to the HCI memory.
 *   @param[in] size        - Size of the HCI memory.
 *
 *   @return None.
 */
void ble_host_hci_init(uint8_t *p_hci_memory, uint32_t size)
{
    ble_host_sal_memory_pool_init(p_hci_memory, size);
    s_ble_host_hci_info.p_hci_memory = p_hci_memory;
}

/**
 *   @brief BLE Host HCI memory pool malloc.
 *
 *   @param[in] size        - Size of the memory to allocate.
 *   @param[in] type_id     - Type ID of the memory to allocate.
 *
 *   @return Pointer to the allocated memory, or NULL if allocation failed.
 */
void *ble_host_hci_malloc(uint32_t size, uint16_t type_id)
{
    return ble_host_sal_memory_malloc(s_ble_host_hci_info.p_hci_memory, size, type_id);
}

/**
 *   @brief BLE Host HCI memory pool free.
 *
 *   @param[in] ptr         - Pointer to the memory to free.
 *
 *   @return None.
 */
void ble_host_hci_free(void *ptr)
{
    ble_host_sal_memory_free(s_ble_host_hci_info.p_hci_memory, ptr);
}

/**
 *   @brief BLE Host HCI register RX event callback.
 *
 *   @param[in] callback - Pointer to the RX event callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_event_callback(ble_host_hci_rx_event_callback_t callback)
{
    s_ble_host_hci_info.hci_rx_event_callback = callback;
}

/**
 *   @brief BLE Host HCI register RX ACL data callback.
 *
 *   @param[in] callback - Pointer to the RX ACL data callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_acl_data_callback(ble_host_hci_rx_acl_data_callback_t callback)
{
    s_ble_host_hci_info.hci_rx_acl_data_callback = callback;
}

/**
 *   @brief BLE Host HCI register RX ISO data callback.
 *
 *   @param[in] callback - Pointer to the RX ISO data callback function.
 *
 *   @return None.
 */
void ble_host_hci_register_rx_iso_data_callback(ble_host_hci_rx_iso_data_callback_t callback)
{
    s_ble_host_hci_info.hci_rx_iso_data_callback = callback;
}

/* OS systerm used only, here empty function */
/**
 *   @brief BLE Host HCI lock task.
 *
 *   @return None.
 */
void ble_host_hci_lock(void)
{
}

/**
 *   @brief BLE Host HCI unlock task.
 *
 *   @return None.
 */
void ble_host_hci_unlock(void)
{
}

//////////////////////////////////////////////////////////////////////////////////////////
//                          HCI RX frame process
//////////////////////////////////////////////////////////////////////////////////////////
/***********************HCI receive packet process *********************/
/**
 *   @brief BLE Host HCI received packet entrance,
 */
static void ble_host_hci_packet_enqueue(const uint8_t *data, uint16_t len)
{
    BLE_HOST_HCI_COMMON_DEBUG("event receive: %s", hex_to_str(data, len));
    // extern void app_serial_send(u8 *data, uint16_t len);
    // app_serial_send((u8 *)data, len);
    ble_host_write_hci_rx_packet_to_btsnoop(data, len);

    /* Enqueue HCI H4 event packet to the Host HCI EVT RX fifo, process it by hci received event task */
    bool enqueue = true;
    if (data[0] == BLE_HCI_H4_EVT) {
        struct ble_hci_evt_h4 *p_evt = (struct ble_hci_evt_h4 *) data;

        switch (p_evt->opcode) {
        case BLE_HCI_EVCODE_COMMAND_COMPLETE:
        {
            ble_host_hci_rx_cmd_complete(p_evt);
            enqueue = false;
        } break;

        case BLE_HCI_EVCODE_COMMAND_STATUS:
        {
            ble_host_hci_rx_cmd_status(p_evt);
            enqueue = false;
        } break;

        default:
            break;
        }
    }

    if (enqueue) {
        struct ble_host_hci_rx_fifo *p_rx_fifo = BLE_HCI_RX_MALLOC(len + sizeof(struct ble_host_hci_rx_fifo));
        if (p_rx_fifo == NULL) {
            BLE_HOST_HCI_COMMON_ERROR("hci rx fifo malloc failed");
            return;
        }
        //        BLE_HOST_HCI_COMMON_INFO("event enqueue: %s", hex_to_str(data, len));
        memcpy(p_rx_fifo->hci_packet, data, len);
        STAILQ_INSERT_TAIL(&s_ble_host_hci_info.hci_rx_fifo, p_rx_fifo, next);
        /* HCI H4 frame processing (will be called by HCI HOST side RX thread) */
    }
}

static uint16_t ble_host_hci_rx_packet_parse(const uint8_t *data, uint16_t len)
{
    // todo: parse HCI H4 packet header.
    if (data[0] == BLE_HCI_H4_EVT) {
        struct ble_hci_evt_h4 *p_evt = (struct ble_hci_evt_h4 *) data;
        return sizeof(struct ble_hci_evt_h4) + p_evt->length;
    } else if (data[0] == BLE_HCI_H4_ACL) {
        struct ble_host_hci_acl_data_h4 *p_acl = (struct ble_host_hci_acl_data_h4 *) data;
        return sizeof(struct ble_host_hci_acl_data_h4) + p_acl->dataTotalLength;
    } else if (data[0] == BLE_HCI_H4_ISO) {
        struct ble_host_hci_iso_h4 *p_iso = (struct ble_host_hci_iso_h4 *) data;
        return sizeof(struct ble_host_hci_iso_h4) + p_iso->hdr.dataTotalLen;
    }

    return len;
}

void ble_host_hci_rx_packet(uint8_t *data, unsigned int len)
{
    while (len != 0) {
        uint16_t parsed_len = ble_host_hci_rx_packet_parse(data, len);
        ble_host_hci_packet_enqueue(data, parsed_len);
        len -= parsed_len;
        data += parsed_len;
    }
}

static struct ble_host_hci_rx_fifo *ble_host_hci_packet_dequeue(void)
{
    if (STAILQ_EMPTY(&s_ble_host_hci_info.hci_rx_fifo)) {
        return NULL;
    }

    struct ble_host_hci_rx_fifo *p_packet = STAILQ_FIRST(&s_ble_host_hci_info.hci_rx_fifo);
    STAILQ_REMOVE_HEAD(&s_ble_host_hci_info.hci_rx_fifo, next);
    return p_packet;
}

/* HCI H4 RX task (will be called by HCI HOST side RX thread) */
void ble_host_hci_rx_task(void)
{
    /* Dequeue HCI H4 packet from the Host HCI RX fifo */
    struct ble_host_hci_rx_fifo *p_packet = ble_host_hci_packet_dequeue();
    if (p_packet != NULL) {
        /* Parse the HCI H4 packet header */
        uint8_t  pkt_type = p_packet->hci_packet[0];
        uint8_t *data = p_packet->hci_packet + 1; // skip packet type.
        switch (pkt_type) {
        case BLE_HCI_H4_EVT:
        {
            if (s_ble_host_hci_info.hci_rx_event_callback != NULL) {
                s_ble_host_hci_info.hci_rx_event_callback(data);
            }
        } break;
        case BLE_HCI_H4_ACL:
        {
            if (s_ble_host_hci_info.hci_rx_acl_data_callback != NULL) {
                s_ble_host_hci_info.hci_rx_acl_data_callback(data);
            }
        } break;
        case BLE_HCI_H4_ISO:
        {
            if (s_ble_host_hci_info.hci_rx_iso_data_callback != NULL) {
                s_ble_host_hci_info.hci_rx_iso_data_callback(data);
            }
        } break;
        default:
            break;
        }

        BLE_HCI_RX_FREE(p_packet);
    }
}

void ble_host_hci_send_packet(const uint8_t *data, uint16_t len)
{
    ble_host_write_hci_tx_packet_to_btsnoop(data, len);
    ble_host_sal_hci_send_packet(data, len);
}

//////////////////////////////////////////////////////////////////////////////////////////
//                          HCI TX ACL data send process
//////////////////////////////////////////////////////////////////////////////////////////
// static void ble_host_hci_acl_send(uint16_t handle_pb_bc, const void *data, uint16_t len)
// {
// #define BLE_HCI_ACL_SIZE                251
//     uint8_t packet[BLE_HCI_ACL_SIZE + 1];
//     struct hci_acl_dat *acl_data = (struct hci_acl_dat *) &packet[1];

// //    uint16_t handle = BLE_HCI_DATA_HANDLE(handle_pb_bc);
// //    uint8_t pb_flag = BLE_HCI_DATA_PB(handle_pb_bc);
// //    uint8_t bc_flag = BLE_HCI_DATA_BC(handle_pb_bc);
//     acl_data->hdh_handle_pb_bc = handle_pb_bc;
//     acl_data->hdh_len = len;

//     if (len != 0) {
//         memcpy(acl_data->data, data, len);
//     }

//     /* Prepare the HCI H4 formatted packet for the command */

//     /* first byte is the packet indicator */
//     packet[0] = BLE_HCI_H4_ACL;
//     /* total HCI H4 formatted packet length */
//     len += sizeof(struct hci_acl_dat) + 1;
//     memcpy(packet + 1, acl_data, len - 1);

//     /* Send H4 formatted HCI command to the Host HCI TX fifo, these fifo will be sent to the controller */
//     ble_host_hci_send_packet(packet, len);
// }

// /// @brief          Transmits an HCI ACL data packet, this function consumes the supplied mbuf, regardless of the outcome
// /// @param conn     - The connection to transmit the packet on.
// /// @param om       - The mbuf containing the ACL data to transmit.
// /// @return         - 0: success; nonzero: failure.
// int ble_host_hci_acl_tx(struct ble_host_conn *conn, struct mbuf_acl *macl)
// {
//     /* If this conn is already backed up, don't even try to send. */
//     if (STAILQ_FIRST(&conn->acl_tx_data_q) != NULL) {
//         return BLE_HOST_ERR_RETRY;
//     }

//     uint16_t handle = conn->handle;
//     uint8_t pb_flag;
//     uint8_t bc_flag;

//     (void) pb_flag;
//     (void) bc_flag;

//     /* TODO: ble_host_hci_acl_send(); */
//     ble_host_hci_acl_send(handle, macl->acl_data, macl->acl_len);

//     return 0;
// }
