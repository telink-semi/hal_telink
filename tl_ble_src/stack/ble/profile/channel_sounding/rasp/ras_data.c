/********************************************************************************************************
 * @file    ras_data.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "ras_internal.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"
#define REDEF_LOG_EN                                     (0)

#define CS_STEP_METADATA_LENGTH                          (sizeof(cs_step_value_t))                          // 3 //u8 mode, channel, len
#define CS_STEP_MAX_NUM_ANTENNA_PATHS                    4
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_BASE (sizeof(cs_step_mode2_t) + sizeof(cs_step_tone_t)) // 1 + 4 = 5
#define CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X    (sizeof(cs_step_tone_t))                           // = 4 => LENGTH = 5 + num_antenna_paths*4
#define CS_STEP_MAX_LENGTH                               ((sizeof(cs_step_mode1_t)) + (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_BASE + (CS_STEP_MAX_NUM_ANTENNA_PATHS * CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)))
#define RAS_FILTER_ANTENNA_PATH_OFFSET_MODE2             3                                                  //offset within mode2 filter field, to access antenna path filter settings
#define RAS_FILTER_ANTENNA_PATH_OFFSET_MODE3             10                                                 //offset within mode3 filter field, to access antenna path filter settings

//#define RAS_STEP_FILTER               1       //set in appconfig.h

#define AP1_POS0                                          0x00
#define AP2_POS0                                          0x01
#define AP3_POS0                                          0x02
#define AP4_POS0                                          0x03
#define AP1_POS1                                          0x00
#define AP2_POS1                                          0x04
#define AP3_POS1                                          0x08
#define AP4_POS1                                          0x0C
#define AP1_POS2                                          0x00
#define AP2_POS2                                          0x10
#define AP3_POS2                                          0x20
#define AP4_POS2                                          0x30
#define AP1_POS3                                          0x00
#define AP2_POS3                                          0x40
#define AP3_POS3                                          0x80
#define AP4_POS3                                          0xC0

#define API_MASK_POS0                                     0x03 //0b00000011

#define RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP1_2 0x00
#define RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP3   0x02
#define RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP4   0x08

/* antenna permutation index - needed for filter */
const u8 antennaPermutationIndexTable[32] = {
    //nAP == 2
    AP1_POS0 | AP2_POS1, //0
    AP2_POS0 | AP1_POS1, //1
    //nAP == 3
    AP1_POS0 | AP2_POS1 | AP3_POS2, //0
    AP2_POS0 | AP1_POS1 | AP3_POS2, //1
    AP1_POS0 | AP3_POS1 | AP2_POS2, //2
    AP3_POS0 | AP1_POS1 | AP2_POS2, //3
    AP3_POS0 | AP2_POS1 | AP1_POS2, //4
    AP2_POS0 | AP3_POS1 | AP1_POS2, //5
    //nAP == 4
    AP1_POS0 | AP2_POS1 | AP3_POS2 | AP4_POS3, //0
    AP2_POS0 | AP1_POS1 | AP3_POS2 | AP4_POS3, //1
    AP1_POS0 | AP3_POS1 | AP2_POS2 | AP4_POS3, //2
    AP3_POS0 | AP1_POS1 | AP2_POS2 | AP4_POS3, //3
    AP3_POS0 | AP2_POS1 | AP1_POS2 | AP4_POS3, //4
    AP2_POS0 | AP3_POS1 | AP1_POS2 | AP4_POS3, //5
    AP1_POS0 | AP2_POS1 | AP4_POS2 | AP3_POS3, //6
    AP2_POS0 | AP1_POS1 | AP4_POS2 | AP3_POS3, //7
    AP1_POS0 | AP4_POS1 | AP2_POS2 | AP3_POS3, //8
    AP4_POS0 | AP1_POS1 | AP2_POS2 | AP3_POS3, //9
    AP4_POS0 | AP2_POS1 | AP1_POS2 | AP3_POS3, //10
    AP2_POS0 | AP4_POS1 | AP1_POS2 | AP3_POS3, //11
    AP1_POS0 | AP4_POS1 | AP3_POS2 | AP2_POS3, //12
    AP4_POS0 | AP1_POS1 | AP3_POS2 | AP2_POS3, //13
    AP1_POS0 | AP3_POS1 | AP4_POS2 | AP2_POS3, //14
    AP3_POS0 | AP1_POS1 | AP4_POS2 | AP2_POS3, //15
    AP3_POS0 | AP4_POS1 | AP1_POS2 | AP2_POS3, //16
    AP4_POS0 | AP3_POS1 | AP1_POS2 | AP2_POS3, //17
    AP4_POS0 | AP2_POS1 | AP3_POS2 | AP1_POS3, //18
    AP2_POS0 | AP4_POS1 | AP3_POS2 | AP1_POS3, //19
    AP4_POS0 | AP3_POS1 | AP2_POS2 | AP1_POS3, //20
    AP3_POS0 | AP4_POS1 | AP2_POS2 | AP1_POS3, //21
    AP3_POS0 | AP2_POS1 | AP4_POS2 | AP1_POS3, //22
    AP2_POS0 | AP3_POS1 | AP4_POS2 | AP1_POS3, //23
};

// void blc_rass_prepareNextProcedureEntry(blt_ras_dataset_t* rasDataset);

static u16 __attribute__((unused)) blt_ras_getLocalIndexForRangingCounter(u16 connHandle, u16 rangingCounter);
static u16                         blt_ras_getLocalIndexForProcedureCounter(blt_ras_dataset_t *rasDataset, u16 procedureCounter);
static void                        blt_ras_clearProcedureData(blt_ras_proc_ctrl_t *procCtrl, bool freeMemory);
static void                        blt_ras_procedureHeaderFill(blc_rass_prot_head_t *procedureHead, u8 *startAddr);
static u8                         *blt_ras_stepDataProc(u8 *writePtr, u8 *srcPtr, u8 stepsNum);
static void                        blt_ras_mergeSubevtsToProcedure(blt_ras_proc_ctrl_t *procCtrl);

/* Compress - part only executed on RAS server */
#if (RAS_STEP_FILTER)
static u8 blt_rass_applyStepFilter(blt_ras_filter_t *f, u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 *readPtr, u8 *filteredStep);
#endif
static u8 *blt_rass_stepDataToProtocolData(blt_ras_filter_t *filter, u8 *writePtr, u8 **readPtr, u8 stepsNum, u8 subeventDoneStatus, u8 role, u8 rtt_type, u8 numAntennaPaths);
/* Decompress - part only executed on RAS client */
#if (RAS_STEP_FILTER)
static u8 blt_rasc_unpackStepFilter(blt_ras_filter_t *f, u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 *readPtr, u8 *unpackedStep);
#endif
static u8 *blt_rasc_stepDataToProcedureData(blt_ras_filter_t *filter, u8 *writePtr, u8 **readPtr, u8 **readPtrLocal, u8 stepsNum, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 noLocalProcedure_ForIOP);


#if (RAS_DEBUG_PRINTBUFFERS)
void log_buffer(const void *buff, u16 len)
{
    const uint8_t *b_p   = buff;
    u16            to_go = len;

    while (to_go != 0) {
        u16 chunk_len;

        chunk_len = min(32, to_go);
        BLC_RAS_DATA_LOG("%s", hex_to_str(b_p, chunk_len));
        debugwait();
        b_p += chunk_len;
        to_go -= chunk_len;
    }
}
#endif

blt_ras_dataset_t *rasAllDataset[(LL_MAX_ACL_CEN_NUM + LL_MAX_ACL_PER_NUM)];

blt_ras_dataset_t *blc_ras_getDataset(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);

    if (idx < 0) {
        BLC_RAS_DATA_LOG("Critical issue !! Invalid connHandle: %x", connHandle);
        debugwait();
        return NULL;
    }

    return rasAllDataset[idx];
}

void blc_ras_writeDataset(int index, blt_ras_dataset_t *pointer)
{
    rasAllDataset[index] = pointer;
}

ble_sts_t blc_rap_csConfigComplete(hci_le_csConfigCompleteEvt_t *pConfigComplete)
{
    u16 connHandle = pConfigComplete->Connection_Handle;

    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    u8 index = pConfigComplete->Config_ID;
    if (index < RAS_MAX_CS_CONFIG) {
        if (pConfigComplete->Status == 0) {
            rasDataset->config[index].valid   = TRUE;
            rasDataset->config[index].role    = pConfigComplete->Role;
            rasDataset->config[index].rttType = pConfigComplete->RTT_Type;
        } else { //TODO: doublecheck possible status values for hci_le_csConfigCompleteEvt_t
            rasDataset->config[index].valid = FALSE;
        }
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

//it can handle both protocol and procedure data pointer as input
u16 blc_ras_extractRangingCounter(u8 *dataPtr)
{
    if (!dataPtr) {
        goto failed;
    }
    blc_rass_prot_head_t *procedureHead = (blc_rass_prot_head_t *)(dataPtr);
    BLC_RAS_DATA_LOG("extractRangingCounter dataPtr: %x, rangCtr %x", dataPtr, procedureHead->data.procedureCounter);
    debugwait();

    return procedureHead->data.procedureCounter;
failed:
    return RAS_INVALID_INDEX_PROCEDURE;
}

blt_ras_filter_t *blt_ras_getFilter(u16 connHandle)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }
    return &rasDataset->filter;
failed:
    return NULL;
}

ble_sts_t blt_ras_initFilterDefault(blt_ras_dataset_t *rasDataset)
{
    blt_ras_filter_t *filter = &rasDataset->filter;
    if (!rasDataset) {
        goto failed;
    }
    filter->mode0.raw = 0xFFFF; //0x000F;
    filter->mode1.raw = 0xFFFF; //0x007F;
    filter->mode2.raw = 0xFFFF; //0x007F;
    filter->mode3.raw = 0xFFFF;
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_ras_setFilterMode(blt_ras_dataset_t *rasDataset, u8 mode, u16 filterValue)
{
    BLC_RAS_DATA_LOG("setFilter mode: %x, mask %x", mode, filterValue);
    debugwait();
    TTF_LOG("setFilter mode: %x, mask %x", mode, filterValue);
    debugwait();

    blt_ras_filter_t *filter = &rasDataset->filter;
    if (!rasDataset) {
        goto failed;
    }
    switch (mode) {
    case 0:
        filter->mode0.raw = filterValue;
        break;
    case 1:
        filter->mode1.raw = filterValue;
        break;
    case 2:
        filter->mode2.raw = filterValue;
        break;
    case 3:
        filter->mode3.raw = filterValue;
        break;
    default:
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        break;
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

u8 blc_ras_getStepLength(u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths)
{
    u8 ret = 0;

    switch (mode) {
    case 0:
        ret = (role) ? CS_STEP_DATA_LENGTH_MODE0_REFLECTOR : CS_STEP_DATA_LENGTH_MODE0_INITIATOR;
        return ret;
    case 1:
        ret = (rtt_type) ? CS_STEP_DATA_LENGTH_MODE1_RTT_SOUNDING : CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
        return ret;
    case 2:
        ret = CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_BASE + numAntennaPaths * CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X;
        return ret;
    case 3:
        ret = (rtt_type) ? CS_STEP_DATA_LENGTH_MODE1_RTT_SOUNDING : CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
        ret += CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_BASE + numAntennaPaths * CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X;
        return ret;
    default:
        BLC_RAS_DATA_LOG("getStepLength - Invalid mode !");
        return ret;
    }
    return ret;
}

#if (RAS_STEP_FILTER)
/* Compress - part only executed on RAS server */
static u8 blt_rass_applyStepFilter(blt_ras_filter_t *f, u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 *readPtr, u8 *filteredStep)
{
    u8 ret = 0; //length of the filtered field

    u8 apiTableOffset = 0;
    switch (numAntennaPaths) {
    case 3:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP3;
        break;
    case 4:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP4;
        break;
    case 1:
    case 2:
    default:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP1_2;
        break;
    }

    switch (mode) {
    case 0:
    {
        BLC_RAS_DATA_LOG("mode0: PQ%d, PR%d, PA%d, R%d, MFO%d, %x, %x", f->mode0.bit.PacketQuality, f->mode0.bit.PacketRssi, f->mode0.bit.PacketAntenna, role, f->mode0.bit.MeasuredFreqOffset, filteredStep, readPtr);
        debugwait();

        if (f->mode0.bit.PacketQuality) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode0.bit.PacketRssi) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode0.bit.PacketAntenna) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++;                                                                            //always progress
        if ((role == CHANNEL_SOUNDING_ROLE_INITIATOR) && (f->mode0.bit.MeasuredFreqOffset)) { //needs to be initiator and have filter bit set to include this info
            STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode0_t, Measured_Freq_Offset));
            ret += member_sizeof(cs_step_mode0_t, Measured_Freq_Offset);
        }
        //          readPtr++; //always progress //not needed as we end here
        return ret;
    }
    case 1:
    {
        BLC_RAS_DATA_LOG("mode1: PQ%d, PN%d, PR%d, ToD%d, PA%d, PP1%d, PP2%d, rtt_t%d, %x", f->mode1.bit.PacketQuality, f->mode1.bit.PacketNadm, f->mode1.bit.PacketRssi, f->mode1.bit.ToDToA, f->mode1.bit.PacketAntenna, f->mode1.bit.PacketPct1, f->mode1.bit.PacketPct2, rtt_type, filteredStep);
        debugwait();

        if (f->mode1.bit.PacketQuality) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode1.bit.PacketNadm) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode1.bit.PacketRssi) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++;                                          //always progress
        if (f->mode1.bit.ToDToA) {                          //2 bytes
            STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode1_t, ToA_ToD));
            ret += member_sizeof(cs_step_mode1_t, ToA_ToD); //sizeof cs_step_mode1_t.u8 ToA_ToD[2];
        }
        readPtr += member_sizeof(cs_step_mode1_t, ToA_ToD); //always progress
        if (f->mode1.bit.PacketAntenna) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++;      //always progress
        if (rtt_type) { //Sounding
            if (f->mode1.bit.PacketPct1) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode1_t, Packet_PCT1));
                ret += member_sizeof(cs_step_mode1_t, Packet_PCT1);
            }
            readPtr += member_sizeof(cs_step_mode1_t, Packet_PCT1); //always progress
            if (f->mode1.bit.PacketPct2) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode1_t, Packet_PCT2));
                ret += member_sizeof(cs_step_mode1_t, Packet_PCT2);
            }
            //              readPtr += member_sizeof(cs_step_mode1_t, Packet_PCT2); //always progress //not needed as we end here
        }
        return ret;
    }
    case 2:
    {
        BLC_RAS_DATA_LOG("mode2: API%d, TPK%d, TQIK%d, nAP%d, %x", f->mode2.bit.AntennaPermutationIdx, f->mode2.bit.TonePctK, f->mode2.bit.ToneQualityIndicatorK, numAntennaPaths, filteredStep);
        debugwait();

        u8 api = *readPtr; // AntennaPermutationIdx
        if (f->mode2.bit.AntennaPermutationIdx) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress

        u16 antennaPathMask = 0;
        for (u8 i = 0; i < numAntennaPaths; i++) {
            u8 apiTableMask = API_MASK_POS0 << (2 * i);                                //0b00000011 //0b00001100 //0b00110000 //0b11000000
            u8 currentAP    = (antennaPermutationIndexTable[apiTableOffset + api] & apiTableMask) >> (2 * i);
            antennaPathMask = 1 << (RAS_FILTER_ANTENNA_PATH_OFFSET_MODE2 + currentAP); // 3 => AntennaPath1

            if (f->mode2.raw & antennaPathMask) {
                if (f->mode2.bit.TonePctK) {
                    STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                    ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                }
                readPtr += member_sizeof(cs_step_tone_t, Tone_PCT); //always progress
                if (f->mode2.bit.ToneQualityIndicatorK) {
                    U8_TO_STREAM(filteredStep, *readPtr);
                    ret++;
                }
                readPtr++;                         //always progress
            } else {
                readPtr += sizeof(cs_step_tone_t); //always progress - skip the whole 4 (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
            }
        }
        //antenna path remains for the extension tone
        if (f->mode2.raw & antennaPathMask) {
            if (f->mode2.bit.TonePctK) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                ret += member_sizeof(cs_step_tone_t, Tone_PCT);
            }
            readPtr += member_sizeof(cs_step_tone_t, Tone_PCT); //always progress
            if (f->mode2.bit.ToneQualityIndicatorK) {
                U8_TO_STREAM(filteredStep, *readPtr);
                ret++;
            }
            readPtr++;                         //always progress
        } else {
            readPtr += sizeof(cs_step_tone_t); //always progress - skip the whole 4 (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
        }

        return ret;
    }
    case 3:
    {
        BLC_RAS_DATA_LOG("mode3: PQ%d, PN%d, PR%d, ToD%d, PA%d, PP1%d, PP2%d, API%d, TPK%d, TQIK%d, rtt_t%d, nAP%d, %x", f->mode3.bit.PacketQuality, f->mode3.bit.PacketNadm, f->mode3.bit.PacketRssi, f->mode3.bit.ToDToA, f->mode3.bit.PacketAntenna, f->mode3.bit.PacketPct1, f->mode3.bit.PacketPct2, f->mode3.bit.AntennaPermutationIdx, f->mode3.bit.TonePctK, f->mode3.bit.ToneQualityIndicatorK, rtt_type, numAntennaPaths, filteredStep);
        debugwait();

        //"mode1 part" of mode3
        if (f->mode3.bit.PacketQuality) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode3.bit.PacketNadm) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress
        if (f->mode3.bit.PacketRssi) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++;                                          //always progress
        if (f->mode3.bit.ToDToA) {                          //2 bytes
            STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode3_t, ToA_ToD));
            ret += member_sizeof(cs_step_mode3_t, ToA_ToD); //sizeof cs_step_mode3_t.u8 ToA_ToD[2];
        }
        readPtr += member_sizeof(cs_step_mode3_t, ToA_ToD); //always progress
        if (f->mode3.bit.PacketAntenna) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++;      //always progress
        if (rtt_type) { //Sounding
            if (f->mode3.bit.PacketPct1) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode3_t, Packet_PCT1));
                ret += member_sizeof(cs_step_mode3_t, Packet_PCT1);
            }
            readPtr += member_sizeof(cs_step_mode3_t, Packet_PCT1); //always progress
            if (f->mode3.bit.PacketPct2) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_mode3_t, Packet_PCT2));
                ret += member_sizeof(cs_step_mode3_t, Packet_PCT2);
            }
            readPtr += member_sizeof(cs_step_mode3_t, Packet_PCT2); //always progress
        }
        //"mode2 part" of mode3
        u8 api = *readPtr; // AntennaPermutationIdx
        if (f->mode3.bit.AntennaPermutationIdx) {
            U8_TO_STREAM(filteredStep, *readPtr);
            ret++;
        }
        readPtr++; //always progress

        u16 antennaPathMask = 0;
        for (u8 i = 0; i < numAntennaPaths; i++) {
            u8 apiTableMask = API_MASK_POS0 << (2 * i);                                //0b00000011 //0b00001100 //0b00110000 //0b11000000
            u8 currentAP    = (antennaPermutationIndexTable[apiTableOffset + api] & apiTableMask) >> (2 * i);
            antennaPathMask = 1 << (RAS_FILTER_ANTENNA_PATH_OFFSET_MODE3 + currentAP); // 10 => AntennaPath1

            if (f->mode3.raw & antennaPathMask) {
                if (f->mode3.bit.TonePctK) {
                    STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                    ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                }
                readPtr += member_sizeof(cs_step_tone_t, Tone_PCT); //always progress
                if (f->mode3.bit.ToneQualityIndicatorK) {
                    U8_TO_STREAM(filteredStep, *readPtr);
                    ret++;
                }
                readPtr++;                         //always progress
            } else {
                readPtr += sizeof(cs_step_tone_t); //always progress - skip the whole 4 (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
            }
        }
        //antenna path remains for the extension tone
        if (f->mode3.raw & antennaPathMask) {
            if (f->mode3.bit.TonePctK) {
                STR_TO_STREAM(filteredStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                ret += member_sizeof(cs_step_tone_t, Tone_PCT);
            }
            readPtr += member_sizeof(cs_step_tone_t, Tone_PCT); //always progress
            if (f->mode3.bit.ToneQualityIndicatorK) {
                U8_TO_STREAM(filteredStep, *readPtr);
                ret++;
            }
            readPtr++;                         //always progress
        } else {
            readPtr += sizeof(cs_step_tone_t); //always progress - skip the whole 4 (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
        }

        return ret;
    }
    default:
        BLC_RAS_DATA_LOG("applyStepFilter - Invalid mode!");
        return ret;
    }
}
#endif

/* Compress - part only executed on RAS server */
static u8 *blt_rass_stepDataToProtocolData(blt_ras_filter_t *filter, u8 *writePtr, u8 **readPtr, u8 stepsNum, u8 subeventDoneStatus, u8 role, u8 rtt_type, u8 numAntennaPaths)
{
    u8 stepsAborted = (subeventDoneStatus == CS_SUBEVT_DONE) ? 0 : 1; //logic got flipped in the spec now 0 means valid

    BLC_RAS_DATA_LOG("StepsNum: %d", stepsNum);
    debugwait();

    for (int i = 0; i < stepsNum; i++) {
        cs_step_value_t *stepValue = (cs_step_value_t *)*readPtr;
        BLC_RAS_DATA_LOG("%d stpV: %x, ch: %d, md: %d, len: %d", i, stepValue, stepValue->channel, stepValue->mode, stepValue->len);
        debugwait();

        if (!stepsAborted) { //valid, as the logic is inverted now
            U8_TO_STREAM(writePtr, ((stepsAborted & 0x01) << 7) | ((stepValue->mode & 0x03) << 0));
            *readPtr += CS_STEP_METADATA_LENGTH; //sizeof(stepValue->mode) + sizeof(stepValue->channel) + sizeof(stepValue->len); +3, these come from cs_step_value_t

            /* len validation purpose only */
#if (RAS_DEBUG_STEP_LEN_VALIDATION)
            u8 expected_len = blc_ras_getStepLength(stepValue->mode, role, rtt_type, numAntennaPaths);
            if (expected_len != stepValue->len) {
                BLC_RAS_DATA_LOG("Step len differs! Expected: %d, StepLen: %d", expected_len, stepValue->len);
            }
#endif

#if(RAS_STEP_FILTER)
            u8 filteredStep[CS_STEP_MAX_LENGTH];
            u8 filteredLen = blt_rass_applyStepFilter(filter, stepValue->mode, role, rtt_type, numAntennaPaths, *readPtr, filteredStep);

            STR_TO_STREAM(writePtr, filteredStep, filteredLen);

            BLC_RAS_DATA_LOG("W/o filtr: %s", hex_to_str(*readPtr, stepValue->len));
            debugwait();
            BLC_RAS_DATA_LOG("Filtered : %s", hex_to_str(filteredStep, filteredLen));
            debugwait();
#else
            (void)filter;
            STR_TO_STREAM(writePtr, *readPtr, stepValue->len);
#endif
            *readPtr += stepValue->len;
        }
        else { //not valid, step_data length is 0 and metadata bits 0-6 are zeroed
            U8_TO_STREAM(writePtr, (stepsAborted & 0x01) << 7);
            BLC_RAS_DATA_LOG("Step invalid! subeventDoneStatus %x, stepsAborted %x", subeventDoneStatus, stepsAborted);
            *readPtr += CS_STEP_METADATA_LENGTH + stepValue->len; //skip
        }
    }
    return writePtr;
}

/* Compress - part only executed on RAS server */
ble_sts_t blt_rass_procedureDataToProtocolData(blt_ras_prot_ctrl_t *outputProtCtrl, blt_ras_proc_ctrl_t *inputProcCtrl, blt_ras_dataset_t *rasDataset)
{
    blt_ras_filter_t *filter = &rasDataset->filter;

    BLC_RAS_DATA_LOG("Filter: %x, %x, %x, %x", filter->mode0.raw, filter->mode1.raw, filter->mode2.raw, filter->mode3.raw);

    u8 *readPtr  = (u8 *)(inputProcCtrl->proc.pData);
    u8 *writePtr = (u8 *)(outputProtCtrl->prot.pData);

    //prepare some useful parameters
    blc_rass_prot_head_t *procedure_head = (blc_rass_prot_head_t *)(readPtr);
    u8                    configId       = procedure_head->data.proCountCfgID;
    u8                    role           = rasDataset->config[configId].role;
    u8                    rttType        = rasDataset->config[configId].rttType;
    BLC_RAS_DATA_LOG("Config configId: %d, Role:%d, rttType", configId, role, rttType);

    // antenna paths represented as a antenna paths mask format in RAS protocol, where 0b1111 means all 4 antenna paths
    u8 antennaPathsMask = (1 << procedure_head->data.numAntennaPaths) - 1; // 2^x-1 //x=4 => 0b1111

    STR_TO_STREAM(writePtr, readPtr, PROCEDURE_HEAD_LEN - 1);
    U8_TO_STREAM(writePtr, (antennaPathsMask & 0x3F));
    readPtr += PROCEDURE_HEAD_LEN;

    //for each subevent in the procedure
    for (int i = 0; i < inputProcCtrl->subEvtNum; i++) {
        blc_rass_data_body_t *subeventHead       = (blc_rass_data_body_t *)(readPtr);
        u8                    stepsNum           = subeventHead->numStepsReported;
        u8                    subeventDoneStatus = subeventHead->subeventDoneStatus;

        memcpy(writePtr, readPtr, SUBEVENT_HEAD_LEN);
        writePtr += SUBEVENT_HEAD_LEN;
        readPtr += SUBEVENT_HEAD_LEN;

        writePtr = blt_rass_stepDataToProtocolData(filter, writePtr, &readPtr, stepsNum, subeventDoneStatus, role, rttType, procedure_head->data.numAntennaPaths);
    }

    outputProtCtrl->prot.dataLen = (writePtr - outputProtCtrl->prot.pData); //procCtrl->protocolProcDataLen = writePtr - procCtrl->protocolProcStartaddr;

    BLC_RAS_DATA_LOG("wPtr %x, protStAddr %x, protLen %d", writePtr, outputProtCtrl->prot.pData, outputProtCtrl->prot.dataLen);
    debugwait();

    //checking if I reached an expected spot with the readPtr
    if ((inputProcCtrl->proc.pData + inputProcCtrl->proc.dataLen) != readPtr) {
        BLC_RAS_DATA_LOG("Proc len diff! Start: %x, Len:%x ReadPtr: %x", inputProcCtrl->proc.pData, inputProcCtrl->proc.dataLen, readPtr);
    }
#if (RAS_DEBUG_PRINTBUFFERS)
    // debug only
    BLC_RAS_DATA_LOG("procedure: %x, %d", inputProcCtrl->proc.pData, inputProcCtrl->proc.dataLen);
    log_buffer(inputProcCtrl->proc.pData, inputProcCtrl->proc.dataLen);
    BLC_RAS_DATA_LOG("protocol: %x, %d", outputProtCtrl->prot.pData, outputProtCtrl->prot.dataLen);
    log_buffer(outputProtCtrl->prot.pData, outputProtCtrl->prot.dataLen);
    ///
#endif
    return BLE_SUCCESS;
}

/* Compress - part only executed on RAS server, only for realtime */
ble_sts_t blt_rass_procedureSubeventToProtocolSubevent(blc_rass_subevt_data_t *outputProtSubEvt, blc_rass_subevt_data_t *inputProcSubEvt, blc_rass_prot_head_t *procedureHead, u8 first, blt_ras_dataset_t *rasDataset)
{
    blt_ras_filter_t *filter = &rasDataset->filter;

    BLC_RAS_DATA_LOG("Filter subev: %x, %x, %x, %x", filter->mode0.raw, filter->mode1.raw, filter->mode2.raw, filter->mode3.raw);

    u8 *readPtr  = (u8 *)(inputProcSubEvt->pSubEvt);
    u8 *writePtr = (u8 *)(outputProtSubEvt->pSubEvt);

    if (first) {
        BLC_RAS_DATA_LOG("blt_rass_procedureSubeventToProtocolSubevent first");
        blc_rass_prot_head_t *procHead = (blc_rass_prot_head_t *)(readPtr);
        // antenna paths represented as a antenna paths mask format in RAS protocol, where 0b1111 means all 4 antenna paths
        u8 antennaPathsMask = (1 << procHead->data.numAntennaPaths) - 1; // 2^x-1 //x=4 => 0b1111

        STR_TO_STREAM(writePtr, readPtr, PROCEDURE_HEAD_LEN - 1);
        U8_TO_STREAM(writePtr, (antennaPathsMask & 0x3F));
        readPtr += PROCEDURE_HEAD_LEN;
    }

    //prepare some useful parameters
    u8 configId = procedureHead->data.proCountCfgID;
    u8 role     = rasDataset->config[configId].role;
    u8 rttType  = rasDataset->config[configId].rttType;
    BLC_RAS_DATA_LOG("Config configId: %d, Role: %d, rttType: %d", configId, role, rttType);

    //only a single subevent, no loop
    blc_rass_data_body_t *subeventHead       = (blc_rass_data_body_t *)(readPtr);
    u8                    stepsNum           = subeventHead->numStepsReported;
    u8                    subeventDoneStatus = subeventHead->subeventDoneStatus;

    BLC_RAS_DATA_LOG("pSubEvt:%x, stepsNum: %d, subeventDoneStatus: %x", subeventHead, stepsNum, subeventDoneStatus);

    memcpy(writePtr, readPtr, SUBEVENT_HEAD_LEN);
    writePtr += SUBEVENT_HEAD_LEN;
    readPtr += SUBEVENT_HEAD_LEN;

    //pass CS_SUBEVT_DONE in order to mark steps as valid ... "assumed" for real realtime - TODO: check if it makes sense to pass subeventDoneStatus instead
    writePtr = blt_rass_stepDataToProtocolData(filter, writePtr, &readPtr, stepsNum, CS_SUBEVT_DONE, role, rttType, procedureHead->data.numAntennaPaths);

    outputProtSubEvt->subEvtLen = writePtr - outputProtSubEvt->pSubEvt; // procCtrl->protocolProcDataLen = writePtr - procCtrl->protocolProcStartaddr;

    BLC_RAS_DATA_LOG("wPtr %x, protStAddr %x, protLen %d", writePtr, outputProtSubEvt->pSubEvt, outputProtSubEvt->subEvtLen);
    debugwait();

#if (RAS_DEBUG_PRINTBUFFERS)
    // debug only
    BLC_RAS_DATA_LOG("procedure: %x, %d", inputProcSubEvt->pSubEvt, inputProcSubEvt->subEvtLen);
    log_buffer(inputProcSubEvt->pSubEvt, inputProcSubEvt->subEvtLen);
    BLC_RAS_DATA_LOG("protocol: %x, %d", outputProtSubEvt->pSubEvt, outputProtSubEvt->subEvtLen);
    log_buffer(outputProtSubEvt->pSubEvt, outputProtSubEvt->subEvtLen);
    ///
#endif

    return BLE_SUCCESS;
}

#if (RAS_STEP_FILTER)
/* Decompress - part only executed on RAS client, reverse of blt_rass_applyStepFilter */
static u8 blt_rasc_unpackStepFilter(blt_ras_filter_t *f, u8 mode, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 *readPtr, u8 *unpackedStep)
{
    u8 ret = 0; //length of the filtered field

    u8 apiTableOffset = 0;
    switch (numAntennaPaths) {
    case 3:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP3;
        break;
    case 4:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP4;
        break;
    case 1:
    case 2:
    default:
        apiTableOffset = RAS_ANTENNA_PERMUTATION_INDEX_TABLE_OFFSET_NAP1_2;
        break;
    }

    switch (mode) {
    case 0:
    {
        BLC_RAS_DATA_LOG("mode0: PQ%d, PR%d, PA%d, R%d, MFO%d, %x, %x", f->mode0.bit.PacketQuality, f->mode0.bit.PacketRssi, f->mode0.bit.PacketAntenna, role, f->mode0.bit.MeasuredFreqOffset, unpackedStep, readPtr);
        debugwait();

        if (f->mode0.bit.PacketQuality) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode0.bit.PacketRssi) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode0.bit.PacketAntenna) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
            if (f->mode0.bit.MeasuredFreqOffset) { //needs to be initiator and have filter bit set to include this info
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode0_t, Measured_Freq_Offset));
                ret += member_sizeof(cs_step_mode0_t, Measured_Freq_Offset);
                //readPtr++; //not needed as we end here
            } else {
                u8 len = member_sizeof(cs_step_mode0_t, Measured_Freq_Offset);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
        }
        return ret;
    }
    case 1:
    {
        BLC_RAS_DATA_LOG("mode1: PQ%d, PN%d, PR%d, ToD%d, PA%d, PP1%d, PP2%d, rtt_t%d, %x", f->mode1.bit.PacketQuality, f->mode1.bit.PacketNadm, f->mode1.bit.PacketRssi, f->mode1.bit.ToDToA, f->mode1.bit.PacketAntenna, f->mode1.bit.PacketPct1, f->mode1.bit.PacketPct2, rtt_type, unpackedStep);
        debugwait();

        if (f->mode1.bit.PacketQuality) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode1.bit.PacketNadm) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode1.bit.PacketRssi) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode1.bit.ToDToA) {                          //2 bytes
            STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode1_t, ToA_ToD));
            ret += member_sizeof(cs_step_mode1_t, ToA_ToD); //sizeof cs_step_mode1_t.u8 ToA_ToD[2];
            readPtr += member_sizeof(cs_step_mode1_t, ToA_ToD);
        } else {
            u8 len = member_sizeof(cs_step_mode1_t, ToA_ToD);
            memset(unpackedStep, FILTER_UNPACK_FILLER, len);
            unpackedStep += len;
        }
        if (f->mode1.bit.PacketAntenna) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (rtt_type) { //Sounding
            if (f->mode1.bit.PacketPct1) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode1_t, Packet_PCT1));
                ret += member_sizeof(cs_step_mode1_t, Packet_PCT1);
                readPtr += member_sizeof(cs_step_mode1_t, Packet_PCT1);
            } else {
                u8 len = member_sizeof(cs_step_mode1_t, Packet_PCT1);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
            if (f->mode1.bit.PacketPct2) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode1_t, Packet_PCT2));
                ret += member_sizeof(cs_step_mode1_t, Packet_PCT2);
                //readPtr += member_sizeof(cs_step_mode1_t, Packet_PCT2); //not needed as we end here
            } else {
                u8 len = member_sizeof(cs_step_mode1_t, Packet_PCT2);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
        }
        return ret;
    }
    case 2:
    {
        BLC_RAS_DATA_LOG("mode2: API%d, TPK%d, TQIK%d, nAP%d, %x", f->mode2.bit.AntennaPermutationIdx, f->mode2.bit.TonePctK, f->mode2.bit.ToneQualityIndicatorK, numAntennaPaths, unpackedStep);
        debugwait();

        u8 api = *readPtr; // AntennaPermutationIdx
        if (f->mode2.bit.AntennaPermutationIdx) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        u16 antennaPathMask = 0;
        for (u8 i = 0; i < numAntennaPaths; i++) {
            u8 apiTableMask = API_MASK_POS0 << (2 * i);                                //0b00000011 //0b00001100 //0b00110000 //0b11000000
            u8 currentAP    = (antennaPermutationIndexTable[apiTableOffset + api] & apiTableMask) >> (2 * i);
            antennaPathMask = 1 << (RAS_FILTER_ANTENNA_PATH_OFFSET_MODE2 + currentAP); // 3 => AntennaPath1

            if (f->mode2.raw & antennaPathMask) {
                if (f->mode2.bit.TonePctK) {
                    STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                    ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                    readPtr += member_sizeof(cs_step_tone_t, Tone_PCT);
                } else {
                    u8 len = member_sizeof(cs_step_tone_t, Tone_PCT);
                    memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                    unpackedStep += len;
                }
                if (f->mode2.bit.ToneQualityIndicatorK) {
                    U8_TO_STREAM(unpackedStep, *readPtr);
                    ret++;
                    readPtr++;
                } else {
                    U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
                }
            } else {
                u8 len = member_sizeof(cs_step_tone_t, Tone_PCT) + member_sizeof(cs_step_tone_t, Tone_Quality_Indicator); //recreate the whole 4 bytes (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
        }
        //antenna path remains for the extension tone
        if (f->mode2.raw & antennaPathMask) {
            if (f->mode2.bit.TonePctK) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                readPtr += member_sizeof(cs_step_tone_t, Tone_PCT);
            } else {
                u8 len = member_sizeof(cs_step_tone_t, Tone_PCT);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
            if (f->mode2.bit.ToneQualityIndicatorK) {
                U8_TO_STREAM(unpackedStep, *readPtr);
                ret++;
                readPtr++;
            } else {
                U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
            }
        } else {
            u8 len = member_sizeof(cs_step_tone_t, Tone_PCT) + member_sizeof(cs_step_tone_t, Tone_Quality_Indicator); //recreate the whole 4 bytes (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
            memset(unpackedStep, FILTER_UNPACK_FILLER, len);
            unpackedStep += len;
        }
        return ret;
    }
    case 3:
    {
        BLC_RAS_DATA_LOG("mode3: PQ%d, PN%d, PR%d, ToD%d, PA%d, PP1%d, PP2%d, API%d, TPK%d, TQIK%d, rtt_t%d, nAP%d, %x", f->mode3.bit.PacketQuality, f->mode3.bit.PacketNadm, f->mode3.bit.PacketRssi, f->mode3.bit.ToDToA, f->mode3.bit.PacketAntenna, f->mode3.bit.PacketPct1, f->mode3.bit.PacketPct2, f->mode3.bit.AntennaPermutationIdx, f->mode3.bit.TonePctK, f->mode3.bit.ToneQualityIndicatorK, rtt_type, numAntennaPaths, unpackedStep);
        debugwait();

        //"mode1 part" of mode3
        if (f->mode3.bit.PacketQuality) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode3.bit.PacketNadm) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode3.bit.PacketRssi) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (f->mode3.bit.ToDToA) {                          //2 bytes
            STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode3_t, ToA_ToD));
            ret += member_sizeof(cs_step_mode3_t, ToA_ToD); //sizeof cs_step_mode3_t.u8 ToA_ToD[2];
            readPtr += member_sizeof(cs_step_mode3_t, ToA_ToD);
        } else {
            u8 len = member_sizeof(cs_step_mode3_t, ToA_ToD);
            memset(unpackedStep, FILTER_UNPACK_FILLER, len);
            unpackedStep += len;
        }
        if (f->mode3.bit.PacketAntenna) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }
        if (rtt_type) { //Sounding
            if (f->mode3.bit.PacketPct1) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode3_t, Packet_PCT1));
                ret += member_sizeof(cs_step_mode3_t, Packet_PCT1);
                readPtr += member_sizeof(cs_step_mode3_t, Packet_PCT1);
            } else {
                u8 len = member_sizeof(cs_step_mode3_t, Packet_PCT1);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
            if (f->mode3.bit.PacketPct2) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_mode3_t, Packet_PCT2));
                ret += member_sizeof(cs_step_mode3_t, Packet_PCT2);
                readPtr += member_sizeof(cs_step_mode3_t, Packet_PCT2);
            } else {
                u8 len = member_sizeof(cs_step_mode3_t, Packet_PCT2);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
        }
        //"mode2 part" of mode3

        u8 api = *readPtr; // AntennaPermutationIdx
        if (f->mode3.bit.AntennaPermutationIdx) {
            U8_TO_STREAM(unpackedStep, *readPtr);
            ret++;
            readPtr++;
        } else {
            U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
        }

        u16 antennaPathMask = 0;
        for (u8 i = 0; i < numAntennaPaths; i++) {
            u8 apiTableMask = API_MASK_POS0 << (2 * i);                                //0b00000011 //0b00001100 //0b00110000 //0b11000000
            u8 currentAP    = (antennaPermutationIndexTable[apiTableOffset + api] & apiTableMask) >> (2 * i);
            antennaPathMask = 1 << (RAS_FILTER_ANTENNA_PATH_OFFSET_MODE3 + currentAP); // 10 => AntennaPath1

            if (f->mode3.raw & antennaPathMask) {
                if (f->mode3.bit.TonePctK) {
                    STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                    ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                    readPtr += member_sizeof(cs_step_tone_t, Tone_PCT);
                } else {
                    u8 len = member_sizeof(cs_step_tone_t, Tone_PCT);
                    memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                    unpackedStep += len;
                }
                if (f->mode3.bit.ToneQualityIndicatorK) {
                    U8_TO_STREAM(unpackedStep, *readPtr);
                    ret++;
                    readPtr++;
                } else {
                    U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
                }
            } else {
                u8 len = member_sizeof(cs_step_tone_t, Tone_PCT) + member_sizeof(cs_step_tone_t, Tone_Quality_Indicator); //recreate the whole 4 bytes (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
        }
        //antenna path remains for the extension tone
        if (f->mode3.raw & antennaPathMask) {
            if (f->mode3.bit.TonePctK) {
                STR_TO_STREAM(unpackedStep, readPtr, member_sizeof(cs_step_tone_t, Tone_PCT));
                ret += member_sizeof(cs_step_tone_t, Tone_PCT);
                readPtr += member_sizeof(cs_step_tone_t, Tone_PCT);
            } else {
                u8 len = member_sizeof(cs_step_tone_t, Tone_PCT);
                memset(unpackedStep, FILTER_UNPACK_FILLER, len);
                unpackedStep += len;
            }
            if (f->mode3.bit.ToneQualityIndicatorK) {
                U8_TO_STREAM(unpackedStep, *readPtr);
                ret++;
                readPtr++;
            } else {
                U8_TO_STREAM(unpackedStep, FILTER_UNPACK_FILLER);
            }
        } else {
            u8 len = member_sizeof(cs_step_tone_t, Tone_PCT) + member_sizeof(cs_step_tone_t, Tone_Quality_Indicator); //recreate the whole 4 bytes (CS_STEP_DATA_LENGTH_MODE2_NUM_ANTENNA_PATHS_X)
            memset(unpackedStep, FILTER_UNPACK_FILLER, len);
            unpackedStep += len;
        }
        return ret;
    }
    default:
        BLC_RAS_DATA_LOG("unpackStepFilter - Invalid mode!");
        return ret;
    }
}
#endif

/* Decompress - part only executed on RAS client, reverse of blt_rass_stepDataToProtocolData */
static u8 *blt_rasc_stepDataToProcedureData(blt_ras_filter_t *filter, u8 *writePtr, u8 **readPtr, u8 **readPtrLocal, u8 stepsNum, u8 role, u8 rtt_type, u8 numAntennaPaths, u8 noLocalProcedure_ForIOP)
{
    u8 *stepMetaDataReadPtrLocal = *readPtrLocal;

    BLC_RAS_DATA_LOG("StepsNum prot: %d", stepsNum);
    debugwait();

    for (int i = 0; i < stepsNum; i++) {
        // TBD: How to handle aborted bit, if at all - stepHead->data.abortedBit
        blc_rass_step_head_t *stepMetaDataReadPtr = (blc_rass_step_head_t *)*readPtr;
        u8                    mode                = stepMetaDataReadPtr->data.mode;
        u8                    abortedBit          = stepMetaDataReadPtr->data.abortedBit;
        *readPtr += sizeof(blc_rass_step_head_t);

        u8 channel = 0;                 //IOP TESTING

        if (!noLocalProcedure_ForIOP) { //IOP TESTING
            //the only way I see in rebuilding channel information is to take it from our own local
            //corresponding CS data
            cs_step_value_t *stepValueLocal = (cs_step_value_t *)stepMetaDataReadPtrLocal;
            u8               modeLocal      = stepValueLocal->mode;
            channel                         = stepValueLocal->channel;
            u8 lenLocal                     = stepValueLocal->len;
            stepMetaDataReadPtrLocal += CS_STEP_METADATA_LENGTH + lenLocal; // CS_STEP_METADATA_LENGTH + skip local step data

            //sanity check validation
            if (mode != modeLocal) {
                BLC_RAS_DATA_LOG("Mode diff! Local: %d, Remote: %d, abortedBit:%x", modeLocal, mode, abortedBit);
                debugwait();
            }
        }

        //after decompression, the length is always the full unfiltered length
        u8 unpackedLen = blc_ras_getStepLength(mode, role, rtt_type, numAntennaPaths);

        //write "cs_step_value_t" metadata //role correct here for remote - already flipped in blt_rasc_protocolDataToProcedureData from local
        cs_step_value_t *stepValueRemote = (cs_step_value_t *)writePtr;
        stepValueRemote->mode            = mode;
        stepValueRemote->channel         = channel;
        stepValueRemote->len             = unpackedLen;
        BLC_RAS_DATA_LOG("%d stpVRem: %x, ch: %d, md: %d, len: %d, abortedBit: %x", i, stepValueRemote, stepValueRemote->channel, stepValueRemote->mode, stepValueRemote->len, abortedBit);
        debugwait();
        writePtr += CS_STEP_METADATA_LENGTH;


        if(!abortedBit) {
#if(RAS_STEP_FILTER)
            u8 unpackedStep[CS_STEP_MAX_LENGTH];
            u8 filteredLen = blt_rasc_unpackStepFilter(filter, mode, role, rtt_type, numAntennaPaths, *readPtr, unpackedStep);

            STR_TO_STREAM(writePtr, unpackedStep, unpackedLen);

            BLC_RAS_DATA_LOG("Bef.unpack: %s", hex_to_str(*readPtr, filteredLen)); debugwait();
            BLC_RAS_DATA_LOG("Unpacked : %s", hex_to_str(unpackedStep, unpackedLen)); debugwait();
            *readPtr += filteredLen;
#else
            (void)filter;
            STR_TO_STREAM(writePtr, *readPtr, unpackedLen); //writePtr += unpackedLen
            *readPtr += unpackedLen;
#endif
        }
        else {
            memset(writePtr, ABORTED_UNPACK_FILLER, unpackedLen);
            writePtr += unpackedLen;
            //no increase on *readPtr, as aborted step len == 0
            }
        }

    *readPtrLocal = stepMetaDataReadPtrLocal;
    // *readPtr = stepDataReadPtr;
    return writePtr;
}

/* Decompress - part only executed on RAS client, reverse of blt_rass_procedureDataToProtocolData */
ble_sts_t blt_rasc_protocolDataToProcedureData(blt_ras_proc_ctrl_t *procCtrlRemoteOutput, blt_ras_proc_ctrl_t *protCtrlRemoteInput, blt_ras_dataset_t *localRasDataset)
{
    debugwait();

    if ((!procCtrlRemoteOutput) || (!protCtrlRemoteInput) || (!localRasDataset)) {
        goto failed;
    }

    u8 *readPtr  = (u8 *)(protCtrlRemoteInput->proc.pData);
    u8 *writePtr = (u8 *)(procCtrlRemoteOutput->proc.pData);

    if ((!readPtr) || (!writePtr)) {
        goto failed;
    }

    //prepare some useful parameters
    blc_rass_prot_head_t *procedureHeadRemote = (blc_rass_prot_head_t *)(readPtr);

    //lookup for the local procedure with the same procedure number
    blt_ras_data_ctrl_t *dataCtrlLocal = (blt_ras_data_ctrl_t *)&(localRasDataset->dataCtrl);
    blt_ras_proc_ctrl_t *procCtrlLocal = NULL;

    u16 localIndexForProcedureCounter = blt_ras_getLocalIndexForProcedureCounter(localRasDataset, procedureHeadRemote->data.procedureCounter);
    if (localIndexForProcedureCounter != RAS_INVALID_INDEX_PROCEDURE) {
        procCtrlLocal = (blt_ras_proc_ctrl_t *)&(dataCtrlLocal->procCtrl[localIndexForProcedureCounter]);
    }

    u8                    iopNoLocalProcedure = FALSE;
    u8                    configId            = 0;
    u8                    numAntennaPaths     = 0;
    blc_rass_prot_head_t *procedureHeadLocal  = NULL;

    //ES-26610 - dont use remote information. Use remote only when local is not present
    if (!procCtrlLocal) {                                                                   //use remote header for necessary data
        iopNoLocalProcedure = TRUE;                                                         //TODO make conditional compilation when removing IOP
        configId            = procedureHeadRemote->data.proCountCfgID;                      //if only remote, then use remote info - expected only in IOP
        numAntennaPaths     = blt_calBit1Number(procedureHeadRemote->data.numAntennaPaths); //antennaPathsMask to numAntennaPaths using hamming weight
    } else {                                                                                //use local head for necessary data
        procedureHeadLocal = (blc_rass_prot_head_t *)(procCtrlLocal->proc.pData);
        configId           = procedureHeadLocal->data.proCountCfgID;
        numAntennaPaths    = procedureHeadLocal->data.numAntennaPaths;                      //local procedure has the information available directly, without decoding
#if (RAS_DEBUG_PRINTBUFFERS)
        BLC_RAS_DATA_LOG("Local:");
        debugwait();
        log_buffer(procCtrlLocal->proc.pData, procCtrlLocal->proc.dataLen);
        debugwait();
#endif
    }
#if (RAS_DEBUG_PRINTBUFFERS)
    BLC_RAS_DATA_LOG("Remote:");
    debugwait();
    log_buffer(protCtrlRemoteInput->proc.pData, protCtrlRemoteInput->proc.dataLen);
    debugwait();
#endif
    //no matching procedure found //TODO reenable after removing IOP
    if (procCtrlLocal == NULL) {
        BLC_RAS_DATA_LOG("No matching proc! procedureCounter: %d, rangingCounter:%d", procedureHeadRemote->data.procedureCounter, protCtrlRemoteInput->rangingCounter); //ranging Counter is not reliable here. Procedure counter is.
                                                                                                                                                                        //      return CS_PROC_ABORT; //Temporarily disabled for testing ! //IOP TESTING
    }

    u8 *readPtrLocal = NULL;
    if (!iopNoLocalProcedure) {
        readPtrLocal = (u8 *)(procCtrlLocal->proc.pData);
    }

    //we use this function to decode remote data and due to that
    //we reverse the role, as the remote role will be the opposite of the local one
    u8 role     = (localRasDataset->config[configId].role) ? CS_CONFIG_INITIATOR_ROLE : CS_CONFIG_REFLECTOR_ROLE;
    u8 rtt_type = localRasDataset->config[configId].rttType;
    BLC_RAS_DATA_LOG("Config configId: %d, Role:%d, rttType:%d, nAP:%d, iopNoLocal:%d, pLocal:%x", configId, role, rtt_type, numAntennaPaths, iopNoLocalProcedure, procCtrlLocal);
    debugwait();

    STR_TO_STREAM(writePtr, readPtr, PROCEDURE_HEAD_LEN - 1);
    U8_TO_STREAM(writePtr, (numAntennaPaths & 0x3F));
    readPtr += PROCEDURE_HEAD_LEN;
    if (!iopNoLocalProcedure) {
        readPtrLocal += PROCEDURE_HEAD_LEN;
    }

    BLC_RAS_DATA_LOG("rPtr %x, protStAddr %x, protLen %d", readPtr, protCtrlRemoteInput->proc.pData, protCtrlRemoteInput->proc.dataLen);
    debugwait();

    //while protocol procedure data remains
    while ((readPtr - protCtrlRemoteInput->proc.pData) < protCtrlRemoteInput->proc.dataLen) {
        //a new subevent starts
        procCtrlRemoteOutput->subEvtNum++; //procCtrlRemote->subEvtsNums++;
        blc_rass_data_body_t *subeventHead = (blc_rass_data_body_t *)(readPtr);
        u8                    stepsNum     = subeventHead->numStepsReported;
        //u8 subeventDoneStatus = subeventHead->subeventDoneStatus;
        BLC_RAS_DATA_LOG("stepsNum: %d", stepsNum);
        debugwait();

        //sanity check
        blc_rass_data_body_t *subeventHeadLocal = (blc_rass_data_body_t *)(readPtrLocal);
        u8                    stepsNumLocal     = subeventHeadLocal->numStepsReported;
        if ((stepsNum != stepsNumLocal) && (!iopNoLocalProcedure)) { //IOP TESTING
            BLC_RAS_DATA_LOG("StepsNum difference! Remote: %d, Local:%d", stepsNum, stepsNumLocal);
            debugwait();
        }

        STR_TO_STREAM(writePtr, readPtr, SUBEVENT_HEAD_LEN);
        readPtr += SUBEVENT_HEAD_LEN;
        if (!iopNoLocalProcedure) {
            readPtrLocal += SUBEVENT_HEAD_LEN;
        }
        writePtr = blt_rasc_stepDataToProcedureData(&localRasDataset->filter, writePtr, &readPtr, &readPtrLocal, stepsNum, /*subeventDoneStatus,*/ role, rtt_type, numAntennaPaths, iopNoLocalProcedure); //IOP TESTING

        BLC_RAS_DATA_LOG("rPtr %x, protStAddr %x, protLen %d", readPtr, protCtrlRemoteInput->proc.pData, protCtrlRemoteInput->proc.dataLen);
        debugwait();
    }
    procCtrlRemoteOutput->proc.dataLen = writePtr - procCtrlRemoteOutput->proc.pData;
    BLC_RAS_DATA_LOG("wPtr %x, procStAddr %x, procLen %d", writePtr, procCtrlRemoteOutput->proc.pData, procCtrlRemoteOutput->proc.dataLen);
    debugwait();

    //checking if I reached an expected spot with the readPtr
    if ((protCtrlRemoteInput->proc.pData + protCtrlRemoteInput->proc.dataLen) != readPtr) {
        BLC_RAS_DATA_LOG("Protocol len difference! Start: %x, Len:%x ReadPtr: %x", protCtrlRemoteInput->proc.pData, protCtrlRemoteInput->proc.dataLen, readPtr);
        debugwait();
    }

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static u16 __attribute__((unused)) blt_ras_getLocalIndexForRangingCounter(u16 connHandle, u16 rangingCounter)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    for (int i = 0; i < dataCtrl->storedNum; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);

        if (procCtrl->rangingCounter == (rangingCounter & 0xFFF)) {
            BLC_RAS_DATA_LOG("Index found rangingCounter:%d i: %d procCtrl:%x", rangingCounter, i, procCtrl);
            return i;
        }
    }
    BLC_RAS_DATA_LOG("Index not found rangingCounter:%d ", rangingCounter);
failed:
    return RAS_INVALID_INDEX_PROCEDURE;
}

static u16 blt_ras_getLocalIndexForProcedureCounter(blt_ras_dataset_t *rasDataset, u16 procedureCounter)
{
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl); //local.procedure_ctrl_buf;
    if (!rasDataset) {
        goto failed;
    }

    for (int i = 0; i < dataCtrl->storedNum; i++) {
        blt_ras_proc_ctrl_t  *procCtrl           = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
        blc_rass_prot_head_t *procedureHeadLocal = NULL;

        if (procCtrl->proc.pData) {
            procedureHeadLocal = (blc_rass_prot_head_t *)procCtrl->proc.pData;
        } else {
            if ((procCtrl->subEvtNum) && (procCtrl->subEvtData[0].pSubEvt)) {
                procedureHeadLocal = (blc_rass_prot_head_t *)procCtrl->subEvtData[0].pSubEvt;
            }
        }
        if (!procedureHeadLocal) {
            goto failed;
        }

        BLC_RAS_DATA_LOG("getLocalIndexForProcedureCounter i: %d procCtr:%d procCtrl:%d", i, procedureHeadLocal->data.procedureCounter, procCtrl);

        if (procedureHeadLocal->data.procedureCounter == (procedureCounter & 0xFFF)) {
            BLC_RAS_DATA_LOG("Index found procedureCounter:%d i: %d procCtrl:%d", procedureCounter, i, procCtrl);
            return i;
        }
    }
    BLC_RAS_DATA_LOG("Index not found procedureCounter:%d ", procedureCounter);
failed:
    return RAS_INVALID_INDEX_PROCEDURE;
}

blt_rass_procedure_query_result_t blt_rass_procedureQuery(blt_ras_dataset_t *rasDataset, u16 rangingCounter)
{
    blt_rass_procedure_query_result_t queryData = {0};

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
    if (!rasDataset) {
        goto failed;
    }

    if (dataCtrl->storedNum < 1) {
        return queryData;
    }

    u8 maxStoredNum;
    #if (LL_CS_SNIFFER_MODE_ENABLE)
        maxStoredNum = RAS_PROCEDURE_COUNT;
    #else
        maxStoredNum = dataCtrl->storedNum;
    #endif
    for (int i = 0; i < maxStoredNum; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
        BLC_RAS_DATA_LOG("pProcCtrl: %x i: %d", procCtrl, i);
        debugwait();
        BLC_RAS_DATA_LOG("queryIndex pDataCtrl:%x, pProcStAddr: %x", dataCtrl, dataCtrl->procCtrl[i].proc.pData);
        debugwait();

        #if (LL_CS_SNIFFER_MODE_ENABLE)
            if (procCtrl->rangingCounter != RAS_INVALID_INDEX_PROCEDURE)
        #endif
            {
                if (rangingCounter == (procCtrl->rangingCounter & 0xFFF)) {
                    queryData.status   = 1;
                    queryData.index    = i;
                    queryData.procData = procCtrl;

                    BLC_RAS_DATA_LOG("Query pDataCtrl %x pProcCtrl %x rangCtr %d, procStart %x, procLen %d", dataCtrl, procCtrl, procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen);
                    debugwait();
                    break;
                }
            }

        if (dataCtrl->storedNum == i + 1) {
            BLC_RAS_DATA_LOG("procedureQuery index not found %d", rangingCounter);
        }
    }
failed:
    return queryData;
}

static void blt_ras_clearProcedureData(blt_ras_proc_ctrl_t *procCtrl, bool freeMemory)
{
    BLC_RAS_DATA_LOG("ClearProcedureData:%d", freeMemory);
    if (freeMemory) {
        if (procCtrl->proc.pData != NULL) {
            BLC_RAS_DATA_LOG("Freeing procStartaddr:%x", procCtrl->proc.pData);
            free_nonreten(procCtrl->proc.pData);
        }
        procCtrl->proc.pData   = NULL;
        procCtrl->proc.dataLen = 0;

        for (u8 i = 0; i < procCtrl->subEvtNum; i++) {
            if (procCtrl->subEvtData[i].pSubEvt != NULL) {
                BLC_RAS_DATA_LOG("Freeing subEvtData: %x", procCtrl->subEvtData[i].pSubEvt, procCtrl->subEvtData[i].subEvtLen);
                free_nonreten(procCtrl->subEvtData[i].pSubEvt);
            }
            procCtrl->subEvtData[i].pSubEvt   = NULL;
            procCtrl->subEvtData[i].subEvtLen = 0;
        }
    } else {
        procCtrl->proc.pData   = NULL;
        procCtrl->proc.dataLen = 0;
        for (u8 i = 0; i < procCtrl->subEvtNum; i++) {
            procCtrl->subEvtData[i].pSubEvt   = NULL;
            procCtrl->subEvtData[i].subEvtLen = 0;
        }
    }
    procCtrl->subEvtNum      = 0;
    procCtrl->proc.dataLen   = 0;
    procCtrl->rangingCounter = RAS_INVALID_INDEX_PROCEDURE;
#if (RAS_TIMEOUT_EN)
    procCtrl->timestamp = 0;
#endif
}

void blt_ras_clearAndInitializeLocal(blt_ras_dataset_t *rasDataset)
{
    BLC_RAS_DATA_LOG("blt_ras_clearAndInitializeLocal");
    debugwait();

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    /* Free all allocations */
    for (int i = 0; i < RAS_PROCEDURE_COUNT; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
#if (RAS_TIMEOUT_EN)
        procCtrl->timestamp = 0;
#endif
        blt_ras_clearProcedureData(procCtrl, TRUE); // free all alocated memory
    }
    dataCtrl->storedNum = 0;
}

blt_ras_response_enum blt_ras_procedureDeleteLocal(blt_ras_dataset_t *rasDataset, u16 rangingCounter)
{
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    BLC_RAS_DATA_LOG("blt_ras_procedureDeleteLocal called storedNum %d", dataCtrl->storedNum);
    debugwait();

    if (dataCtrl->storedNum == 0) {
        BLC_RAS_DATA_LOG("index delete error - empty");
        return CS_RAS_NO_RECORDS_FOUND;
    }

    if (dataCtrl->storedNum == 1) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[0]);

        if (rangingCounter == procCtrl->rangingCounter) {
            BLC_RAS_DATA_LOG("One record in store. Removing by buffer re-init.");
            blt_ras_clearAndInitializeLocal(rasDataset); //final one removed, buffer can be cleared
            return CS_RAS_SUCCESS;
        }
        BLC_RAS_DATA_LOG("delete No records found");
        return CS_RAS_NO_RECORDS_FOUND;
    }

    for (int i = 0; i < dataCtrl->storedNum; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);

        if (rangingCounter == procCtrl->rangingCounter) {
            //u16 deletedProcedureLen = procCtrl->proc.dataLen;
            BLC_RAS_DATA_LOG("Record found at index %d. Deleting....", i);
            blt_ras_clearProcedureData(procCtrl, TRUE);

            if (i < dataCtrl->storedNum - 1) { // there are entries behind entry we removed, we need to put these entries an index-up
                for (int k = i; k < dataCtrl->storedNum - 1; k++) {
                    blt_ras_proc_ctrl_t *procCtrl_this = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
                    blt_ras_proc_ctrl_t *procCtrl_next = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k + 1]);
                    memcpy(procCtrl_this, procCtrl_next, sizeof(blt_ras_proc_ctrl_t));
                    if (k == dataCtrl->storedNum - 2) { //optimization - only needed on final iteration
                        blt_ras_clearProcedureData(procCtrl_next, FALSE); // Only clear pointer data, dont free memory
                    }
                }
            }
            dataCtrl->storedNum = dataCtrl->storedNum - 1;
            return CS_RAS_SUCCESS;
        }
    }
    return CS_RAS_NO_RECORDS_FOUND;
}

static u8 *blt_ras_stepDataProc(u8 *writePtr, u8 *srcPtr, u8 stepsNum)
{
    u8 *stepPtr = srcPtr;
    for (int i = 0; i < stepsNum; i++) {
        u32 stepData    = 0;
        u8  stepDataLen = 0;
        STREAM_TO_U24(stepData, stepPtr);
        U24_TO_STREAM(writePtr, stepData);
        stepDataLen = U32_BYTE2(stepData);

        STR_TO_STREAM(writePtr, stepPtr, stepDataLen);
        stepPtr += stepDataLen;
    }

    return writePtr;
}

ble_sts_t blc_ras_csProcedureEnComplete(hci_le_csProcedureEnableCompleteEvt_t *procedureHead)
{
    BLC_RAS_DATA_LOG("blc_ras_csProcedureEnComplete");
    debugwait();
    u16                connHandle = procedureHead->Connection_Handle;
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
    BLC_RAS_DATA_LOG("procedureEnComplete connHandle=%x, storedNum=%d configId=%d selectedTxPower=%x", connHandle, dataCtrl->storedNum, procedureHead->Config_ID, procedureHead->Selected_TX_Power);
    dataCtrl->selectedTxPower = procedureHead->Selected_TX_Power;
#if (TTF_EN)
    TTF_LOG("blc_ras_csProcedureEnComplete selectedTxPower=%02X", dataCtrl->selectedTxPower);
    ttf_log_buffer_with_label(procedureHead, sizeof(hci_le_csProcedureEnableCompleteEvt_t), "csProcedureEnableCompleteEvt:");
#endif
    // blc_rass_prepareNextProcedureEntry(rasDataset);

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static void blt_ras_mergeSubevtsToProcedure(blt_ras_proc_ctrl_t *procCtrl)
{
    u16 procedureLength = 0;
    for (u8 i = 0; i < procCtrl->subEvtNum; i++) {
        procedureLength += procCtrl->subEvtData[i].subEvtLen;
    }
    procCtrl->proc.dataLen = procedureLength;
    u8 *tempProcedureBuf   = (u8 *)malloc_nonreten(procedureLength);
    if (tempProcedureBuf == NULL) {
        #if (LL_CS_SNIFFER_MODE_ENABLE)
            procCtrl->proc.dataLen = 0;
        #endif

        BLC_RAS_DATA_LOG("ERROR! Out of memory spot 003");
        goto failed;
    }
    procCtrl->proc.pData = tempProcedureBuf;
    for (u8 i = 0; i < procCtrl->subEvtNum; i++) {
        if (procCtrl->subEvtData[i].pSubEvt != NULL) {
            memcpy(tempProcedureBuf, procCtrl->subEvtData[i].pSubEvt, procCtrl->subEvtData[i].subEvtLen);
            tempProcedureBuf += procCtrl->subEvtData[i].subEvtLen;
            free_nonreten(procCtrl->subEvtData[i].pSubEvt);
            procCtrl->subEvtData[i].pSubEvt = NULL;
        } else {
            BLC_RAS_DATA_LOG("Issue !! pSubEvt is NULL rangCtr: %d, i: %d", procCtrl->rangingCounter, i);
        }
    }
    BLC_RAS_DATA_LOG("mergeSubevtsToProcedure rangCtr: %d, procedureLength: %d, pStart: %x, pEnd: %x", procCtrl->rangingCounter, procedureLength, procCtrl->proc.pData, tempProcedureBuf);
failed:
    return;
}

static void blt_ras_procedureHeaderFill(blc_rass_prot_head_t *procedureHead, u8 *startAddr)
{
    u8 *writePtr = startAddr;
    if (!writePtr) {
        goto failed;
    }
    BLC_RAS_DATA_LOG("procedureHeaderFill start wptr = %x, record number = %x", writePtr, procedureHead->data.procedureCounter);
    U16_TO_STREAM(writePtr, ((procedureHead->data.procedureCounter & 0x0FFF) | (procedureHead->data.proCountCfgID << 12)));
    U8_TO_STREAM(writePtr, procedureHead->data.selectedTxPower);
    U8_TO_STREAM(writePtr, ((procedureHead->data.numAntennaPaths & 0x3F) /*| (rasDataset->local.proc_head_data.data.PCTFormat << 6)*/))
failed:
    return;
}

ble_sts_t blc_ras_csSubeventResultData(hci_le_csSubeventResultEvt_t *resultEvt)
{
    u16                connHandle = resultEvt->Connection_Handle;
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        BLC_RAS_DATA_LOG("rasDataset is NULL");
#if (RAS_DEBUG_PRINTBUFFERS)
        log_buffer((void *)resultEvt, 100);
        debugwait();
#endif
        goto failed;
    }

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
    blt_ras_proc_ctrl_t *procCtrl = NULL;

    //the spec defined overwritten gets triggered in blt_rass_procedureDataReady. This should never get executed.
    //this can be kept just in case some old records somehow dont get properly removed
    if (dataCtrl->storedNum >= RAS_PROCEDURE_COUNT) {
        BLC_RAS_DATA_LOG("dataCtrl->storedNum >= RAS_PROCEDURE_COUNT - should never happen!");
        BLC_RAS_DATA_LOG("Overwritten storedNum %d", dataCtrl->storedNum);

        procCtrl                      = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[0]);
        u16 overwrittenRangingCounter = procCtrl->rangingCounter; //0 index is the oldest record
        BLC_RAS_DATA_LOG("procCtrl: %x, overwrittenRangingCounter: %d", procCtrl, overwrittenRangingCounter);

        //blc_rass_procedureDataOverwritten(connHandle, overwrittenRangingCounter); //not sending it here
        //lets delete here instead of when the overwritten msg gets sent/confirmed, as we need space now
        blt_ras_procedureDeleteLocal(rasDataset, overwrittenRangingCounter);
        blt_rasc_setLocalDataReady(connHandle);
    }

    s8 procIndex = dataCtrl->storedNum;
    procCtrl     = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[procIndex]);

    BLC_RAS_DATA_LOG("csSubeventResultData rangCtr %d, procStart %x, procLen %d, pSubEvt %x, subevtnum %d", procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen, procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt, procCtrl->subEvtNum);
    debugwait();
    BLC_RAS_DATA_LOG("Stored: storedCnt %d", dataCtrl->storedNum);
    debugwait();

    if (procCtrl->rangingCounter != (resultEvt->Procedure_Counter & 0xFFF)) {
        BLC_RAS_DATA_LOG("New rangingCounter fmProcCtrl %d, fmEvt %d", procCtrl->rangingCounter, resultEvt->Procedure_Counter);
        
        //inform server instance (if present) about a new procedure
        blt_rass_newProcedure(connHandle);
        //inform client instance (if relevant) about a new procedure
        blt_rasc_newProcedure(connHandle);
    }

    u8 *tempSubevtBuf = (u8 *)malloc_nonreten(SUBEVENT_DATA_LEN);
    u8  offset        = 0;

    if (tempSubevtBuf == NULL) {
        BLC_RAS_DATA_LOG("ERROR! Out of memory spot 001");
        goto failed2;
    }

    blc_rass_prot_head_t *procedureHead  = (blc_rass_prot_head_t *)&(procCtrl->procedureHead);
    procCtrl->rangingCounter             = resultEvt->Procedure_Counter & 0xFFF;
    procedureHead->data.procedureCounter = resultEvt->Procedure_Counter & 0xFFF;
    procedureHead->data.proCountCfgID    = resultEvt->Config_ID;         //(resultEvt->Procedure_Counter&0x0F)|((resultEvt->Config_ID<<4)&0xF0); /* Counter of the procedure (the lower four bits)*/
    procedureHead->data.selectedTxPower  = dataCtrl->selectedTxPower;    //gets set in blc_ras_csProcedureEnComplete
    procedureHead->data.numAntennaPaths  = resultEvt->Num_Antenna_Paths; //translate number to mask
    TTF_LOG("selectedTxPower=%02X", procedureHead->data.selectedTxPower);
    BLC_RAS_DATA_LOG("proCountCfgID=%02X, Procedure_Counter=%02d, Config_ID=%02X, selectedTxPower=%02X, numAntennaPaths=%02X",
                     procedureHead->data.proCountCfgID,
                     resultEvt->Procedure_Counter,
                     resultEvt->Config_ID,
                     procedureHead->data.selectedTxPower,
                     procedureHead->data.numAntennaPaths);

    //procedure header gets stored with the first subevent
    if (procCtrl->subEvtNum == 0) {
        // blc_rass_prot_head_t* procedureHead = (blc_rass_prot_head_t*)tempSubevtBuf;
        blt_ras_procedureHeaderFill(procedureHead, (u8 *)(tempSubevtBuf));
        offset = PROCEDURE_HEAD_LEN;
    }

    //come back to the proper one - in case we executed overwritten
    procCtrl     = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[procIndex]);
    u8 *writePtr = (u8 *)(tempSubevtBuf + offset); //(procCtrl->subeventPtr);

    BLC_RAS_DATA_LOG("result afterovr rangCtr %d, procStart %x, procLen %d, subevtnum %d", procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen, /*procCtrl->protocolProcStartaddr, procCtrl->protocolProcDataLen, procCtrl->subeventPtr,*/ procCtrl->subEvtNum);
    debugwait();

    /* -> Subevent Index
     * -> Start ACL Conn Event
     * -> Frequency Compensation
     * -> Procedure Done Status
     * -> Subevent Done Status
     * -> Reference Power Level
     * -> Num Antenna Paths
     * -> Num Steps Reported
     * -> Subevent Ranging Data[]
     * */

    U16_TO_STREAM(writePtr, resultEvt->Start_ACL_Conn_Event);
    U16_TO_STREAM(writePtr, resultEvt->Frequency_Compensation);
    U8_TO_STREAM(writePtr, ((resultEvt->Procedure_Done_Status & 0x0F) | (resultEvt->Subevent_Done_Status << 4)));
    U8_TO_STREAM(writePtr, resultEvt->Abort_Reason); //RAS_d0.9r04

    U8_TO_STREAM(writePtr, resultEvt->Reference_Power_Level);

    u8 numSteps = resultEvt->Num_Steps_Reported;
    U8_TO_STREAM(writePtr, numSteps);

    writePtr = tempSubevtBuf + offset + SUBEVENT_HEAD_LEN;

    BLC_RAS_DATA_LOG("StACLCoEv=%02X, FreqComp=%02X, PrDone=%02X, SeDone=%02X, RefPwr=%02X, numst=%d",
                     resultEvt->Start_ACL_Conn_Event,
                     resultEvt->Frequency_Compensation,
                     resultEvt->Procedure_Done_Status,
                     resultEvt->Subevent_Done_Status,
                     resultEvt->Reference_Power_Level,
                     numSteps);

    writePtr = blt_ras_stepDataProc(writePtr, (u8 *)resultEvt->Step_Mode, resultEvt->Num_Steps_Reported);

    u16 subevtLen = (writePtr - tempSubevtBuf);
    BLC_RAS_DATA_LOG("subevent result end wptr = %x, subPtr = %x, subevtlen = %d", writePtr, tempSubevtBuf, subevtLen);

    if ((resultEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (resultEvt->Subevent_Done_Status == CS_SUBEVT_ABORT)) {
        BLC_RAS_DATA_LOG("subevent done----------------------------------");
        u8 *pSubevt = (u8 *)malloc_nonreten(subevtLen);
        if (pSubevt == NULL) {
            if (tempSubevtBuf != NULL) {
                free_nonreten(tempSubevtBuf);
                procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt = NULL;
                procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = 0;
                tempSubevtBuf = NULL;
            }
            BLC_RAS_DATA_LOG("ERROR! Out of memory spot 002");
            goto failed2;
        }
        procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt   = pSubevt;
        procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = subevtLen;
        memcpy(pSubevt, tempSubevtBuf, subevtLen);
        free_nonreten(tempSubevtBuf);
        tempSubevtBuf = NULL;

        //REAL REALTIME
        u8 last = FALSE;

        if ((resultEvt->Procedure_Done_Status == CS_PROC_DONE) || (resultEvt->Procedure_Done_Status == CS_PROC_ABORT)) {
            last = TRUE;
        }
        /*this function is only executed on the server side.*/
        blt_rass_procedureDataReadyIntermediate(connHandle, procCtrl, last);
        procCtrl->subEvtNum++;
    } else { //update pointers and pass to "continue"
        procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt   = tempSubevtBuf;
        procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = subevtLen;
        BLC_RAS_DATA_LOG("csSubeventResult tbcont pSubEvt %x, subEvtLen %d", procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt, procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen);
    }
    BLC_RAS_DATA_LOG("result start rangCtr %d, procStart %x, procLen %d, pSubEvt %x, subevtnum %d", procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen, procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt, procCtrl->subEvtNum);
    debugwait();

    if ((resultEvt->Procedure_Done_Status == CS_PROC_DONE) || (resultEvt->Procedure_Done_Status == CS_PROC_ABORT)) {
        BLC_RAS_DATA_LOG("procedure done----------------------------------");

        if(BLE_SUCCESS != blt_rass_IsRealTimeReport(connHandle)){
            //on server side.realtime report not merge subevent data to procedure.
            blt_ras_mergeSubevtsToProcedure(procCtrl);
            dataCtrl->storedNum++;

            #if (LL_CS_SNIFFER_MODE_ENABLE)
                if(procCtrl->proc.dataLen){
                    blc_rasc_local_ranging_data_evt_t evt = {
                        .connHandle = connHandle,
                        .dataPtr    = procCtrl->proc.pData,
                        .dataLen    = procCtrl->proc.dataLen,
                    };
                    blt_prf_sendEvent(connHandle, CS_EVT_LOCAL_RANGING_DATA, &evt, sizeof(evt));
                }
            #endif
        }
        
        blt_rasc_setLocalDataReady(connHandle);
        /*avoid this function re-entry, this function is only called in mainloop*/
        blt_rasc_issueDataReadyAppEvent(connHandle);/* executing it here must ensure that blt_ll_cs_loop_hci_subevent() can only be executed in the mainloop */

        BLC_RAS_DATA_LOG("resultdata: pDataCtrl %x pProcCtrl %x rangCtr %d, procStart %x, procLen %d", dataCtrl, procCtrl, procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen);
        debugwait();


#if (RAS_TIMEOUT_EN)
        procCtrl->timestamp = stimer_get_tick();
        BLC_RAS_DATA_LOG("Setting timestamp:%lu for rangCtr:%d", procCtrl->timestamp, procCtrl->rangingCounter);
#endif
        /*this function is only executed on the server side.*/
        blt_rass_procedureDataReady(connHandle, procCtrl->rangingCounter);
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
failed2:
    return HCI_ERR_LIMIT_REACHED;
}

ble_sts_t blc_ras_csSubeventResultContinueData(hci_le_csSubeventResultContinueEvt_t *continueEvt)
{
    u16                connHandle = continueEvt->Connection_Handle;
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_data_ctrl_t *dataCtrl  = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
    s8                   procIndex = dataCtrl->storedNum;
    blt_ras_proc_ctrl_t *procCtrl  = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[procIndex]);

    u8 *tempSubevtBuf = procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt;
    u16 subevtLen     = procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen;
    u8  offset        = 0;

    if (procCtrl->subEvtNum == 0) {
        offset = PROCEDURE_HEAD_LEN;
    }

    //update subevent header information
    u8 *writePtr = (u8 *)(tempSubevtBuf + offset + member_sizeof(blc_rass_data_body_t, startAclConnEvent) + member_sizeof(blc_rass_data_body_t, frequencyCompensation)); //subevent header skip u16 Start_ACL_Conn_Event and u16 Frequency_Compensation
    BLC_RAS_DATA_LOG("continue wPtr %x", writePtr);
    U8_TO_STREAM(writePtr, ((continueEvt->Procedure_Done_Status & 0x0F) | (continueEvt->Subevent_Done_Status << 4)));
    U8_TO_STREAM(writePtr, continueEvt->Abort_Reason);                                                                                                                   //RAS_d0.9r04

    writePtr += 1;                                                                                                                                                       //skip "Reference Power Level [SubeventIndex]"
    u8 numSteps = *writePtr + continueEvt->Num_Steps_Reported;                                                                                                           //increase the previously stored numSteps is subevent header
    BLC_RAS_DATA_LOG("continue origNumSt %d wPtr %x, numStAfter %d", *writePtr, writePtr, numSteps);

    U8_TO_STREAM(writePtr, numSteps);

    //go to the end of currently handled subevevt
    writePtr = (u8 *)(tempSubevtBuf + subevtLen);

    writePtr = blt_ras_stepDataProc(writePtr, (u8 *)continueEvt->Step_Mode, continueEvt->Num_Steps_Reported);

    subevtLen = (writePtr - tempSubevtBuf);

    BLC_RAS_DATA_LOG("continue wPtr = %x, numStReported = %d", writePtr, continueEvt->Num_Steps_Reported);
    BLC_RAS_DATA_LOG("continue result end wptr = %x, subPtr = %x, subevtlen = %d, status = %d", writePtr, tempSubevtBuf, subevtLen, continueEvt->Subevent_Done_Status);

    if ((continueEvt->Subevent_Done_Status == CS_SUBEVT_DONE) || (continueEvt->Subevent_Done_Status == CS_SUBEVT_ABORT)) {
        BLC_RAS_DATA_LOG("subevent done----------------------------------");
        u8 *pSubevt = (u8 *)malloc_nonreten(subevtLen);
        if (pSubevt == NULL) {
            BLC_RAS_DATA_LOG("ERROR! Out of memory spot 004");
            if (tempSubevtBuf != NULL) {
                free_nonreten(tempSubevtBuf);
                tempSubevtBuf = NULL;
                procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt = NULL;
                procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = 0;
            }
            goto failed2;
        }
        procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt   = pSubevt;
        procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = subevtLen;
        memcpy(pSubevt, tempSubevtBuf, subevtLen);
        free_nonreten(tempSubevtBuf);
        tempSubevtBuf = NULL;

        //REAL REALTIME
        u8 last = FALSE;

        if ((continueEvt->Procedure_Done_Status == CS_PROC_DONE) || (continueEvt->Procedure_Done_Status == CS_PROC_ABORT)) {
            last = TRUE;
        }
        /*this function is only executed on the server side.*/
        blt_rass_procedureDataReadyIntermediate(connHandle, procCtrl, last);
        procCtrl->subEvtNum++;
    } else { //update pointers and pass to "continue"
        procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt   = tempSubevtBuf;
        procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen = subevtLen;
        BLC_RAS_DATA_LOG("csSubeventResultContinue tbcont pSubEvt %x, subEvtLen %d", procCtrl->subEvtData[procCtrl->subEvtNum].pSubEvt, procCtrl->subEvtData[procCtrl->subEvtNum].subEvtLen);
    }

    if ((continueEvt->Procedure_Done_Status == CS_PROC_DONE) || (continueEvt->Procedure_Done_Status == CS_PROC_ABORT)) {
        BLC_RAS_DATA_LOG("procedure done----------------------------------");
        if(BLE_SUCCESS != blt_rass_IsRealTimeReport(connHandle)){
            //on server side.realtime report not merge subevent data to procedure.
            blt_ras_mergeSubevtsToProcedure(procCtrl);
            dataCtrl->storedNum++;

            #if (LL_CS_SNIFFER_MODE_ENABLE)
                if(procCtrl->proc.dataLen){
                    blc_rasc_local_ranging_data_evt_t evt = {
                        .connHandle = connHandle,
                        .dataPtr    = procCtrl->proc.pData,
                        .dataLen    = procCtrl->proc.dataLen,
                    };
                    blt_prf_sendEvent(connHandle, CS_EVT_LOCAL_RANGING_DATA, &evt, sizeof(evt));
                }
            #endif
        }
        blt_rasc_setLocalDataReady(connHandle);
        /*avoid this function re-entry, this function is only called in mainloop*/
        blt_rasc_issueDataReadyAppEvent(connHandle);/* executing it here must ensure that blt_ll_cs_loop_hci_subevent() can only be executed in the mainloop */

#if (RAS_TIMEOUT_EN)
        procCtrl->timestamp = stimer_get_tick();
        BLC_RAS_DATA_LOG("Setting timestamp:%lu for rangCtr:%d", procCtrl->timestamp, procCtrl->rangingCounter);
#endif
        /*this function is only executed on the server side.*/
        blt_rass_procedureDataReady(connHandle, procCtrl->rangingCounter);
    }

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
failed2:
    return HCI_ERR_LIMIT_REACHED;
}

//Do not use blt_ras_rollingSegmentToIndex with RAS_LOST_SEGMENT_WILDCARD
u8 blt_ras_rollingSegmentToIndex(u8 rollingSegment, u8 segmentCount)
{
    // if(RAS_LOST_SEGMENT_WILDCARD == rollingSegment) {
    //     return RAS_LOST_SEGMENT_WILDCARD;
    // }

    if (rollingSegment == 1 || rollingSegment == 3) {
        return 0; //first
    }

    if (rollingSegment & 0x02) {
        // if(segmentCount != RAS_LOST_SEGMENT_WILDCARD) {
        return (segmentCount - 1); //last one
        // }
        // else {
        //  //TODO: Here we make a HUGE assumption we dont go over the rollingSegment, but as we dont know segmentCount in this spot...
        //     //Compiler will merge it with the final return. Its written explicitly, so its clear whats happening
        //     (rollingSegment >> 2);
        // }
    }
    return (rollingSegment >> 2);
}

u8 blt_ras_indexToRollingSegment(u8 index, u8 segmentCount)
{
    if (RAS_LOST_SEGMENT_WILDCARD == index) {
        return RAS_LOST_SEGMENT_WILDCARD;
    }

    if (index == 0) {
        if (index == (segmentCount - 1)) {
            return 3; //first and last
        }
        return 1;     //first
    }

    if (index == (segmentCount - 1)) {
        return ((index << 2) | 0x02);
    }
    //also when segmentCount == RAS_LOST_SEGMENT_WILDCARD
    return (index << 2);
}

ble_sts_t blc_rap_clearAndInitializeLocal(u16 connHandle)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_clearAndInitializeLocal(rasDataset); //no persistence between connections

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

blc_ras_query_result_t blc_rap_procedureQuery(u16 connHandle, u16 rangingCounter)
{
    blc_ras_query_result_t queryData = {0};

    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    if (dataCtrl->storedNum < 1) {
        return queryData;
    }

    for (int i = 0; i < dataCtrl->storedNum; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
        BLC_RAS_DATA_LOG("pProcCtrl: %x i: %d", procCtrl, i);
        debugwait();
        BLC_RAS_DATA_LOG("queryIndex pDataCtrl:%x, pProcStAddr: %x", dataCtrl, dataCtrl->procCtrl[i].proc.pData);
        debugwait();

        if (rangingCounter == (procCtrl->rangingCounter & 0xFFF)) {
            queryData.pData   = procCtrl->proc.pData;
            queryData.dataLen = procCtrl->proc.dataLen;

            BLC_RAS_DATA_LOG("blc_rap_procedureQuery pDataCtrl %x pProcCtrl %x rangCtr %d, procStart %x, procLen %d", dataCtrl, procCtrl, procCtrl->rangingCounter, procCtrl->proc.pData, procCtrl->proc.dataLen);
            debugwait();
            break;
        }

        if (dataCtrl->storedNum == i + 1) {
            BLC_RAS_DATA_LOG("blc_rap_procedureQuery index not found %d", rangingCounter);
        }
    }
failed:
    return queryData;
}

ble_sts_t blc_rap_procedureDeleteLocal(u16 connHandle, u16 rangingCounter)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }
    blt_ras_procedureDeleteLocal(rasDataset, rangingCounter);
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_protocolDataToProcedureData(u16 connHandle, u8 *inputData, u32 inputDataLen, u8 *outputData, u32 *outputDataLen)
{
    blt_ras_dataset_t *localRasDataset = blc_ras_getDataset(connHandle);
    if (!localRasDataset) {
        goto failed;
    }

    //INPUT
    blt_ras_proc_ctrl_t remoteProtCtrl;
    remoteProtCtrl.proc.pData     = inputData;
    remoteProtCtrl.proc.dataLen   = inputDataLen;
    u16 extractedRangingCounter   = blc_ras_extractRangingCounter(remoteProtCtrl.proc.pData);
    remoteProtCtrl.rangingCounter = extractedRangingCounter;

    //OUTPUT
    blt_ras_proc_ctrl_t remoteProcCtrl;
    remoteProcCtrl.proc.pData   = outputData;
    remoteProcCtrl.proc.dataLen = 0;

    //decompress from ras protocol format to regular format - done in procedure_ctrl_buf_ble buffer
    blt_rasc_protocolDataToProcedureData(&remoteProcCtrl, &remoteProtCtrl, localRasDataset);

    *outputDataLen = remoteProcCtrl.proc.dataLen;
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

/* PTS_TESTING */
#if(RAS_IOPTEST_ENABLE)
void blc_ras_iop_initConfig(u16 connHandle, u8 role, u8 rttType)
{
    blt_ras_dataset_t* rasDataset = blc_ras_getDataset(connHandle);
    if(!rasDataset) {
        goto failed;
    }

    for(u8 i = 0; i < RAS_MAX_CS_CONFIG; i++) {
        rasDataset->config[i].valid = TRUE;
        rasDataset->config[i].role = role ? CS_CONFIG_REFLECTOR_ROLE : CS_CONFIG_INITIATOR_ROLE;
        rasDataset->config[i].rttType = rttType;
    }

failed:
    return;
}
#endif