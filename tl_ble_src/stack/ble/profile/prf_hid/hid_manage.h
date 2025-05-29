#ifdef MCU_CORE_D25F_ENABLE
#ifndef HID_MANAGE_H
#define HID_MANAGE_H
enum{
    HID_SUCCESS,
    HID_FAIL,
    HID_NULL_POINTER,
    HID_INVALID_PARAM,
    HID_WAIT_ACK
};
typedef uint8_t (*hid_manage_obtain_data_t)(uint8_t*, const uint8_t);
typedef uint8_t (*hid_manage_send_data_t)(uint8_t*, const uint8_t);

int hid_manage_init(uint8_t max_sdu_len, hid_manage_obtain_data_t hid_manage_obtain_data);
void hid_manage_set_current_mode(uint8_t repetition,uint8_t confirmation);
void hid_manage_reset_current_mode(void);
void hid_manage_set_recive_ack_seq_num(uint8_t ackSeqNum);
uint8_t hid_manage_make_hybrid_mode_data(uint8_t *sdu_data,uint8_t reportID);
uint8_t hid_manage_parse_hybrid_mode_data(const uint8_t *sdu_data, uint8_t sdu_len, uint8_t **report_data, hid_manage_send_data_t hid_manage_send_data);
int hid_manage_check_state(void);
#endif //HID_MANAGE_H
#endif
