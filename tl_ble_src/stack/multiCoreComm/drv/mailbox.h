
#include"tl_common.h"

typedef void (*mailbox_rx_cb_f)(u8*);

#define BTBLE_MAILBOX_TX_BUFFER_NUM 32

void mailbox_init(mailbox_rx_cb_f cb);

void mailbox_loop(void);

void mailbox_send_data(u8* data);

void mailbox_n22_to_d25_irq_handler(void);
