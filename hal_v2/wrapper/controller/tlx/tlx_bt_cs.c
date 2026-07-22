/********************************************************************************************************
 * @file    tlx_bt_cs.c
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
#include "stack/ble/ble.h"
#include "tlx_bt_cs.h"
#include "tlx_bt_buffer.h"

#if (FREERTOS_ENABLE)
#include "app_port_freertos.h"
#endif



#if (ANTENNA_SWITCHING_AUTO_EN)
//Initialize multi-antenas
cs_ant_switch_config_t ant_cfg = {
    .ant_default_seq_value   = 0,
    .ant_ctrl_seq_base_value = ANTENNA_SWITCHING_CTRL_BASE,
};

#if (CHIP_TYPE == CHIP_TYPE_TL322X)
    rf_cs_ant_switch_ctrl ant_switch_ctrl[] = {
            {ANTENNA_SWITCHING_SEL_0_PIN, ATSEL_0,},
    };
#endif
#endif // #if (ANTENNA_SWITCHING_AUTO_EN)

#if (ANTENNA_SWITCHING_AUTO_EN)
void app_Init_CS_Multi_ANT_Switch(void){
    blc_cs_antenna_switch_config_init(&ant_cfg);
    rf_cs_ant_switch_pin_init(ant_switch_ctrl, sizeof(ant_switch_ctrl)/sizeof(rf_cs_ant_switch_ctrl));
}
#endif
/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void)
{
    blc_ll_initCsReflectorModule();

    //Initialize CS buffer
    blc_ll_initCsConfigParam(app_CsConfigParam, APP_CS_CONFIG_NUM);

    blc_ll_initCsPhyRxFifo(app_cs_rx_buf, CS_RX_FIFO_SIZE, CS_RX_FIFO_NUM);

    blc_ll_initCsTransportRxFifo(app_cs_transport_rx_buf, CS_TRANSPORT_RX_FIFO_SIZE, CS_TRANSPORT_RX_FIFO_NUM);

    // Initialize the channel sounding HCI receive FIFO
    blc_cs_initCsHciRxFifo(hciCsSubeventRxFifo, CS_SUBEVENT_BUFF_LEN_MAX, HCI_RX_FIFO_NUM);

    blc_ll_initCsPctDataBuff(pct_raw_data, CS_PCT_DATA_SIZE, MAX_ANT_PATHS_SUPPORT);

    // Initialize CS Step DRBG Info buffer.
    blc_cs_initStepDRBGInfo(app_cs_stepDRBGInfoBuffer, CS_STEP_DRBG_INFO_SIZE, APP_CS_CONFIG_NUM);

#if (ANTENNA_SWITCHING_AUTO_EN)
    //Initialize multi-antenas
    blc_cs_antenna_switch_config_init(&ant_cfg);
    rf_cs_ant_switch_pin_init(ant_switch_ctrl, sizeof(ant_switch_ctrl)/sizeof(rf_cs_ant_switch_ctrl));
#endif

#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
    blc_cs_disableGpioPinsFromD4ToD7();
#endif

    //Set cs use tx power level
    blc_cs_set_tx_power_level(CS_USE_TX_POWER_LEVEL);

    // init ACL - CS Tag map, used when exist multiple cs.
    blc_cs_connHandle_algo4Tag_map_init();

#if (CONFIG_CS_SUPPORT_INITIATOR)
    // Init algorithm-4 estimate buffer.return 0:success; -1:buffer null; -2:tag number invalid
    estimate_buff_init(CS_MAX_NUM, cs_tag_buffer);
    blc_cs_setAlgo4MaxDist(CS_ALGO_MAX_DISTANCE);
#endif

#if (CS_SUPPORT_PRIVATE_CONTROL_PDU)
    blc_ll_initPrivateControl();
#endif

    //Load calibration table for RTT.
    blc_loadCsCali_table(flash_sector_calibration + CALIB_OFFSET_CALI_TABLE_HEADER_INFO);
}