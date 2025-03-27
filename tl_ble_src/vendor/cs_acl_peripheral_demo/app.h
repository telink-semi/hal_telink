/********************************************************************************************************
 * @file    app.h
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
#ifndef VENDOR_APP_H_
#define VENDOR_APP_H_

typedef enum{
    NULL_EXCH       = 0,
    CAP_EXCH        = 1,
    SET_DEFAULT     = 2,
    FAE_EXCH        = 3,
    CFG_EXCH        = 4,
    SEC_EXCH        = 5,
    SET_PROC_PARAM  = 6,
    CS_PROC_EN_EXCH = 7,
}eCsProcStatus;

typedef enum{
    NULL_EXCH_CMPLT         = 0,
    CAP_EXCH_CMPLT          = BIT(0),
    SET_DFT_CMPLT           = BIT(1),
    FAE_EXCH_CMPLT          = BIT(2),
    CFG_EXCH_CMPLT          = BIT(3),
    SEC_EXCH_CMPLT          = BIT(4),
    SET_PROC_PARAM_CMPLT    = BIT(5),
    CS_PROC_EN_CMPLT        = BIT(6),

}eCsProcCmpltStatusMask;

#define CS_MAX_NUM          4

typedef struct{
    u16 connhandle;
    u8  config_id;
    u8  acl_role;
    u8  exch_start_state;
    u8  exch_cmplt_state;
    u32 exchange_tick;

}cs_control_t;

typedef struct __attribute__((packed)) {
    cs_control_t cs_ctrl[CS_MAX_NUM];
}cs_app_control_t;

extern cs_app_control_t cs_app_ctrl;


int user_addCsCtrlByHadle(u16 connhadle);
int user_clrCsCtrlByHadle(u16 connhadle);
int user_getCsCtrlByHadle(u16 connhadle);
void user_initCsCtrl(void);
void user_setCsProcStartStatus(u8 index,eCsProcStatus status);
void user_setCsProcCmpltStatus(u8 index,eCsProcCmpltStatusMask status);
void user_clrCsProcCmpltStatus(u8 index,eCsProcCmpltStatusMask status);

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */

void user_init_normal(void);

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop (void);


/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback (u32 h, u8 *p, int n);


/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback (u32 h, u8 *para, int n);


/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt);

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 *             handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 *             attention 1: if you have more flash write or erase action, you should should add more type and process them
 *             attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 *                          stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 *             attention that, we use: [op_addr_begin, op_addr_end)
 *             e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 *                  but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 */
void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end);


#endif /* VENDOR_APP_H_ */
