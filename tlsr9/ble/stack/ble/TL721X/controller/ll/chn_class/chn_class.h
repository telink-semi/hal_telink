/********************************************************************************************************
 * @file    chn_class.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef CHN_CLASSIFICATION_H_
#define CHN_CLASSIFICATION_H_


/*! \brief   Channel classification reporting enable field. */
#define     CHN_CLASSIFICATION_REPORTING_DISABLE    0 //Disable channel classification reporting
#define     CHN_CLASSIFICATION_REPORTING_ENABLE     1 //Enable channel classification reporting

/*! \brief  Channel classification spacing/delay field (Unit: 200ms) */
#define     LL_CHN_CLASSIFICATION_SPACING_MAX       150 //max
#define     LL_CHN_CLASSIFICATION_SPACING_MIN       5 //min
#define     LL_CHN_CLASSIFICATION_SPACING_UNIT      200

/*! \brief   AFH_Channel_Assessment_Mode field. */
#define     CONTROLLER_CHN_ASSESSMENT_DISABLE       0 //Controller channel assessment disabled
#define     CONTROLLER_CHN_ASSESSMENT_ENABLE        1 //Controller channel assessment ensabled

/*! \brief   Channel classification channel status. */
typedef enum {
    CHN_STATUS_UNKNOWN,
    CHN_STATUS_GOOD,
    CHN_STATUS_RSVD,
    CHN_STATUS_BAD,
    CHN_STATUS_INVALID  = 0xFF,
}chn_status_t;


typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u8  enable;
    u8  minSpacing;
    u8  maxDelay;
}rf_pkt_ll_chn_rpt_ind_t;   //LL_CHANNEL_REPORTING_IND

typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u8  cChnClass[10]; //uint2[37].
}rf_pkt_ll_chn_status_t;   //LL_CHANNEL_STATUS_IND


typedef struct{
    u32     minChnSpacingUs;
    u32     maxChnDelayUs;
    u32     chnStsMonitorRdyTick; /* peripheral role used */
    u32     lastChnStsSendTick;   /* peripheral role used */
    u32     lastChnStsRcvdTick;   /* center role used */
    u32     chnRptIndSendPending; /* center role used */

    bool    chnRptEnable;
    u8      chnStsIndSendPending; /* peripheral role used */
    u8      chnMinSpacing; /* center role used */
    u8      chnMaxDelay;   /* center role used */

    u8      chnc_occpied;
    u8      reserved[3];
}ll_chnc_cb_t;


/**
 * @brief      This function is used to initialize the ChnClassification feature
 * @param[in]  none
 * @return     none
 */
void        blc_ll_initChnClass_feature(void);

/**
 * @brief       This function is used to known whether the Controller's channel
 *              assessment scheme is enabled or disabled.
 * @param[out]  pChnAssMode - .
 * @return      Status     - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_chnclassRdAfhChnAssessmentMode(u8 *pChnAssMode);

/**
 * @brief      This function is used to controls whether the Controller's channel
 *             assessment scheme is enabled or disabled.
 * @param[in]  chnAssMode - 0x00: disable; 0x01: enable.
 * @return     Status     - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_chnclassWrAfhChnAssessmentMode(u8 chnAssMode);

#endif /* CHN_CLASSIFICATION_H_ */
