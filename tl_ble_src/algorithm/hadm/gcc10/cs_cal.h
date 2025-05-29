/********************************************************************************************************
 * @file    cs_cal.h
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

#ifndef _CS_CAL_H_
#define _CS_CAL_H_

#include "tl_common.h"
#include "stack/ble/ble.h"

typedef struct __attribute__((packed))  {

    signed char refPowerLvl;
    unsigned char antennaPaths;
    unsigned char stepChannel;
    unsigned char *stepStart;
}cs_step_mode_t;

typedef struct __attribute__((packed))  {
    unsigned char stepMode;
    unsigned short stepNums;
    unsigned char modeLen;
    cs_step_mode_t dataCtrl[0];
}cs_step_ctrl_t;

typedef enum{
    BLC_RANGING_ALGORITHM_1                 = BIT(0),
    BLC_RANGING_ALGORITHM_2                 = BIT(1),
    BLC_RANGING_ALGORITHM_3                 = BIT(2),
} blc_ranging_algorithm_enum;

#ifdef HOST_V2_ENABLE
#include "stack/ble/controller/ll/chn_sound/cs_stack.h"
#include "algorithm/hadm/gcc10/tlk_algo1/include/libcs_tlk1.h"

#define BLC_FPU_CALC_UNFINISHED     0x00
#define BLC_FPU_CALC_FINISHED       0x55

//float calcFreq(float IQData[], int IQLen, float cfoCoarse, float sampleRate);
typedef struct
{
    float   *IQdata;
    int     IQLen;
    float   cfoCoarse;
    float   sampleRate;
    float   cs_cfo;

    int     chn;
    float   mfo;
    int     status;
}calcFreq_t;

//parameterPesCollectDataSDK pesCollectDataInitSDK(int n, int role, int dataRate, signed char internalDelay[], int ICMode);
typedef struct
{
    int         n;
    int         role;
    int         dataRate;
    signed char *internalDelay;
    int         ICMode;
    parameterPesCollectDataSDK paraPesSDK;
    int         status;
}pesCollectDataInitSDK_t;


//int calcPesInfoFine(int tx_timestamp[], double sync_timestamp[], int t_sy_center_delta, char chan_idx[], short cte_sync[], parameterPesCollectDataSDK para);
typedef struct
{
    int     tx_timestamp;
    double  sync_timestamp;
    int     t_sy_center_delta;
    char    chan_idx;
    short   cte_sync;
    parameterPesCollectDataSDK parameterPesCollectData;
    int     status;
}calcPesInfoFine_t;

//int calcPesInfoSDK(int tx_timestamp[], int sync_timestamp[], int t_sy_center_delta, char chan_idx[], short cte_sync[], parameterPesCollectDataSDK para);
typedef struct
{
    int     *tx_timestamp;
    int     *sync_timestamp;
    int     t_sy_center_delta;
    char    *chan_idx;
    short   *cte_sync;
    parameterPesCollectDataSDK param;
    int     status;
}calcPesInfoSDK_t;

//u8 blt_cs_nadm_detect(u8 *raw_pkt, cs_config_t *csCfg, cs_rx_para_t *cs_rx_para, parameterPesCollectDataSDK paraPesSDK)
typedef struct
{
    u8              *raw_pkt;
    cs_config_t     *csCfg;
    cs_rx_para_t    *cs_rx_para;
    parameterPesCollectDataSDK  paraPesSDK;
    int             status;
}blt_cs_nadm_detect_t;

//int calcTesInfoAsicHardFix(int IQData[], int IQLen,int qualityLevels[], signed char* ampFactor, int *realValOut, int *imagValOut);
typedef struct
{
    int          *IQData;
    int          IQLen;
    int          *qualityLevels;
    u8           ampFactor;
    int          realValOut;
    int          imagValOut;
    int          toneQuality;
    int          status;
}calcTesInfoAsicHardFix_t;

//int calcTesInfoAsicSoft(int realVal, int imagVal, int iq_timeStamp, int trx_timeStamp, const float fae, const int t_pm_center_delta,const int role, float if_adjustment, const signed char cali[], int ICMode, DIGITTYPE output[]);
typedef struct
{
    float               fae_channel;
    float               cs_if_adjustment_channel;
    int                 realVal;
    int                 imagVal;
    int                 iq_timeStamp;
    int                 trx_timeStamp;
    float               fae;
    int                 t_pm_center_delta;
    int                 role;
    float               if_adjustment;
    signed char         *cali;
    int                 ICMode;
    DIGITTYPE           *output;
    int                 status;
}calcTesInfoAsicSoft_t;

//int compressTesInfo(DIGITTYPE ipm[], signed char ampFactors[], int len, int bits, float rpl_before, int rpl_max, int rpl_min);
typedef struct
{
    DIGITTYPE   *ipm;
    signed char *ampFactors;
    int         len;
    int         bits;
    int         rpl_max;
    int         rpl_min;
    int         gain;
    float       rpl_before;
    int         rpl_after;
    int         status;
}compressTesInfo_t;
#endif

/**
 * @brief       Copy config complete Data for distance calculate algorithm 2
 * @param[in]   pData: config complete Data
 * @param[in]   dataLen: length of config complete Data
 * @return      None
 */
void blc_Algo2_CopyConfigCompleteData(void* pData, u32 dataLen);

/**
 * @brief       Copy procedure enable complete for distance calculate algorithm 2
 * @param[in]   ACI: Antenna Configuration Index used in the CS procedure.
 * @param[in]   tx_power: Power level that it will use for the CS procedures
 * @param[in]   subevent_len:  Selected maximum duration of each CS subevent during the CS procedure
 * @param[in]   subevents_per_event: Number of CS subevents anchored off the same ACL connection event
 * @param[in]   event_interval: Number of ACL connection events between consecutive CS event anchor points
 * @param[in]   procedure_interval: Selected interval between consecutive CS procedures,
 * @param[in]   procedure_count:
 * @return      None
 */
void blc_Algo2_CopyProcedureEnableCompleteData(u8 ACI, s8 tx_power, u32 subevent_len, u8 subevents_per_event,
                                               u16 event_interval, u16 procedure_interval, u16 procedure_count);

/**
 * @brief       to get local procedure data,include channel
 * @param[in]   connHandle: ACL connect handle
 * @param[in]   rangingCounter
 * @return      pointer to local procedure data buffer.
 */
blt_ras_proc_ctrl_t *blc_getLocalProcedureData(u16 connHandle, u16 rangingCounter);
/**
 * @brief       to get remote & local procedure data,include channel
 * @param[in]   connHandle: ACL connect handle
 * @param[in]   *remoteProcedureCtrl: local procedure data buffer.
 * @param[in]   *rangingData: pointer to local data.
 * @param[in]   length: length of local data.
 * @return      errcode
 */
u32 blc_restoreProcedureData(u16 connHandle, blt_ras_proc_ctrl_t *remoteProcedureCtrl,blt_ras_proc_ctrl_t *localProcedureCtrl, u8 *pRangingData);

/**
 * @brief       calculate distance
 * @param[in]   connHandle: ACL connect handle
 * @param[in]   *procCtrlInitiator: Initiator procedure data buffer.
 * @param[in]   *procCtrlReflector: Reflector procedure data buffer.
 * @param[in]   mainMode: procedure main mode
 * @param[out]  pointer to distance.
 * @return      errcode
 */
s32 csCalculateDistance(u16 connHandle, blt_ras_proc_ctrl_t *procCtrlInitiator, blt_ras_proc_ctrl_t *procCtrlReflector, u8 mainMode, float *distance);
/**
 * @brief       enable cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_enableAlgoMask(u32 mask);
/**
 * @brief       enable cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_addAlgoMask(u32 mask);
/**
 * @brief       enable cs algorithm mask
 * @param[in]   algorithmMask: cs algorithm mask
 * @return      none
 */
void blc_cs_removeAlgoMask(u32 mask);

/**
 * @brief       get cs algorithm mask
 * @param[in]   none
 * @return      algorithmMask: cs algorithm mask
 */
u32 blc_cs_getAlgoMask(void);

/**
 * @brief       set cs algorithm 3 threshold
 * @param[in]   cs threshold
 * @return      none
 */
void blc_cs_setAlgo3Thd(float cs_thd_s, float cs_thd_m);

/**
 * @brief       get cs algorithm 3 threshold
 * @param[in]   none
 * @return      threshold
 */
float blc_cs_getAlgo3Thd(void);

/**
 * @brief       Initialize internal delay
 * @param[in]   none
 * @return      none
 */
void blc_cs_initInternalDelay(void);

/**
 * @brief       Initialize cs algorithm3
 * @param[in]   none
 * @return      none
 */
void blc_cs_algo3Init(void);

/**
 * @brief       Initialize cs ranging log
 * @param[in]   none
 * @return      none
 */
void blc_cs_initRangingLog(void);

#endif /*_CS_CAL_H_ */
