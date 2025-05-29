/********************************************************************************************************
 * @file    libcs_tlk1.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifdef __cplusplus
extern "C" {
#endif

#define CS_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define CS_ALGO1_VERSION CS_VERSION_INT(1, 1, 69)

#define JAGUAR 0
#define TERCEL 1

#include "config.h"
#if (CHIP_TYPE == CHIP_TYPE_TL721X) || (CHIP_TYPE == CHIP_TYPE_TL322X)
#define ICMODE TERCEL
#else
#define ICMODE JAGUAR
#endif

//system const
#define INITIATOR 0
#define REFLECTOR 1

#define IFFREQ (1e6)
#define MAXOSR (4)
#define PI (3.141592653589793)
#define SPEEDOFLIGHT (299792458)
#define SAMPLERATE (4e6)
#define FIF (1e6)

#define MAXIQLEN (804) // 416 for 32bits, so 800 for 128bits

//fcs
#define MAXFCSLEN (100*MAXOSR) //80us in standard

//tesCollectData
#define MAXTESLEN (50*MAXOSR) // 40/20/10 us in standard
#define CALIFACTOR (0.015625) //bit width for cali array

//pesCollectData
#define MAXRSLEN (128) //128bits
#define MAXGAUSSLEN (7)
#define RTTN (250)
#define MAXPESCORRWIN (10)

//tesCalcDistance
#define MAXCHANNUM 80
#define MAXMATLEN 40
#define CBPFDELAY (4.6023e-13)


#define FLOATPOINT 0
#define ANDES 1

#if  FLOATPOINT
#define DIGITTYPE float
#else
#if ANDES
#include <nds_math_types.h>
#define DIGITTYPE int //#define DIGITTYPE q31_t
#else
#define DIGITTYPE __INT32_TYPE__
#endif
#endif

struct __attribute__((packed))  complex {
    DIGITTYPE real;
    DIGITTYPE imag;
};
typedef struct __attribute__((packed))  complex complex;


//Receive the return value of the TES initialization stage and provide it to the
//algorithm for distance calculation.
struct __attribute__((packed))  parameterConstTes {
    float cPhase2R;
    float firstPeakTolx;
    DIGITTYPE invDen;
    DIGITTYPE firstPeakMinLevel;
    float tol2;
    float invPREC;
    float invPRECSQRT;
    float l2s2R;
    float x[MAXCHANNUM];
    float xPow;
    float fstep;
    int channum;
    int matlen;
    int hmatlen;
    int numsep;
    int meandiv;
    float stepSize;
};
typedef struct __attribute__((packed))  parameterConstTes parameterConstTes;


struct __attribute__((packed))  parameterPesCollectDataSDK {
    int fclk;
    int n;
    int role;
    float tick2halfns;
    int osr;
    signed char internalDelay[79];
};
typedef struct __attribute__((packed))  parameterPesCollectDataSDK parameterPesCollectDataSDK;

struct __attribute__((packed))  parameterPesCalcDistanceSDK {
    int n;
    float xTick;
};
typedef struct __attribute__((packed))  parameterPesCalcDistanceSDK parameterPesCalcDistanceSDK;

//Receive the return value of the PES initialization stage and provide it to the
//algorithm for distance calculation.
struct __attribute__((packed))  parameterConstPes {
    int fclk;
    float syncClk2SampClk;
    float SampClk2syncClk;
    int searchMedian;
    int startPos;
    int corrWin;
    int osr;
    int bits;
};
typedef struct __attribute__((packed))  parameterConstPes parameterConstPes;

struct __attribute__((packed))  parameterConstNadm {
    float adThr[2];
    float adStep[2];
    float adStepInv[2];
    float drr[MAXRSLEN*MAXOSR];
    float data0[(MAXRSLEN + 2) * MAXOSR + MAXGAUSSLEN];
    float data1[(MAXRSLEN+2)*MAXOSR];
    int idxOffset;
};
typedef struct parameterConstNadm parameterConstNadm;


/*
 * @brief           This function is mainly used to return the version number of the algorithm library.
 * @param[in]       none.
 * @return          The version of hadm.a.
 */
int get_version(void);

/*
 * @brief           This function is mainly used to estimate the frequency offset information through IQ data.
 * @param[in]       IQData  -   IQ data collected during the FCS phase.
 * @param[in]       IQLen   -   The number of groups of IQ data collected during the FCS phase.
 * @param[in]       cfoCoarse   -   coarse CFO(carrier frequency offset)estimation value.
 * @param[in]       sampleRate  -   Sampling frequency of the FCS stage.
 * @return          The frequency offset between initiator and reflector.
 */
__attribute__((section(".ram_code")))
float calcFreq(float IQData[], int IQLen, float cfoCoarse, float sampleRate);

//pes for SDK
__attribute__((section(".ram_code")))
parameterPesCollectDataSDK pesCollectDataInitSDK(int n, int role, int dataRate, signed char internalDelay[], int ICMode);
parameterPesCalcDistanceSDK pesCalcDistanceInitSDK(int nAverage);
__attribute__((section(".ram_code")))
int calcPesInfoSDK(int tx_timestamp[], int sync_timestamp[], int t_sy_center_delta, char chan_idx[], short cte_sync[], parameterPesCollectDataSDK para);
__attribute__((section(".ram_code")))
int calcPesInfoFine(int tx_timestamp[], double sync_timestamp[], int t_sy_center_delta, char chan_idx[], short cte_sync[], parameterPesCollectDataSDK para);
int calcPesPct(int IQData[], int IQLen, float cfo, int soundingSeqLen, int iq_start, int iq_sync, int offsetsMin, int offsetsMax, int *packet_pct, parameterPesCollectDataSDK paraPesSDK);

float pesCalcDistSDK(short cte_sync1[], short cte_sync2[], int sync_flag[],float distSync1[], parameterPesCalcDistanceSDK para);
//pesCollectData
parameterConstPes pesInit(int fclk, int dataRate, int searchMedian, int corrWin, int bits);
parameterConstNadm nadmInit(float adThr[], float adStep[], int idxOffset);
double calcFineSyncAA(int iqData[], int IQLen, int iq_start_tstamp1, int iq_sync_tstamp1, int aaSeq[], parameterConstPes paraPes);
double calcFineSyncAARS(int iqData[], int IQLen, int iq_start_tstamp1, int iq_sync_tstamp1, int aaSeq[], int rsSeq[], int rsSeqLen, int *maxPos, parameterConstPes paraPes);
int calcPesNadm(int IQData[], int IQLen, int rsSeq[], int rsSeqLen, int maxPos, int adType, float *rdm,  parameterConstPes para, parameterConstNadm paraNadm);


//tesCollectData
#define COMPRESIDUECFO 0
__attribute__((section(".ram_code")))
float calcTesInfoAsicHard(int IQData[], int IQLen, int cfo, int ICMode, signed char* ampFactor, int *realValOut, int *imagValOut);
__attribute__((section(".ram_code")))
int calcTesInfoAsicHardFix(int IQData[], int IQLen,int qualityLevels[], signed char* ampFactor, int *realValOut, int *imagValOut);

__attribute__((section(".ram_code")))
int calcTesInfoAsicSoft(int realVal, int imagVal, int iq_timeStamp, int trx_timeStamp, const float fae, const int t_pm_center_delta,const int role, float if_adjustment, const signed char cali[], int ICMode, DIGITTYPE output[]);

/*
 * @brief           This function is mainly used to calculate the information of Tes.
 * @param[in]       IQData          -   The address where the compensation value is stored.
 * @param[in]       compArr         -   The number of groups of IQ data collected.
 * @param[in]       IQLen           -   The step value of the angle.
 * @param[in]       iq_timeStamp    -   The timestamp when the IQ data collection begins.
 * @param[in]       trx_timeStamp   -   TX RX conversion time.
 * @param[in]       cfo             -   The frequency offset at both ends of initiator and reflect, plus and minus signs are determined by the character.
 * @param[in]       if_adjustment   -   Frequency compensation value.
 * @param[in]       cali            -   calibration value.
 * @param[in]       output          -   Processed IQ data.
 * @return          raw data of Tone Quality Indicator.
 */
float calcTesInfo(int IQData[], int IQLen, int iq_timeStamp,  int trx_timeStamp, const float fae, const int t_pm_center_delta,const int role, float if_adjustment, const signed char  cali[],int ICMode, DIGITTYPE output[]);
/*
 * @brief           This function is mainly used to compress IQ data collected during the tes phase.
 * @param[in]       ipm     -   IQ data processed by calcTesInfo.
 * @param[in]       IQLen   -   The number of groups of IQ data collected.
 * @return          0:success; others:false.
 */
__attribute__((section(".ram_code")))
int compressTesInfo(DIGITTYPE ipm[], signed char ampFactors[], int len, int bits, float rpl_before, int rpl_max, int rpl_min);

//tesCalcDistance
/*
 * @brief           This function is mainly used for initializing the distance calculation algorithm in TES mode.
 * @param[in]       none.
 * @return          The parameter of distance calculate.
 */
parameterConstTes tesInit(int channum, float fstep, float stepSize);

/*
 * @brief           This function is mainly used to calculate the compensation value.
 * @param[in]       I[]     -   IQ data processed by initiator.
 * @param[in]       Q[]     -   IQ data processed by reflector.
 * @param[in]       cali    -   Calibration compensation.
 * @param[in]       H2WR[]  -   The address where the results of the operation are stored.
 * @return          0:success; others:false.
 */
int calcIpmPct(int I[], int rpl_I[], int Q[], int rpl_Q[], complex H2WR[], parameterConstTes para);

/*
 * @brief           This function is used to estimate the distance by means of phase difference.
 * @param[in]       H2WR[]  -   IQ data processed by calcIpmPct.
 * @param[in]       T2WR[]  -   Intermediate calculation results.
 * @param[in]       T2WRDiffMean    -   Intermediate calculation results.
 * @param[in]       para    -   The results produced by tesInit.
 * @return          The result of distance.
 */
float tesPhase(const complex H2WR[],  float T2WR[], float* T2WRDiffMean, float* likeliness, parameterConstTes para);

/*
 * @brief           This function is used to estimate the distance by means of phase difference.
 * @param[in]       H2WR[]  -   IQ data processed by calcIpmPct.
 * @param[in]       T2WR[]  -   Intermediate calculation results.
 * @param[in]       T2WRDiffMean    -   Intermediate calculation results.
 * @param[in]       para    -   The results produced by tesInit.
 * @param[in]       likeliness -    Variables for debugging.
 * @param[in]       nIterMaxEig -   Variables for debugging.
 * @param[in]       nIterPS     -   Intermediate calculation results.
 * @param[in]       nSigCnt     -   Intermediate calculation results.
 * param[in]        EVDCap      -   Intermediate calculation results.
 * @return          The result of distance.
 */
float tesMusic(complex H2WR[], float T2WR[], float T2WRDiffMean, parameterConstTes para, float* likeliness, int* nIterMaxEig, int* nIterPS, int* nSigCnt, float* EVDCap);
float tesMusic2(complex H2WR[], float T2WR[], float T2WRDiffMean, parameterConstTes para, float* likeliness, int* nIterMaxEig, int* nIterPS, int* nSigCnt, float* EVDCap);
float tesPhaseMedian(const complex H2WR[], float T2WROffsetIn, parameterConstTes para);

//tesCombineDistance
float distCombine(float distN, int stepSizeN, int distTypeN, float distM, int stepSizeM, int distTypeM);

#ifdef __cplusplus
}
#endif
