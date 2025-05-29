/********************************************************************************************************
 * @file    csa2.c
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


#if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)


_attribute_ble_data_retention_ ll_chn_index_calc_callback_t ll_chn_index_calc_cb = NULL;


_attribute_ble_data_retention_ channel_algorithm_t local_chsel = CHANNEL_SELECTION_ALGORITHM_1;

static u8 csa2_calcSubEvent1Map(struct le_channel_map *map, u16 counter, u16 channelIdentifier);

void blc_ll_initChannelSelectionAlgorithm_2_feature(void)
{
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2 << 14);
    local_chsel = CHANNEL_SELECTION_ALGORITHM_2;

    ll_chn_index_calc_cb = csa2_calcSubEvent1Map;
}

/////////////////////////////////////// LE CSA2 //////////////////////////////////////////////
/*
 * @brief   CSA2:Permutation operation
 * @param   16bit input
 * @return  16bit output
 * @reference core 5.4 | Vol 6, Part B 4.5.8.3.2 Figure 4.51 page 2820
 */
#if (!STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#else
__INLINE
#endif
u32 csa2_permutation(u32 x)
{
    x = (((x & 0xaaaa) >> 1) | ((x & 0x5555) << 1));
    x = (((x & 0xcccc) >> 2) | ((x & 0x3333) << 2));
    x = (((x & 0xf0f0) >> 4) | ((x & 0x0f0f) << 4));
    return x;
}

    /*
 * @brief CSA2: Multiply, Add, and Modulo block operation.
 * @param: 16bit input.
 * @return  16bit output
 * @reference core 5.4 | Vol 6, Part B 4.5.8.3.2 Figure 4.52 page 2820
 */
    #define MAM(a, b) ((a) * 17 + b)

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif
static u32 csa2_permutation_and_MAM(u32 a, u32 b)
{
    a = csa2_permutation(a);
    return MAM(a, b);
}

/*
 *  @brief  CSA2: Event pseudo-random number generation
 *  @param  counter: The 16-bit input counter changes for each event.
 *                   For periodic advertising it is the event counter paEventCounter
 *                   For isochronous logical transports, it is bits 0 to 15 of the event
 *                   counter bigEventCounter or cisEventCounter.
 *          ch_id  : The 16-bit input channelIdentifier is fixed for any given connection or
 *                   periodic advertising; it is calculated from the Access Address by:
 *                   channelIdentifier = (Access Address31-16) XOR (Access Address15-0)
 *  @return prn_s. if want prn_e need prn_e = prn_s ^ channelIdentifier
 *  @reference core 5.4 | Vol 6, Part B 4.5.8.3 Figure 4.53 page 2821
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_only_
#endif
static u16 csa2_pseudoRandomNumberGenerate(u16 counter, u16 channelIdentifier)
{
    u32 prn_s = counter ^ channelIdentifier;

    prn_s = csa2_permutation_and_MAM(prn_s, channelIdentifier);
    prn_s = csa2_permutation_and_MAM(prn_s, channelIdentifier);
    prn_s = csa2_permutation_and_MAM(prn_s, channelIdentifier);

    return prn_s;
}

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_only_
#endif 
static u8 csa2_calcSubEvent1(struct le_channel_map* map, u16 prn_e, u8 *remappingIndexUsedChannel)
{
    /*
    * unmappedChannel is then calculated as prn_e modulo 37.
    * reference core 5.4 | Vol 6, Part B 4.5.8.3.4 Figure 4.54 page 2821
    */
    u8 unmappedChanel = prn_e % 37;

    /*
     * Event mapping to used channel index process.
     * first unmapped channel is used channel.
     * reference core 5.4 | Vol 6, Part B 4.5.8.3.4 Figure 4.55 page 2822
     */

    /*
    * remappingIndexOfLastUsedChannel = Index of UnmappedChannel in the remapping table
    */
    if (map->chmTbl[unmappedChanel >> 3] & BIT(unmappedChanel & 0x07)) {
        if (remappingIndexUsedChannel) {
            u8 left = 0, right = map->numUsedChn - 1;
            u8 mid;
            while (left <= right) {
                mid = (right + left) >> 1;
                if (map->rempChmTbl[mid] == unmappedChanel) {
                    break;
                } else if (unmappedChanel > map->rempChmTbl[mid]) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            *remappingIndexUsedChannel = mid;
            //          for(u8 idxOfUnmappedChn = 0; idxOfUnmappedChn < map->numUsedChn; idxOfUnmappedChn++){
            //              if(map->rempChmTbl[idxOfUnmappedChn] == channel_unmapped){
            //                  tlkapi_printf(0, "%d %d %d %s\n", idxOfUnmappedChn, channel_unmapped, map->rempChmTbl[idxOfUnmappedChn], hex_to_str(map->rempChmTbl, 37));
            //                  *remappingIndexUsedChannel = idxOfUnmappedChn;
            //                  break;
            //              }
            //          }
        }
        return unmappedChanel;
    }

    /*
    * If unmappedChannel is the index of an unused channel according to the channel
    * map, then the channel index for the event is calculated from prn_e and N (the
    * number of used channels) by first calculating the value remappingIndex as:
    */
    u8 remap_index = (map->numUsedChn * prn_e) >> 16;

    if (remappingIndexUsedChannel) {
        *remappingIndexUsedChannel = remap_index;
    }
    /*
     * Then using remappingIndex as an index into the remapping table to obtain
     * the channel index for the event.
     */
    return map->rempChmTbl[remap_index];
}

/*
 * @brief CSA2: generate ACL or Periodic advertising event, CIS or BIS sub event 1 channel index.
 * @param:  pChnParam: the channel map information.
 *          counter:    For ACL connection it is the connection event counter connEventCounter.
 *                      For periodic advertising it is the event counter paEventCounter.
 *                      For PAwR, it is the XOR of the two event counters paEventCounter and paSubEventCounter.
 *                      For isochronous logical transports, it is bits 0 to 15 of the event counter bigEventCounter or cisEventCounter.
 *          channelIdentifier: fixed for any given connection or periodic advertising train.
 *                      channelIdentifier = (Access Address[31-16]) ^ (Access Address[15-0])
 * @return  16bit output
 * @reference core 5.4 | Vol 6, Part B 4.5.8.3.2 Figure 4.52 page 2820
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_only_
#endif 
static u8 csa2_calcSubEvent1Map(struct le_channel_map* map, u16 counter, u16 channelIdentifier)
{
    u16 prn_e = csa2_pseudoRandomNumberGenerate(counter, channelIdentifier) ^ channelIdentifier;
    return csa2_calcSubEvent1(map, prn_e, NULL);
}

    #if (LL_FEATURE_ENABLE_ISO ||                                            \
         LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER || \
         LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
/*
 * @brief CSA2: generate next sub event channel index.
 * @param:  pChnParam: the channel map information.
 *          counter:    For ACL connection it is the connection event counter connEventCounter.
 *                      For periodic advertising it is the event counter paEventCounter.
 *                      For PAwR, it is the XOR of the two event counters paEventCounter and paSubEventCounter.
 *                      For isochronous logical transports, it is bits 0 to 15 of the event counter bigEventCounter or cisEventCounter.
 *          channelIdentifier: fixed for any given connection or periodic advertising train.
 *                      channelIdentifier = (Access Address[31-16]) ^ (Access Address[15-0])
 *          subEventNum: sub event number. 1 is start sub event.
 * @return  16bit output
 * @reference core 5.4 | Vol 6, Part B 4.5.8.3.2 Figure 4.52 page 2820
 * qihang.mou test in 2024/01/19 eagle chip, tick unit is 1/16us
 * running 32MHz: sub event 1: 128 tick, other: 48 tick.
 * running 48MHz: sub event 1: 88 tick, other: 32 tick.
 * running 96MHz: sub event 1: 48 tick, other: 16 tick.
 */
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif 
u8 blt_ll_generateNextChannel(struct csa2_param *pChnParam, u16 counter, u16 channelIdentifier, int subEventNum)
{
    if (subEventNum == 1) {
        pChnParam->prn_s = csa2_pseudoRandomNumberGenerate(counter, channelIdentifier);
        u16 prn_e        = pChnParam->prn_s ^ channelIdentifier;
        tlkapi_printf(0, "prn_e=%d\n", prn_e);
        return csa2_calcSubEvent1(&pChnParam->map, prn_e, &pChnParam->remappingIndexUsedChannel);
    } else {
        if (subEventNum == 2) {
            tlkapi_printf(0, "remappingIndexOfLastUsedChannel=%d\n", pChnParam->lastSubEventIndex);
        }

        u16 prnSubEvent_lu = csa2_permutation_and_MAM(pChnParam->lastPrnSubEvent_lu, channelIdentifier);

        u16 prnSubEvent_se = prnSubEvent_lu ^ channelIdentifier;

        int value = (prnSubEvent_se * (pChnParam->map.numUsedChn - 2 * pChnParam->map.d + 1)) >> 16;

        u8 subEventIndex = (pChnParam->lastSubEventIndex + pChnParam->map.d + value) % pChnParam->map.numUsedChn;

        tlkapi_printf(0, "prnSubEvent_se=%d, subEventIndex=%d\n", prnSubEvent_se, subEventIndex);

        pChnParam->lastPrnSubEvent_lu = prnSubEvent_lu;
        pChnParam->lastSubEventIndex  = subEventIndex;
        return pChnParam->map.rempChmTbl[subEventIndex];
    }
}

    /////////////////// LE CHANNEL SELECTION ALGORITHM #2 SAMPLE DATA FOR SUBEVENT ///////////////////////
        #if 0 //test CSA#2 for subEvent, sample data in core 5.4 vol 6 part C 3 LE CAS#2
//  unsigned char chnMap[5] = {0x00, 0x06, 0xE0, 0x00, 0x1E};
    unsigned char chnMap[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
    struct csa2_param pChnParam;
    memcpy(pChnParam.map.chmTbl, chnMap, 5);
    csa2_calculateMapInfo(&pChnParam.map);
    for(int i=1; i<=10; i++)
    {
        u32 last_time = clock_time();
        u8 map = blt_ll_generateNextChannel(&pChnParam, 6, 0x305F, i);
        u8 time_us = (clock_time()- last_time);
        tlkapi_printf(1, "i=%d, map is %d, calc time=%dus\n", i, map, time_us);
    }
        #endif
    #endif


    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    csa2_calculateMapInfo(struct le_channel_map *mapInfo)
{
    /*
     * A remapping table is built that contains all the used channels in ascending
     * order, indexed from zero.
     */
    mapInfo->numUsedChn = 0;
    foreach (k, 37) {
        if (mapInfo->chmTbl[k >> 3] & BIT(k & 0x07)) {
            mapInfo->rempChmTbl[mapInfo->numUsedChn++] = k;
        }
    }

    u8 N       = mapInfo->numUsedChn;
    mapInfo->d = max(1, max(min(3, N - 5), min(11, (N - 10) >> 1)));
}


#endif //end of  LL_FEATURE_ENABLE_ CHANNEL_SELECTION_ALGORITHM2
