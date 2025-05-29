#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host.h"
#include "../inc/ble_host_sal.h"

#include "inc/ble_hci.h"
#include "inc/ble_hci_cmd.h"
#include "inc/ble_hci_evt.h"
#include "inc/ble_hci_log.h"


#define BLE_VHCI_TIMEOUT_MS 2000

struct ble_host_hci_cmd_ack
{
    int           controller_status;
    volatile bool ack_status;
    uint16_t      expected_opcode;
    uint8_t      *rsp;
    uint8_t       rsp_len;
};

static struct ble_host_hci_cmd_ack s_hci_cmd_ack;

static hci_transport_layer_process_t s_hci_transport_layer_process_callback;

static uint32_t ble_host_hci_send_strategy = HCI_SEND_STRATEGY_WAIT_ACK_ALWAYS;

void ble_host_hci_cmd_strategy_set(uint32_t strategy)
{
    ble_host_hci_send_strategy = strategy;
}
/**
 *   @brief BLE Host HCI command transport layer receive process.
 *
 *   @param[in] callback - Pointer to the HCI transport layer receive process callback function.
 *
 *   @return None.
 */
void ble_host_hci_cmd_transport_layer_receive_process(hci_transport_layer_process_t callback)
{
    s_hci_transport_layer_process_callback = callback;
}

//////////////////////////////////////////////////////////////////////////////////////////
//                          HCI TX command send process
//////////////////////////////////////////////////////////////////////////////////////////
/**
 *   @brief BLE Host HCI command send process.
 *
 *   @param[in] opcode      - HCI command opcode.
 *   @param[in] cmd         - Pointer to the HCI command data.
 *   @param[in] cmd_len     - Length of the HCI command data.
 *
 *   @return None.
 */
static void ble_host_hci_cmd_send(uint16_t opcode, const void *cmd, uint8_t cmd_len)
{
    uint16_t               hci_cmd_len = cmd_len + sizeof(struct ble_hci_cmd_h4);
    uint8_t                packet[hci_cmd_len];
    struct ble_hci_cmd_h4 *hci_cmd = (struct ble_hci_cmd_h4 *)&packet[0];

    /* Prepare the HCI H4 formatted packet for the command */
    BLE_HOST_HCI_COMMON_CMD_INFO("HCI CMD send opcode: 0x%04x, %s", opcode, hex_to_str(cmd, cmd_len));

    /* first byte is the packet indicator */
    hci_cmd->type   = BLE_HCI_H4_CMD;
    hci_cmd->opcode = opcode;
    hci_cmd->length = cmd_len;

    if (cmd_len != 0) {
        memcpy(hci_cmd->data, cmd, cmd_len);
    }
    /* Send H4 formatted HCI command to the Host HCI TX fifo, these fifo will be sent to the controller */
    ble_host_hci_send_packet(packet, hci_cmd_len);
}

/**
 *   @brief BLE Host HCI command set command expected response return value.
 *
 *   @param[in] expected_opcode - HCI command opcode.
 *   @param[in] params_buf      - Pointer to the HCI command response data.
 *   @param[in] params_buf_len  - Length of the HCI command response data.
 *
 *   @return None.
 */
static void ble_host_hci_set_cmd_rsp(uint16_t expected_opcode, uint8_t *params_buf, uint8_t params_buf_len)
{
    s_hci_cmd_ack.ack_status      = false;
    s_hci_cmd_ack.expected_opcode = expected_opcode;
    s_hci_cmd_ack.rsp_len         = params_buf_len;
    s_hci_cmd_ack.rsp             = params_buf;
}

/**
 *   @brief BLE Host HCI command wait for ACK process.
 *
 *   @return 0-0xFF for BLE controller return status.
 *            - BLE_HCI_ERR(BLE_HOST_HCI_ERR_WAIT_TIMEOUT) for wait wack timeout.
 *            - BLE_HCI_ERR(BLE_HOST_ERR_CONTROLLER) for controller return packet error.
 */
static int ble_host_hci_wait_for_ack(void)
{
    int rc;

    u32 t = ble_host_sal_get_current_time();
    rc    = BLE_HCI_ERR(BLE_HOST_HCI_ERR_WAIT_TIMEOUT);

    /* Block for ack, loop until timeout or ack received */
    while (!ble_host_sal_is_time_exceed(t, BLE_VHCI_TIMEOUT_MS)) {
        /* Process HCI H4 packets from the Host HCI RX fifo */
        // add transport layer receive process here
        if (s_hci_transport_layer_process_callback != NULL) {
            s_hci_transport_layer_process_callback();
        }

        if (s_hci_cmd_ack.ack_status) {
            rc = s_hci_cmd_ack.controller_status;
            BLE_HOST_HCI_COMMON_CMD_INFO("ble_host_hci_wait_for_ack: %d", rc);
            break;
        }
    }

    return rc;
}

/**
 *   @brief BLE Host HCI command send and wait for ACK process.
 *
 *   @param[in] opcode      - HCI command opcode.
 *   @param[in] cmd         - Pointer to the HCI command data.
 *   @param[in] cmd_len     - Length of the HCI command data.
 *   @param[out] rsp        - Pointer to the HCI command response data.
 *   @param[in] rsp_len     - Length of the HCI command response data.
 *
 *   @return 0-0xFF for BLE controller return status.
 *            - BLE_HCI_ERR(BLE_HOST_HCI_ERR_WAIT_TIMEOUT) for wait wack timeout.
 *            - BLE_HCI_ERR(BLE_HOST_ERR_CONTROLLER) for controller return packet error.
 */
int ble_host_hci_cmd_tx(uint16_t opcode, const void *cmd, uint8_t cmd_len, void *rsp, uint8_t rsp_len)
{
    int rc;
    static uint8_t bypass = 0;
    /* If the send strategy is set to wait for ACK always, then bypass is disabled */
    if (HCI_SEND_STRATEGY_WAIT_ACK_ALWAYS == ble_host_hci_send_strategy) {
        bypass = 0;
    }
    /* If the send strategy is set to wait for ACK bypass,and time out once occurs, then bypass is enabled */
    if (bypass) {
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_WAIT_TIMEOUT);
    }

    ble_host_hci_lock();

    ble_host_hci_set_cmd_rsp(opcode, rsp, rsp_len);

    ble_host_hci_cmd_send(opcode, cmd, cmd_len);

    rc = ble_host_hci_wait_for_ack();
    if (rc == BLE_HCI_ERR(BLE_HOST_HCI_ERR_WAIT_TIMEOUT)) {
        if (HCI_SEND_STRATEGY_WAIT_ACK_BYPASS == ble_host_hci_send_strategy) {
            BLE_HOST_HCI_COMMON_CMD_ERROR("ble_host_hci_cmd_tx: wait ack timeout,enable bypass mode");
            bypass = 1;
        }
    }
    ble_host_hci_unlock();
    return rc;
}

/**
 *   @brief BLE Host HCI command send and wait for ACK process.
 *
 *   @param[in] opcode      - HCI command opcode.
 *   @param[in] cmd         - Pointer to the HCI command data.
 *   @param[in] cmd_len     - Length of the HCI command data.
 *
 *   @return None.
 */
void ble_host_hci_cmd_tx_no_rsp(uint16_t opcode, const void *cmd, uint8_t cmd_len)
{
    ble_host_hci_lock();

    ble_host_hci_cmd_send(opcode, cmd, cmd_len);

    ble_host_hci_unlock();
}

int ble_host_hci_vs_cmd_tx(uint16_t ocf, const void *cmdbuf, uint8_t cmdlen, void *rspbuf, uint8_t rsplen)
{
    int rc;

    rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR, ocf),
                             cmdbuf,
                             cmdlen,
                             rspbuf,
                             rsplen);

    return rc;
}

//////////////////////////////////////////////////////////////////////////////////////////
//                          HCI TX command wait for ACK process
//////////////////////////////////////////////////////////////////////////////////////////
/**
 *   @brief BLE Host HCI command receive command complete event process.
 *
 *   @param[in] p_evt - Pointer to the HCI command complete event.
 *
 *   @return None.
 */
void ble_host_hci_rx_cmd_complete(void *p_evt)
{
    struct ble_hci_evt_h4 *p_cmd_complete_evt = p_evt;

    const struct ble_hci_ev_command_complete     *ev  = (void *)p_cmd_complete_evt->data;
    const struct ble_hci_ev_command_complete_nop *nop = (void *)p_cmd_complete_evt->data;
    uint16_t                                      opcode;

    struct ble_host_hci_cmd_ack *ack = &s_hci_cmd_ack;

    if (p_cmd_complete_evt->length < (int)sizeof(*ev)) {
        ack->controller_status = BLE_HOST_ERR_SUCC;
        if (p_cmd_complete_evt->length < (int)sizeof(*nop)) {
            ack->controller_status = BLE_HCI_ERR(BLE_HOST_ERR_CONTROLLER);
        }

        /* nop is special as it doesn't have status and response */

        opcode = nop->opcode;
        if (opcode != BLE_HCI_OPCODE_NOP) {
            ack->controller_status = BLE_HCI_ERR(BLE_HOST_ERR_CONTROLLER);
        }

        /* TODO Process num_pkts field. */
        ack->ack_status = true;
    } else {
        opcode = ev->opcode;

        /* TODO Process num_pkts field. */
        if (ack->expected_opcode != opcode) {
            BLE_HOST_HCI_COMMON_ERROR("Command complete unexpected opcode: expected 0x%04x, got 0x%04x", ack->expected_opcode, opcode);
        } else {
            if (ack->rsp_len == p_cmd_complete_evt->length - sizeof(*ev) && ack->rsp != NULL) {
                memcpy(ack->rsp, ev->return_params, ack->rsp_len);
            }
            ack->controller_status = ev->status;
            ack->ack_status        = true;
        }
    }
}

/**
 *   @brief BLE Host HCI command receive command status event process.
 *
 *   @param[in] p_evt - Pointer to the HCI command status event
 *
 *   @return None.
 */
void ble_host_hci_rx_cmd_status(void *p_evt)
{
    struct ble_hci_evt_h4                  *p_cmd_status_evt = p_evt;
    const struct ble_hci_ev_command_status *ev               = (void *)p_cmd_status_evt->data;
    struct ble_host_hci_cmd_ack            *ack              = &s_hci_cmd_ack;

    if (p_cmd_status_evt->length != sizeof(*ev)) {
        ack->controller_status = BLE_HCI_ERR(BLE_HOST_ERR_CONTROLLER);
        return;
    }

    /* XXX: Process num_pkts field. */
    if (ev->opcode != ack->expected_opcode) {
        BLE_HOST_HCI_COMMON_ERROR("Command status unexpected opcode: expected 0x%04x, got 0x%04x", ack->expected_opcode, ev->opcode);
    } else {
        ack->controller_status = ev->status;
        ack->ack_status        = true;
    }
}
