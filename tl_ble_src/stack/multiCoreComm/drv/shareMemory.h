#ifndef STACK_HAL_HAL_TL322X_DUAL_CORE_SHARE_MEMORY_SM_H_
#define STACK_HAL_HAL_TL322X_DUAL_CORE_SHARE_MEMORY_SM_H_
#include "tl_common.h"

typedef enum
{
    TLK_SHARE_MEMOTY_STATUS_NOT_READY,
    TLK_SHARE_MEMOTY_STATUS_READY,
} tlk_sm_status_e;

typedef enum
{
    TLK_SHARE_MEMOTY_SUCCESS,
    TLK_SHARE_MEMOTY_NOT_READY,
    TLK_SHARE_MEMOTY_FULL,
}tlk_sm_ret_e;

typedef enum
{
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_BLE,
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_BT,
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_TPSLL,
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_LOG_USB,
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_LOG_UART,
    TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX,
}tlk_sm_message_type_e;


#define SHARE_MEMORY_CB_NUM 10

typedef struct __attribute__((packed))
{
    tlk_fifo1_t fifo;
    u16         status;
    u16         reserved;
    void (*rxCb[SHARE_MEMORY_CB_NUM])(u8 *, u32);
} tlk_sm_fifo_t;

typedef void (*tlk_sm_rx_cb_f)(u8 *, u32);

void share_memory_fifo_init(tlk_sm_fifo_t* smFifo,u8 *p, u32 num, u32 size);

void share_memory_register_fifo_receive_cb(tlk_sm_fifo_t* smFifo,tlk_sm_rx_cb_f cb[]);

void share_memory_set_fifo_status(tlk_sm_fifo_t* smFifo,tlk_sm_status_e status);

int  share_memory_data_pop(tlk_sm_fifo_t* smFifo);

int  share_memory_data_popAll(tlk_sm_fifo_t* smFifo);

tlk_sm_ret_e share_memory_data_push(tlk_sm_fifo_t* smFifo,tlk_sm_message_type_e type, u8 *data, u32 dataLen);

#endif /* STACK_HAL_HAL_TL751X_DUAL_CORE_SHARE_MEMORY_SM_H_ */
