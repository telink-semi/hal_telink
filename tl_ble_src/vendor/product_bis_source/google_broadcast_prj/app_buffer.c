/********************************************************************************************************
 * @file    app_buffer.c
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
#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_GOOGLE_BROADCAST_SOURCE)

    #include "app_buffer.h"
    #include "app_config.h"
    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"


/*********************************** Extended ADV data buffer allocation, Begin ************************************/
/**
 * @brief   Extended ADV parameters buffer, used to hold extended ADV parameters which used in SDK.
 *          ADV_SET_PARAM_LENGTH is determined by SDK, user can not change !!!
 */
_attribute_ble_data_retention_ u8 app_extAdvSetParam_buf[ADV_SET_PARAM_LENGTH * APP_EXT_ADV_SETS_NUMBER];


/**
 * @brief   Extended ADV data buffer.
 *          if any ADV Set use ADV data(e.g. Extended, Connectable), this buffer should be defined and initialized.
 *          if no  ADV Set use ADV data(e.g. Extended, Scannable), this buffer is not necessary.
 */
_attribute_ble_data_retention_ u8 app_extAdvData_buf[APP_EXT_ADV_DATA_LENGTH * APP_EXT_ADV_SETS_NUMBER];


/**
 * @brief   Extended Scan Response data buffer.
 *          if any ADV Set use scan response data(e.g. Extended, Scannable), this buffer should be defined and initialized.
 *          if no  ADV Set use scan response data(e.g. Extended, Connectable), this buffer is not necessary.
 */
//_attribute_ble_data_retention_    u8  app_extScanRspData_buf[APP_EXT_SCANRSP_DATA_LENGTH * APP_EXT_ADV_SETS_NUMBER];

/*********************************** Extended ADV data buffer allocation, End** ************************************/


/************************* EXT ADV && PERD ADV buffer FIFO allocation, Begin *******************************/
_attribute_ble_data_retention_ u8 app_peridAdvSet_buffer[PERD_ADV_PARAM_LENGTH * APP_PERID_ADV_SETS_NUMBER];
_attribute_ble_data_retention_ u8 app_peridAdvData_buffer[APP_PERID_ADV_DATA_LENGTH * APP_PERID_ADV_SETS_NUMBER];
/**************************  EXT ADV && PERD ADV buffer FIFO allocation, End *******************************/


/************************* BIS BCST TX ISO PDU buffer FIFO allocation, Begin ******************************/
/**
 * @brief   BIS BCST TX buffer. size & number defined in app_buffer.h
 */
_attribute_ble_data_retention_ u8 app_bisBcstTxfifo[BIS_TX_PDU_FIFO_SIZE * BIS_TX_PDU_FIFO_NUM * APP_BIS_NUM_IN_ALL_BIG_BCST] = {0};

/************************* BIS BCST TX ISO PDU buffer FIFO allocation, End ********************************/


/************************ BIS BCST TX ISOAL SDU buffer FIFO allocation, Begin *****************************/
_attribute_ble_data_retention_ u8 app_bis_sdu_in_fifo[BIS_SDU_IN_FIFO_SIZE * BIS_SDU_IN_FIFO_NUM * APP_BIS_NUM_IN_ALL_BIG_BCST];
/************************ BIS BCST TX ISOAL SDU buffer FIFO allocation, End  ******************************/


/********************************** BIS BCST Global Data Definition, Begin ***************************************/
_attribute_ble_data_retention_ u8 app_bigBcstParam[BIG_BCST_PARAM_LENGTH * APP_BIG_BCST_NUMBER];
_attribute_ble_data_retention_ u8 app_bisToatlParam[BIS_PARAM_LENGTH * APP_TOTAL_BIS_NUMBER];
/********************************** BIS BCST Global Data Definition, End  ****************************************/


#endif
