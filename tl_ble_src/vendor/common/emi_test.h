/********************************************************************************************************
 * @file    emi_test.h
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

#ifndef VENDOR_COMMON_EMI_TEST_H_
#define VENDOR_COMMON_EMI_TEST_H_


/**
 * @brief   Application EMI test enable or disable
 *          user can set it in app_config.h, default disable if user not set it
 */
#ifndef APP_EMI_TEST_ENABLE
    #define APP_EMI_TEST_ENABLE 0
#endif


/**
 * @brief Define where the configuration data will be saved in the binary file.
 */
#define PA_SETTING_ADDR      (0x04)
#define GENERAL_SETTING_ADDR (0x16)

/**
 * @brief       Enumeration type of PA state.
 */
typedef enum
{
    PA_SETTING_STATE_INIT   = 0x01,
    PA_SETTING_STATE_TX     = 0x02,
    PA_SETTING_STATE_RX     = 0x03,
    PA_SETTING_STATE_BYPASS = 0x04,
} pa_setting_state_e;

/**
 * @brief       This function serves to execute PA operations according to the state.
 * @param[in]   state      - PA working state.
 * @return      none
 */
extern void pa_operation(pa_setting_state_e state);


/**
 * @brief       This function serves to converts the pin value of a single byte to the actual pin value.
 * @para[in]    value  - One-byte pin value.
 * @return      Result of actual pin value.
 */
#define get_pin(value) (((unsigned short)((value) >> 3) << 8) | BIT((value) & 0x07))

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      One-byte result data.
 */
extern unsigned char read_data8(unsigned int addr);

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      Two-byte result data.
 */
extern unsigned short read_data16(unsigned int addr);

/**
 * @brief       This function serves to read data saved in binary file.
 * @param[in]   addr   - Position in binary file.
 * @return      Four-byte result data.
 */
extern unsigned int read_data32(unsigned int addr);

/**
 * @brief       This function serves to initialize PA setting.
 * @param[in]   addr       - Address to save PA related operations.
 * @param[in]   bypass_en  - To set PA bypass mode, 0:disable 1:enable.
 * @return      none
 */
extern void pa_setting_init(unsigned int addr, unsigned char bypass_en);


/**
 * @brief       This function serves to EMI Init
 * @return      none
 */
void emi_init(void);

/**
 * @brief       This function serves to read EMI initial setting.
 * @return      none
 */

void emi_setting_init(void);

/**
 * @brief       This function serves to User Init emi
 * @return      none
 */

void app_user_init_emi(void);


/**
 * @brief     This function serves to EMI ServiceLoop
 * @return    none
 */
void emi_serviceloop(void);


#endif /* VENDOR_COMMON_EMI_TEST_H_ */
