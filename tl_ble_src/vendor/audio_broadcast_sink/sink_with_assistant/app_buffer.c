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
#include "../sink_config.h"
#if (SINK_VERSION == SINK_WITH_ASSISTANT_VERSION)

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


/********************* ACL connection LinkLayer TX & RX data FIFO allocation, Begin *******************************/

/**
 * @brief   ACL RX buffer, shared by all connections to hold LinkLayer RF RX data.
 *          user should define and initialize this buffer if either ACL Central or ACL Peripheral is used.
 */
_attribute_ble_data_retention_ u8 app_acl_rx_fifo[ACL_RX_FIFO_SIZE * ACL_RX_FIFO_NUM] = {0};


/**
 * @brief   ACL Peripheral TX buffer, shared by all peripheral connections to hold LinkLayer RF TX data.
 *          ACL Peripheral TX buffer should be defined only when ACl connection peripheral role is used.
 */
_attribute_ble_data_retention_ u8 app_acl_per_tx_fifo[ACL_PERIPHR_TX_FIFO_SIZE * ACL_PERIPHR_TX_FIFO_NUM * ACL_PERIPHR_MAX_NUM] = {0};

/******************** ACL connection LinkLayer TX & RX data FIFO allocation, End ***********************************/


/***************** ACL connection L2CAP RX & TX data Buffer allocation, Begin **************************************/
/**
 * @brief   L2CAP TX Data buffer for ACL Central
 *          if GATT server on ACL Central used, this buffer must be defined and initialized.
 *          if GATT server on ACL Central not used, this buffer can be saved.
 */
//_attribute_ble_data_retention_    u8 app_cen_l2cap_tx_buf[ACL_CENTRAL_MAX_NUM * CENTRAL_L2CAP_BUFF_SIZE];


/**
 * @brief   L2CAP RX Data buffer for ACL Peripheral
 */
_attribute_ble_data_retention_ u8 app_per_l2cap_rx_buf[ACL_PERIPHR_MAX_NUM * PERIPHR_L2CAP_BUFF_SIZE];


/**
 * @brief   L2CAP TX Data buffer for ACL Peripheral
 *          GATT server on ACL Peripheral use this buffer.
 */
_attribute_ble_data_retention_ u8 app_per_l2cap_tx_buf[ACL_PERIPHR_MAX_NUM * PERIPHR_L2CAP_BUFF_SIZE];

/***************** ACL connection L2CAP RX & TX data Buffer allocation, End ****************************************/


/************************* BIS SYNC RX ISO PDU buffer FIFO allocation, Begin ******************************/
/**
 * @brief   BIS SYNC RX buffer. size & number defined in app_buffer.h
 */
_attribute_iram_noinit_data_ u8 app_bisSyncRxfifo[BIS_RX_PDU_FIFO_SIZE * BIS_RX_PDU_FIFO_NUM * APP_BIS_NUM_IN_ALL_BIG_SYNC] = {0};

/************************* BIS SYNC RX ISO PDU buffer FIFO allocation, End ********************************/


/************************ BIS SYNC RX ISOAL SDU buffer FIFO allocation, Begin *****************************/
_attribute_iram_noinit_data_ u8 app_bis_sdu_out_fifo[BIS_SDU_OUT_FIFO_SIZE * BIS_SDU_OUT_FIFO_NUM * APP_BIS_NUM_IN_ALL_BIG_SYNC];
/************************ BIS SYNC RX ISOAL SDU buffer FIFO allocation, End  ******************************/


/********************************** BIS BSYNC Global Data Definition, Begin ***************************************/
_attribute_ble_data_retention_ u8 app_bigSyncParam[BIG_SYNC_PARAM_LENGTH * APP_BIG_SYNC_NUMBER];
_attribute_ble_data_retention_ u8 app_bisToatlParam[BIS_PARAM_LENGTH * APP_TOTAL_BIS_NUMBER];
/********************************** BIS BSYNC Global Data Definition, End  ****************************************/

#endif //SINK_VERSION == SINK_WITH_ASSISTANT_VERSION
