/********************************************************************************************************
 * @file    acl_async.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"
#include "acl_async_stack.h"
#include "acl_async.h"
#if (LL_ASYNC_LEA_EN)

    #define ASYNC_LEA_LOG_EN 0

async_ctrl_t asyncCtrl;

async_data_cb_t async_tx_cb = NULL;
async_data_cb_t async_rx_cb = NULL;

async_timer_shaft_t asyncOrderedBlock   = {0};
async_timer_shaft_t asyncUnorderedBlock = {0};

__attribute__((aligned(4))) u8 async_tx_fifo_b[ASYNC_TX_FIFO_SIZE * ASYNC_TX_FIFO_NUM];

__attribute__((aligned(4))) my_fifo_t async_tx_fifo = {ASYNC_TX_FIFO_SIZE, ASYNC_TX_FIFO_NUM, 0, 0, async_tx_fifo_b};

__attribute__((aligned(4))) u8 async_rx_fifo_b[ASYNC_RX_FIFO_SIZE * ASYNC_RX_FIFO_NUM];

__attribute__((aligned(4))) my_fifo_t async_rx_fifo = {ASYNC_RX_FIFO_SIZE, ASYNC_RX_FIFO_NUM, 0, 0, async_rx_fifo_b};

void blc_async_registerDataHandler(async_data_cb_t tx_cb, async_data_cb_t rx_cb)
{
    asyncCtrl.leaUsed         = 1;
    asyncCtrl.aclIntervalTick = 10000 * SYSTEM_TIMER_TICK_1US;
    async_tx_cb               = tx_cb;
    async_rx_cb               = rx_cb;
}

blc_async_sts_e blc_async_push_message(u32 syncTime, blc_async_message_t *pMessage)
{
    for (u8 index = 0; index < LL_MAX_ACL_CONN_NUM; index++) {
        if (blms[index].async_lea_link && asyncCtrl.connState) {
            u32              r = irq_disable();
            blc_async_sdu_t *pSdu =
                (blc_async_sdu_t *)(async_tx_fifo.p + (async_tx_fifo.wptr & (async_tx_fifo.num - 1)) * async_tx_fifo.size);
            pSdu->ackIndex     = 0;
            pSdu->eventCounter = blms[index].conn_inst_mark;
            pSdu->syncTick     = syncTime * SYSTEM_TIMER_TICK_1US;
            memcpy(&pSdu->message.type, (u8 *)pMessage, sizeof(blc_async_message_t));
            async_tx_fifo.wptr++;
            async_tx_fifo.wptr         = (async_tx_fifo.wptr & (async_tx_fifo.num - 1));
            u8               format[1] = {ASYNC_LEA_OP_UI_MSG};
            extern ble_sts_t blt_l2cap_pushData_2_controller(u16 connHandle, u16 cid, u8 * format, int format_len, u8 *pDate, int data_len);
            u8               ret = blt_l2cap_pushData_2_controller(blms[index].acl_conHandle, L2CAP_CID_NULL, format, 1, (u8 *)&pSdu->eventCounter, sizeof(blc_async_sdu_t));
            if (ret != BLE_SUCCESS) {
                tlkapi_printf(ASYNC_LEA_LOG_EN, "l2cap data push error\n", r);
                irq_restore(r);
                return BLC_ASYNC_ERROR_DATA;
            }
            irq_restore(r);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "async message push success\n", r);
            return BLC_ASYNC_SUCCESS;
        }
    }
    return BLC_ASYNC_ERROR_LINK;
}

void blt_l2cap_asyncLeaDataControl(u16 connHandle, rf_packet_l2cap_t *ptrAttr)
{
    if (blms[connHandle & CONN_IDX_MASK].async_lea_link) {
        if (ptrAttr->opcode == ASYNC_LEA_OP_GET_TIMING) {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###get timing info###\n");
            if (blt_async_sendTimingInfo() != BLC_ASYNC_SUCCESS) {
                asyncCtrl.flow = ASYNC_FLOW_PERIPHERAL_SEND_TIMING;
            }
        } else if (ptrAttr->opcode == ASYNC_LEA_OP_TIMING) {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###timing info process###\n");
            blt_async_timingInfoProcess(ptrAttr->data);
        } else {
            blc_async_rx_handler((u8 *)ptrAttr);
        }
    }
}

void blc_async_rx_handler(u8 *p)
{
    rf_packet_l2cap_t *pAsync = (rf_packet_l2cap_t *)p;
    if (pAsync->chanId != 0x00 && pAsync->opcode != ASYNC_LEA_OP_UI_MSG && pAsync->l2capLen != 21) {
        return;
    }
    blc_async_sdu_t *pSdu =
        (blc_async_sdu_t *)(async_rx_fifo.p + (async_rx_fifo.wptr & (async_rx_fifo.num - 1)) * async_rx_fifo.size);

    memcpy((u8 *)&pSdu->eventCounter, pAsync->data, sizeof(blc_async_sdu_t));
    async_rx_fifo.wptr++;
    async_rx_fifo.wptr = (async_rx_fifo.wptr & (async_rx_fifo.num - 1));
}

void blc_async_messageLoopProcess()
{
    if (async_tx_fifo.rptr != async_tx_fifo.wptr) {
        blc_async_sdu_t *pSdu =
            (blc_async_sdu_t *)(async_tx_fifo.p + (async_tx_fifo.rptr & (async_tx_fifo.num - 1)) * async_tx_fifo.size);
        for (u8 index = 0; index < LL_MAX_ACL_CONN_NUM; index++) {
            if (blms[index].async_lea_link) {
                u32 r        = irq_disable();
                u32 pastTick = (blms[index].conn_inst_mark - pSdu->eventCounter) * blms[index].conn_intvl_n_1m25 * 1250 * SYSTEM_TIMER_TICK_1US;
                if (pastTick < pSdu->syncTick && async_tx_cb) {
                    if (pSdu->ackIndex > 1) {
                        u32 expectTick = blms[index].ap_tick_mark + pSdu->syncTick -
                                         (blms[index].conn_inst_mark - pSdu->eventCounter) * blms[index].conn_intvl_n_1m25 * 1250 * SYSTEM_TIMER_TICK_1US;
                        async_tx_cb(expectTick, &pSdu->message);
                        pSdu->ackIndex = 0;
                        async_tx_fifo.rptr++;
                        async_tx_fifo.rptr = (async_tx_fifo.rptr & (async_tx_fifo.num - 1));
                    }
                } else {
                    if (pastTick > pSdu->syncTick) {
                        tlkapi_printf(ASYNC_LEA_LOG_EN, "tx time exceed,pastTick[%d],pSdu->syncTick[%d]\n", pastTick, pSdu->syncTick);
                    }
                    async_tx_fifo.rptr++;
                    async_tx_fifo.rptr = (async_tx_fifo.rptr & (async_tx_fifo.num - 1));
                }
                irq_restore(r);
                break;
            }
        }
    }
    if (async_rx_fifo.rptr != async_rx_fifo.wptr) {
        blc_async_sdu_t *pSdu =
            (blc_async_sdu_t *)(async_rx_fifo.p + (async_rx_fifo.rptr & (async_rx_fifo.num - 1)) * async_rx_fifo.size);
        for (u8 index = 0; index < LL_MAX_ACL_CONN_NUM; index++) {
            if (blms[index].async_lea_link) {
                u32 r        = irq_disable();
                u32 pastTick = (blms[index].conn_inst_mark - pSdu->eventCounter) * blms[index].conn_intvl_n_1m25 * 1250 * SYSTEM_TIMER_TICK_1US;
                if (pastTick < pSdu->syncTick && async_rx_cb) {
                    if (pSdu->ackIndex > 1) {
                        u32 expectTick = blms[index].ap_tick_mark + pSdu->syncTick -
                                         (blms[index].conn_inst_mark - pSdu->eventCounter) * blms[index].conn_intvl_n_1m25 * 1250 * SYSTEM_TIMER_TICK_1US -
                                         30 * SYSTEM_TIMER_TICK_1US;
                        async_rx_cb(expectTick, &pSdu->message);
                        pSdu->ackIndex = 0;
                        async_rx_fifo.rptr++;
                        async_rx_fifo.rptr = (async_rx_fifo.rptr & (async_rx_fifo.num - 1));
                    }
                } else {
                    if (pastTick > pSdu->syncTick) {
                        tlkapi_printf(ASYNC_LEA_LOG_EN, "rx time exceed,pastTick[%d],pSdu->syncTick[%d]\n", pastTick, pSdu->syncTick);
                    }
                    async_rx_fifo.rptr++;
                    async_rx_fifo.rptr = (async_rx_fifo.rptr & (async_rx_fifo.num - 1));
                }
                irq_restore(r);
                break;
            }
        }
    }
}

void blc_async_timingLoopProcess(void)
{
    if (!asyncCtrl.connState) {
        return;
    }

    if (asyncCtrl.flow == ASYNC_FLOW_PERIPHERAL_SEND_TIMING) {
        DBG_TIANXIANG_CHN2_HIGH;
        if ((asyncCtrl.cmdTick) && (!clock_time_exceed(asyncCtrl.cmdTick, (asyncCtrl.cmdInstant * asyncCtrl.aclIntervalTick) / SYSTEM_TIMER_TICK_1US))) {
            return;
        }
        asyncCtrl.cmdTick = 0;
        if (blt_async_sendTimingInfo() == BLC_ASYNC_SUCCESS) {
            DBG_TIANXIANG_CHN2_LOW;
            asyncCtrl.flow = ASYNC_FLOW_SEND_NONE;
        }
    } else if (asyncCtrl.flow == ASYNC_FLOW_CENTRAL_PARAM_UPDATE) {
        DBG_TIANXIANG_CHN2_HIGH;
        if ((asyncCtrl.cmdTick) && (!clock_time_exceed(asyncCtrl.cmdTick, (asyncCtrl.cmdInstant * asyncCtrl.aclIntervalTick) / SYSTEM_TIMER_TICK_1US))) {
            return;
        }
        asyncCtrl.cmdTick = 0;
        if (blt_async_calWindowOffset() == BLC_ASYNC_SUCCESS) {
            DBG_TIANXIANG_CHN2_LOW;
            asyncCtrl.flow = ASYNC_FLOW_SEND_NONE;
        }
    }
}

void blc_async_loopProcess(void)
{
    blc_async_messageLoopProcess();
    blc_async_timingLoopProcess();
}

void blt_async_savePeerTimingInfo(u32 T1, u32 T2, u32 T3, u32 T4, u32 T5)
{
    asyncCtrl.windowStartTick       = T1;
    asyncCtrl.asyncApTick           = T2;
    asyncCtrl.peerAclApTick         = T3;
    asyncCtrl.peerCisAPTick         = T4;
    asyncCtrl.peerCisSyncDelayTick  = T5;
    asyncCtrl.windowSizeOffsetBslot = 0;
}

void blt_async_timingInfoProcess(u8 *data)
{
    async_timing_info_t *timingInfo = (async_timing_info_t *)data;
    asyncCtrl.peerAclApTick         = timingInfo->aclAPTick;
    asyncCtrl.peerCisAPTick         = timingInfo->cisAPTick;
    asyncCtrl.peerCisSyncDelayTick  = timingInfo->cisSyncDelayTick;
    if (timingInfo->isoIntervalUs) {
        asyncCtrl.isoIntervalUs = timingInfo->isoIntervalUs;
    }
    asyncCtrl.flow = ASYNC_FLOW_CENTRAL_PARAM_UPDATE;
}

blc_async_sts_e blc_async_calAsyncApPoint(u16 num)
{
    asyncCtrl.asyncApTick = 0;
    for (u8 index = 0; index < LL_MAX_ACL_CONN_NUM; index++) {
        if (blms[index].async_lea_link) {
            u32 r = irq_disable();
            if (blms[index].connState != CONN_STATUS_ESTABLISH || blms[index].conn_inst_mark < 3 || (blms[index].conn_update_union.update_mark & CONN_UPDATE_CMD)) {
                irq_restore(r);
                return BLC_ASYNC_ERROR_STATE;
            }
            asyncCtrl.asyncApTick = blms[index].ap_tick_mark + num * blms[index].conn_intvl_tick;
            if (blms[index].conn_inst_mark % 2) {
                asyncCtrl.asyncApTick += blms[index].conn_intvl_tick;
            }
            if (asyncCtrl.isoIntervalUs && (asyncCtrl.isoIntervalUs * SYSTEM_TIMER_TICK_1US) != asyncCtrl.aclIntervalTick) {
                asyncCtrl.aclIntervalTick = asyncCtrl.isoIntervalUs * SYSTEM_TIMER_TICK_1US;
                asyncCtrl.updateIndex     = 1;
            }
            irq_restore(r);
            return BLC_ASYNC_SUCCESS;
        }
    }
    return BLC_ASYNC_SUCCESS;
}

void blc_async_calOwnCisApPoint(void)
{
    asyncCtrl.ownCisAPTick = 0;
    for (u8 index = bltCisMng.maxNum_cisMaster; index < bltCisMng.maxNum_cisConn; index++) {
        extern ll_cis_conn_t *global_pCisConn;
        ll_cis_conn_t        *pCisConn = (ll_cis_conn_t *)(global_pCisConn + index);

        if (pCisConn->cis_established) {
            u32 r                         = irq_disable();
            asyncCtrl.ownCisSyncDelayTick = pCisConn->cis_sync_delay * SYSTEM_TIMER_TICK_1US;
            asyncCtrl.isoIntervalUs       = pCisConn->iso_intvl_us;
            u32 ownCisApTick              = pCisConn->cis_expect_tick;
            while (ownCisApTick < asyncCtrl.asyncApTick) {
                ownCisApTick += pCisConn->iso_intvl_tick;
            }
            asyncCtrl.ownCisAPTick = (ownCisApTick - asyncCtrl.asyncApTick) % (asyncCtrl.aclIntervalTick);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "iso interval us %d\n", pCisConn->iso_intvl_tick / SYSTEM_TIMER_TICK_1US);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "asyncCtrl.ownCisAPTick %x\n", asyncCtrl.ownCisAPTick);
            irq_restore(r);
            return;
        }
    }
}

blc_async_sts_e blc_async_calOwnAclApPoint(void)
{
    asyncCtrl.ownAclApTick = 0;
    for (u8 index = 0; index < LL_MAX_ACL_CONN_NUM; index++) {
        if (blms[index].connState == CONN_STATUS_ESTABLISH && !blms[index].async_lea_link) {
            u32 r = irq_disable();
            if (blms[index].conn_update_union.update_mark & CONN_UPDATE_CMD) {
                irq_restore(r);
                return BLC_ASYNC_ERROR_STATE;
            }
            u32 ownAclApTick = blms[index].ap_tick_mark;
            while (ownAclApTick < asyncCtrl.asyncApTick) {
                ownAclApTick += blms[index].conn_intvl_tick;
            }
            asyncCtrl.ownAclApTick = (ownAclApTick - asyncCtrl.asyncApTick) % asyncCtrl.aclIntervalTick;
            irq_restore(r);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "own Acl Ap Tick %d\n", asyncCtrl.ownAclApTick);
            return BLC_ASYNC_SUCCESS;
        }
    }
    return BLC_ASYNC_SUCCESS;
}

void blt_async_cisConnCallback(u32 ownCisSyncDlyUs, u32 isoIntervalUs)
{
    asyncCtrl.ownCisSyncDelayTick = ownCisSyncDlyUs * SYSTEM_TIMER_TICK_1US;
    asyncCtrl.isoIntervalUs       = isoIntervalUs;
    if (asyncCtrl.connHandle & BLM_CONN_HANDLE) {
        asyncCtrl.flow       = ASYNC_FLOW_CENTRAL_PARAM_UPDATE;
        asyncCtrl.cmdTick    = clock_time();
        asyncCtrl.cmdInstant = 2;
    } else if (asyncCtrl.connHandle & BLS_CONN_HANDLE) {
        asyncCtrl.flow       = ASYNC_FLOW_PERIPHERAL_SEND_TIMING;
        asyncCtrl.cmdTick    = clock_time();
        asyncCtrl.cmdInstant = 2;
    }
}

void blt_async_connStateCallback(u16 connHandle, u8 connState)
{
    if (IS_ASYNC_LEA_LINK(connHandle)) {
        if (connState) {
            asyncCtrl.connState  = 1;
            asyncCtrl.connHandle = connHandle;
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###async connected###\n");
            if (connHandle & BLS_CONN_HANDLE) {
                asyncCtrl.flow       = ASYNC_FLOW_PERIPHERAL_SEND_TIMING;
                asyncCtrl.cmdTick    = clock_time();
                asyncCtrl.cmdInstant = 2;
            }
        } else {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###async disconnected###\n");
            asyncCtrl.connState = 0;
            asyncCtrl.flow      = ASYNC_FLOW_SEND_NONE;
        }
    } else {
        if (connState) {
            if (asyncCtrl.connHandle & BLM_CONN_HANDLE) {
                asyncCtrl.flow       = ASYNC_FLOW_CENTRAL_PARAM_UPDATE;
                asyncCtrl.cmdTick    = clock_time();
                asyncCtrl.cmdInstant = 5;
            } else if (asyncCtrl.connHandle & BLS_CONN_HANDLE) {
                asyncCtrl.flow       = ASYNC_FLOW_PERIPHERAL_SEND_TIMING;
                asyncCtrl.cmdTick    = clock_time();
                asyncCtrl.cmdInstant = 5;
            }
        }
    }
}

void blt_async_connUpdateCallback(u16 connHandle)
{
    if (IS_ASYNC_LEA_LINK(connHandle)) {
        if (asyncCtrl.connHandle & BLM_CONN_HANDLE) {
            if (asyncCtrl.peerAclApTick) {
                asyncCtrl.peerAclApTick = (asyncCtrl.peerAclApTick + asyncCtrl.aclIntervalTick - asyncCtrl.windowOffsetBslot * SYSTEM_TIMER_TICK_625US) % asyncCtrl.aclIntervalTick;
            }
            if (asyncCtrl.peerCisAPTick) {
                asyncCtrl.peerCisAPTick = (asyncCtrl.peerCisAPTick + asyncCtrl.aclIntervalTick - asyncCtrl.windowOffsetBslot * SYSTEM_TIMER_TICK_625US) % asyncCtrl.aclIntervalTick;
            }
        }
    } else {
        if (asyncCtrl.connHandle & BLM_CONN_HANDLE) {
            asyncCtrl.flow       = ASYNC_FLOW_CENTRAL_PARAM_UPDATE;
            asyncCtrl.cmdTick    = clock_time();
            asyncCtrl.cmdInstant = 5;
        } else if (asyncCtrl.connHandle & BLS_CONN_HANDLE) {
            asyncCtrl.flow       = ASYNC_FLOW_PERIPHERAL_SEND_TIMING;
            asyncCtrl.cmdTick    = clock_time();
            asyncCtrl.cmdInstant = 5;
        }
    }
}

void blt_async_orderedBlockAdd(u32 startTick, u32 endTick)
{
    if (!asyncOrderedBlock.block[asyncOrderedBlock.conuts].blockValid) {
        asyncOrderedBlock.block[asyncOrderedBlock.conuts].start_us   = startTick;
        asyncOrderedBlock.block[asyncOrderedBlock.conuts].end_us     = endTick;
        asyncOrderedBlock.block[asyncOrderedBlock.conuts].blockValid = 1;
        asyncOrderedBlock.conuts++;
    }
}

void blt_async_unorderedBlockAdd(u32 startTick, u32 endTick)
{
    if (!asyncUnorderedBlock.block[asyncUnorderedBlock.conuts].blockValid) {
        asyncUnorderedBlock.block[asyncUnorderedBlock.conuts].start_us   = startTick;
        asyncUnorderedBlock.block[asyncUnorderedBlock.conuts].end_us     = endTick;
        asyncUnorderedBlock.block[asyncUnorderedBlock.conuts].blockValid = 1;
        asyncUnorderedBlock.conuts++;
    }
}

void blt_async_clearOrderedTimerShaft(void)
{
    memset((u8 *)&asyncOrderedBlock, 0, sizeof(async_timer_shaft_t));
}

void blt_async_clearUnorderedTimerShaft(void)
{
    memset((u8 *)&asyncUnorderedBlock, 0, sizeof(async_timer_shaft_t));
}

void blt_async_creatUnorderedTimerShaft(void)
{
    blt_async_clearUnorderedTimerShaft();
    if (asyncCtrl.ownAclApTick) {
        u32 ownAclStartUs = asyncCtrl.ownAclApTick / SYSTEM_TIMER_TICK_1US;
        u32 ownAclEndUs   = ownAclStartUs + 1250;
        blt_async_unorderedBlockAdd(ownAclStartUs, ownAclEndUs);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "ownAclStartUs %d\n", ownAclStartUs);
    }
    if (asyncCtrl.ownCisAPTick) {
        u32 ownCisStartUs = asyncCtrl.ownCisAPTick / SYSTEM_TIMER_TICK_1US;
        u32 ownCisEndUs   = ownCisStartUs + asyncCtrl.ownCisSyncDelayTick / SYSTEM_TIMER_TICK_1US;
        blt_async_unorderedBlockAdd(ownCisStartUs, ownCisEndUs);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "ownCisStartUs %d\n", ownCisStartUs);
    }
    if (asyncCtrl.peerAclApTick) {
        u32 peerAclStartUs = asyncCtrl.peerAclApTick / SYSTEM_TIMER_TICK_1US;
        u32 peerAclEndUs   = peerAclStartUs + 1250;
        blt_async_unorderedBlockAdd(peerAclStartUs, peerAclEndUs);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "peerAclStartUs %d\n", peerAclStartUs);
    }
    if (asyncCtrl.peerCisAPTick) {
        u32 peerCisStartUs = asyncCtrl.peerCisAPTick / SYSTEM_TIMER_TICK_1US;
        u32 peerCisEndUs   = peerCisStartUs + (asyncCtrl.peerCisSyncDelayTick / SYSTEM_TIMER_TICK_1US);
        blt_async_unorderedBlockAdd(peerCisStartUs, peerCisEndUs);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "peerCisStartUs %d\n", peerCisStartUs);
    }
}

void blt_async_calTimingGapPosition(void)
{
    //clear the ordered block
    blt_async_clearOrderedTimerShaft();

    u8 i = 0;
    u8 j = 0;

    u32 end_us          = 0;
    u32 start_us        = 0;
    u32 timerGap        = 0;
    u32 authorPoint     = 0;
    u32 asyncIntervalUs = asyncCtrl.aclIntervalTick / SYSTEM_TIMER_TICK_1US;
    //first step,determine if the end of the timer shaft exceed the interval,
    //if it is,featch the exceed part as a new timer block
    for (i = 0; i < asyncUnorderedBlock.conuts; i++) {
        if (end_us < asyncUnorderedBlock.block[i].end_us) {
            end_us = asyncUnorderedBlock.block[i].end_us;
        }
    }

    if (end_us > asyncIntervalUs) {
        u32 boundary_us = end_us - asyncIntervalUs;
        tlkapi_printf(ASYNC_LEA_LOG_EN, "end_us %d\n", end_us);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "boundary_us %d\n", boundary_us);
        if (boundary_us > asyncIntervalUs) {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###error,async sequential complete coverage###\n");
            return; //error;
        }
        blt_async_unorderedBlockAdd(0, boundary_us);
    }

    //second step,sort all blocks by start time in unordered timer shaft.
    async_timer_block_t block_temp = {0};
    for (i = 0; i < asyncUnorderedBlock.conuts; i++) {
        for (j = 0; j < asyncUnorderedBlock.conuts - i - 1; j++) {
            if (asyncUnorderedBlock.block[j].start_us > asyncUnorderedBlock.block[j + 1].start_us) {
                block_temp.start_us                       = asyncUnorderedBlock.block[j].start_us;
                block_temp.end_us                         = asyncUnorderedBlock.block[j].end_us;
                asyncUnorderedBlock.block[j].start_us     = asyncUnorderedBlock.block[j + 1].start_us;
                asyncUnorderedBlock.block[j].end_us       = asyncUnorderedBlock.block[j + 1].end_us;
                asyncUnorderedBlock.block[j + 1].start_us = block_temp.start_us;
                asyncUnorderedBlock.block[j + 1].end_us   = block_temp.end_us;
            }
        }
    }
    tlkapi_printf(ASYNC_LEA_LOG_EN, "ordered shift sort\n");
    for (i = 0; i < asyncOrderedBlock.conuts; i++) {
        if (asyncOrderedBlock.block[i].blockValid) {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "i [%d]\n", i);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "start_us [%d]\n", asyncUnorderedBlock.block[i].start_us);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "end_us [%d]\n", asyncUnorderedBlock.block[i].end_us);
        }
    }

    //third step,merge overlap timer blocks in unordered timer shaft,delete the overlapped timer blocks.
    u8 counts = asyncUnorderedBlock.conuts;
    for (i = 0; i < counts; i++) {
        if (asyncUnorderedBlock.block[i].blockValid) {
            for (j = i + 1; j < counts; j++) {
                if (asyncUnorderedBlock.block[j].blockValid) {
                    if (asyncUnorderedBlock.block[i].end_us >= asyncUnorderedBlock.block[j].start_us) {
                        asyncUnorderedBlock.block[j].blockValid = 0;
                        if (asyncUnorderedBlock.block[i].end_us < asyncUnorderedBlock.block[j].end_us) {
                            asyncUnorderedBlock.block[i].end_us = asyncUnorderedBlock.block[j].end_us;
                        }
                    }
                }
            }
        }
    }
    //fourth step,add all blocks to ordered timer shaft
    for (i = 0; i < counts; i++) {
        if (asyncUnorderedBlock.block[i].blockValid) {
            blt_async_orderedBlockAdd(asyncUnorderedBlock.block[i].start_us, asyncUnorderedBlock.block[i].end_us);
        }
    }

    tlkapi_printf(ASYNC_LEA_LOG_EN, "merged shift\n");
    for (i = 0; i < asyncOrderedBlock.conuts; i++) {
        if (asyncOrderedBlock.block[i].blockValid) {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "i [%d]\n", i);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "start_us [%d]\n", asyncUnorderedBlock.block[i].start_us);
            tlkapi_printf(ASYNC_LEA_LOG_EN, "end_us [%d]\n", asyncUnorderedBlock.block[i].end_us);
        }
    }

    typedef struct
    {
        u32 gapWidth;
        u32 gapPosition;
    } gap_type_t;

    gap_type_t gap[6]    = {0};
    u8         gapCounts = 0;
    //fifth step,calculate the time gap of all timer blocks
    //find the total start and end of all timer blocks
    start_us = asyncOrderedBlock.block[0].start_us;
    end_us   = asyncOrderedBlock.block[0].end_us;
    for (i = 0; i < asyncOrderedBlock.conuts; i++) {
        if (asyncOrderedBlock.block[i].blockValid) {
            end_us = asyncOrderedBlock.block[i].end_us;
        }
    }

    //if start timer gap valid
    if (start_us > 1500) {
        timerGap                   = start_us;
        authorPoint                = 0;
        gap[gapCounts].gapWidth    = start_us;
        gap[gapCounts].gapPosition = 0;
        gapCounts++;
    }
    //if end timer gap valid
    if (asyncIntervalUs > end_us) {
        if ((asyncIntervalUs - end_us) > 1500) {
            timerGap                   = asyncIntervalUs - end_us;
            authorPoint                = end_us;
            gap[gapCounts].gapWidth    = timerGap;
            gap[gapCounts].gapPosition = end_us;
            gapCounts++;
        }
    }
    //if middle timer gap valid
    if (asyncOrderedBlock.conuts > 1) {
        for (i = 0; i < asyncOrderedBlock.conuts - 1; i++) {
            if ((asyncOrderedBlock.block[i + 1].start_us - asyncOrderedBlock.block[i].end_us) > 1500) {
                timerGap                   = asyncOrderedBlock.block[i + 1].start_us - asyncOrderedBlock.block[i].end_us;
                authorPoint                = asyncOrderedBlock.block[i].end_us;
                gap[gapCounts].gapWidth    = timerGap;
                gap[gapCounts].gapPosition = asyncOrderedBlock.block[i].end_us;
                gapCounts++;
            }
        }
    }
    u32 gapMax      = 0;
    u32 gapPosition = 0;
    for (u8 i = 0; i < gapCounts; i++) {
        if (gapMax < gap[i].gapWidth) {
            gapMax      = gap[i].gapWidth;
            gapPosition = gap[i].gapPosition;
        }
        tlkapi_printf(ASYNC_LEA_LOG_EN, "gap[i].gapWidth %d\n", gap[i].gapWidth);
        tlkapi_printf(ASYNC_LEA_LOG_EN, "gap[i].gapPosition %d\n", gap[i].gapPosition);
    }
    asyncCtrl.gapPosTick = (gapPosition * SYSTEM_TIMER_TICK_1US);
    tlkapi_printf(ASYNC_LEA_LOG_EN, "asyncCtrl.gapPosTick %x\n", asyncCtrl.gapPosTick);
}

blc_async_sts_e blt_async_sendTimingInfo(void)
{
    if (!asyncCtrl.connState) {
        return BLC_ASYNC_ERROR_STATE;
    }
    if (blc_async_calAsyncApPoint(2) != BLC_ASYNC_SUCCESS) {
        return BLC_ASYNC_ERROR_STATE;
    }
    if (blc_async_calOwnAclApPoint() != BLC_ASYNC_SUCCESS) {
        return BLC_ASYNC_ERROR_STATE;
    }
    blc_async_calOwnCisApPoint();
    async_timing_info_t timingInfo = {0};
    timingInfo.aclAPTick           = asyncCtrl.ownAclApTick;
    timingInfo.cisAPTick           = asyncCtrl.ownCisAPTick;
    timingInfo.cisSyncDelayTick    = asyncCtrl.ownCisSyncDelayTick;
    timingInfo.isoIntervalUs       = asyncCtrl.isoIntervalUs;
    u32              r             = irq_disable();
    extern ble_sts_t blt_l2cap_pushData_2_controller(u16 connHandle, u16 cid, u8 * format, int format_len, u8 *pDate, int data_len);
    u8               format[1] = {ASYNC_LEA_OP_TIMING};
    u8               ret       = blt_l2cap_pushData_2_controller(BM_CLR(asyncCtrl.connHandle, BIT(4)), L2CAP_CID_NULL, format, 1, (u8 *)&timingInfo.aclAPTick, sizeof(async_timing_info_t));
    irq_restore(r);
    if (ret != BLE_SUCCESS) {
        return BLC_ASYNC_RESOURCE_INSUFFICIENT;

        tlkapi_printf(ASYNC_LEA_LOG_EN, "l2cap data push error-send timing\n", r);
    }

    return BLC_ASYNC_SUCCESS;
}

void blt_async_calWindowSizeOffset(void)
{
    blc_async_calOwnAclApPoint();
    blc_async_calOwnCisApPoint();
    blt_async_creatUnorderedTimerShaft();
    blt_async_calTimingGapPosition();
    u32 asyncAPTick = asyncCtrl.gapPosTick;
    while (1) {
        if (asyncCtrl.windowStartTick < asyncAPTick &&
            (asyncAPTick < (asyncCtrl.windowStartTick + (BLMS_WINSIZE + 1) * SYSTEM_TIMER_TICK_1250US))) {
            u32 tickRemain                  = asyncAPTick - asyncCtrl.windowStartTick;
            asyncCtrl.windowSizeOffsetBslot = tickRemain / SYSTEM_TIMER_TICK_625US + 1;
            break;
        }
        asyncAPTick = asyncAPTick + asyncCtrl.aclIntervalTick * SYSTEM_TIMER_TICK_1US;
    }
    tlkapi_printf(ASYNC_LEA_LOG_EN, "###asyncCtrl.windowSizeOffsetBslot %d\n###", asyncCtrl.windowSizeOffsetBslot);
}

blc_async_sts_e blt_async_calWindowOffset(void)
{
    u8  ifUpdate = 0;
    u32 asyncIntervalUs;
    if (!asyncCtrl.connState) {
        return BLC_ASYNC_ERROR_STATE;
    }
    if (blc_async_calAsyncApPoint(6) != BLC_ASYNC_SUCCESS) {
        return BLC_ASYNC_ERROR_STATE;
    }
    if (blc_async_calOwnAclApPoint() != BLC_ASYNC_SUCCESS) {
        return BLC_ASYNC_ERROR_STATE;
    }
    asyncCtrl.gapPosTick = 0;
    asyncIntervalUs      = asyncCtrl.aclIntervalTick / SYSTEM_TIMER_TICK_1US;
    blc_async_calOwnCisApPoint();
    blt_async_creatUnorderedTimerShaft();
    blt_async_calTimingGapPosition();
    asyncCtrl.windowOffsetBslot = 0;
    if (asyncCtrl.updateIndex) {
        asyncCtrl.updateIndex = 0;
        ifUpdate              = 1;
    }
    if (asyncCtrl.gapPosTick) {
        asyncCtrl.windowOffsetBslot = asyncCtrl.gapPosTick / SYSTEM_TIMER_TICK_625US + 1;
        if ((asyncCtrl.windowOffsetBslot * SYSTEM_TIMER_TICK_625US - asyncCtrl.gapPosTick) < 500 * SYSTEM_TIMER_TICK_1US) {
            asyncCtrl.windowOffsetBslot++;
        }
        ifUpdate = 1;
    }
    if (ifUpdate) {
        u32 ret = blc_ll_updateConnection(BM_CLR(asyncCtrl.connHandle, BIT(4)), CONN_INTERVAL_10MS, CONN_INTERVAL_60MS, 0, CONN_TIMEOUT_3S, 0, 0);
        if (ret != BLE_SUCCESS) {
            asyncCtrl.flow = ASYNC_FLOW_CENTRAL_PARAM_UPDATE;
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###update command error-ret %d\n###", ret);
        } else {
            tlkapi_printf(ASYNC_LEA_LOG_EN, "###update command send success - asyncCtrl.windowOffsetBslot %d\n###", asyncCtrl.windowOffsetBslot);
        }
    }
    return BLC_ASYNC_SUCCESS;
}

#endif
