
#ifdef MCU_CORE_D25F_ENABLE
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "hid_manage.h"
#include "stack/ble/host/ble_host_internal.h"

typedef struct __attribute__((packed)) {
    u8 length;
    u8 sequenceNumber;
    u8 reportId;
    u8 data[0];
} ullhid_data_t;

static uint8_t s_hid_manage_confirmation = 0;
static uint8_t s_hid_manage_repetition = 0;
static uint8_t s_hid_manage_sequenceNumber = 0;
static uint8_t s_hid_manage_recvAckSeqNum = 0;
static uint8_t s_hid_manage_reportIndex = 0;
static uint8_t s_hid_manage_flushData = 0;
static uint8_t s_hid_manage_maxPDULen = 0;
static uint8_t *s_hid_manage_buffer_cache = NULL;
static hid_manage_obtain_data_t s_hid_manage_obtain_data = NULL;

int hid_manage_init(uint8_t max_sdu_len, hid_manage_obtain_data_t hid_manage_obtain_data) {
    if (s_hid_manage_buffer_cache || !hid_manage_obtain_data || !max_sdu_len) {
        return HID_FAIL;
    }
//    struct ble_sm_proc *proc = (struct ble_sm_proc *)ble_host_malloc(sizeof(struct ble_sm_proc), BLE_HOST_MALLOC_SMP_MANAGER);
    s_hid_manage_buffer_cache = ble_host_malloc_v0((max_sdu_len + 1) * 2, BLE_PRF_HIDS_SERVER_TYPE);
    if (s_hid_manage_buffer_cache == NULL) {
        return HID_FAIL;
    }
    s_hid_manage_maxPDULen = max_sdu_len;
    s_hid_manage_obtain_data = hid_manage_obtain_data;
    memset(s_hid_manage_buffer_cache, 0, s_hid_manage_maxPDULen);
    return HID_SUCCESS;
}

void hid_manage_set_current_mode(uint8_t repetition, uint8_t confirmation) {
    s_hid_manage_repetition = repetition;
    s_hid_manage_confirmation = confirmation;

    s_hid_manage_sequenceNumber = 0;
    s_hid_manage_recvAckSeqNum = 0;
    s_hid_manage_reportIndex = 0;
    s_hid_manage_flushData = 1;
    if (s_hid_manage_buffer_cache) {
        memset(s_hid_manage_buffer_cache, 0, s_hid_manage_maxPDULen);
    }
}

void hid_manage_reset_current_mode(void) {
    hid_manage_set_current_mode(0, 0);
}

void hid_manage_set_recive_ack_seq_num(uint8_t ackSeqNum) {
    s_hid_manage_recvAckSeqNum = ackSeqNum;
    if (s_hid_manage_recvAckSeqNum == s_hid_manage_sequenceNumber) {
        // s_hid_manage_flushData = 1;
    }
}

int hid_manage_check_state(void) {
    if (!s_hid_manage_confirmation || s_hid_manage_recvAckSeqNum == s_hid_manage_sequenceNumber) {
        return HID_SUCCESS;
    }
    return HID_WAIT_ACK;
}

uint8_t hid_manage_make_hybrid_mode_data(uint8_t *sdu_data, uint8_t reportID) {
    uint8_t *last_sdu = s_hid_manage_buffer_cache;
    uint8_t *user_data = s_hid_manage_buffer_cache + s_hid_manage_maxPDULen;
    uint8_t user_data_len = s_hid_manage_maxPDULen;
    uint8_t element_len = 0;
    uint8_t used_data_len = 0;
    ullhid_data_t *txSdu = NULL;

    if (s_hid_manage_buffer_cache == NULL || s_hid_manage_obtain_data == NULL || sdu_data == NULL || user_data ==
        NULL) {
        tlk_printf("error param is null");
        return 0;
    }

    // tlk_printf("reportID %d",reportID);
    // tlk_printf("s_hid_manage_sequenceNumber %d",s_hid_manage_sequenceNumber);
    if (hid_manage_check_state() == HID_WAIT_ACK) {
        // if (!s_hid_manage_flushData) {
            used_data_len = last_sdu[0];
            memcpy(sdu_data, last_sdu + 1, last_sdu[used_data_len]);
            return used_data_len;
        // }
    }

    user_data_len = s_hid_manage_obtain_data(user_data, user_data_len);
    if (user_data_len == 0) {
        tlk_printf("obtain user data fail");
        return 0;
    }
    element_len = user_data_len + sizeof(ullhid_data_t);
    s_hid_manage_sequenceNumber++;
    if (s_hid_manage_repetition) {
        u8 max_sdu_num = s_hid_manage_maxPDULen / element_len;
        if (max_sdu_num == 0) {
            return 0;
        }

        if (s_hid_manage_reportIndex < max_sdu_num) {
            txSdu = (ullhid_data_t *) (sdu_data + s_hid_manage_reportIndex * element_len);
            s_hid_manage_reportIndex++;
        } else {
            memcpy(sdu_data, sdu_data + element_len, element_len * (max_sdu_num - 1));
            txSdu = (ullhid_data_t *) (sdu_data + element_len * (max_sdu_num - 1));
        }
        txSdu->length = user_data_len;
        txSdu->sequenceNumber = s_hid_manage_sequenceNumber;
        txSdu->reportId = reportID;
        memcpy(txSdu->data, user_data, user_data_len);
        used_data_len = element_len * s_hid_manage_reportIndex;
    } else {
        ullhid_data_t *ullhid_date = (ullhid_data_t *) sdu_data;
        ullhid_date->length = user_data_len;
        ullhid_date->sequenceNumber = s_hid_manage_sequenceNumber;
        ullhid_date->reportId = reportID;
        memcpy(ullhid_date->data, user_data, user_data_len);
        used_data_len = element_len;
    }

    if (s_hid_manage_confirmation) {
        // s_hid_manage_flushData = 0;
        last_sdu[0] = used_data_len;
        memcpy(last_sdu + 1, sdu_data, used_data_len);
    }
    // tlk_printf("pdata %s",hex_to_str(sdu_data,used_data_len));
    return used_data_len;
}

uint8_t hid_manage_parse_hybrid_mode_data(const uint8_t *sdu_data, uint8_t sdu_len, uint8_t **report_data,
                                          hid_manage_send_data_t hid_manage_send_data) {
    ullhid_data_t *element = (ullhid_data_t *) sdu_data;
    uint8_t element_len = element->length + sizeof(ullhid_data_t);
    while (element_len < sdu_len && element->length > 0) {
        sdu_len -= element_len;
        sdu_data += element_len;
        element = (ullhid_data_t *) sdu_data;
    }
    if (s_hid_manage_confirmation) {
        if (element->length == 0) {
            hid_manage_set_recive_ack_seq_num(element->sequenceNumber);
            return 0;
        }
        ullhid_data_t ack_element;
        ack_element.length = 0;
        ack_element.reportId = element->reportId;
        ack_element.sequenceNumber = element->sequenceNumber;
        if (hid_manage_send_data) {
            hid_manage_send_data((u8 *) &ack_element, 3);
        }
    }
    *report_data = element->data;
    return element->length;
}
#endif
