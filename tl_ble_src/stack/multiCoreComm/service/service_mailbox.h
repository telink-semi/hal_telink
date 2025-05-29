
#ifndef STACK_MULTICORECOMM_SERVICE_SERVICE_MAILBOX_H_
#define STACK_MULTICORECOMM_SERVICE_SERVICE_MAILBOX_H_
#include"../comm.h"

typedef enum
{
    TLK_MESSAGE_FROM_D25F_TO_N22_HCI_TX_ADDRESS = 1,
    TLK_MESSAGE_FROM_D25F_TO_N22_SYNC_TX_ADDRESS,
    TLK_MESSAGE_FROM_D25F_TO_N22_LOG_TX_ADDRESS,
    TLK_MESSAGE_FROM_D25F_TO_N22_HCI_RX_ADDRESS,
    TLK_MESSAGE_FROM_D25F_TO_N22_TEST,
    TLK_MESSAGE_FROM_D25F_TO_N22_SM_HCISCO_RX_ADDRESS,
    TLK_MESSAGE_FROM_D25F_TO_N22_TWS_SYNC_USBID,
    TLK_MESSAGE_FROM_D25F_TO_N22_FLASH_WR_ADDRESS,
    TLK_MESSAGE_FROM_D25F_TO_N22_MAX,
}tlk_mailbox_from_d25f_to_n22_e;

typedef enum
{
    TLK_MESSAGE_FROM_N22_TO_D25F_SYNC_DATA_READY = 1,
    TLK_MESSAGE_FROM_N22_TO_D25F_TEST,
    TLK_MESSAGE_FROM_N22_TO_D25F_SM_DATA_READY,
    TLK_MESSAGE_FROM_N22_TO_D25F_STIMER_START_EVT,

    /*Channel Sounding */
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_ADDRESS,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_DATA_READY,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_FPU_CALC_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_FREQ_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_PES_COLLECT_DATA_INIT_SDK_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_FINE_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_SDK_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_NADM_DETECT_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_HARD_FIX_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_SOFT_TRIGGER,
    TLK_MESSAGE_FROM_N22_TO_D25F_CS_COMPRESS_TEST_INFO_TRIGGER,

    TLK_MESSAGE_FROM_N22_TO_D25F_MAX,
}tlk_mailbox_from_n22_to_d25f_e;

typedef enum
{
    TLK_SM_DATA_READY_TYPE_HCI = 0,
    TLK_SM_DATA_READY_TYPE_LOG,
    TLK_SM_DATA_READY_TYPE_MAX_NUM,
}tlk_sm_data_ready_type_e;


typedef void(*tlk_mailbox_receive_cb_f)(u8*);

#define TLK_MAILBOX_RECEIVE_CB_NUM       16


/**
 * @brief      This function servers to init mailbox service.
 * @param[in]  none
 * @return     none
 */
void tlk_mailbox_service_init(void);

/**
 * @brief      This function servers to send cmd
 * @param[in]  cmd   - in d25f,search in tlk_mailbox_from_d25f_to_n22_e.
 *                     in n22, search in tlk_mailbox_from_n22_to_d25f_e.
 * @param[in]  data  - data corresponding to cmd,7 bytes at most.
 * @return     none
 */
void tlk_mailbox_send_data(u8 cmd,u8* data);

/**
 * @brief      This function servers to register rx callback in mailbox.
 * @param[in]  cmd - in d25f,search in tlk_mailbox_from_n22_to_d25f_e.
 *                   in n22, search in tlk_mailbox_from_d25f_to_n22_e.
 * @param[in]  cb  - receive callback corresponding to cmd.
 * @return     none
 */
void tlk_mailbox_register_message_cb(u8 cmd,tlk_mailbox_receive_cb_f cb);

/**
 * @brief      This function servers to process mailbox loop task.
 * @param[in]  cmd - none
 * @return     none
 */
void tlk_mailbox_service_loop(void);

#endif /* STACK_MULTICORECOMM_SERVICE_SERVICE_MAILBOX_H_ */
