/********************************************************************************************************
 * @file    chn_sound.h
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
#ifndef STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHANNEL_SOUND_H_
#define STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHANNEL_SOUND_H_


#include "stack/ble/hci/hci_cmd.h"
#include "chn_initiator.h"
#include "chn_reflector.h"


#define     CS_PARAM_LENGTH                         4176    //user can't modify this value !!!

typedef enum{
    CS_ROLE_DISABLE = 0,
    CS_INITIATOR_ROLE = BIT(0),
    CS_REFLECTOR_ROLE= BIT(1),
    CS_INIT_REFL_ROLE = BIT(0)|BIT(1),
}cs_role_t;


typedef enum{
    CS_PARAM_INITIATOR_ROLE = 0,
    CS_PARAM_REFLECTOR_ROLE = 1,
}cs_param_role_t;

typedef enum{
    CS_CONFIG_INITIATOR_ROLE = 0,
    CS_CONFIG_REFLECTOR_ROLE = 1,
}cs_config_role_t;

/**
 * @brief      for user to initialize channel sounding module and allocate channel sounding configuration buffer.
 * @param[in]  pParamBuf - start address of channel sounding configuration parameters buffer
 * @param[in]  cs_config_num - channel sounding configuration number that application layer may use
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initCsModule_initConfigParametersBuffer(u8 *pParamBuf, int cs_config_num);




/**
 * @brief      for user to initialize channel sounding RX FIFO.
 * @param[in]  pRxBuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initCsRxFifo(u8 *pRxBuf, int fifo_size, int fifo_num);


/**
 * @brief      for user to read local CS capabilities.
 * @param[out] pRetParam - refer to hci command of "LE CS Read Local Supported Capabilities command"
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_readLocalSupportedCap(hci_le_cs_readLocalSupportedCap_retParam_t* pRetParam);

/**
 * @brief      for user to read remote device CS capabilities.
 * @param[in]  handle - ACL handle
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_readRemoteSupportedCap(u16 handle);

/**
 * @brief      for user to  write the cached copy of the CS capabilities that are supported by the remote Controller.
 * @param[in]  pCS_param - refer to HCI_LE_CS_Write_Cached_Remote_Supported_Capabilities command in core spec
 * @param[out] pRetParam - include status and Connection_Handle.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_writeCachedRemoteSupportedCap(
            hci_le_cs_writeCachedRemoteSupportedCap_cmdParam_t *pCS_param,
            hci_le_cs_writeCachedRemoteSupportedCap_retParam_t *pRetParam);

/**
 * @brief      for user to start or restart the Channel Sounding Security Start procedure.
 * @param[in]  connHandle - ACL connection handle
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_security_enable(u16 connHandle);

/**
 * @brief      for user to  to set default CS settings in the local Controller .
 * @param[in]  pSetting - refer to  HCI_LE_CS_Set_Default_Settings command in core spec
 * @param[out] retParam - include status and Connection_Handle.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_setDefaultSettings(hci_le_cs_setDefaultSetting_cmdParam_t *pSetting,
                                            hci_le_cs_setDefaultSetting_retParam_t * retParam);

/**
 * @brief      for user to read the per-channel mode0 Frequency Actuation Error table of the remote Controller.
 * @param[in]  connHandle - ACL connection handle
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_readRemoteFAE_table(u16 connHandle);

/**
 * @brief      for user to  to  write a cached copy of the per-channel mode 0 Frequency Actuation
 *             Error table of the remote device in the local Controller.
 * @param[in]  table - Per-channel mode 0 Frequency Actuation Error table of the local Controller
 * @param[out] retParam - include status and Connection_Handle.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_writeCachedRemoteFAE_table(u16 connHandle, u8* table,
                                                hci_le_cs_writeChchedRemoteFAE_retParam_t *retParam);

/**
 * @brief      for user to create a new CS configuration.
 * @param[in]  pConfig - CS configuration refer to  HCI_LE_CS_Create_Config command
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_createConfig(hci_le_cs_creatConfig_cmdParam_t *pConfig);

/**
 * @brief      for user to set the parameters for the scheduling of one or more CS procedures by the local Controller.
 * @param[in]  pParam - CS procedure parameters refer to HCI_LE_CS_Set_Procedure_Parameters command
 * @param[out] retParam - include status and Connection_Handle.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_setProcedureParam(hci_le_cs_setProcedureParame_cmdParam_t *pParam,
                                        hci_le_cs_setProcedureParam_retParam_t *ret);

/**
 * @brief      for user to enable or disable the scheduling of CS procedures.
 * @param[in]  pCmd - refer to HCI_LE_CS_Procedure_Enable command
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_procedureEnable(hci_le_cs_enableProcedure_cmdParam_t *pCmd);

/**
 * @brief      for user to remove a CS configuration.
 * @param[in]  pCmd - refer to HCI_LE_CS_Remove_Config command.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_removeConfig(u16 connHandle, u8 config_ID);

/**
 * @brief      for user to update the channel classification based on its local information.
 * @param[in]  chn - channel bit fields.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_cs_setChannelClassification(u8 *chn);


int blc_ll_getCsSecExchStatus(u16 connHandle);
int blc_ll_test_CsFaeExchCtrl(u16 connHandle);
int blc_cs_resetByHandle(u16 connHandle);

s32 blc_ll_cs_getMode1InternalCircuitDelay(cs_param_role_t role);
void blc_ll_cs_setMode1InternalCircuitDelay(cs_param_role_t role, s32 circuit_delay);









#endif /* STACK_BLE_CONTROLLER_LL_CHN_SOUND_CHANNEL_SOUND_H_ */
