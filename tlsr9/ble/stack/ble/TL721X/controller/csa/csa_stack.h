/********************************************************************************************************
 * @file    csa_stack.h
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
#ifndef STACK_BLE_CSA_STACK_H_
#define STACK_BLE_CSA_STACK_H_


//See the Core_v5.0(Vol 6/Part B/4.5.8, "Data Channel Index Selection") for more information.
typedef enum {
    CHANNEL_SELECTION_ALGORITHM_1       =   0x00,
    CHANNEL_SELECTION_ALGORITHM_2       =   0x01,
} channel_algorithm_t;

/*
 * @brief Telink controller channel map must.
 * The struct must be 4-byte aligned.
 */
struct le_channel_map {
    u8 chmTbl[5];
    u8 numUsedChn;
    u8 rempChmTbl[37];  //can optimize to 37 later
    u8 d;       //calculated: d = max(1, max(min(3, N-5)), min(11, (N-10)/2)))
};

struct csa2_param{
    struct le_channel_map map;

    /* last Used prn(se_n) */
    union{
        u16 prn_s;  //se_n = 1;
        u16 lastPrnSubEvent_lu; //se_n > 1;
    };

    //index of last used Channel(se_n)
    union{
        u8 remappingIndexUsedChannel;   //se_n = 1;
        u8 lastSubEventIndex;       //se_n > 1;
    };
    u8 rsvd;
};


typedef u8 (*ll_chn_index_calc_callback_t)(struct le_channel_map* map, u16 counter, u16 channelIdentifier);

extern ll_chn_index_calc_callback_t ll_chn_index_calc_cb;
extern channel_algorithm_t local_chsel;


void blt_csa1_calculateChannelTable(u8* chm, u8 hop, u8 *ptbl);


void csa2_calculateMapInfo(struct le_channel_map *mapInfo);
u8 blt_csa2_calculateChannel_index(u8 chm[5], u16 event_cntr, u16 channel_id, u8 *remap_tbl, u8 channel_used_num);
u8 blt_ll_generateNextChannel(struct csa2_param *pChnParam, u16 counter, u16 channelIdentifier, int subEventNum);

#endif /* CSA_STACK_H_ */
