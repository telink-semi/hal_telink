/********************************************************************************************************
 * @file    app_buffer.h
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
#pragma once

#include "app_config.h"
#include "tl_common.h"


#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)



/*********************************** Extended ADV data buffer allocation, Begin ************************************/
/**
 * @brief   Extended Advertising Set number and data buffer length
 *
 *  APP_EXT_ADV_SETS_NUMBER:
 *  Number of extended ADV Sets, user set value, should be in range of 1 ~ "ADV_SETS_NUMBER_MAX"
 *
 *  APP_EXT_ADV_DATA_LENGTH:
 *  Extended ADV Data Length, user set value, should be in range of 31 ~ 1650
 *  if ADV Set number bigger than 1, this length should be maximum value of ADV data length for all ADV Sets
 *  if all ADV Set use legacy ADV mode, 31 byte is enough
 *
 *
 *  APP_EXT_SCANRSP_DATA_LENGTH:
 *  Extended ADV Scan Response data Length, user set value, should be in range of 31 ~ 1650
 *  if ADV set number bigger than 1, this length should be maximum value of scan response data length for all ADV Sets
 *  if all ADV Set use legacy ADV mode, 31 byte is enough
 */
#define APP_EXT_ADV_SETS_NUMBER         1   //user set value
#define APP_EXT_ADV_DATA_LENGTH         200 //user set value
//#define APP_EXT_SCANRSP_DATA_LENGTH       200 //user set value



extern u8 app_extAdvSetParam_buf[];
extern u8 app_extAdvData_buf[];
//extern u8 app_extScanRspData_buf[];
/*********************************** Extended ADV data buffer allocation, End** ************************************/




/*********************************** PERID ADV buffer FIFO allocation, Begin ***************************************/

/*
 * @brief   Periodic Advertising Set number and data buffer length
 *
 * APP_PERID_ADV_SETS_NUMBER:
 * Number of Supported Periodic Advertising Sets, no exceed "PERIODIC_ADV_NUMBER_MAX"
 *
 * APP_PERID_ADV_DATA_LENGTH:
 * Maximum Periodic Advertising Data Length. can not exceed 1650.
 */
#define APP_PERID_ADV_SETS_NUMBER       1   //user set value
#define APP_PERID_ADV_DATA_LENGTH       200 //user set value

extern  u8 app_peridAdvSet_buffer[];
extern  u8 app_peridAdvData_buffer[];
/*********************************** PERID ADV buffer FIFO allocation, End ****************************************/




/******************************** BIS BCST TX ISO PDU buffer FIFO allocation, Begin **********************************/

/**
 * @brief   BIS_TX_MAX_PDU is the maximum number of data octets that can be carried in a BIS Data PDU
 * refer to BLE SPEC "Vol 6, Part B, 2.6 ISOCHRONOUS PHYSICAL CHANNEL PDU"
 *
 * usage limitation:
 * 1. should be in range of 0 ~ 251
 */
#define BIS_TX_MAX_PDU                  200  //user set value //LC3 10ms-> 120B, mic 4B plus, 124B enough

/**
 * @brief   BIS BCST TX buffer size & number
 *          BIS BCST TX buffer is shared by all bis_bcst to hold LinkLayer RF TX BIS data.
 *
 * usage limitation for BIS_TX_PDU_FIFO_SIZE:
 * 1. must use BIS_PDU_ALIGN4_TXBUFF to calculate, user can not change !!!
 *
 * usage limitation for BIS_TX_PDU_FIFO_NUM:
 * 1. must be: 2^n, (power of 2)
 * 2. at least 2; recommended value: 2, 4, 8, 16, 32, 64
 */
#define BIS_TX_PDU_FIFO_SIZE            BIS_PDU_ALIGN4_TXBUFF(BIS_TX_MAX_PDU)  //user can not change !!!
#define BIS_TX_PDU_FIFO_NUM             (32)

extern  u8  app_bisBcstTxfifo[];
/****************************** BIS BCST TX ISO PDU buffer FIFO allocation, End **************************************/




/*************************** BIS BCST TX ISOAL SDU buffer FIFO allocation, Begin *************************************/

/*
 * @brief   BIS SDU in buffer
            SDU in is SDU from host to controller, so SDU in buffer is used to hold local SDU data which will send to peer device.
 *
 * BIS_SDU_IN_OCTETS_MAX
 * 1. should be in range of 0 ~ 4095
 *
 * BIS_SDU_IN_FIFO_SIZE
 * 1. must use CAL_IAL_ISO_SDU_FIFO_SIZE to calculate, user can not change !!!
 *
 * BIS_SDU_IN_FIFO_NUM
 * 1. must be: 2^n, (power of 2)
 * 2. at least 2; recommended value: 2, 4, 8, 16, 32, 64
 */
#define BIS_SDU_IN_OCTETS_MAX           200 //user set value //LC3 10ms-> 120B, iso hdr 8B plus, 128B enough
#define BIS_SDU_IN_FIFO_SIZE            CAL_IAL_ISO_SDU_FIFO_SIZE(BIS_SDU_IN_OCTETS_MAX)  //user can not change !!!
#define BIS_SDU_IN_FIFO_NUM             (32)

extern  u8  app_bis_sdu_in_fifo[];
/***************************** BIS BCST TX ISOAL SDU buffer FIFO allocation, End  ************************************/




/********************************** BIS BCST Global Data Definition, Begin *****************************************/

#define APP_BIG_BCST_NUMBER             1   // Number of Supported BIG_BCST number (must <= LL_BIG_BCST_NUM_MAX)
#define APP_BIG_SYNC_NUMBER             0   // Number of Supported BIG_SYNC number (must <= LL_BIG_SYNC_NUM_MAX)
#define APP_BIS_NUM_IN_PER_BIG_BCST     2   // Number of Supported BISes in Per BIG_BCST (must <= LL_BIS_IN_PER_BIG_BCST_NUM_MAX)
#define APP_BIS_NUM_IN_PER_BIG_SYNC     2   // Number of Supported BISes in Per BIG_SYNC (must <= LL_BIS_IN_PER_BIG_SYNC_NUM_MAX)
#define APP_PRESENTATION_DELAY          40*1000 //presentation delay

// The total number of BIS supported by BIG_BCST
#define APP_BIS_NUM_IN_ALL_BIG_BCST     (APP_BIG_BCST_NUMBER * APP_BIS_NUM_IN_PER_BIG_BCST)
// The total number of BIS supported by BIG_SYNC
#define APP_BIS_NUM_IN_ALL_BIG_SYNC     (APP_BIG_SYNC_NUMBER * APP_BIS_NUM_IN_PER_BIG_SYNC)
// The total number of BIS supported by BIG (BCST and SYNC)
#define APP_TOTAL_BIS_NUMBER            (APP_BIS_NUM_IN_ALL_BIG_BCST + APP_BIS_NUM_IN_ALL_BIG_SYNC)

extern  u8  app_bigBcstParam[];
extern  u8  app_bisToatlParam[];

/********************************** BIS BCST Global Data Definition, end  *****************************************/




#endif /* INTER_TEST_MODE */
