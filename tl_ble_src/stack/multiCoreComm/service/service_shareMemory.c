#include "common/compiler.h"
#include"service_shareMemory.h"
#include"service_mailbox.h"
#include"driver.h"
#if(TLK_MESSAGE_D25F == 1)
//define data block entity in d25f.
tlk_sm_fifo_t   d25fHciTXFifo;
u8              d25fHciTxBuffer[TLK_SM_HCI_TX_BUFFER_SIZE * TLK_SM_HCI_TX_BUFFER_NUM];
tlk_sm_fifo_t   d25fHciRxFifo;
u8              d25fHciRxBuffer[TLK_SM_HCI_RX_BUFFER_SIZE * TLK_SM_HCI_RX_BUFFER_NUM];
tlk_sm_fifo_t   d25fSyncRXFifo;
u8              d25fSyncRxBuffer[TLK_SM_SYNC_BUFFER_SIZE * TLK_SM_SYNC_BUFFER_NUM];
tlk_sm_fifo_t   d25fLogRXFifo;
u8              d25fLogRxBuffer[TLK_SM_LOG_BUFFER_SIZE * TLK_SM_LOG_BUFFER_NUM];
#if MAILBOX_FALSH_WR_NOTIFY
tlk_sm_fifo_t   d25fFlashWRFifo;
u8              d25fFlashWRBuffer[TLK_SM_FLASH_WR_BUFFER_SIZE * TLK_SM_FLASH_WR_BUFFER_NUM]={0};
#endif
tlk_sm_fifo_t  *d25fCSRawPctFifo = NULL;


tlk_sm_rx_cb_f d25fHciRxCb[SHARE_MEMORY_CB_NUM] =
{
};

tlk_sm_rx_cb_f d25fSyncRxCb[SHARE_MEMORY_CB_NUM] =
{
};

tlk_sm_rx_cb_f d25fLogRxCb[SHARE_MEMORY_CB_NUM] =
{

};


//d25fHciTXFifo
_attribute_ram_code_ tlk_sm_ret_e tlk_d25f_hci_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen)
{
    if ((data[0]==0x04) && (data[1]==0x3e) && (data[3]==0x02)) {
        // adv report
    }
    else {
        tlkapi_send_string_data(1, "hci tx",data,dataLen);
    }
    //tlksys_mutex_lock(TLKSYS_MUTEX_H2C);
    tlk_sm_ret_e retSts = share_memory_data_push(&d25fHciTXFifo,type,data,dataLen);
    //tlksys_mutex_unlock(TLKSYS_MUTEX_H2C);
    return retSts;
}


//d25fHciRxFifo
_attribute_ram_code_ void tlk_d25f_register_hci_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb)
{
    if(cb!=NULL && type < TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
    {
        d25fHciRxCb[type] = cb;
        share_memory_register_fifo_receive_cb(&d25fHciRxFifo,d25fHciRxCb);
    }
}

//d25fSyncRXFifo
_attribute_ram_code_ void tlk_d25f_register_sync_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb)
{
    if(cb!=NULL && type < TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
    {
        d25fSyncRxCb[type] = cb;
        share_memory_register_fifo_receive_cb(&d25fSyncRXFifo,d25fSyncRxCb);
    }
}

//d25fLogRXFifo
_attribute_ram_code_ void tlk_d25f_register_log_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb)
{
    if(cb!=NULL && type < TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
    {
        d25fLogRxCb[type]=cb;
        share_memory_register_fifo_receive_cb(&d25fLogRXFifo,d25fLogRxCb);
    }
}

u8 malbox_status = 0;

_attribute_ram_code_ void tlk_mailbox_flash_read_write_notify(u8 status)
{
#if MAILBOX_FALSH_WR_NOTIFY
    if(malbox_status == 0){
        return;
    }
    //tlkapi_printf(1, "tlk_mailbox_flash_read_write_notify, status=%d\r\n",status);
    tlkapi_debug_handler();
    u8 cmd[7]={0};
    u32 address = (u32)&d25fFlashWRFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    d25fFlashWRFifo.reserved = status;
    if (status == 1) {
        tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_FLASH_WR_ADDRESS,cmd);
        delay_us(200);
    }
#endif
}

#elif(TLK_MESSAGE_N22 == 1)
tlk_sm_fifo_t *n22HciTxFifo = NULL;
tlk_sm_fifo_t *n22SyncTxFifo = NULL;
tlk_sm_fifo_t *n22LogTxFifo = NULL;
tlk_sm_fifo_t *n22HciRxFifo = NULL;
tlk_sm_fifo_t *n22FlashWRFifo = NULL;

_attribute_data_retention_  u8            n22CSRawPctBuffer[TLK_SM_CS_RAW_PCT_BUFFER_NUM*TLK_SM_CS_RAW_PCT_BUFFER_SIZE]={0};
_attribute_data_retention_  tlk_sm_fifo_t n22CSRawPctFifo;

tlk_sm_rx_cb_f n22fHciRxCb[SHARE_MEMORY_CB_NUM] =
{
};


//n22HciTxFifo
_attribute_ram_code_ tlk_sm_ret_e tlk_n22_hci_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen)
{
    if ((data[0]==0x04) && (data[1]==0x3e) && (data[3]==0x02)) {
        // adv report
    }
    else {
        tlkapi_send_string_data(1, "hci tx",data,dataLen);
    }
    tlk_sm_ret_e retSts = share_memory_data_push(n22HciTxFifo,type,data,dataLen);

    return retSts;
}

//n22SyncTxFifo
_attribute_ram_code_ tlk_sm_ret_e tlk_n22_sync_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen)
{
    tlk_sm_ret_e retSts = share_memory_data_push(n22SyncTxFifo,type,data,dataLen);
    if(retSts == TLK_SHARE_MEMOTY_SUCCESS)
    {
        u8 cmd[8] = {0};
        tlk_mailbox_send_data(TLK_MESSAGE_FROM_N22_TO_D25F_SYNC_DATA_READY,cmd);
    }
    return retSts;
}

//n22SyncTxFifo
_attribute_ram_code_ tlk_sm_ret_e tlk_n22_log_send_message(tlk_sm_message_type_e type,u8 *data, u32 dataLen)
{
    tlk_sm_ret_e retSts = share_memory_data_push(n22LogTxFifo,type,data,dataLen);
    return retSts;
}

//n22HciRxFifo
_attribute_ram_code_ void tlk_n22_register_hci_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb)
{
    if(cb!=NULL && type < TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
    {
        n22fHciRxCb[type] = cb;
        share_memory_register_fifo_receive_cb(n22HciRxFifo,n22fHciRxCb);
    }
}
#endif

_attribute_ram_code_ int tlk_sm_cs_raw_buff_is_ready(void)
{
#if (TLK_MESSAGE_D25F == 1)
    return d25fCSRawPctFifo->status;
#elif(TLK_MESSAGE_N22 == 1)
    return n22CSRawPctFifo.status;
#endif
}

u32 tlk_sm_get_cs_raw_buff_addr(void)
{
#if (TLK_MESSAGE_D25F == 1)
    return ((u32)d25fCSRawPctFifo->fifo.p + (u32)BIT(31));
#elif(TLK_MESSAGE_N22 == 1)
    return (u32)n22CSRawPctBuffer;
//    return (u32)n22CSRawPctFifo.fifo.p;
//    return (u32)n22CSRawPctFifo.fifo.p + (u32)BIT(31);
#endif
}

volatile u32 AAA_MESSAGE_LOOP = 0;
extern volatile u32 AAA_SYNC_TX_ADDRESS;
extern volatile u32 AAA_FLASH_WR_ADDRESS;

_attribute_ram_code_ void tlk_share_memory_service_init(void)
{
#if(TLK_MESSAGE_D25F == 1)
    u8 cmd[7]={0};

    u32 address = (u32)&d25fHciTXFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    share_memory_fifo_init(&d25fHciTXFifo,d25fHciTxBuffer, TLK_SM_HCI_TX_BUFFER_NUM, TLK_SM_HCI_TX_BUFFER_SIZE);
    tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_HCI_RX_ADDRESS,cmd);
    delay_us(50);

    address = (u32)&d25fHciRxFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    share_memory_fifo_init(&d25fHciRxFifo,d25fHciRxBuffer, TLK_SM_HCI_RX_BUFFER_NUM, TLK_SM_HCI_RX_BUFFER_SIZE);
    share_memory_register_fifo_receive_cb(&d25fHciRxFifo,d25fHciRxCb);
    tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_HCI_TX_ADDRESS,cmd);
    delay_us(50);

    address = (u32)&d25fLogRXFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    share_memory_fifo_init(&d25fLogRXFifo,d25fLogRxBuffer, TLK_SM_LOG_BUFFER_NUM, TLK_SM_LOG_BUFFER_SIZE);
    share_memory_register_fifo_receive_cb(&d25fLogRXFifo,d25fLogRxCb);
    tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_LOG_TX_ADDRESS,cmd);
    delay_ms(1);

    address = (u32)&d25fSyncRXFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    share_memory_fifo_init(&d25fSyncRXFifo,d25fSyncRxBuffer, TLK_SM_SYNC_BUFFER_NUM, TLK_SM_SYNC_BUFFER_SIZE);
    share_memory_register_fifo_receive_cb(&d25fSyncRXFifo,d25fSyncRxCb);
    tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_SYNC_TX_ADDRESS,cmd);
    delay_us(50);

    do{
        tlk_mailbox_service_loop();
        static unsigned int nowTick2 = 0;
        if(clock_time_exceed(nowTick2, 200*1000)){
            nowTick2 = clock_time();
            #ifdef GPIO_LED_RED
                gpio_toggle(GPIO_LED_RED);
            #endif

            tlkapi_printf(1, "wait d25fSyncRXFifo.status: %d\r\n",d25fSyncRXFifo.status);
            tlkapi_debug_handler();
        }
    }while(d25fSyncRXFifo.status != TLK_SHARE_MEMOTY_STATUS_READY);
    tlkapi_printf(1, "wait d25fSyncRXFifo.status: %d\r\n",d25fSyncRXFifo.status);
    tlkapi_debug_handler();

    #if MAILBOX_FALSH_WR_NOTIFY
        address = (u32)&d25fFlashWRFifo;
        cmd[3] = (u8)(address&0xff);
        cmd[4] = (u8)(address>>8&0xff);
        cmd[5] = (u8)(address>>16&0xff);
        cmd[6] = (u8)(address>>24&0xff);
        share_memory_fifo_init(&d25fFlashWRFifo,d25fFlashWRBuffer, TLK_SM_FLASH_WR_BUFFER_SIZE, TLK_SM_FLASH_WR_BUFFER_NUM);
        d25fFlashWRFifo.status = TLK_SHARE_MEMOTY_STATUS_NOT_READY;
        d25fFlashWRFifo.reserved = 0;
        tlk_mailbox_send_data(TLK_MESSAGE_FROM_D25F_TO_N22_FLASH_WR_ADDRESS,cmd);
        delay_us(50);
        do{
            tlk_mailbox_service_loop();
            static unsigned int nowTick2 = 0;
            if(clock_time_exceed(nowTick2, 200*1000)){
                nowTick2 = clock_time();
                #ifdef GPIO_LED_RED
                    gpio_toggle(GPIO_LED_RED);
                #endif

                tlkapi_printf(1, "wait d25fFlashWRFifo.status: %d\r\n",d25fSyncRXFifo.status);
                tlkapi_debug_handler();
            }
        }while(d25fFlashWRFifo.status != TLK_SHARE_MEMOTY_STATUS_READY);
        tlkapi_printf(1, "wait d25fFlashWRFifo.status: %d\r\n",d25fSyncRXFifo.status);
        tlkapi_debug_handler();
    #endif
    #ifdef GPIO_LED_RED
        gpio_write(GPIO_LED_RED, 0);
    #endif
#elif(TLK_MESSAGE_N22 == 1)
    while(AAA_SYNC_TX_ADDRESS == 0);
    while(AAA_FLASH_WR_ADDRESS == 0);
    u8 cmd[7]={0};
    u32 address = (u32)&n22CSRawPctFifo;
    cmd[3] = (u8)(address&0xff);
    cmd[4] = (u8)(address>>8&0xff);
    cmd[5] = (u8)(address>>16&0xff);
    cmd[6] = (u8)(address>>24&0xff);
    share_memory_fifo_init(&n22CSRawPctFifo, n22CSRawPctBuffer, TLK_SM_CS_RAW_PCT_BUFFER_NUM, TLK_SM_CS_RAW_PCT_BUFFER_SIZE);
    tlk_mailbox_send_data(TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_ADDRESS,cmd);
    delay_us(50);
    do{
        tlk_mailbox_service_loop();
        static unsigned int nowTick2 = 0;
        if(clock_time_exceed(nowTick2, 200*1000)){
            nowTick2 = clock_time();
            #ifdef GPIO_LED_BLUE
                gpio_toggle(GPIO_LED_BLUE);
            #endif

            tlkapi_printf(1, "wait n22CSRawPctFifo.status: %d\r\n",n22CSRawPctFifo.status);
            tlkapi_debug_handler();
        }
    }while(n22CSRawPctFifo.status != TLK_SHARE_MEMOTY_STATUS_READY);
    #ifdef GPIO_LED_BLUE
        gpio_write(GPIO_LED_BLUE, 0);
    #endif
    tlkapi_printf(1, "wait n22CSRawPctFifo.status: %d\r\n",n22CSRawPctFifo.status);
    tlkapi_debug_handler();

#endif
}

_attribute_ram_code_ void tlk_share_memory_service_loop(void)
{
#if(TLK_MESSAGE_D25F == 1)
    share_memory_data_pop(&d25fHciRxFifo);
    share_memory_data_pop(&d25fLogRXFifo);
#elif(TLK_MESSAGE_N22 == 1)
    share_memory_data_pop(n22HciRxFifo);

    #if 0
    static volatile unsigned int tickCSCheck = 0;
    if (clock_time_exceed(tickCSCheck, 500*1000)) {
        tickCSCheck = clock_time();
        tlkapi_printf(1, "d25f_rx_cs_raw_pct_data:%x\r\n",n22CSRawPctFifo.fifo.wptr);
        u8 *p = n22CSRawPctFifo.fifo.p + (n22CSRawPctFifo.fifo.wptr & (n22CSRawPctFifo.fifo.num-1)) * n22CSRawPctFifo.fifo.size;
        static u8 numt = 0;
        numt++;
        p[0] = numt;
        p[1] = 1;
        p[2] = 2;
        p[3 ]= 3;
        p[4] = 4;
        p[5] = 5;
        p[6] = 6;
        p[7] = numt;
        n22CSRawPctFifo.fifo.wptr++;

        tlkapi_send_string_data(1, "n22 mailbox_send_data", p, 8);
        u8 cmd[7]={0};
        u32 address = (u32)&n22CSRawPctFifo;
        cmd[3] = (u8)(address&0xff);
        cmd[4] = (u8)(address>>8&0xff);
        cmd[5] = (u8)(address>>16&0xff);
        cmd[6] = (u8)(address>>24&0xff);
        tlk_mailbox_send_data(TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_DATA_READY,cmd);

        gpio_toggle(GPIO_PG6);
    }
    #endif
#endif
}


void tlk_share_memory_service_hci_handler(void)
{
#if(TLK_MESSAGE_D25F == 1)
    share_memory_data_popAll(&d25fHciRxFifo);
#endif
}

void tlk_share_memory_service_log_handler(void)
{
#if(TLK_MESSAGE_D25F == 1)
    share_memory_data_popAll(&d25fLogRXFifo);
#endif
}



