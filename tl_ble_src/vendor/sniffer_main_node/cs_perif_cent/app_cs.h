/********************************************************************************************************
 * @file    app_cs.h
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
#ifndef APP_CS_H_
#define APP_CS_H_

#include "app_config.h"
#include "vendor/common/sniffer_common/sniffer_common.h"

#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

typedef enum
{
    NULL_EXCH       = 0,
    CAP_EXCH        = 1,
    SET_DEFAULT     = 2,
    FAE_EXCH        = 3,
    CFG_EXCH        = 4,
    SEC_EXCH        = 5,
    SET_PROC_PARAM  = 6,
    CS_PROC_EN_EXCH = 7,
} eCsProcStatus;

typedef enum
{
    NULL_EXCH_CMPLT      = 0,
    CAP_EXCH_CMPLT       = BIT(0),
    SET_DFT_CMPLT        = BIT(1),
    FAE_EXCH_CMPLT       = BIT(2),
    CFG_EXCH_CMPLT       = BIT(3),
    SEC_EXCH_CMPLT       = BIT(4),
    SET_PROC_PARAM_CMPLT = BIT(5),
    CS_PROC_EN_CMPLT     = BIT(6),

} eCsProcCmpltStatusMask;

#define CS_MAX_NUM 4

typedef struct __attribute__((packed))
{
    u16 connhandle;
    u8  config_id;
    u8  acl_role;
    u8  exch_start_state;
    u8  exch_cmplt_state;
    u32 exchange_tick;
} cs_control_t;

typedef struct __attribute__((packed))
{
    cs_control_t cs_ctrl[CS_MAX_NUM];
} cs_app_control_t;

typedef struct __attribute__((packed))
{
    u16 Connection_Handle;
    u8  Config_ID;
    u8  Main_Mode;
    u8  Sub_Mode;
    u8  Role;
    u8  RTT_Type;
    u8  valid;
} app_cs_config_t;

typedef struct __attribute__((packed))
{
    float state;
    float err_cov;
    float proc_noise_cov;
    float msr_noise_cov;
    float kal_gain;
    u32   update_tick;
} kalmanFilter_t;

extern cs_app_control_t cs_app_ctrl;

extern chn_sound_capabilities_t appCsLocalSupportCap;

/**
 * @brief      Add CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x01 success 0x00 failed
 */
int user_addCsCtrlByHadle(u16 connhandle);

/**
 * @brief      Clear CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x01 success 0x00 failed
 */
int user_clrCsCtrlByHadle(u16 connhandle);

/**
 * @brief      Get index of CS procedure control block by acl connect handle
 * @param[in]  connhandle ACL connect handle
 * @return     0x00 failed
 *             else : index
 */
int user_getCsCtrlByHadle(u16 connhandle);

/**
 * @brief      Set procedure control block start status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure start status
 * @return     None
 */
void user_setCsProcStartStatus(u8 index, eCsProcStatus status);

/**
 * @brief      Set procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_setCsProcCmpltStatus(u8 index, eCsProcCmpltStatusMask status);

/**
 * @brief      Clear procedure control block complete status
 * @param[in]  index: index of procedure control block
 * @param[in]  status: procedure complete status
 * @return     None
 */
void user_clrCsProcCmpltStatus(u8 index, eCsProcCmpltStatusMask status);

/**
 * @brief      Initialize CS procedure control block
 * @param[in]  None
 * @return     None
 */
void user_initCsCtrl(void);

/**
 * @brief      BLE CS config complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_config_complete_event_handle(u8 *p);

/**
 * @brief      BLE CS procedure enable complete event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_procedure_enable_complete_event_handle(u8 *p);

/**
 * @brief      BLE CS subevent result event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_event_handle(u8 *p);

/**
 * @brief      BLE CS subevent result continue event handler
 * @param[in]  p    Pointer point to event parameter buffer.
 * @return
 */
void app_le_cs_subevent_result_continue_event_handle(u8 *p);

/**
 * @brief      BLE channel sounding procedure control loop
 * @param      None
 * @return     None
 */
void cs_procedure_ctrl(void);

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void);

/**
 * @brief      Find unsused cs config buffer
 * @param[in]  None
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_findUnusedCSConfig(void);

/**
 * @brief      Get cs config buffer by acl connect handle and config ID
 * @param[in]  connhandle ACL connect handle
 * @param[in]  Config_ID  config ID
 * @return     Pointer to unused CS config buffer
 */
app_cs_config_t *blc_getCSConfig(u16 connHandle, u8 Config_ID);

/**
 * @brief      Find unsused cs config buffer
 * @param[in]  None
 * @return     None
 */
void chipDly_handle_mainloop(void);

#endif
#endif /* APP_CS_H_ */
