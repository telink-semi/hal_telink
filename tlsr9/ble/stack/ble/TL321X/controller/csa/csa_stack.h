/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
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
