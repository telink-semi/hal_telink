
#include"../comm.h"
#include"../drv/shareMemory.h"
#define TLK_SM_HCI_TX_BUFFER_SIZE       700
#define TLK_SM_HCI_TX_BUFFER_NUM        16

#define TLK_SM_HCI_RX_BUFFER_SIZE       700
#define TLK_SM_HCI_RX_BUFFER_NUM        16

#define TLK_SM_LOG_BUFFER_SIZE          200
#define TLK_SM_LOG_BUFFER_NUM           32

#define TLK_SM_SYNC_BUFFER_SIZE         16//256
#define TLK_SM_SYNC_BUFFER_NUM          4//32

#define TLK_SM_CS_RAW_PCT_BUFFER_SIZE   16
#define TLK_SM_CS_RAW_PCT_BUFFER_NUM    8

#define TLK_SM_FLASH_WR_BUFFER_SIZE     32
#define TLK_SM_FLASH_WR_BUFFER_NUM      4


u32 tlk_sm_get_cs_raw_buff_addr(void);

int tlk_sm_cs_raw_buff_is_ready(void);

void tlk_share_memory_service_init(void);

void tlk_share_memory_service_loop(void);

void tlk_share_memory_service_hci_handler(void);

void tlk_share_memory_service_log_handler(void);

#if(TLK_MESSAGE_D25F == 1)
void tlk_d25f_register_hci_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb);

void tlk_d25f_register_sync_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb);

void tlk_d25f_register_log_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb);

tlk_sm_ret_e tlk_d25f_hci_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen);

#elif(TLK_MESSAGE_N22 == 1)
tlk_sm_ret_e tlk_n22_hci_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen);

tlk_sm_ret_e tlk_n22_sync_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen);

tlk_sm_ret_e tlk_n22_log_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen);

void tlk_n22_register_hci_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb);
#endif

