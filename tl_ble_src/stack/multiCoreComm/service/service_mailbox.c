
#include"../comm.h"
#include"service_mailbox.h"
#include"../drv/mailbox.h"
#include"../drv/shareMemory.h"
#include"driver.h"
typedef void(*tlk_mailbox_reveive_cb_f)(u8*);

tlk_mailbox_reveive_cb_f tlkMailboxReceiveCb[TLK_MAILBOX_RECEIVE_CB_NUM];

_attribute_ram_code_ void tlk_mailbox_receive_process(u8* cmd)
{
    if((cmd[0]<TLK_MAILBOX_RECEIVE_CB_NUM) && (tlkMailboxReceiveCb[cmd[0]]!=NULL))
    {
        tlkMailboxReceiveCb[cmd[0]](&cmd[1]);
    }
}

#if(TLK_MESSAGE_N22 == 1)

extern tlk_sm_fifo_t *n22HciTxFifo;
extern tlk_sm_fifo_t *n22SyncTxFifo;
extern tlk_sm_fifo_t *n22LogTxFifo;
extern tlk_sm_fifo_t *n22HciRxFifo;
#if MAILBOX_FALSH_WR_NOTIFY
extern tlk_sm_fifo_t *n22FlashWRFifo;
#endif
extern tlk_sm_rx_cb_f n22fHciRxCb[SHARE_MEMORY_CB_NUM];

volatile u32 AAA_HCI_TX_ADDRESS = 0;
_attribute_ram_code_ void tlk_mailbox_n22_set_hci_tx_fifo(u8* data)
{
    n22HciTxFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24));
    AAA_HCI_TX_ADDRESS = (u32)n22HciTxFifo;
    share_memory_set_fifo_status(n22HciTxFifo,TLK_SHARE_MEMOTY_STATUS_READY);
}

volatile u32 AAA_HCI_RX_ADDRESS = 0;
_attribute_ram_code_ void tlk_mailbox_n22_set_hci_rx_fifo(u8* data)
{

    n22HciRxFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24));
    AAA_HCI_RX_ADDRESS = (u32)(n22HciRxFifo);
    share_memory_set_fifo_status(n22HciRxFifo,TLK_SHARE_MEMOTY_STATUS_READY);
    extern int blc_hci_handler(u8 *p, int n);
    extern void tlk_n22_register_hci_receive_cb(tlk_sm_message_type_e type,tlk_sm_rx_cb_f cb);
    tlk_n22_register_hci_receive_cb(TLK_SHARE_MEMOTY_MESSAGE_TYPE_BLE, (tlk_sm_rx_cb_f)blc_hci_handler);
    share_memory_register_fifo_receive_cb(n22HciRxFifo,n22fHciRxCb);
}

volatile u32 AAA_LOG_TX_ADDRESS = 0;
_attribute_ram_code_ void tlk_mailbox_n22_set_log_tx_fifo(u8* data)
{
    n22LogTxFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24));
    AAA_LOG_TX_ADDRESS = (u32)(n22LogTxFifo);
    share_memory_set_fifo_status(n22LogTxFifo,TLK_SHARE_MEMOTY_STATUS_READY);
}

volatile u32 AAA_SYNC_TX_ADDRESS = 0;
_attribute_ram_code_ void tlk_mailbox_n22_set_sync_tx_fifo(u8* data)
{
    n22SyncTxFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24));
    AAA_SYNC_TX_ADDRESS = (u32)(n22SyncTxFifo);
    share_memory_set_fifo_status(n22SyncTxFifo,TLK_SHARE_MEMOTY_STATUS_READY);
}

#if MAILBOX_FALSH_WR_NOTIFY
volatile u32 AAA_FLASH_WR_ADDRESS = 0;
_attribute_ram_code_ void tlk_mailbox_n22_set_falsh_wr_fifo(u8* data)
{
    if (n22FlashWRFifo == NULL) {
        n22FlashWRFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24));
        AAA_FLASH_WR_ADDRESS = (u32)(n22FlashWRFifo);
        share_memory_set_fifo_status(n22FlashWRFifo,TLK_SHARE_MEMOTY_STATUS_READY);
    }
    else if (n22FlashWRFifo->status == TLK_SHARE_MEMOTY_STATUS_READY) {
        while (n22FlashWRFifo->reserved == 1) {
            gpio_toggle(GPIO_PG6);
        }
    }
}
#endif

#elif(TLK_MESSAGE_D25F == 1)
extern tlk_sm_fifo_t d25fSyncRXFifo;
_attribute_ram_code_ void tlk_mailbox_d25f_sync_data_process(u8* data)
{
    (void) data;
    share_memory_data_pop(&d25fSyncRXFifo);
}

volatile u8 AAA_D25F_TEST_BUFFER[7] = {0};
_attribute_ram_code_ void tlk_mailbox_d25f_test_process(u8* data)
{
    memcpy((u8*)AAA_D25F_TEST_BUFFER,data,7);
}


extern tlk_sm_fifo_t  *d25fCSRawPctFifo;
_attribute_ram_code_ void tlk_mailbox_d25f_set_cs_raw_pct_fifo(u8* data)
{
    d25fCSRawPctFifo = (tlk_sm_fifo_t*)((u32)data[3] + ((u32)data[4]<<8) + ((u32)(data)[5]<<16) + ((u32)(data)[6]<<24) + (u32)BIT(31)) ;
    share_memory_set_fifo_status(d25fCSRawPctFifo,TLK_SHARE_MEMOTY_STATUS_READY);
}

extern void app_ble_host_cs_raw_pct_process(uint8_t *data, unsigned int len);
_attribute_ram_code_ void tlk_mailbox_d25f_set_cs_raw_pct_data_ready(u8* data)
{
    (void)data;
    gpio_toggle(GPIO_PH0);
    if ((d25fCSRawPctFifo != NULL) &&
        (d25fCSRawPctFifo->status == TLK_SHARE_MEMOTY_STATUS_READY) &&
        (d25fCSRawPctFifo->fifo.rptr != d25fCSRawPctFifo->fifo.wptr))
    {
        u8 *p = (u8 *)((u32)BIT(31) + (u32)(d25fCSRawPctFifo->fifo.p + (d25fCSRawPctFifo->fifo.rptr & (d25fCSRawPctFifo->fifo.num - 1)) * d25fCSRawPctFifo->fifo.size));
        d25fCSRawPctFifo->fifo.rptr++;
        app_ble_host_cs_raw_pct_process(p, d25fCSRawPctFifo->fifo.size);
        //tlkapi_send_string_data(1, "d25f_rx_cs_raw_pct_data",p,8);
    }
}

_attribute_ram_code_ void tlk_mailbox_d25f_cs_fpu_calc(u8* data)
{
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();

    if (fp == NULL) {
        return;
    }

    u32 type = *((u32 *)fp);

    switch(type)
    {
    case 0:
        break;
    case 1:
        break;
    default:
        tlkapi_send_string_data(1, "tlk_mailbox_d25f_cs_fpu_calc, error calc type!!!",0,0);
        break;
    }
}

#endif


_attribute_ram_code_ void tlk_mailbox_send_data(u8 cmd,u8* data)
{
    u8 sendData[8] = {0};
    sendData[0] = cmd;
    tmemcpy(&sendData[1],data,7);
    mailbox_send_data(sendData);
}

_attribute_ram_code_ void tlk_mailbox_register_message_cb(u8 cmd,tlk_mailbox_receive_cb_f cb)
{
    if((cmd<TLK_MAILBOX_RECEIVE_CB_NUM) && cb!=NULL)
    {
        tlkMailboxReceiveCb[cmd] = cb;
    }
}

_attribute_ram_code_ void tlk_mailbox_d25f_calcFreq(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_calcFreq(u8* data);
    blt_calcFreq(fp);
}
_attribute_ram_code_ void tlk_mailbox_d25f_pesCollectDataInitSDK(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_pesCollectDataInitSDK(u8* data);
    blt_pesCollectDataInitSDK(fp);
}

_attribute_ram_code_ void tlk_mailbox_d25f_calcPesInfoFine(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_calcPesInfoFine(u8* data);
    blt_calcPesInfoFine(fp);
}

_attribute_ram_code_ void tlk_mailbox_d25f_calcPesInfoSDK(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_calcPesInfoSDK(u8* data);
    blt_calcPesInfoSDK(fp);
}

_attribute_ram_code_ void tlk_mailbox_d25f_cs_nadm_detect(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_cs_nadm_detect(u8* data);
    blt_cs_nadm_detect(fp);
}

_attribute_ram_code_ void tlk_mailbox_d25f_calcTesInfoAsicHardFix(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_calcTesInfoAsicHardFix(u8* data);
    blt_calcTesInfoAsicHardFix(fp);
}
_attribute_ram_code_ void tlk_mailbox_d25f_calcTesInfoAsicSoft(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_calcTesInfoAsicSoft(u8* data);
    blt_calcTesInfoAsicSoft(fp);
}
_attribute_ram_code_ void tlk_mailbox_d25f_compressTesInfo(u8* data)
{
    (void) data;
    u8 *fp = (u8 *)tlk_sm_get_cs_raw_buff_addr();
    extern void blt_compressTesInfo(u8* data);
    blt_compressTesInfo(fp);
}

_attribute_ram_code_ void tlk_mailbox_service_init(void)
{
    mailbox_init(tlk_mailbox_receive_process);//register general mailbox receive entry.

    #if(TLK_MESSAGE_N22 == 1)
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_D25F_TO_N22_HCI_TX_ADDRESS,tlk_mailbox_n22_set_hci_tx_fifo);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_D25F_TO_N22_HCI_RX_ADDRESS,tlk_mailbox_n22_set_hci_rx_fifo);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_D25F_TO_N22_SYNC_TX_ADDRESS,tlk_mailbox_n22_set_sync_tx_fifo);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_D25F_TO_N22_LOG_TX_ADDRESS,tlk_mailbox_n22_set_log_tx_fifo);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_D25F_TO_N22_FLASH_WR_ADDRESS,tlk_mailbox_n22_set_falsh_wr_fifo);
    n22FlashWRFifo = NULL;
    mailbox_set_irq_mask_n22();
    clic_interrupt_vector_en(IRQ_IRQ_MAILBOX_D25_TO_N22);
    clic_interrupt_enable(IRQ_IRQ_MAILBOX_D25_TO_N22);
    #elif(TLK_MESSAGE_D25F == 1)
    extern  u8 malbox_status;
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_SYNC_DATA_READY,tlk_mailbox_d25f_sync_data_process);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_TEST,tlk_mailbox_d25f_test_process);
    /* Channel Sounding */
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_ADDRESS,tlk_mailbox_d25f_set_cs_raw_pct_fifo);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_RAW_PCT_DATA_READY,tlk_mailbox_d25f_set_cs_raw_pct_data_ready);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_FPU_CALC_TRIGGER,tlk_mailbox_d25f_cs_fpu_calc);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_FREQ_TRIGGER,tlk_mailbox_d25f_calcFreq);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_PES_COLLECT_DATA_INIT_SDK_TRIGGER,tlk_mailbox_d25f_pesCollectDataInitSDK);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_FINE_TRIGGER,tlk_mailbox_d25f_calcPesInfoFine);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_SDK_TRIGGER,tlk_mailbox_d25f_calcPesInfoSDK);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_NADM_DETECT_TRIGGER,tlk_mailbox_d25f_cs_nadm_detect);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_HARD_FIX_TRIGGER,tlk_mailbox_d25f_calcTesInfoAsicHardFix);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_SOFT_TRIGGER,tlk_mailbox_d25f_calcTesInfoAsicSoft);
    tlk_mailbox_register_message_cb(TLK_MESSAGE_FROM_N22_TO_D25F_CS_COMPRESS_TEST_INFO_TRIGGER,tlk_mailbox_d25f_compressTesInfo);

    mailbox_set_irq_mask_d25f();
    plic_interrupt_enable(IRQ_MAILBOX_N22_TO_D25);
    core_interrupt_enable();
    malbox_status = 1;
    #endif
}

_attribute_ram_code_ void tlk_mailbox_service_loop(void)
{
    mailbox_loop();
}

