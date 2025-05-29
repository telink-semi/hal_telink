

#include"shareMemory.h"

#if (MCU_CORE_TYPE == CHIP_TYPE_TL322X)

_attribute_ram_code_ tlk_sm_ret_e share_memory_data_push(tlk_sm_fifo_t* smFifo,tlk_sm_message_type_e type, u8 *data, u32 dataLen)
{
    if ((smFifo->fifo.rptr & (smFifo->fifo.num - 1)) == (((smFifo->fifo.wptr+ 1) & (smFifo->fifo.num - 1))))
    {
        return TLK_SHARE_MEMOTY_FULL;
    }
    if(smFifo == NULL)
    {
        return TLK_SHARE_MEMOTY_NOT_READY;
    }
    if (smFifo->status == TLK_SHARE_MEMOTY_STATUS_READY)
    {
        u8 *p      = smFifo->fifo.p + (smFifo->fifo.wptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
        u32 calLen = (dataLen > (smFifo->fifo.size - 8) ? (smFifo->fifo.size - 8) : dataLen);
        U32_TO_STREAM(p, type);
        U32_TO_STREAM(p, calLen);
        tmemcpy(p, data, calLen);
        smFifo->fifo.wptr++;
        return TLK_SHARE_MEMOTY_SUCCESS;
    }
    else
    {
        return TLK_SHARE_MEMOTY_NOT_READY;
    }
}

_attribute_ram_code_ int share_memory_data_pop(tlk_sm_fifo_t* smFifo)
{
    if ((smFifo != NULL) && (smFifo->status == TLK_SHARE_MEMOTY_STATUS_READY) && smFifo->fifo.wptr != smFifo->fifo.rptr)
    {
        u8 *p      = smFifo->fifo.p + (smFifo->fifo.rptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
        u32 type   = 0;
        u32 calLen = 0;
        BYTE_TO_UINT32(type, p);
        p += 4;
        BYTE_TO_UINT32(calLen, p);
        p += 4;
        if ((p[0]==0x04) && (p[1]==0x3e) && (p[3]==0x02)) {
            // adv report
        }
        else {
            tlkapi_send_string_data(1, "hci rx",p,calLen);
        }

        if((smFifo->rxCb[type]!=NULL)&&type<TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
        {
            smFifo->rxCb[type](p,calLen);
        }
        smFifo->fifo.rptr++;
        return 1;
    }
    return 0;
}

_attribute_ram_code_ int share_memory_data_popAll(tlk_sm_fifo_t* smFifo)
{
    if ((smFifo == NULL) || (smFifo->status != TLK_SHARE_MEMOTY_STATUS_READY)){
        return 0;
    }
    while(smFifo->fifo.wptr != smFifo->fifo.rptr)
    {
        u8 *p      = smFifo->fifo.p + (smFifo->fifo.rptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
        u32 type   = 0;
        u32 calLen = 0;
        BYTE_TO_UINT32(type, p);
        p += 4;
        BYTE_TO_UINT32(calLen, p);
        p += 4;
        if((smFifo->rxCb[type] != NULL) && type < TLK_SHARE_MEMOTY_MESSAGE_TYPE_MAX)
        {
            smFifo->rxCb[type](p,calLen);
        }
        smFifo->fifo.rptr++;
    }
    return 1;
}

_attribute_ram_code_ void share_memory_set_fifo_status(tlk_sm_fifo_t* smFifo,tlk_sm_status_e status)
{
    smFifo->status = status;
}

_attribute_ram_code_ void share_memory_fifo_init(tlk_sm_fifo_t* smFifo,u8 *p, u32 num, u32 size)
{
    smFifo->fifo.p    = p;
    smFifo->fifo.num  = num;
    smFifo->fifo.size = size;
    smFifo->fifo.rptr = smFifo->fifo.wptr = 0;
}

_attribute_ram_code_ void share_memory_register_fifo_receive_cb(tlk_sm_fifo_t* smFifo,tlk_sm_rx_cb_f cb[])
{
    for (u8 i = 0; i < SHARE_MEMORY_CB_NUM; i++) {
        smFifo->rxCb[i] = cb[i];
    }
}

#endif //#if(MCU_CORE_TYPE == MCU_CORE_TL751X)
