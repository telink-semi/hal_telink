/********************************************************************************************************
 * @file    app_cs.h
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

#ifndef _APP_CS_H_
#define _APP_CS_H_

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

#define MAX_DISTANCE_CNT_SUPPORT 3

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
    float state;
    float err_cov;
    float proc_noise_cov;
    float msr_noise_cov;
    float kal_gain;
} kalmanFilter_t;

typedef struct __attribute__((packed))
{
    float          lastValidDist;
    float          lastValidFiltDist;
    float          lastValidAmplitudeDist;
    kalmanFilter_t kf;
} app_filter_ctrl;

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

extern cs_app_control_t         cs_app_ctrl;
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
 * @brief      Initialize app ranging filter
 * @param[in]  None
 * @return     None
 */
void app_rangingFilter_init(void);

/**
 * @brief      CS distance filter, first amplitude filter, second kalman filter.
 * @param[in]  *ctrl   Pointer of filter control block.
 * @param[in]  in      in data
 * @param[out] *out    Pointer of out data.
 * @return
 */
void app_rangingFilter(app_filter_ctrl *ctrl, float in, float *out);

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

#if PRF_ENABLE
void app_le_cs_procedure_enable_complete_event_handle(u8 *p);
void app_le_cs_subevent_result_event_handle(u8 *p);
void app_le_cs_subevent_result_continue_event_handle(u8 *p);
void app_le_cs_iop_setConnectionHandle(u16 connHandle); //used to set a connection handle for static iop data
#endif

#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)
#endif /* _APP_CS_H_ */
