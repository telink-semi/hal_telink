
#include"mailbox.h"
#include"../comm.h"
#include"drivers.h"
#include "../service/service_mailbox.h"
#if (MCU_CORE_TYPE == CHIP_TYPE_TL322X)

static mailbox_rx_cb_f mailboxRxCb;
static tlk_fifo1_t     mailboxFifo;
static u8              mailboxTxBuffer[8 * BTBLE_MAILBOX_TX_BUFFER_NUM];

_attribute_ram_code_ void mailbox_init(mailbox_rx_cb_f cb)
{
    mailboxFifo.p = mailboxTxBuffer;
    mailboxFifo.rptr = mailboxFifo.wptr = 0;
    mailboxFifo.num = BTBLE_MAILBOX_TX_BUFFER_NUM;
    mailboxFifo.size = 8;
    if(cb)
    {
        mailboxRxCb = cb;
    }
}

_attribute_ram_code_ void mailbox_send_data(u8* data)
{
    #if(TLK_MESSAGE_N22 == 1)
    if ((data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_FREQ_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_FINE_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_PES_INFO_SDK_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_NADM_DETECT_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_HARD_FIX_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_CALC_TES_INFO_ASIC_SOFT_TRIGGER) ||
        (data[0]==TLK_MESSAGE_FROM_N22_TO_D25F_CS_COMPRESS_TEST_INFO_TRIGGER))
    {
        while(mailbox_get_irq_status_d25f()){};
        mailbox_n22_set_d25f_msg((u32*)data);
    }
    else
    {
        if (mailbox_get_irq_status_d25f()) {
            u8* p      = mailboxFifo.p + (mailboxFifo.wptr & (mailboxFifo.num - 1)) * mailboxFifo.size;
            tmemcpy(p,data,8);
            mailboxFifo.wptr++;
        }
        else {
            mailbox_n22_set_d25f_msg((u32*)data);
        }
    }
    #elif(TLK_MESSAGE_D25F == 1)
    if(mailbox_get_irq_status_n22())
    {
//        tlkapi_send_string_data(1, "mailbox_send_data: n22 busy!!!",0,0);
        u8* p      = mailboxFifo.p + (mailboxFifo.wptr & (mailboxFifo.num - 1)) * mailboxFifo.size;
        tmemcpy(p,data,8);
        mailboxFifo.wptr++;
    }
    else
    {
//        tlkapi_send_string_data(1, "mailbox_send_data",data,8);
        mailbox_d25f_set_n22_msg((u32*)data);
    }
    #endif

}

_attribute_ram_code_ void mailbox_loop(void)
{
    if(mailboxFifo.wptr!=mailboxFifo.rptr)
    {
        u8* pData = mailboxFifo.p + (mailboxFifo.rptr & (mailboxFifo.num - 1)) * mailboxFifo.size;
        #if(TLK_MESSAGE_N22 == 1)
        if(mailbox_get_irq_status_d25f())
        {
            return;
        }
        else
        {
            mailbox_n22_set_d25f_msg((u32*)pData);
            mailboxFifo.rptr++;
        }
        #elif(TLK_MESSAGE_D25F == 1)
        if(mailbox_get_irq_status_n22())
        {
            return;
        }
        else
        {
            mailbox_d25f_set_n22_msg((u32*)pData);
            mailboxFifo.rptr++;
        }
        #endif
    }
}

#if(TLK_MESSAGE_N22 == 1)
_attribute_ram_code_sec_noinline_ void mailbox_d25_to_n22_irq_handler(void)
{
    if (mailbox_get_irq_status_n22()) {
        uint8_t msg[8] = {0};
        mailbox_n22_get_d25f_msg((u32*)msg);
       // tlkapi_send_string_data(1, "mailbox_d25_to_n22_irq_handler",msg,8);
        if(mailboxRxCb){
            mailboxRxCb(msg);
        }
    }
}
CLIC_ISR_REGISTER(mailbox_d25_to_n22_irq_handler, IRQ_IRQ_MAILBOX_D25_TO_N22)
#elif(TLK_MESSAGE_D25F == 1)
_attribute_ram_code_sec_noinline_ void mailbox_n22_to_d25_irq_handler(void)
{
    if (mailbox_get_irq_status_d25f()) {
        uint8_t msg[8] = {0};
        mailbox_d25f_get_n22_msg((u32*)msg);
        if(mailboxRxCb){
            mailboxRxCb(msg);
        }
    }
}
PLIC_ISR_REGISTER(mailbox_n22_to_d25_irq_handler, IRQ_MAILBOX_N22_TO_D25)
#endif

#endif
