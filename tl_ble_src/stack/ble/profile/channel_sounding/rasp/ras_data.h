/********************************************************************************************************
 * @file    ras_data.h
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
#pragma once

/**  local procedure data format:
 +-----------------------------------------------------------------------------------------------------------------------------------------------+
 |          procedure header       |                                            subevent header & data                                           |
 |--------------+------------------+--------------------------------+--------------------------------+--------+----------------------------------|
 |              4byte              |    subevent 0 header & data    |   subevent 1 header & data     | ...... |     subevent n header & data     |
 |--------------+------------------+--------------------------------+--------------------------------+--------+----------------------------------|
                                   |    subevent header | step data |
                                   +--------------------+-----------+
                                   |      8 byte        |      N    |
 * a mode 0 step max length is 3(mode + len + chn) + 5  (step data:Packet_Quality|Packet_RSSI|Packet_Antenna|Measured_Freq_Offset) //cs_step_mode0_t
 * a mode 1 step max length is 3(mode + len + chn) + 14 (step data:Packet_Quality|Packet_NADM|Packet_RSSI|ToA_ToD[2]|Packet_Antenna|Packet_PCT1[4]|Packet_PCT2[4]) //cs_step_mode1_t
 * a mode 2 step max length is 3(mode + len + chn) + 5  (step data:Antenna_Permutation_Index|Tone[]{Tone_PCT[3]|Tone_Quality_Indicator}) // cs_step_mode2_t
 *
 *
 * protocol data / ranging data format:
 * step data not contain step len and step channel information, other information is same as local procedure data.
 *
 *
 */


/**
 * @brief Maximum stored procedure count
 */
#ifndef RAS_PROCEDURE_COUNT
    #define RAS_PROCEDURE_COUNT 2
#endif

/**
 * @brief Ranging header, a part of Ranging data body,length is 4 byte
 * @param procedureCounter    :   12 bit  :   procedure counter - stores only lower 12 bits of the CS procedure
 *        proCountCfgID       :   4  bit  :   CS configuration identifier
 *        selectedTxPower     :   8  bit  :   Transmit power level used for CS procedure. Range: -127 to 20 dBm
 *        numAntennaPaths     :   6  bit  :   antennaPathsMask in RAS protocol format - different data encoding
 *        pctFormat           :   2  bit  :   Phase/IQ format(IQ = 0, phase = 1)
 */
#define RANGING_HEADER_LEN 4

/**
 * @brief Subevent header, a part of Ranging data body,length is 8 byte
 * @param startAclConnEvent         :   16 bit  :   Starting ACL connection event count for the results reported in the event
 *        frequencyCompensation     :   16 bit  :   Frequency compensation value in units of 0.01 ppm (15-bit signed integer)
 *        rangingDoneStatus         :   4  bit  :   0x0 = All results complete for the CS Procedure
 *                                                  0x1 = Partial results with more to follow for the CS procedure
 *                                                  0xF = All subsequent CS Procedures aborted
 *        subeventDoneStatus        :   4  bit  :   0x0 = All results complete for the CS Subevent
 *                                                  0xF = Current CS Subevent aborted
 *        rangingAbortReason        :   4  bit  :   Indicates the abort reason when Procedure Done Status is set to 0xF,
 *                                                  otherwise the default value is set to zero.
 *                                                  0x0 = Report with no abort
 *                                                  0x1 = Abort because of local Host or remote request
 *                                                  0x2 = Abort because filtered channel map has less than 15 channels
 *                                                  0x3 = Abort because the channel map update instant has passed
 *                                                  0xF = Abort because of unspecified reasons
 *        subeventAbortReason       :   4  bit  :   Indicates the abort reason when Subevent Done Status is set to 0xF,
 *                                                  otherwise the default value is set to zero.
 *                                                  0x0 = Report with no abort
 *                                                  0x1 = Abort because of local Host or remote request
 *                                                  0x2 = Abort because no CS_SYNC (mode 0) received
 *                                                  0x3 = Abort because of scheduling conflicts or limited resources
 *                                                  0xF = Abort because of unspecified reasons
 *        referencePowerLevel       :   8  bit  :   Reference power level. Range: -127 to 20. Units: dBm
 *        numStepsReported          :   8  bit  :   Number of steps in the CS Subevent for which results are reported.
 *                                                  In case the Subevent is aborted, the Number Of Steps Reported can be set to zero
 */
#define SUBEVENT_HEADER_LEN 8

/**
 * @brief       Maximum buffer for single subevent data - gets allocated on incoming subevent events / subevent continue events
 *              If the subevent is first, need contain 4 byte ranging header information
 *              The max step count of a single subevent is 160
 */
#define SUBEVENT_DATA_LEN (RANGING_HEADER_LEN + SUBEVENT_HEADER_LEN + CS_SUBEVENT_STEP_LEN_MAX * CS_SUBEVENT_STEP_CNT_MAX)


/**
 * @brief       Expected maximum procedure size
 *              Currently used only for realtime procedures to protocol conversion (memory allocation for protocol buffer)
 *              and in protocol to procedure conversion in blc_ras_calcRangData, where it gets freed right after
 *              A procedure max contain 32 subevents and max 256 steps.
 */
#define PROCEDURE_DATA_LEN (RANGING_HEADER_LEN + CS_SUBEVENT_PER_PROCEDURE_MAX * SUBEVENT_HEADER_LEN + CS_SUBEVENT_STEP_LEN_MAX * CS_STEPS_PER_PROCEDURE_MAX)


/**
 * @brief       Filter filler value on decompression. When RAS ranging data gets converted back to subevent event data format, it fills any missing unknown information with this value.
 */
#define FILTER_UNPACK_FILLER 0xFF

/**
 * @brief       Aborted step filler value on decompression. When RAS ranging data gets converted back to subevent event data format, it fills any missing unknown information with this value.
 */
#define ABORTED_UNPACK_FILLER 0xFF

/**
 * @brief       Filter "filterValue" for mode 0
 */
typedef union __attribute__((packed))
{
    u16 raw;

    struct
    {
        u16 PacketQuality      : 1;
        u16 PacketRssi         : 1;
        u16 PacketAntenna      : 1;
        u16 MeasuredFreqOffset : 1; //field relevant when role == ini
        u16 RFU                : 12;
    } bit;
} blc_ras_filter_mode0_t;

/**
 * @brief       Filter "filterValue" for mode 1
 */
typedef union __attribute__((packed))
{
    u16 raw;

    struct
    {
        u16 PacketQuality : 1;
        u16 PacketNadm    : 1;
        u16 PacketRssi    : 1;
        u16 ToDToA        : 1;
        u16 PacketAntenna : 1;
        u16 PacketPct1    : 1; //field relevant when rtt_type != 0
        u16 PacketPct2    : 1; //field relevant when rtt_type != 0
        u16 RFU           : 9;
    } bit;
} blc_ras_filter_mode1_t;

/**
 * @brief       Filter "filterValue" for mode 2
 */
typedef union __attribute__((packed))
{
    u16 raw;

    struct
    {
        u16 AntennaPermutationIdx : 1;
        u16 TonePctK              : 1;
        u16 ToneQualityIndicatorK : 1;
        u16 AntennaPath1          : 1;
        u16 AntennaPath2          : 1; //field relevance depends on AP number
        u16 AntennaPath3          : 1; //field relevance depends on AP number
        u16 AntennaPath4          : 1; //field relevance depends on AP number
        u16 RFU                   : 9;
    } bit;
} blc_ras_filter_mode2_t;

/**
 * @brief       Filter "filterValue" for mode 3
 */
typedef union __attribute__((packed))
{
    u16 raw;

    struct
    {
        u16 PacketQuality         : 1;
        u16 PacketNadm            : 1;
        u16 PacketRssi            : 1;
        u16 ToDToA                : 1;
        u16 PacketAntenna         : 1;
        u16 PacketPct1            : 1; //field relevant when rtt_type != 0
        u16 PacketPct2            : 1; //field relevant when rtt_type != 0
        u16 AntennaPermutationIdx : 1;
        u16 TonePctK              : 1;
        u16 ToneQualityIndicatorK : 1;
        u16 AntennaPath1          : 1;
        u16 AntennaPath2          : 1; //field relevance depends on AP number
        u16 AntennaPath3          : 1; //field relevance depends on AP number
        u16 AntennaPath4          : 1; //field relevance depends on AP number
        u16 RFU                   : 2;
    } bit;
} blc_ras_filter_mode3_t;

/**
 * @brief       the data structure of RAS Filter.
 */
typedef struct __attribute__((packed))
{
    blc_ras_filter_mode0_t mode0;
    blc_ras_filter_mode1_t mode1;
    blc_ras_filter_mode2_t mode2;
    blc_ras_filter_mode3_t mode3;
} blt_ras_filter_t;

/**
 * @brief       Start procedure. Should be called by app on relevant HCI command execution.
 * @param[in]   *procedureHead: the pointer of procedure parameter.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_ras_csProcedureEnComplete(hci_le_csProcedureEnableCompleteEvt_t *procedureHead);

/**
 * @brief       Handle subevent result event from HCI. Should be called by app on relevant HCI command execution.
 * @param[in]   *resultEvt: the pointer of subevent result event data.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_ras_csSubeventResultData(hci_le_csSubeventResultEvt_t *resultEvt);

/**
 * @brief       Handle subevent result continue event from HCI. Should be called by app on relevant HCI command execution.
 * @param[in]   *resultEvt: the pointer of subevent result continue event data.
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_ras_csSubeventResultContinueData(hci_le_csSubeventResultContinueEvt_t *continueEvt);

/**
 * @brief       the data structure of queried local procedure.
 * pData        - pointer to a complete procedure, NULL if not found
 * dataLen      - length of data
 */
typedef struct __attribute__((packed))
{
    u8 *pData;
    u16 dataLen;
} blc_ras_query_result_t;

/**
 * @brief       Pull ranging counter / procedure counter from the buffer containing a procedure / protocol data
 * warning      - this function will not recognise if the dataPtr points to a procedure / protocol data and simply return what it find at relevant offset
 * @param[in]   dataPtr: pointer to procedure / protocol data
 * @return      ranging counter / procedure counter
 */

u16 blc_ras_extractRangingCounter(u8 *dataPtr);

/**
 * @brief       Return a structure containing pointer and length of the local procedure
 * @param[in]   connHandle: ACL handle
 * @param[in]   rangingCounter: ranging counter of the queried procedure
 * @return      a structure containing a pointer and length of the relevant procedure or empty structure if not found
 */
blc_ras_query_result_t blc_rap_procedureQuery(u16 connHandle, u16 rangingCounter);

/**
 * @brief       Remove a locally stored procedure
 * @param[in]   connHandle: ACL handle
 * @param[in]   rangingCounter: ranging counter of the queried procedure
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rap_procedureDeleteLocal(u16 connHandle, u16 rangingCounter);

/**
 * @brief       A procedure for converting a received RAS ranging data (protocol data) back to subevent result like format (procedure data)
 * @param[in]   connHandle: ACL handle
 * @param[in]   inputData: pointer to protocol data buffer received over RAS
 * @param[in]   inputDataLen: length of the input protocol data buffer
 * @param[in][out]  outputData: pointer to a buffer into which the resulting data will get decoded. Needs to be allocated prior to calling the function.
 * @param[in][out]  *outputDataLen: pointer to a variable with which the output length will get returned
 * @return      BLE_SUCCESS - success
 *              other       - error
 */
ble_sts_t blc_rapc_protocolDataToProcedureData(u16 connHandle, u8 *inputData, u32 inputDataLen, u8 *outputData, u32 *outputDataLen);
