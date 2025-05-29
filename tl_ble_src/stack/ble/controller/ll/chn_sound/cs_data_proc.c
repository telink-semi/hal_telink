/********************************************************************************************************
 * @file    cs_data_proc.c
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
#include "algorithm/hadm/gcc10/tlk_algo1/include/libcs_tlk1.h"
#include "algorithm/hadm/gcc10/cs_cal.h"

#include <math.h>

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)

    #if OS_SUP_EN
        #include "stack/ble/os_sup/os_sup.h"
        #include "stack/ble/os_sup/os_sup_stack.h"
    #endif

//role _support can set by API init


    #ifndef DBG_CS_LOG
        #define DBG_CS_LOG 1
    #endif

/***********global variables need only one copy in multi config/ reflector and initiator******************/
rf_packet_cs_t       pkt_CS;
rf_packet_cs_mode1_t pkt_CS_m1;
rf_packet_cs_mode0_t pkt_CS_m0;
u32                  cs_tick_tx_on;
u8                   cs_rx_agc_gain;
/***********************************************************************************/

_attribute_data_retention_ u16 pctRawBuffIdx = 0;
_attribute_data_retention_ int g_pctRawDataBuff[PCT_SIZE * STEP_NUM_PER_SUBEVENT];
_attribute_data_retention_ u8  g_csDataResultEvtBuff[CS_SUBEVENT_BUFF_LEN_MAX];

_attribute_data_retention_ char ampFactors[(MAX_ANT_PATHS_SUPPORT + 1) * STEP_NUM_PER_SUBEVENT] = {0}; // max 160 steps per subevent
_attribute_data_retention_ int ampFactorsCnt                                                   = 0;

#if(GOOGLE_LR20_CALI_EN)
_attribute_data_retention_ s8 chip_intlDly_calVal[79 * 2] = {35,41,42,35,40,39,38,41,23,51,38,40,35,44,40,39,37,43,41,40,42,38,49,29,46,34,27,51,55,20,45,36,52,28,48,35,53,27,55,24,59,9,55,24,54,28,57,13,58,12,54,24,52,29,56,21,57,18,60,0,57,18,57,19,61,4,60,-12,60,10,50,35,61,-7,62,-8,61,-14,59,21,62,8,54,-32,62,-14,61,2,61,-1,61,-1,56,-25,57,-24,61,-12,62,-8,55,-28,55,-29,60,-19,63,7,63,-7,45,-45,49,-42,59,-26,61,-21,44,-44,51,-35,60,-15,58,-23,55,-31,45,-44,42,-47,63,-11,46,-45,36,-54,50,-41,55,-33,54,-36,22,-62,36,-56,43,-51,51,-43,42,-53,35,-58,42,-54};
#else
_attribute_data_retention_ s8 chip_intlDly_calVal[79 * 2] = {63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0, 63, 0};
#endif
_attribute_data_retention_ s8 fcal_cali_table[79] = {0};
//_attribute_data_retention_ u16 g_mode2IQ_len = 0;


/***********global variables for hadm alg******************/
_attribute_ble_data_retention_ cs_rx_fifo_t cs_rx_fifo;


//(SIGNAL + NOISE) / NOISE
#define CS_THRESHOLD_GOOD   80 //17.9;//286.0;//SNR Good threshold
#define CS_THRESHOLD_BAD    30 //4.1;//2.25;//SNR Bad threshold


signed char blt_cs_calPctRpl(cs_step_value_t *pStep, u8 stepNum, u8 agcGain);

/* Now the internal delay value is same, not cali and read from flash address FLASH_CHIP_INTERNAL_DELAY_CALI_2M_FLASH.
 * 13300--26600--28m
 * 13370--26740--8m
 * 13373--26746--6m
 * 13372--26744
 * The cs_mode1_phy1M_internalDelay value is set 26712, the value 26744 is from branch "banch_keyCar",
 * value 26712 is better than value 26744, test with 17m cable.
 */

_attribute_data_retention_ s8 cs_mode1_phy1M_internalDelay[79] = {0};

    #if 0
float cs_if_adjustment79[LL_CS_CHANNEL_NUM_MAX] = {-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,
        122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,
        -61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,
        -61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,
        122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,
        -61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,
        -61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,
        122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,
        -61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,
        -61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,
        122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500,122.0703125000,
        -61.0351562500,-61.0351562500,122.0703125000,-61.0351562500,-61.0351562500};

static float fae79[79] = {-122.0703,0,-61.0352,-122.0703,0,-61.0352,-122.0703,0,-61.0352,-122.0703,0,-61.0352,
        -122.0703,0,-61.0352,-122.0703,0,-61.0352,-122.0703,0,-61.0352,-122.0703,0,-61.0352,-122.0703,0,-61.0352,
        -122.0703,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,
        -61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,
        61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,-61.0352,61.0352,0,
        -61.0352,61.0352}; // copy from driver
    #else
s8 cs_if_adjustment79[LL_CS_CHANNEL_NUM_MAX] = {-1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1, 2, -1, -1};

static s8 fae79[79] = {-2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, -2, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1, 0, -1, 1}; // copy from driver
    #endif

    //s8 cs_mode0_rssi = 0x7F;
    /***********************************************************************************/

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_
        u16
    blt_cs_calcMaxProcLenSubevtCount(cs_config_t *pCsCfg)
{
    u16 maxSubEvtCnt = 0;
    u16 csEvtCnt;
    u8  subEvtFloor = 0;

    st_ll_conn_t *pAcl = (st_ll_conn_t *)(u32)&blms[pCsCfg->aclHandle & CONN_IDX_MASK];

    // debug
    #if (0)
    CS_LL_LOG("Calculate max Subevent Count according to max_procedure_len");
    CS_LL_LOG("max_Procedure_len:%d", pCsCfg->Max_Procedure_Len);
    CS_LL_LOG("Acl interval:%d", pAcl->conn_intvl_n_1m25);
    CS_LL_LOG("Event_Interval:%d", pCsCfg->Event_Interval);
    CS_LL_LOG("subevent_per_event:%d", pCsCfg->Subevents_Per_Event);
    CS_LL_LOG("subevent_len:%d", pCsCfg->Subevent_Len);
    CS_LL_LOG("subevent_interval:%d", pCsCfg->subEvtIntvl_625us);
    #endif

    csEvtCnt          = pCsCfg->Max_Procedure_Len / (2 * pCsCfg->Event_Interval * pAcl->conn_intvl_n_1m25);
    u32 procLenMargin = 625 * (pCsCfg->Max_Procedure_Len - 2 * csEvtCnt * pCsCfg->Event_Interval * pAcl->conn_intvl_n_1m25); // max_procedure_len:625us  conn_interval:1.25ms
    if (pCsCfg->Subevents_Per_Event == 1) {
        if (pCsCfg->Subevent_Len <= procLenMargin) {
            subEvtFloor = 1;
        } else {
            subEvtFloor = 0;
        }
    } else {
        if (procLenMargin < pCsCfg->Subevent_Len) {
            subEvtFloor = 0;
        } else {
            if (procLenMargin > (pCsCfg->Subevent_Len + (pCsCfg->Subevents_Per_Event - 1) * pCsCfg->subEvtIntvl_625us * 625)) {
                subEvtFloor = pCsCfg->Subevents_Per_Event;
            } else {
                subEvtFloor = (procLenMargin - pCsCfg->Subevent_Len) / (pCsCfg->subEvtIntvl_625us * 625) + 1;
            }
        }
    }

    if (procLenMargin - subEvtFloor * pCsCfg->subEvtIntvl_625us * 625 > 0 && subEvtFloor < pCsCfg->Subevents_Per_Event) {
        gCsMng.gGlobal_pCsCfg->subEvtContinue = 1; // not a complete a subEvt, but step can't be ignored!!!
        gCsMng.gGlobal_pCsCfg->subEvtMargin   = procLenMargin - subEvtFloor * pCsCfg->subEvtIntvl_625us * 625;
    }

    maxSubEvtCnt = csEvtCnt * pCsCfg->Subevents_Per_Event + subEvtFloor;

    return maxSubEvtCnt;
}


    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ slip_window_step_t *
    blt_cs_getSlipWindow(void)
{
    u8 slipIdx = gCsMng.blt_pCsCfg->slip_stepReadIdx % SLIP_WINDOW_STEP_NUM;
    return (slip_window_step_t *)&gCsMng.blt_pCsCfg->slip_window_step[slipIdx];
}

    //Assume slip_stepWriteIdx is greater than 1
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ slip_window_step_t *
    blt_cs_getLastSlipWindow(void)
{
    u8 slipIdx = ((gCsMng.blt_pCsCfg->slip_stepReadIdx - 1)% SLIP_WINDOW_STEP_NUM);
    return (slip_window_step_t *)&gCsMng.blt_pCsCfg->slip_window_step[slipIdx];
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ slip_window_step_t *
blt_cs_getNextSlipWindow(void)
{
    u8 slipIdx = ((gCsMng.blt_pCsCfg->slip_stepReadIdx +1)% SLIP_WINDOW_STEP_NUM);
    return (slip_window_step_t *)&gCsMng.blt_pCsCfg->slip_window_step[slipIdx];
}

    /**
 * @brief  This function is used to set normal mode1 packet PDU, just send 2 bytes preamble + 4bytes AA + 4 bits trailer.
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_packetSyncPDU(rf_packet_cs_t *pPkt, u32 access_code, slip_window_step_t *pStep, u8 cs_role)
{
    smemcpy((u8 *)&pPkt->accessAddress, &access_code, 4);
    pPkt->preamble[0] = pPkt->preamble[1] = BIT_IS_SET(pPkt->accessAddress, 0) ? 0x55 : 0xAA;
    pPkt->trailer                         = BIT_IS_SET(pPkt->accessAddress, 31) ? 0xA : 0x5;
    pPkt->shift_sequence                  = 0;

    //rtt type not equal to zero.
    if (pStep->seqMode) {
        if (cs_role == CS_INITIATOR_ROLE) {
            pPkt->shift_sequence = (pStep->step_initRttSeq[0] & 0xf);
            for (int i = 0; i < (pStep->seqLen - 1); i++) {
                pPkt->sequence[i] = (pStep->step_initRttSeq[i] >> 4) | ((pStep->step_initRttSeq[i + 1] & 0xf) << 4);
            }
            pPkt->sequence[pStep->seqLen - 1] = (pStep->step_initRttSeq[pStep->seqLen - 1] >> 4);
        } else if (cs_role == CS_REFLECTOR_ROLE) {
            pPkt->shift_sequence = (pStep->step_reflRttSeq[0] & 0xf);
            for (int i = 0; i < (pStep->seqLen - 1); i++) {
                pPkt->sequence[i] = (pStep->step_reflRttSeq[i] >> 4) | ((pStep->step_reflRttSeq[i + 1] & 0xf) << 4);
            }
            pPkt->sequence[pStep->seqLen - 1] = (pStep->step_reflRttSeq[pStep->seqLen - 1] >> 4);
        }
    }

    pPkt->dma_len = rf_tx_packet_dma_len(SYNC_PDU_2M_LEN + pStep->seqLen);
}

    /**
 * @brief  This function is used to set special mode1 packet PDU, just send 1 byte preamble
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ void
    blt_cs_packetSyncPDU_mode1_cali(rf_packet_cs_mode1_t *pPkt, u32 access_code, slip_window_step_t *pStep, u8 cs_role)
{
    smemcpy((u8 *)&pPkt->accessAddress, &access_code, 4);

    pPkt->preamble[0]    = BIT_IS_SET(pPkt->accessAddress, 0) ? 0x55 : 0xAA;
    pPkt->trailer        = BIT_IS_SET(pPkt->accessAddress, 31) ? 0xA : 0x5;
    pPkt->shift_sequence = 0;

    //rtt type not equal to zero.
    if (pStep->seqMode) {
        if (cs_role == CS_INITIATOR_ROLE) {
            pPkt->shift_sequence = (pStep->step_initRttSeq[0] & 0xf);
            for (int i = 0; i < (pStep->seqLen - 1); i++) {
                pPkt->sequence[i] = (pStep->step_initRttSeq[i] >> 4) | ((pStep->step_initRttSeq[i + 1] & 0xf) << 4);
            }
            pPkt->sequence[pStep->seqLen - 1] = (pStep->step_initRttSeq[pStep->seqLen - 1] >> 4);
        } else if (cs_role == CS_REFLECTOR_ROLE) {
            pPkt->shift_sequence = (pStep->step_reflRttSeq[0] & 0xf);
            for (int i = 0; i < (pStep->seqLen - 1); i++) {
                pPkt->sequence[i] = (pStep->step_reflRttSeq[i] >> 4) | ((pStep->step_reflRttSeq[i + 1] & 0xf) << 4);
            }
            pPkt->sequence[pStep->seqLen - 1] = (pStep->step_reflRttSeq[pStep->seqLen - 1] >> 4);
        }
    }
    pPkt->dma_len = rf_tx_packet_dma_len(SYNC_PDU_1M_LEN_MODE1_CALI + pStep->seqLen);
}

ble_sts_t blc_ll_initCsRxFifo(u8 *pRxBuf, int fifo_size, int fifo_num)
{
    // Check if the input buffer pointer is valid
    if (pRxBuf == NULL) {
        return LL_ERR_INVALID_PARAMETER;
    }

    // Ensure fifo_num is a power of 2 and greater than 3
    if (!IS_POWER_OF_2(fifo_num) || fifo_num <= 3) {
        return LL_ERR_INVALID_PARAMETER;
    }

    // Ensure fifo_size is a multiple of 16
    if ((fifo_size & 15) != 0) {
        return LL_ERR_INVALID_PARAMETER;
    }

    // Initialize FIFO structure
    cs_rx_fifo.num          = fifo_num;
    cs_rx_fifo.mask         = fifo_num - 1;
    cs_rx_fifo.size         = fifo_size;
    cs_rx_fifo.size_div_16  = fifo_size >> 4;
    cs_rx_fifo.p_base       = pRxBuf;
    cs_rx_fifo.rptr         = 0;  // Initialize read pointer
    cs_rx_fifo.wptr         = 0;  // Initialize write pointer
    cs_rx_fifo.pCsRxAddr    = NULL;

    return BLE_SUCCESS;
}

ble_sts_t blc_cs_initCsHciRxFifo(u8 *rxFifo, u16 fifo_size, u8 fifo_num)
{
    if (IS_POWER_OF_2(fifo_num)) {
        gCsMng.hciRxFifoNum  = fifo_num;
        gCsMng.hciRxFifoMask = fifo_num - 1;
    } else {
        return LL_ERR_INVALID_PARAMETER;
    }
    gCsMng.hciRxFifo_b   = rxFifo;
    gCsMng.hciRxFifoSize = fifo_size;
    gCsMng.hciFifoRptr = gCsMng.hciFifoWptr = 0;
    return BLE_SUCCESS;
}

static inline u8 *blt_cs_getCsHciRxFifi(void)
{
    u8 *pCsSubevent = gCsMng.hciRxFifo_b + (((gCsMng.hciFifoWptr & gCsMng.hciRxFifoMask)) * gCsMng.hciRxFifoSize);
    return pCsSubevent;
}


    #if (CAP_CALIB_EN)
        #define CAP_CALIB_LOG_EN 0

_attribute_ram_code_ void capCalib(float freq)
{
    int stepFreqOfst = 2000;

    u8 ana_8a = analog_read(0x8a);

    u8 cap = ana_8a & 0x3f;

    if (cap < 10) {
        stepFreqOfst = 4000;
    } else if (cap < 20) {
        stepFreqOfst = 3000;
    }

    if (freq < 0) {
        stepFreqOfst = -stepFreqOfst;
    }

    if ((freq == 0) || (cap == 0x3f && freq > 0) || (cap == 0 && freq < 0)) {
        tlkapi_printf(CAP_CALIB_LOG_EN, "[CAP] can't adjust:freq:%f,cap:%d,stepFreqOfst:%d", freq, cap, stepFreqOfst);
        return;
    }
    s8 step = (int)((float)(freq + stepFreqOfst / 2) / (abs(stepFreqOfst)));

    if (abs(freq) < 2000) {
        step = 0;
    }

    static u8 capStableCount    = 0;
    static u8 freqAbnormalCount = 0;
        #define ADJUST_THRESHOLD 8
    if (step == 0) {
        capStableCount++;
        if (freqAbnormalCount) {
            freqAbnormalCount--;
        }
        if (capStableCount > ADJUST_THRESHOLD) {
            capStableCount = ADJUST_THRESHOLD;
        }
        tlkapi_printf(CAP_CALIB_LOG_EN, "[CAP] needn't adjust:freq:%f,cap:%d", freq, cap);
        return;
    } else if (capStableCount == ADJUST_THRESHOLD) {
        tlkapi_printf(CAP_CALIB_LOG_EN, "[CAP] freq abnormal:freq:%f,cap:%d,count:%d", freq, cap, capStableCount);
        if (abs(freq) < 5000) {
            return;
        }
        freqAbnormalCount++;
        if (freqAbnormalCount == ADJUST_THRESHOLD) {
            capStableCount = 0;
        }
        return;
    }

    tlkapi_printf(CAP_CALIB_LOG_EN, "[CAP] adjust:freq:%f,cap:%d,stepFreqOfst:%d,step:%d", freq, cap, stepFreqOfst, step);

    int freqThreshold = 15000;

    s8 stepThreshold = 2;

    if (abs(freq) > freqThreshold) {
        stepThreshold = 3;
    }

    if (step > stepThreshold) {
        step = stepThreshold;
    } else if (step < -stepThreshold) {
        step = -stepThreshold;
    }

    if ((cap + step) > 0x3f) {
        cap = 0x3f;
    } else if ((cap + step) < 0) {
        cap = 0;
    } else {
        cap = cap + step;
    }

    gCsMng.capCalibValue = cap;
}

    #endif

    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ static s16
    blt_cs_initiator_M0_getFreOffset(u8 raw_IQ[], cs_rx_para_t *cs_rx_para, u8 chn)
{
    s16 cs_mfo = 0;
    cs_rx_flag  *pRxFlag = (cs_rx_flag  *)&raw_IQ[2];
    if ((cs_rx_para->sync_flag & BIT(3)) && pRxFlag->flag.rx_valid) {
        float cfoCoarse;
        float cs_cfo;
        float IQData_float[LL_CS_STEP_IQ_LEN_MAX];

        u16 sample_num = DMA_CS_RFRX_IQ_DATA_LEN(raw_IQ) / 5;
        blt_ll_cs_Convert20BitIQ2Float(&raw_IQ[DMA_CS_RFRX_OFFSET_IQ_DATA], IQData_float, sample_num);

        s32 rx_freq_offset = blt_ll_cs_getStepRxFreqOffset(BLE_1M_PHY, raw_IQ);
        cfoCoarse          = (float)rx_freq_offset;

        #ifdef MCU_CORE_N22_ENABLE
            //TODO
        #else
            cs_cfo = calcFreq(&IQData_float[0], sample_num, cfoCoarse, SAMPLERATE);
        #endif

        #if (0)
            float cs_angleStep;
            cs_angleStep = 2 * PI * cs_cfo / SAMPLERATE;
            calcCompensate(cs_compArr, sample_num, -cs_angleStep); // don't need calculate cs_compArr
        #endif
        //      mfo = (float)((2402 + chn) * 1000); //kHz
        //      mfo = (cs_cfo / 1000) / mfo;
        //      mfo = mfo * (1e6);//Units: 1 ppm
        float mfo = (float)((cs_cfo) / (2402 + chn));

        cs_mfo = (s16)(mfo * 100); //Units: 0.01 ppm
        #if CAP_CALIB_EN
            capCalib(-cs_cfo);
        #endif
    }
    return cs_mfo;
}


    #if (CS_NADM_EN)

u8 blt_cs_nadm_detect(u8 *raw_pkt, cs_config_t *csCfg, cs_rx_para_t *cs_rx_para, parameterPesCollectDataSDK paraPesSDK)
{
    u8 rtt_seqBytes = RTT_Type_SeqNum[csCfg->RTT_Type];
    u8 rtt_seqLen   = rtt_seqBytes * 8;

    u32 rx_iq_start_tstamp    = cs_rx_para->iq_start_tstamp; //start point of tx on
    u32 rx_pkt_iq_sync_tstamp = cs_rx_para->timestamp;

    int   initial_IQData[1024];
    int   rtt_code[rtt_seqBytes * 8];
    u32   cs_rx_rttSeq[4];
    float rdm       = 0.0;
    int   offsetMin = 54;
    int   offestMax = 66;
    u8    adType    = 1;


    //  for(int i = 0; i < rtt_seqBytes>>2; i++){
    //      BYTE_TO_UINT32(cs_rx_rttSeq[i], &raw_pkt[DMA_CS_RFRX_OFFSET_RTT_SEQ(raw_pkt) + 4*i]);
    //  }
    //

    //  for (u16 i = 0; i < rtt_seqLen; i++){
    //      rtt_code[i] = (cs_rx_rttSeq[i/32] >> i) & 1;
    //  }

    // TODO temp feature of NADM, will verify function with moreph30
    blt_ll_cs_Convert20BitIQ2int(&raw_pkt[0], &initial_IQData[0], rtt_seqLen * 2 * 4);

    u8 packetNADM = calcPesNadm(initial_IQData, rtt_seqLen * 2 * 4, 0, rtt_code, rtt_seqLen, adType, rx_iq_start_tstamp, rx_pkt_iq_sync_tstamp, offsetMin, offestMax, &rdm, paraPesSDK);

    return packetNADM;
}

    #endif // #if (CS_NADM_EN)

_attribute_ram_code_ short blt_cs_m1_process(u8 *raw_pkt, cs_config_t *csCfg, cs_rx_para_t *cs_rx_para, u8 *pPktQulity, u8 *pktNADM, u8 csChannel)
{
    (void)pktNADM;
    short cte = 0x8000;
    cs_rx_flag  *pRxFlag = (cs_rx_flag*)&raw_pkt[2];

    if ((pRxFlag->flag.rx_valid) && (cs_rx_para->sync_flag & BIT(3))) {
        u32 tx_on_start_tstamp    = cs_rx_para->tx_on_tstamp; //start point of tx on
        u32 rx_pkt_iq_sync_tstamp = cs_rx_para->timestamp;
        u32 tx_iq_start_tstamp    = cs_rx_para->iq_start_tstamp;

        if (tx_on_start_tstamp != rx_pkt_iq_sync_tstamp) {
            int dataRate = 1e6; // for 1M case now

            #if (MODE1_FINE_RTT &&((CHIP_TYPE == CHIP_TYPE_TL721X)|| (CHIP_TYPE == CHIP_TYPE_TL322X)))
                u32 cs_rx_accessAddr = cs_rx_para->rx_access_address;
                int cs_aaCode[CS_ACCESS_ADDRESS_BIT_SIZE];
                for (u16 i = 0; i < CS_ACCESS_ADDRESS_BIT_SIZE; i++)
                {
                    cs_aaCode[i] = (cs_rx_accessAddr >> i) & 1;
                }

                int searchMedian = MODE1_MEDIAN_POS;     //This value is based on driver test, 1M case
                int corrWin = MODE1_SEARCH_RANGE;       //will search IQ form startPos to startPos + corrWin
                int fclk = 8e6;                         //clk rate of timestamps
                int bits = 12;                          //internal precision of data

                #ifdef MCU_CORE_N22_ENABLE
                    //TODO
                #else
                    parameterConstPes paraPes = pesInit(fclk, dataRate, searchMedian, corrWin, bits);
                #endif

                int iq_data[(IQ_CAL_MODE1_AA_ONLY + 4*MAX_MODE1_SEQ_BITS)*2];


                int rsSeq[MAX_MODE1_SEQ_BITS] = {0};
                u8 seq_len[7] = {0,32,96,32,64,96,128};
                /*RTT_Type_AA_Only   = 0,
                RTT_Type_32bit_ss  = 1,
                RTT_Type_96bit_ss  = 2,
                RTT_Type_32bit_rs  = 3,
                RTT_Type_64bit_rs  = 4,
                RTT_Type_96bit_rs  = 5,
                RTT_Type_128bit_rs = 6,*/
                int rsSeqLen = seq_len[csCfg->RTT_Type & 7];


                blt_ll_cs_Convert20BitIQ2int(&raw_pkt[4+IQ_CAL_START], &iq_data[0], (IQ_CAL_MODE1_AA_ONLY + rsSeqLen *4));

                double fte_sync = 0;
                int maxPos = 0;

                if(csCfg->RTT_Type != RTT_Type_AA_Only){
                    get_pnSeq(rsSeq, &cs_rx_para->step_RttSeq[0], rsSeqLen);
                    #ifdef MCU_CORE_N22_ENABLE
                        //TODO
                    #else
                        fte_sync = calcFineSyncAARS(iq_data, IQ_CAL_MODE1_AA_ONLY + rsSeqLen *4,
                                                    tx_iq_start_tstamp,
                                                    rx_pkt_iq_sync_tstamp,
                                                    cs_aaCode, rsSeq, rsSeqLen, &maxPos, paraPes);
                    #endif
                }else{
                    #ifdef MCU_CORE_N22_ENABLE
                        //TODO
                    #else
                        fte_sync = calcFineSyncAA(iq_data, IQ_CAL_MODE1_AA_ONLY,tx_iq_start_tstamp,rx_pkt_iq_sync_tstamp, cs_aaCode, paraPes);
                    #endif
                }

            #endif

            #ifdef MCU_CORE_N22_ENABLE
                //TODO
            #else
                /**
                 *  First param: number of step
                 *  Second param: role, INITIATOR---0,REFLECTOR---1
                 */
                parameterPesCollectDataSDK paraPesSDK = pesCollectDataInitSDK(1, csCfg->Role, dataRate, cs_mode1_phy1M_internalDelay, ICMODE);

                #if (MODE1_FINE_RTT &&((CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)))
                    calcPesInfoFine((int*)&tx_on_start_tstamp, &fte_sync, csCfg->t_sy_center_delta, (char *)&csChannel, &cte, paraPesSDK);
                #else
                    calcPesInfoSDK((int*)&tx_on_start_tstamp, (int *)&rx_pkt_iq_sync_tstamp, csCfg->t_sy_center_delta, (char *)&csChannel, &cte, paraPesSDK);
                #endif
                #if (CS_NADM_EN)
                    *pktNADM = blt_cs_nadm_detect(raw_pkt, csCfg, cs_rx_para, paraPesSDK);
                #endif
            #endif
        } else {
            *pPktQulity = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
        }
    }
    return cte;
}

    /*
 *  ant_path = 1 , rx data len = (40+40 + 5)*20 + 44 = 1744 = 0x6D0  DMA len = 1744 = 0x6d0
 *  mode2IQ_StartIdx = early_Us *20 +4  = 104
 *  mode2IQ_RxIntval = T_PM + SW = (40 + 0 ) *20 = 800
 */
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_ int
    process_mode2(cs_step_value_t *pStep, u8 csChannel, cs_config_t *csCfg, u8 *raw_pkt, cs_rx_para_t *cs_rx_para)
{
    int   pct[2];
    int toneQuality;
    s32   initial_IQData[320]; // T_PM maximum 40us ->160 sample -> 320 of s32

    cs_rx_flag  *pRxFlag = (cs_rx_flag  *)&raw_pkt[2];
    //T_RD = 5us
    /*
     * For CS step mode-2, the time period between reception and transmission of the center
     * of the CS tone at the antenna port is nominally expressed as:
     * T_PM_CENTER_DELTA = (T_PM + T_SW) × (N_AP + 1) + T_RD + T_IP2
     */
    int t_pm_center_delta = (csCfg->T_SW_Us + csCfg->T_PM_Us) * (csCfg->antennaPathNum + 1) + 5 + csCfg->T_IP2_Us;

    u32 tick_cs_proc_start = cs_rx_para->tick_cs_proc_start;
    u32 rx_iq_start_tstamp = cs_rx_para->iq_start_tstamp;

    #if (CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)
        u32 tx_turnaround_time = (csCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) ? cs_rx_para->tx_frac_time_pos : cs_rx_para->last_tx_pos_tstamp;
    #else
        u32 tx_turnaround_time = (csCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) ? cs_rx_para->pkt_tx_neg_tstamp : cs_rx_para->last_tx_pos_tstamp;
    #endif

    u8 ext_slot = pRxFlag->flag.ext_slot;
    u16 valid_pm_start = csCfg->mode2IQ_StartIdx;

    cs_step_mode2_t *pMode2           = (cs_step_mode2_t *)(&pStep->data[0]);
    pMode2->Antenna_Permutation_Index = cs_rx_para->ant_path_perm_idx;

    for (int atn_path_cnt = 0; atn_path_cnt < (csCfg->antennaPathNum + 1); atn_path_cnt++)
    {
        if ((atn_path_cnt < (csCfg->antennaPathNum + ext_slot)) && pRxFlag->flag.rx_valid)
        {
            blt_ll_cs_Convert20BitIQ2int(&raw_pkt[valid_pm_start], &initial_IQData[0], csCfg->mode2IQ_ValidPMLen);

            // note: for now, we use medium 20us, discard the first and the last 10us, IQ_OffsetTick is the medium tick for each tone.
            #if (CHIP_TYPE == CHIP_TYPE_TL721X)||(CHIP_TYPE == CHIP_TYPE_TL322X)
                    u32 IQ_OffsetTick = (csCfg->mode2IQ_OffsetTick + (csCfg->T_PM_Us + csCfg->T_SW_Us) * atn_path_cnt * SYSTEM_TIMER_TICK_1US) / 3;
            #else
                    u32 IQ_OffsetTick = csCfg->mode2IQ_OffsetTick + (csCfg->T_PM_Us + csCfg->T_SW_Us) * atn_path_cnt * SYSTEM_TIMER_TICK_1US;
            #endif

            int realValOut, imagValOut;
            char ampFactor = 0;
            int rawQualityLevels[] = {CS_THRESHOLD_BAD, CS_THRESHOLD_GOOD};
            int rawQualityLevelLen = sizeof(rawQualityLevels)/sizeof(int);

            #ifdef MCU_CORE_N22_ENABLE
                //TODO
            #else
                toneQuality   = calcTesInfoAsicHardFix(&initial_IQData[0], csCfg->mode2IQ_ValidPMLen, rawQualityLevels, &ampFactor, &realValOut, &imagValOut);
            #endif

            // note: chip internal delay cali value is relevant with csChannel, if without cali value, use chip_intlDly_noCalVal
            // Now, chip_intlDly_calVal is used for TLSR9528B, need cali for 9528A later.
            float fae_channel              = ((float)fae79[csChannel]) * 61.0351562500;
            float cs_if_adjustment_channel = ((float)cs_if_adjustment79[csChannel]) * 61.0351562500;

            #ifdef MCU_CORE_N22_ENABLE
                //TODO
            #else
                calcTesInfoAsicSoft(realValOut,
                                    imagValOut,
                                    (u32)(rx_iq_start_tstamp + IQ_OffsetTick - tick_cs_proc_start),

                                    (u32)(tx_turnaround_time - tick_cs_proc_start),
                                    fae_channel,
                                    t_pm_center_delta,
                                    csCfg->Role,
                                    cs_if_adjustment_channel,
                                    &chip_intlDly_calVal[csChannel * 2],
                                    ICMODE,
                                    pct);
            #endif

            #if (CS_DATA_DEBUG_LOG_EN)
                u32 quality = toneQuality[atn_path_cnt];
                tlkapi_send_string_u32s(DBG_CS_LOG, "chn&quality", (csChannel), (quality));
            #endif

            #if (LL_CS_SNIFFER_MODE_ENABLE)
                tlkapi_printf(DBG_CS_STEP_DATA_EN, "[STK][CS] mode2 chn&quality: 0x%x,%d\r\n", csChannel, toneQuality);
            #endif

            //            pMode2->Tone[atn_path_cnt].Tone_Quality_Indicator = blt_ll_cs_getToneQualityIndicator(toneQuality);
            pMode2->Tone[atn_path_cnt].Tone_Quality_Indicator = toneQuality;
                //save PCT for RPL calculate
            #if (LL_CS_SNIFFER_MODE_ENABLE)
                    if (pctRawBuffIdx > (PCT_SIZE * STEP_NUM_PER_SUBEVENT - 2)) {
                        tlkapi_printf(DBG_CS_DATA_EN, "[STK][CS] mode2 pctRawBuffIdx overflow: %d\r\n", pctRawBuffIdx);
                        pctRawBuffIdx = 0;
                    }
            #endif
            g_pctRawDataBuff[pctRawBuffIdx++] = pct[0];
            g_pctRawDataBuff[pctRawBuffIdx++] = pct[1];

            #if (LL_CS_SNIFFER_MODE_ENABLE)
                    if (ampFactorsCnt > (((MAX_ANT_PATHS_SUPPORT + 1) * STEP_NUM_PER_SUBEVENT) - 2)) {
                        tlkapi_printf(DBG_CS_DATA_EN, "[STK][CS] mode2 ampFactorsCnt overflow: %d\r\n", ampFactorsCnt);
                        ampFactorsCnt = 0;
                    }
            #endif
            ampFactors[ampFactorsCnt++] = ampFactor;

            valid_pm_start += csCfg->mode2IQ_RxIntval;

        } else {
            pMode2->Tone[atn_path_cnt].Tone_Quality_Indicator = CS_STEP_RECEIVE_TONE_QUALITY_LOW;
            smemset(pMode2->Tone[atn_path_cnt].Tone_PCT, 0, 3);
        }

        if ((atn_path_cnt == csCfg->antennaPathNum)) { //extension slot
            if (ext_slot) {
                pMode2->Tone[atn_path_cnt].Tone_Quality_Indicator |= BIT(5);
            } else {
                pMode2->Tone[atn_path_cnt].Tone_Quality_Indicator |= BIT(4);
            }
        }
    }

    //when calculate PCT/RPL,need to know whether ext slot is used to send or receive tone.
    //after calculating PCT/RPL,need to clear the relevant bit.
    pStep->mode    = STEP_MODE_2 | (ext_slot << 7);
    pStep->channel = csChannel;
    pStep->len     = 1 + 4 * (csCfg->antennaPathNum + 1); //Antenna_Permutation_Index(1) + PCT(3) + qulity(1)
    return 0;
}





    #if (0)

    #else

        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
process_mode0(cs_step_value_t *pStep,  u8 csChannel, cs_config_t *csCfg, u8 *raw_pkt, cs_rx_para_t *cs_rx_para)
{
    s8 packetRSSI    = 0x7F;
    u8 packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;

    cs_step_mode0_t *pMode0 = (cs_step_mode0_t *)(&pStep->data[0]);
    cs_rx_flag  *pRxFlag = (cs_rx_flag  *)&raw_pkt[2];

    if (pRxFlag->flag.rx_valid && (cs_rx_para->sync_flag & BIT(3)))
   {
       packetQuality = blt_ll_cs_getPktMatchSyncQuality(raw_pkt);
       packetRSSI    = cs_rx_para->rssi - 110;
    }

    pStep->mode    = STEP_MODE_0;
    pStep->channel = csChannel;
    pMode0->Packet_Quality = packetQuality;
    pMode0->Packet_RSSI    = packetRSSI;
    pMode0->Packet_Antenna = 1;

    #if (LL_CS_SNIFFER_MODE_ENABLE)
        tlkapi_printf(DBG_CS_STEP_DATA_EN, "[STK][CS] mode0 chn&RSSI: 0x%x,%d\r\n", csChannel, pMode0->Packet_RSSI);
    #endif
    /*
     * 48MHZ init:808us
     */
    if (csCfg->Role == CHANNEL_SOUNDING_ROLE_INITIATOR) {
        pMode0->Measured_Freq_Offset = blt_cs_initiator_M0_getFreOffset(raw_pkt, cs_rx_para, csChannel);
            /* TODO:According core spec, mfo of mode0 step should be -10000~100000(-100ppm~100ppm), but current mfo may exceed the range un-normally.
         * When using L4 calculate, will return error code -71(L4_HADM_BLE_MODE0_MEAS_FREQ_OFFSET_INVALID). Now, just make sure MFO valid,
         * our algo will not be influenced, should have a deep debug for this issue. -- yuexin20241203
         */
        #if (CS_TLK_ALGO2_EN)
            if (pMode0->Measured_Freq_Offset >= 10000 || pMode0->Measured_Freq_Offset <= -10000) {
                pMode0->Measured_Freq_Offset = 100;
            }
        #endif
        pStep->len = CS_STEP_DATA_LENGTH_MODE0_INITIATOR;
    } else {
        pStep->len = CS_STEP_DATA_LENGTH_MODE0_REFLECTOR;
    }
    return 0;
}

        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
    process_mode1(cs_step_value_t *pStep, u8 csChannel, cs_config_t *csCfg, u8 *raw_pkt, cs_rx_para_t *cs_rx_para)
{
    s8 packetRSSI    = 0x7F;
    u8 packetQuality = CS_STEP_RECEIVE_PACKET_QUALITY_LOW;
    short cte        = 0x8000;
    u8    packetNADM = ATTACK_UNKNOWN;
    cs_rx_flag  *pRxFlag = (cs_rx_flag  *)&raw_pkt[2];

    if (pRxFlag->flag.rx_valid && (cs_rx_para->sync_flag & BIT(3)))
   {
       packetQuality = blt_ll_cs_getPktMatchSyncQuality(raw_pkt);
       packetRSSI    = cs_rx_para->rssi - 110;
    }


    pStep->mode    = STEP_MODE_1;
    pStep->channel = csChannel;

    cte = blt_cs_m1_process(raw_pkt, csCfg, cs_rx_para, &packetQuality, &packetNADM, csChannel);

    cs_step_mode1_t *pMode1 = (cs_step_mode1_t *)(&pStep->data[0]);
    pMode1->Packet_Quality  = packetQuality;
    pMode1->Packet_NADM     = packetNADM; // todo
    pMode1->Packet_RSSI     = packetRSSI;
    pMode1->ToA_ToD[0]      = U16_LO(cte);
    pMode1->ToA_ToD[1]      = U16_HI(cte);
    pMode1->Packet_Antenna  = 1;

    if (csCfg->RTT_Type) {
        //packet pct
        pStep->len = CS_STEP_DATA_LENGTH_MODE1_RTT_SOUNDING;
        smemset(pMode1->Packet_PCT1, 0, 4);
        smemset(pMode1->Packet_PCT2, 0, 4);
    } else {
        pStep->len = CS_STEP_DATA_LENGTH_MODE1_RTT_AA_ONLY;
    }
    return 0;
}

_attribute_ram_code_ int handle_abort_cs_subevent(cs_config_t *csCfg, cs_rx_para_t *cs_rx_para, u8 *pHciRxFifo)
{
    tlkapi_send_string_data(Google_SRS,"[CS][Mode0 Channel Map] Subevt abort",NULL,0);
    hci_le_csSubeventResultEvt_t *pSubEvent = (hci_le_csSubeventResultEvt_t *)(pHciRxFifo + SUBEVENT_DATA_OFFSET);
    pSubEvent->Subevent_Code                = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT;
    if (gCsMng.blt_pCsCfg->test_mode_en) {
        pSubEvent->Connection_Handle = 0x0fff;                             // just for test mode
    } else {
        pSubEvent->Connection_Handle = csCfg->aclHandle;                   //cs_rx_para->conn_handle;
    }
    pSubEvent->Config_ID              = csCfg->Config_ID;                  //Config_ID
    pSubEvent->Frequency_Compensation = 0xC000;                            //Frequency compensation value is not available, or the role is not initiator
    pSubEvent->Reference_Power_Level  = 10;
    pSubEvent->Procedure_Done_Status  = cs_rx_para->procedure_done_status; //procedure done
    pSubEvent->Subevent_Done_Status   = CS_SUBEVENT_DONE_STATUS_ABORTED;   // subevent done
    pSubEvent->Abort_Reason           = (cs_rx_para->subevent_done_status & 0xf0);
    pSubEvent->Num_Antenna_Paths      = csCfg->antennaPathNum;
    pSubEvent->Num_Steps_Reported     = 0;
    pSubEvent->Start_ACL_Conn_Event   = cs_rx_para->start_acl_conn_event;
    pSubEvent->Procedure_Counter      = cs_rx_para->procedure_counter;
    *(u16 *)pHciRxFifo                = sizeof(hci_le_csSubeventResultEvt_t);
    gCsMng.hciFifoWptr++;
    //    if(hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT)
    //    {
    //        hci_le_csSubeventResult_evt(0, 0, g_csDataResultEvtBuff, 16);
    //    }

    return 0;
}

_attribute_ram_code_ int handle_complete_cs_subevent(cs_config_t *csCfg, cs_rx_para_t *cs_rx_para, u8 stepReportCnt, u16 stepDataOffset, u8 *pHciRxFifo)
{
    tlkapi_send_string_data(Google_SRS,"[CS][Mode0 Channel Map] Subevt success",NULL,0);
    hci_le_csSubeventResultEvt_t *pEvt = (hci_le_csSubeventResultEvt_t *)(pHciRxFifo + SUBEVENT_DATA_OFFSET);

    if (pctRawBuffIdx != 0) {                                                    //only mode2 will run calculate PCT/RPL
        pEvt->Reference_Power_Level = blt_cs_calPctRpl((cs_step_value_t *)&pEvt->Step_Mode, stepReportCnt,
                                                       cs_rx_para->rx_agc_gain); //need to notice length limitation. or maybe find error result.
        tlkapi_send_string_data(CS_DATA_DEBUG_LOG_EN, "RPLrtn", &pEvt->Reference_Power_Level, 1);
    } else {
        pEvt->Reference_Power_Level = 10;                                        //10 dBm   //TODO
    }

        #if (LL_CS_SNIFFER_MODE_ENABLE)
    tlkapi_send_string_u8s(DBG_CS_STEP_DATA_EN, "[STK][CS] subevent agc gain", cs_rx_para->rx_agc_gain);
        #endif
    if (gCsMng.blt_pCsCfg->test_mode_en) {
        pEvt->Connection_Handle = 0x0fff; // just for test mode
    } else {
        pEvt->Connection_Handle = csCfg->aclHandle;
    }
    pEvt->Subevent_Code          = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT;
    pEvt->Config_ID              = csCfg->Config_ID;
    pEvt->Frequency_Compensation = 0xC000;                                            //Frequency compensation value is not available, or the role is not initiator
    pEvt->Procedure_Done_Status  = cs_rx_para->procedure_done_status;                 //procedure done
    pEvt->Subevent_Done_Status   = CS_SUBEVENT_DONE_STATUS_COMPLETE;                  // subevent done
    pEvt->Abort_Reason           = ((cs_rx_para->procedure_done_status & 0xF0) >> 4); //| (cs_rx_para->subevent_done_status & 0xF0);
    pEvt->Num_Antenna_Paths      = csCfg->antennaPathNum;
    pEvt->Num_Steps_Reported     = stepReportCnt;
    pEvt->Start_ACL_Conn_Event   = cs_rx_para->start_acl_conn_event;
    pEvt->Procedure_Counter      = cs_rx_para->procedure_counter;

    #if (CS_DEBUG_MODE)
    if ((blc_cs_getAlgoMask() & BLC_RANGING_ALGORITHM_3) || csCfg->Role == CHANNEL_SOUNDING_ROLE_REFLECTOR) {
        pEvt->Start_ACL_Conn_Event = cs_rx_para->rx_agc_gain;
    }
    #endif

    *(u16 *)pHciRxFifo = stepDataOffset; //to save hci event len
    gCsMng.hciFifoWptr++;
    return 0;
}
        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
    blt_ll_cs_loop_hci_subevent(void)
{
    while (gCsMng.hciFifoRptr != gCsMng.hciFifoWptr) {
        u8 *pHciRxFifo = gCsMng.hciRxFifo_b +
                         (gCsMng.hciFifoRptr & gCsMng.hciRxFifoMask) * gCsMng.hciRxFifoSize;
        hci_le_csSubeventResultEvt_t *pEvt = (hci_le_csSubeventResultEvt_t *)(pHciRxFifo + SUBEVENT_DATA_OFFSET);
        if (pEvt->Subevent_Done_Status == CS_SUBEVENT_DONE_STATUS_ABORTED) {
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT) {
                hci_le_csSubeventResult_evt(0, 0, (u8 *)pEvt, 16);
            }
        } else {
            u16 stepDataOffset = *(u16 *)pHciRxFifo; //subevent result total len
            if (stepDataOffset <= HCI_EVENT_LEN) {
                if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT) {
                    hci_le_csSubeventResult_evt(0, 0, (u8 *)pEvt, stepDataOffset);
                }
            } else {
                u8  csDataTemp[HCI_EVENT_LEN] = {0};
                u8  event_cnt                 = 0;
                u16 offset                    = 16;
                u16 offset_last               = 0;
                u8  stepReportCnt             = pEvt->Num_Steps_Reported;

                while (stepReportCnt) {
                    u16 len      = (event_cnt == 0) ? 16 : 9;
                    u8  step_num = 0;
                    while ((len < HCI_EVENT_LEN) && stepReportCnt) {
                        u8 step_len = 3 + pHciRxFifo[offset + SUBEVENT_DATA_OFFSET + 2];
                        if ((len + step_len) <= HCI_EVENT_LEN) {
                            offset += step_len;
                            len += step_len;
                            stepReportCnt--;
                            step_num++;
                            if (stepReportCnt == 0) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                    u8 subevent_done = (stepReportCnt > 0) ? CS_SUBEVENT_DONE_STATUS_PARTIAL : CS_SUBEVENT_DONE_STATUS_COMPLETE;
                    if (event_cnt == 0) {
                        smemcpy(csDataTemp, pHciRxFifo + 2, len);
                        offset_last += len;

                        hci_le_csSubeventResultEvt_t *pResult = (hci_le_csSubeventResultEvt_t *)csDataTemp;
                        pResult->Procedure_Done_Status        = pEvt->Procedure_Done_Status ? pEvt->Procedure_Done_Status : subevent_done; //procedure done
                        pResult->Subevent_Done_Status         = subevent_done;                                                             // subevent done
                        pResult->Num_Steps_Reported           = step_num;

                        if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT) {
                            hci_le_csSubeventResult_evt(0, 0, csDataTemp, len);
                        }
                    } else {
                        smemcpy(csDataTemp + 9, pHciRxFifo + offset_last + SUBEVENT_DATA_OFFSET, len - 9);
                        offset_last += len - 9;

                        hci_le_csSubeventResultContinueEvt_t *pResult = (hci_le_csSubeventResultContinueEvt_t *)csDataTemp;
                        pResult->Subevent_Code                        = HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE;
                        pResult->Connection_Handle                    = pEvt->Connection_Handle;
                        pResult->Config_ID                            = pEvt->Config_ID;
                        pResult->Procedure_Done_Status                = pEvt->Procedure_Done_Status ? pEvt->Procedure_Done_Status : subevent_done; //procedure done
                        pResult->Subevent_Done_Status                 = subevent_done;                                                             //subevent done
                        pResult->Abort_Reason                         = pEvt->Abort_Reason;
                        pResult->Num_Antenna_Paths                    = pEvt->Num_Antenna_Paths;
                        pResult->Num_Steps_Reported                   = step_num;

                        if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT_CONTINUE) {
                            hci_le_csSubeventResultContinue_evt(0, 0, csDataTemp, len);
                        }
                    }
                    event_cnt++;
                }
            }
        }
        gCsMng.hciFifoRptr++;
    }
    return 0;
}

        #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
        #endif
    _attribute_ram_code_ int
    blt_ll_cs_data_loop(void)
{
    if (cs_rx_fifo.rptr != cs_rx_fifo.wptr)
    {
        while (cs_rx_fifo.rptr != cs_rx_fifo.wptr) {
            DBG_CS_CHN7_HIGH;
            u8           *raw_pkt    = (u8 *)(cs_rx_fifo.p_base + (cs_rx_fifo.rptr & cs_rx_fifo.mask) * cs_rx_fifo.size);
            cs_rx_para_t *cs_rx_para = (cs_rx_para_t *)(raw_pkt + DMA_CS_RFRX_OFFSET_TIME_STAMP(raw_pkt));
            cs_rx_flag  *pRxFlag = (cs_rx_flag *)&raw_pkt[2];

//            if (pRxFlag->val) // here will be zero in initiator when lost mode0
            {
                u8          *pHciRxFifo = blt_cs_getCsHciRxFifi();
                cs_config_t *csCfg      = cs_rx_para->config_struct_addr;

                u8 csChannel     = raw_pkt[3] & BLT_CS_STEP_CHANNEL_MASK;

                static u8  stepReportCnt  = 0;
                static u16 stepDataOffset = sizeof(hci_le_csSubeventResultEvt_t);

                hci_le_csSubeventResultEvt_t *pSubEvt = (hci_le_csSubeventResultEvt_t *)(pHciRxFifo + SUBEVENT_DATA_OFFSET);
                cs_step_value_t              *pStep   = (cs_step_value_t *)(((u8 *)pSubEvt) + stepDataOffset);

                if (pRxFlag->flag.mode==STEP_MODE_0) {
                    process_mode0(pStep,  csChannel, csCfg, raw_pkt, cs_rx_para);
                } else if (pRxFlag->flag.mode==STEP_MODE_1) {
                    process_mode1(pStep, csChannel, csCfg, raw_pkt, cs_rx_para);
                } else if (pRxFlag->flag.mode==STEP_MODE_2) {
                    process_mode2(pStep, csChannel, csCfg, raw_pkt, cs_rx_para);
                }
                else{
                    return -1;
                }
                stepDataOffset += 3 + pStep->len;
                stepReportCnt++;

                u8 subevent_done         = cs_rx_para->subevent_done_status & 0xf;
                u8 subevent_abort_status = (cs_rx_para->subevent_done_status >> 4) & 0xf;
                if (subevent_done == CS_SUBEVENT_DONE_STATUS_COMPLETE) {
                    if (subevent_abort_status == SUBEVT_NO_ABORT) {
                        handle_complete_cs_subevent(csCfg, cs_rx_para, stepReportCnt, stepDataOffset, pHciRxFifo);
                    } else //if(subevent_abort_status & SUBEVT_ABORT_NO_MODE0_RECEIVED)
                    {
                        handle_abort_cs_subevent(csCfg, cs_rx_para, pHciRxFifo);
                    }

                    pctRawBuffIdx  = 0;
                    stepReportCnt  = 0;
                    stepDataOffset = sizeof(hci_le_csSubeventResultEvt_t);
                }
            }
            cs_rx_fifo.rptr++;
            DBG_CS_CHN7_LOW;
        }
    }

    return 0;
}
    #endif
    #if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
    #endif
    _attribute_ram_code_
    //cs_step_value_t: u8 mode;u8 channel;u8 len;u8 data[0];
    signed char
    blt_cs_calPctRpl(cs_step_value_t *pStep, u8 stepNum, u8 agcGain)
{
    pctRawBuffIdx &= 0x7fff;
    if (pctRawBuffIdx == 0) { //indicate not mode2
        return 0;
    }

    //first: calculate compress PCT.
    tlkapi_send_string_data(CS_DATA_DEBUG_LOG_EN, "pctIdx", &pctRawBuffIdx, 1);
    tlkapi_send_string_data(CS_DATA_DEBUG_LOG_EN, "pctVal_A", g_pctRawDataBuff, (pctRawBuffIdx * 4 + 4)); //pctRawBuffIdx*4

    int gain_lat[6] = {-3, 15, 27, 39, 51, 63};
    if (agcGain > 6) {
        tlkapi_send_string_data(CS_DATA_DEBUG_LOG_EN, "gainIdxErr", &agcGain, 1);
    }
    int gain = gain_lat[agcGain];

    //-44.81 comes from matlab, tx_power + gain(15dB) - 20*log10(abs(IQ/2048))
    float rpl_before = -44.0981 - (gain - 15); //gCsMng.rpl_factor- gain;
    int   rpl_after;
    #ifdef MCU_CORE_N22_ENABLE
        //TODO
    #else
        rpl_after = compressTesInfo(g_pctRawDataBuff, ampFactors, pctRawBuffIdx, 12, rpl_before, 20, -127);
    #endif

    //second: re-write the calculation PCT value to the related position.
    u16              pctOft   = 0;
    cs_step_value_t *pStepVal = pStep;
    for (u16 stepIdx = 0; stepIdx < stepNum; stepIdx++)
    {
        if ((pStepVal->mode & STEP_MODE_2) == STEP_MODE_2) {
            #define TONE_PCT_OFFSET 1 //Antenna_Permutation_Index
            u8 *pData = &pStepVal->data[TONE_PCT_OFFSET];
            //BIT(7) indicate whether exist ext_slot in the stpe cs data.
            //-1 Antenna_Permutation_Index (1byte); -4 indicate Tone_PCT(3) + TQI(1)
            u8 realPCT_len = (pStepVal->mode & BIT(7)) ? (pStepVal->len - 1) : (pStepVal->len - 1 - 4);
            pStepVal->mode &= ~BIT(7); //clear bit7. spec mode format not include ext slot flag.

            for (u32 tmpLen = 0; tmpLen < realPCT_len; tmpLen += 4) {
                pData[tmpLen]     = U16_LO(g_pctRawDataBuff[pctOft * 2]);
                pData[tmpLen + 1] = ((g_pctRawDataBuff[pctOft * 2] & 0xF00) >> 8) | ((g_pctRawDataBuff[pctOft * 2 + 1] & 0xF) << 4);
                pData[tmpLen + 2] = U16_LO((g_pctRawDataBuff[pctOft * 2 + 1] >> 4));
                pctOft += 1;
            }
        }

        pStepVal = (cs_step_value_t *)&pStepVal->data[pStepVal->len];
    }

    pctRawBuffIdx = 0;
    ampFactorsCnt = 0;

    return rpl_after; //need to confirm with lijing/haili, what is the rpl type? int or unsigned char??? here just set according to Reference_Power_Level
}

_attribute_ram_code_
    s32
    blt_ll_cs_getStepRxFreqOffset(u8 phy, u8 *raw_data)
{
    /**
     * rx freq offset = pkt_fdc x bit_rate x 4000 / 256 / 2 / 3.14159
     * 1M PHY, 1M_bit_rate = 1000, 1M_FO = pkt_fdc x 2487
     * 2M PHY, 2M_bit_rate = 2000, 2M_FO = pkt_fdc x 4974
     */
    s32 freq_offset;

    freq_offset = ((raw_data[DMA_CS_RFRX_OFFSET_FREQ_OFFSET(raw_data) + 1] & 0x07) << 8) | raw_data[DMA_CS_RFRX_OFFSET_FREQ_OFFSET(raw_data)];
    freq_offset = ((freq_offset > 0x3ff) ? (freq_offset - 0x800) : freq_offset);
    if (phy == BLE_2M_PHY) {
        freq_offset *= 4974; //Hz
    } else {
        freq_offset *= 2487; //Hz
    }

    return freq_offset;
}

_attribute_ram_code_
    u8
    blt_ll_cs_getPktMatchSyncQuality(u8 *raw_data)
{
    u8 pktSyncQuality = raw_data[DMA_CS_RFRX_OFFSET_PKT_MATCH_SYNC(raw_data)];

    pktSyncQuality = CS_ACCESS_ADDRESS_BIT_SIZE - pktSyncQuality;

    return (pktSyncQuality > 2 ? 2 : pktSyncQuality);
}

/*
 * Divide the sampled IQ data into data[2*i] and data[2*i+1] every 20 bits.
 * If I/Q data is negative, convert it to a positive number
 */
_attribute_ram_code_ void blt_ll_cs_Convert20BitIQ2Float(u8 *data_src, float *data_dest, u16 len_sample)
{
    u32 i;

    for (i = 0; i < len_sample; i++) {
        u32 tempI = data_src[i * 5] + (data_src[i * 5 + 1] << 8) + ((data_src[i * 5 + 2] & 0x0F) << 16);
        u32 tempQ = ((data_src[i * 5 + 2] & 0xF0) >> 4) + (data_src[i * 5 + 3] << 4) + ((data_src[i * 5 + 4]) << 12);

        //Negative numbers are converted to positive numbers
        if (tempI & BIT(19)) {
            tempI -= BIT(20);
        }
        //Negative numbers are converted to positive numbers
        if (tempQ & BIT(19)) {
            tempQ -= BIT(20);
        }

        data_dest[i * 2]     = (float)(s32)tempI;
        data_dest[i * 2 + 1] = (float)(s32)tempQ;
    }
}

/*
 * Divide the sampled IQ data into data[2*i] and data[2*i+1] every 20 bits.
 * If I/Q data is negative, convert it to a positive number
 */
_attribute_ram_code_ void blt_ll_cs_Convert20BitIQ2int(u8 *data_src, s32 *data_dest, u16 len_sample)
{
    u32 i;

    for (i = 0; i < len_sample; i++) {
        u32 tempI = data_src[i * 5] + (data_src[i * 5 + 1] << 8) + ((data_src[i * 5 + 2] & 0x0F) << 16);
        u32 tempQ = ((data_src[i * 5 + 2] & 0xF0) >> 4) + (data_src[i * 5 + 3] << 4) + ((data_src[i * 5 + 4]) << 12);

        //Negative numbers are converted to positive numbers
        if (tempI & BIT(19)) {
            tempI -= BIT(20);
        }
        //Negative numbers are converted to positive numbers
        if (tempQ & BIT(19)) {
            tempQ -= BIT(20);
        }

        data_dest[i * 2]     = tempI;
        data_dest[i * 2 + 1] = tempQ;
    }
}

u8 blt_ll_cs_getToneQualityIndicator(int toneQualityRaw)
{
    u8 toneQuality_indicator;

    if (toneQualityRaw >= CS_THRESHOLD_GOOD) {
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_GOOD;
    } else if (toneQualityRaw < CS_THRESHOLD_BAD) {
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_LOW;
    } else {
        toneQuality_indicator = CS_STEP_RECEIVE_TONE_QUALITY_MEDIUM;
    }

    return toneQuality_indicator;
}

#endif
