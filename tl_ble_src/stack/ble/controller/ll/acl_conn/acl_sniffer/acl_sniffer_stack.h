/********************************************************************************************************
 * @file    acl_sniffer_stack.h
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
#ifndef ACL_SNIFFER_STACK_H_
#define ACL_SNIFFER_STACK_H_


#include "stack/ble/ble_stack.h"
#include "stack/ble/controller/ll/ll_stack.h"
#include "stack/ble/controller/ll/acl_conn/acl_stack.h"




#if (LL_RSSI_SNIFFER_MODE_ENABLE)

/******************************* acl_sniffer common star ******************************************************************/
#define BLT_ACL_SNIFFER_MASTER_FLAG                 BIT(7)
#define BLT_ACL_SNIFFER_SLAVE_FLAG                  BIT(6)
#define BLT_ACL_SNIFFER_CHANNEL_MASK                0x3F


#ifndef DEBUG_SNIFFER_REPORT_INSTANT_EN
#define DEBUG_SNIFFER_REPORT_INSTANT_EN             0
#endif

#ifndef SNIFFER_USE_SOME_COMMON_APIS
#define SNIFFER_USE_SOME_COMMON_APIS                0
#endif

#ifndef DEBUG_SNIFFER_NULL_POINTER_EN
#define DEBUG_SNIFFER_NULL_POINTER_EN               0
#endif

#ifndef SNIFFER_RX_NULL_FIFO_SIZE
    #if (DEBUG_SNIFFER_REPORT_INSTANT_EN)
        #define SNIFFER_RX_NULL_FIFO_SIZE           5//raw_pkt[1]~raw_pkt[4]
    #else
        #define SNIFFER_RX_NULL_FIFO_SIZE           4//raw_pkt[1]~raw_pkt[3]
    #endif
#endif

#ifndef SNIFFER_RX_NULL_FIFO_NUM
#define SNIFFER_RX_NULL_FIFO_NUM                    8
#endif

#ifndef RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS
#define RSSI_SNIFFER_SEEK_TOLERANCE_MAX_MS          35
#endif

#define SNIFFER_SEEK_RX_POST_MARGIN_US              200//unit: uS


/**
 * @brief   struct for acl sniffer rx fifo
 */
typedef struct {
    u16     size;
    u8      num;
    u8      wptr;
    u8      rptr;
    u8*     p;
} sniffer_fifo_t;

extern u8 sniffer_rx_null_fifo_b[];
extern sniffer_fifo_t sniffer_rx_null_fifo;

_attribute_aligned_(4)
typedef struct __attribute__((packed)) {
    u16     sync_connHandle;
    u8      sync_CI_phy; //0~3bits: phy_tye: le_phy_type_t; 4~7bits: CI: le_coding_ind_t
    u8      sync_conn_chnSel;

    u32     sync_expectTime;

    u16     sync_conn_inst;
    u16     sync_intvl_n_1m25;//sync_connection_interval    unit: 1.25ms

    u32     sync_accessAddr;
    u32     sync_crcInit;
    u32     sync_conn_timeout;

    u8      sync_conn_chm[5];
    u8      sync_conn_hop;
    u8      sync_chn_idx;
    u8      sync_conn_num_config;
} acl_sniffer_sync_param_t; //ACL connection sniffer parameters for SYNC

_attribute_aligned_(4)
typedef struct __attribute__((packed)) {
    u8      tick_seek_tolerance_ms;
    u8      tick_seek_tolerance_max_ms;
    s16     sSlot_seek_tolerance;

    u32     tick_seek_interval;
    s32     sSlot_seek_interval;

    s32     sSlot_sync_expectTime;
    s32     sSlot_seek_expectTime;

    s16     sSlot_seek_duration;
    u16     jump_num_valid;

    u8      seek_count;
    u8      seek_state;
    u8      seeking;
    u8      seek_stop;

    u32     tick_seek_start;
    //32 Byte

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  snifferTsk_fifo;    //20 Byte

    struct le_channel_map chnParam;     //56 Byte
    u16     seek_chnIdentifier;
    u8      seek_repeat_type;
    u8      seek_repeat_count;

    acl_sniffer_sync_param_t    sync;   //32 Byte
} acl_sniffer_seek_param_t; //ACL connection sniffer parameters for seek anchor

_attribute_aligned_(4)
typedef struct __attribute__((packed)) {
    u8      sniffer_rssi_reportType;
    u8      sniffer_rssi_validFlag;
    u8      sniffer_1st_rxWindowMaxFlag;
    u8      sniffer_rx_num;

    u8      sniffer_seek_halfWindow_ms;
    u8      sniffer_seek_count_max;
    u16     sniffer_seek_timeout_ms;

    u32     sniffer_sync_earlyTime_add;
    u32     sniffer_seek_tick_1st_rx;
} acl_sniffer_param_t; //ACL sniffer common parameters

/**
 * @brief   acl status for compatible mode. It's same mapping of acl slave status and acl master status
 */
enum{
    //0x00
    ACL_STATUS_STANDBY          = 0,
    ACL_STATUS_CONNECTION_SETUP,
    ACL_STATUS_CONNECTION_GENERAL,
    ACL_STATUS_CONNECTION_UPDATE,
    ACL_STATUS_CONNECTION_CHANNEL_MAP,
};

extern void blt_ll_acl_sniffer_mainloop(void);
extern void blt_ll_acl_sniffer_sync_result(u16 syncHandle, u8 *p);
extern void blt_debug_gpio_toggle_acl_sniffer(void);
extern void blt_sniffer_stop_rx_window(int time_us);
extern void blt_ll_acl_everyConnEvent(u16 connHandle, st_ll_conn_t* pc);
extern void blt_ll_acl_chnMapUpdateEvent(u16 connHandle, st_ll_conn_t* pc);

#if (SNIFFER_USE_SOME_COMMON_APIS)
    extern int blt_sniffer_start(int conn_idx);
    extern int irq_acl_sniffer_rx(void);
    extern int blt_sniffer_post(void);

    extern int blt_ll_sniffer_seek_anchor(u8 *cmd);
    extern int blt_ll_buildAclSnifferSeekSchLinklist(u8 role);
    extern int blt_sniffer_seek_start(int seek_idx, u8 role);
    extern int blt_sniffer_seek_post(u8 role);
    extern int irq_acl_sniffer_seek_rx(void);
#endif
/******************************* acl_sniffer common end ******************************************************************/




/******************************* acl_sniffer master start ******************************************************************/
#if (LL_RSSI_SNIFFER_MASTER_ENABLE)

extern u16 acl_mst_connParamUpdateRsp_latency_max;

#if (SNIFFER_USE_SOME_COMMON_APIS)
    extern _attribute_aligned_(4) volatile acl_sniffer_param_t aclSniffer_mst_param;
    extern _attribute_aligned_(4) volatile acl_sniffer_seek_param_t aclSniffer_mst_seek[TSKNUM_SNIFM_SEEK];
#endif

extern void blt_ll_acl_sniffer_mst_mainloop(void);
extern int blc_ll_updateAclSnifferMstSync(u8 *cmd);

int blt_ll_sniffer_mst_seek_anchor(u8 *cmd);
int blt_ll_buildAclSnifferMstSeekSchLinklist(void);
int blt_sniffer_mst_seek_start(int seek_idx);
int blt_sniffer_mst_seek_post(void);
int irq_acl_sniffer_mst_seek_rx(void);
int blt_sniffer_mst_seek2sync(void);

#endif
/******************************* acl_sniffer master end ******************************************************************/



/******************************* acl_sniffer slave start ******************************************************************/
#if (LL_RSSI_SNIFFER_SLAVE_ENABLE)

#if (SNIFFER_USE_SOME_COMMON_APIS)
    extern _attribute_aligned_(4) volatile acl_sniffer_param_t aclSniffer_slv_param;
    extern _attribute_aligned_(4) volatile acl_sniffer_seek_param_t aclSniffer_slv_seek[TSKNUM_SNIFS_SEEK];
#endif

extern void blt_ll_acl_sniffer_slv_mainloop(void);
extern int blc_ll_updateAclSnifferSlvSync(u8 *cmd);

int blt_ll_sniffer_slv_seek_anchor(u8 *cmd);
int blt_ll_buildAclSnifferSlvSeekSchLinklist(void);
int blt_sniffer_slv_seek_start(int seek_idx);
int blt_sniffer_slv_seek_post(void);
int irq_acl_sniffer_slv_seek_rx(void);
int blt_sniffer_slv_seek2sync(void);

#endif
/******************************* acl_sniffer slave end ******************************************************************/

#endif




#endif /* ACL_SNIFFER_STACK_H_ */
