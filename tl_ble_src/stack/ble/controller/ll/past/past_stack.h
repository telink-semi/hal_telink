/********************************************************************************************************
 * @file    past_stack.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef PAST_STACK_H_
#define PAST_STACK_H_


#include "stack/ble/ble_stack.h"

/*! \brief  PAST source. */
enum
{
    PAST_SYNC_SRC_SCAN, //Periodic sync info from ext_scanner
    PAST_SYNC_SRC_BCST, //Periodic sync info from ext_broadcaster
    PAST_SYNC_SRC_TOTAL
};

#define PAST_SYNC_MIN_TIMEOUT 0x000A /*!< Minimum synchronization timeout. */
#define PAST_SYNC_MAX_TIMEOUT 0x4000 /*!< Maximum synchronization timeout. */
#define PAST_MAX_SKIP         0x01F3 /*!< Maximum synchronization skip. */

typedef struct
{
    u8  pastMode;
    u8  cteType;
    u16 pastSkip;
    u16 pastSyncTimeout;
} ll_past_mng_t;

typedef struct
{
    u8  perSyncSrc; /*!< Periodic sync source. */
    u8  pastMode;
    u16 pastRcvdCEt; //Mark CEt

    u8   pastSyncCteType; //AOA/AOD concerned
    u8   past_occpied;
    bool pastRcvdSucc;
    u8   rsvd[1];

    u16 perServiceData; /*!< ID for periodic sync indication. */
    u16 perSyncHandle;  /*!< Periodic sync handle.(src:ext_scan, syncHandle /src:own_prd_bcst, advHandle) */
    u16 pastSkip;
    u16 pastSyncTimeout;

    u32 pastSendPending;
    u32 pastCreateSync; /*!< Create PeriodicAdv sync by receiving LL_PERIODIC_SYNC_IND packet method. */
    u32 pastRcvdTick;   //Special use

#if (1) /* dec special */
    u32 pastRcvdNo;
    u8 *pastDecPending;
    u8  pastTemBuf[48]; /* 39payload +2rf_header +4dma_header */
#endif

} ll_past_cb_t;

extern ll_past_mng_t blt_PastMng;

/* refer to BLE SPEC: Vol 6, Part B, 2.4.2.27 "LL_PERIODIC_SYNC_IND" for more information. */
typedef struct __attribute__((packed))
{
    u8          llid;
    u8          rf_len;
    u8          opcode;
    u16         id;
    sync_info_t syncInfo;
    u16         connEvtCnt;
    u16         lastPaEvtCnt;
    u8          sid   : 4;
    u8          aType : 1;
    u8          sca   : 3;
    u8          phy;
    u8          advA[6];
    u16         syncConnEvtCnt;
} rf_pkt_ll_periodic_sync_ind_t;

/////////////////// PAwR ///////////////////////
/* refer to BLE SPEC: Vol 6, Part B, 2.4.2.40 "LL_PERIODIC_SYNC_WR_IND" for more information. */
typedef struct __attribute__((packed))
{
    rf_pkt_ll_periodic_sync_ind_t pastInd;
    pawr_acad_t                   pawrAcadInfo;
} rf_pkt_ll_periodic_sync_wr_ind_t;

#endif /* PAST_STACK_H_ */
