#pragma once
#if 0
#ifdef __cplusplus
extern "C" {
#endif

#define CS_VERSION_INT(major, minor, micro) (((major) << 16) | ((minor) << 8) | (micro))
#define CS_VERSION CS_VERSION_INT(1, 1, 70)
#define JAGUAR 0
#define TERCEL 1

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
//#include <nds_intrinsic.h>
#define DIGITTYPE int
#else
#define DIGITTYPE __INT32_TYPE__
#endif
#endif

struct complex {
    DIGITTYPE real;
    DIGITTYPE imag;
};
typedef struct complex complex;


struct parameterConstTes {
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
typedef struct parameterConstTes parameterConstTes;


struct parameterPesCollectDataSDK {
    int fclk;
    int n;
    int role;
    float tick2halfns;
    int osr;
    signed char internalDelay[79];
};
typedef struct parameterPesCollectDataSDK parameterPesCollectDataSDK;

struct parameterPesCalcDistanceSDK {
    int n;
    float xTick;
};
typedef struct parameterPesCalcDistanceSDK parameterPesCalcDistanceSDK;


struct parameterConstPes {
    int fclk;
    float syncClk2SampClk;
    float SampClk2syncClk;
    int searchMedian;
    int startPos;
    int corrWin;
    int osr;
    int bits;
    double ssPhi2tick;
};
typedef struct parameterConstPes parameterConstPes;


struct parameterConstNadm {
    float adThr[2];
    float adStep[2];
    float adStepInv[2];
    float drr[MAXRSLEN*MAXOSR];
    float data0[(MAXRSLEN + 2) * MAXOSR + MAXGAUSSLEN];
    float data1[(MAXRSLEN+2)*MAXOSR];
    int idxOffset;
};
typedef struct parameterConstNadm parameterConstNadm;
int get_version(void);
//fcs
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
float pesCalcDistSDK(short cte_sync1[], short cte_sync2[], int sync_flag[],float distSync1[], parameterPesCalcDistanceSDK para);
//pesCollectDataFine
parameterConstPes pesInit(int fclk, int dataRate, int searchMedian, int corrWin, int bits);
parameterConstNadm nadmInit(float adThr[], float adStep[], int idxOffset);
double calcFineSyncAA(int iqData[], int IQLen, int iq_start_tstamp1, int iq_sync_tstamp1, int aaSeq[], parameterConstPes paraPes);
double calcFineSyncAARS(int iqData[], int IQLen, int iq_start_tstamp1, int iq_sync_tstamp1, int aaSeq[], int rsSeq[], int rsSeqLen, int *maxPos, parameterConstPes paraPes);
double calcFineSyncAASS(int iqData[], int IQLen, int iq_start_tstamp1, int iq_sync_tstamp1, int aaSeq[], int ssSeq[], int ssSeqLen, int *maxPos, signed char* ampFactor, int packet_pct[], parameterConstPes paraPes);
int calcPesNadm(int IQData[], int IQLen, int rsSeq[], int rsSeqLen, int maxPos, int adType, float *rdm,  parameterConstPes para, parameterConstNadm paraNadm);


//tesCollectData
#define COMPRESIDUECFO 0
__attribute__((section(".ram_code")))
float calcTesInfoAsicHard(int IQData[], int IQLen, int cfo, int ICMode, signed char* ampFactor, int *realValOut, int *imagValOut);
__attribute__((section(".ram_code")))
int calcTesInfoAsicHardFix(int IQData[], int IQLen,int qualityLevels[], signed char* ampFactor, int *realValOut, int *imagValOut);

__attribute__((section(".ram_code")))
int calcTesInfoAsicSoft(int realVal, int imagVal, int iq_timeStamp, int trx_timeStamp, const float fae, const int t_pm_center_delta,const int role, float if_adjustment, const signed char cali[], int ICMode, DIGITTYPE output[]);


float calcTesInfo(int IQData[], int IQLen, int iq_timeStamp, int trx_timeStamp, const float fae, const int t_pm_center_delta,const int role, float if_adjustment, const signed char cali[], int ICMode, DIGITTYPE output[]);
__attribute__((section(".ram_code")))
int compressTesInfo(DIGITTYPE ipm[], signed char ampFactors[], int len, int bits, float rpl_before, int rpl_max, int rpl_min);

//tesCalcDistance
parameterConstTes tesInit(int channum, float fstep, float stepSize);
int calcIpmPct(int I[], int rpl_I[], int Q[], int rpl_Q[], complex H2WR[], parameterConstTes para);
float tesPhase(const complex H2WR[], float T2WR[], float* T2WRDiffMean, float* likeliness, parameterConstTes para);
float tesMusic(complex H2WR[], float T2WR[], float T2WRDiffMean, parameterConstTes para, float* likeliness, int* nIterMaxEig, int* nIterPS, int* nSigCnt, float* EVDCap);
float tesMusic2(complex H2WR[], float T2WR[], float T2WRDiffMean, parameterConstTes para, float* likeliness, int* nIterMaxEig, int* nIterPS, int* nSigCnt, float* EVDCap);
float tesPhaseMedian(const complex H2WR[], float T2WROffsetIn, parameterConstTes para);

//tesCombineDistance
float distCombine(float distN, int stepSizeN, int distTypeN, float distM, int stepSizeM, int distTypeM);

#ifdef __cplusplus
}
#endif
#endif
