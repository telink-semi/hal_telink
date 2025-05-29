/********************************************************************************************************
 * @file    hci_cmd_le_misc.c
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
#include <string.h>
#include "common/types.h"


#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../inc/ble_hci.h"
#include "../inc/ble_hci_cmd.h"
#include "../inc/ble_hci_log.h"

// HCI command LE functions.


int ble_host_hci_le_set_event_mask(const struct ble_hci_le_set_event_mask_cp *p_le_event_mask)
{
    if (p_le_event_mask == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le set event mask null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_EVENT_MASK),
                                 p_le_event_mask,
                                 sizeof(struct ble_hci_le_set_event_mask_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE set event mask return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_read_buffer_size(struct ble_hci_le_rd_buf_size_rp *p_le_buf_size)
{
    if (p_le_buf_size == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read buffer size null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_BUF_SIZE),
                                 NULL,
                                 0,
                                 p_le_buf_size,
                                 sizeof(struct ble_hci_le_rd_buf_size_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read buffer size return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_read_buffer_size_v2(struct ble_hci_le_rd_buf_size_v2_rp *p_le_buf_size_v2)
{
    if (p_le_buf_size_v2 == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read buffer size v2 null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_BUF_SIZE_V2),
                                 NULL,
                                 0,
                                 p_le_buf_size_v2,
                                 sizeof(struct ble_hci_le_rd_buf_size_v2_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read buffer size v2 return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_read_local_supported_features(struct ble_hci_le_rd_loc_supp_feat_rp *p_le_local_supp_feat)
{
    if (p_le_local_supp_feat == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read local supported features null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_LOC_SUPP_FEAT),
                                 NULL,
                                 0,
                                 p_le_local_supp_feat,
                                 sizeof(struct ble_hci_ip_rd_loc_supp_feat_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read local supported features return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_set_random_address(const struct ble_hci_le_set_rand_addr_cp *p_le_rand_addr)
{
    if (p_le_rand_addr == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE set random address null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_RAND_ADDR),
                                 p_le_rand_addr,
                                 sizeof(struct ble_hci_le_set_rand_addr_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE set random address return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_connection_update(const struct ble_hci_le_conn_update_cp *p_conn_update)
{
    if (p_conn_update == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE connection update null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_CONN_UPDATE),
                                 p_conn_update,
                                 sizeof(struct ble_hci_le_conn_update_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE connection update return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_set_host_channel_classification(const struct ble_hci_le_set_host_chan_class_cp *p_host_chan_class)
{
    if (p_host_chan_class == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE set host channel classification null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_HOST_CHAN_CLASS),
                                 p_host_chan_class,
                                 sizeof(struct ble_hci_le_set_host_chan_class_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE set host channel classification return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_read_channel_map(const struct ble_hci_le_rd_chan_map_cp *p_rd_chan_map, struct ble_hci_le_rd_chan_map_rp *p_rd_chan_map_rp)
{
    if (p_rd_chan_map == NULL || p_rd_chan_map_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read channel map null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_CHAN_MAP),
                                 p_rd_chan_map,
                                 sizeof(struct ble_hci_le_rd_chan_map_cp),
                                 p_rd_chan_map_rp,
                                 sizeof(struct ble_hci_le_rd_chan_map_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read channel map return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_read_remote_features(const struct ble_hci_le_rd_rem_feat_cp *p_rd_rem_feat)
{
    if (p_rd_rem_feat == NULL ) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read remote features null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_REM_FEAT),
                                 p_rd_rem_feat,
                                 sizeof(struct ble_hci_le_rd_rem_feat_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE read remote features return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_encrypt(const struct ble_hci_le_encrypt_cp *p_le_encrypt, struct ble_hci_le_encrypt_rp *p_le_encrypt_rp)
{
    if (p_le_encrypt == NULL || p_le_encrypt_rp == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE encrypt null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_ENCRYPT),
                                 p_le_encrypt,
                                 sizeof(struct ble_hci_le_encrypt_cp),
                                 p_le_encrypt_rp,
                                 sizeof(struct ble_hci_le_encrypt_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE encrypt return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_rand(struct ble_hci_le_rand_rp *p_le_rand)
{
    if (p_le_rand == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE rand null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RAND),
                                 NULL,
                                 0,
                                 p_le_rand,
                                 sizeof(struct ble_hci_le_rand_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE rand return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_gen_rand(void *dst, int len)
{
    BLE_HOST_HCI_LE_CMD_INFO("[HCI][CMD] LE_Rand");

    struct ble_hci_le_rand_rp rsp;
    uint8_t *u8ptr;
    int chunk_sz;
    int rc = 0;

    u8ptr = dst;
    while (len > 0) {
        rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RAND),
                               NULL, 
                               0, 
                               &rsp, 
                               sizeof(rsp));
        if (rc != 0) {
            break;
        }

        chunk_sz = min(len, (int)sizeof(rsp));
        memcpy(u8ptr, &rsp.random_number, chunk_sz);

        len -= chunk_sz;
        u8ptr += chunk_sz;
    }

    BLE_HOST_HCI_LE_CMD_INFO("LE rand return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_start_encryption(const struct ble_hci_le_start_encrypt_cp *p_le_start_encrypt)
{
    if (p_le_start_encrypt == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE start encryption null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_START_ENCRYPT),
                                 p_le_start_encrypt,
                                 sizeof(struct ble_hci_le_start_encrypt_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE start encryption return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_long_term_key_request_reply(const struct ble_hci_le_lt_key_req_reply_cp *p_le_lt_key_req_reply)
{
    if (p_le_lt_key_req_reply == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE long term key request reply null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_REPLY),
                                 p_le_lt_key_req_reply,
                                 sizeof(struct ble_hci_le_lt_key_req_reply_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE long term key request reply return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_long_term_key_request_negative_reply(const struct ble_hci_le_lt_key_req_neg_reply_cp *p_le_lt_key_req_neg_reply)
{
    if (p_le_lt_key_req_neg_reply == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE long term key request negative reply null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_NEG_REPLY),
                                 p_le_lt_key_req_neg_reply,
                                 sizeof(struct ble_hci_le_lt_key_req_neg_reply_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE long term key request negative reply return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_read_supported_states(struct ble_hci_le_rd_supp_states_rp *p_le_supp_states)
{
    if (p_le_supp_states == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE read supported states null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_RD_SUPP_STATES),
                                 NULL,
                                 0,
                                 p_le_supp_states,
                                 sizeof(struct ble_hci_le_rd_supp_states_rp));

    BLE_HOST_HCI_LE_CMD_INFO("LE read supported states return error code 0x%x", rc);

    return rc;
}
//TODO: add more le command functions here

int ble_host_hci_le_set_host_feature(const struct ble_hci_le_set_host_feature_cp *p_le_host_feature)
{
    if (p_le_host_feature == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("le set host feature null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }
    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_SET_HOST_FEATURE),
                                 p_le_host_feature,
                                 sizeof(struct ble_hci_le_set_host_feature_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE set host feature return error code 0x%x", rc);
    return rc;
}

int ble_host_hci_le_ltk_request_reply(struct ble_hci_le_lt_key_req_reply_cp *p_le_lt_key_req_reply)
{
    if(p_le_lt_key_req_reply == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE LTK request reply null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_REPLY),
                                 p_le_lt_key_req_reply,
                                 sizeof(struct ble_hci_le_lt_key_req_reply_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE LTK request reply return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_le_ltk_request_negative_reply(struct ble_hci_le_lt_key_req_neg_reply_cp *p_le_lt_key_req_neg_reply)
{
    if (p_le_lt_key_req_neg_reply == NULL) {
        BLE_HOST_HCI_LE_CMD_ERROR("LE LTK request negative reply null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_NEG_REPLY),
                                 p_le_lt_key_req_neg_reply,
                                 sizeof(struct ble_hci_le_lt_key_req_neg_reply_cp),
                                 NULL,
                                 0);

    BLE_HOST_HCI_LE_CMD_INFO("LE LTK request negative reply return error code 0x%x", rc);

    return rc;
}

int ble_host_hci_ltk_request_reply(uint16_t connHandle, uint8_t *ltk)
{
    BLE_HOST_HCI_LE_CMD_INFO("[HCI][CMD] LTK_Request_Reply", ltk, 16);
    struct ble_hci_le_lt_key_req_reply_cp cmd;
    cmd.conn_handle = connHandle;
    memcpy(cmd.ltk, ltk, 16);

    return ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_REPLY),
                               &cmd,
                               sizeof(cmd),
                               NULL,
                               0);
}

int ble_host_hci_ltk_request_negative_reply(uint16_t connHandle)
{
    BLE_HOST_HCI_LE_CMD_INFO("[HCI][CMD] LTK_Request_Negative_Reply");

    struct ble_hci_le_lt_key_req_neg_reply_cp cmd;
    cmd.conn_handle = connHandle;

    return ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_LE, BLE_HCI_OCF_LE_LT_KEY_REQ_NEG_REPLY),
                               &cmd,
                               sizeof(cmd),
                               NULL,
                               0);
}
