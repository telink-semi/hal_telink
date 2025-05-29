/* ***************************************************************************
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved.
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of Samsung
 * Electronics Co., Ltd. ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it
 * only in accordance with the terms of the license agreement you entered
 * into with Samsung Electronics Co., Ltd. ("SAMSUNG")
 * SAMSUNG MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SAMSUNG SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 ****************************************************************************/

#ifndef EXAMPLE_PROJECTCONFIG_H_
#define EXAMPLE_PROJECTCONFIG_H_

/**
 * \def TAG_CONFIG_LOG_LEVEL_ERROR
 *
 * Print error level log by calling TAG_LOG_E, CHECK_RESULT_EQ
 * and CHECK_RESULT_NOT_EQ macros.
 */
#define TAG_CONFIG_LOG_LEVEL_ERROR

/**
 * \def TAG_CONFIG_LOG_LEVEL_INFO
 *
 * Print information level log by calling TAG_LOG_I macro.
 */
#define TAG_CONFIG_LOG_LEVEL_INFO

/**
 * \def TAG_CONFIG_LOG_LEVEL_DEBUG
 *
 * Print debug level log by calling TAG_LOG_D macro.
 */
//#define TAG_CONFIG_LOG_LEVEL_DEBUG

/**
 * \def TAG_CONFIG_USE_UWB_CHARACTERISTICS
 *
 * Uncomment this to enable UWB related characteristics on Tag Control GATT Service.
 *
 * Used in:
 *      src/TagControlService.c
 *      src/TagBleCharacteristic.c
 *      port/xxx/PortBle.c
 *      inc/TagBleGattUUID.h
 *      inc/TagConstant.h
 *      inc/TagControlService.h
 *      inc/TagBleCharacteristic.h
 *
 * \note The developer need to add hardware specific implementation
 * to each characteristics (e.g. UWB Power, UWB Parameter) This feature just enable
 * characteristics only.
 *
 * \warning This config must be enabled only when the hardware support UWB functionality.
 */
//#define TAG_CONFIG_USE_UWB_CHARACTERISTICS

/**
 * \def TAG_CONFIG_USE_DEBUG_CHARACTERISTICS
 *
 * Uncomment this to enable DEBUG characteristic on Tag Control GATT Service.
 *
 * \note The developer need to add project specific implementation to PortLog(TBD)
 * API for responding debug characteristic GATT write and indication.
 *
 */
#define TAG_CONFIG_USE_DEBUG_CHARACTERISTICS

/**
 * \def TAG_TASK_PRIORITY
 *
 * Configure SDK main task priority.
 *
 * \warning Recommend it should be above idle and less than timer service
 */
//#define TAG_CONFIG_TASK_PRIORITY    10

/**
 * \def TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP
 *
 * The SDK prints hex value of memory area for debugging purpose
 *
 * Used in:
 *      include/TagUtil.h
 *      src/TagUtil.c
 *
 * The SDK prints the hex value of given memory area by TagUtilDumpMem API.
 * Uncomment to dump memory area by TagUtilDumpMem for debugging purpose.
 *
 * \warning This option must be disabled for production release
 */
//#define TAG_CONFIG_DEBUG_PRINT_MEMORY_DUMP

/**
 * \def TAG_CONFIG_DEBUG_BLOCK_SLEEP_MODE
 *
 * The SDK blocks system enter to sleep mode.
 * This would be done by set PortSleepWakeLock with PORT_WAKELOCK_PERFORMANCE.
 * This option can be used only for debugging purpose.
 *
 * \warning This option must be disabled for production release
 */
//#define TAG_CONFIG_DEBUG_BLOCK_SLEEP_MODE

/**
 * \def TAG_CONFIG_USE_DEVICE_INFO_HEADER
 *
 * Use TagDeviceInfo.h file to get device's serial number and private key
 *
 * Used in: conf/TagDeviceInfo.h
 *
 * The device's serial number and private key should be stored at NV (Non Volatile)
 * area (e.g. flash memory, secure element hardware) with secure manner.
 * This option helps developer to get serial number and public key from code area
 * which is specified by conf/TagDeviceInfo.h file to ease integration test.
 *
 * \warning This option must be disabled for production release
 */
#define TAG_CONFIG_USE_DEVICE_INFO_HEADER

/**
 * \def TAG_CONFIG_USE_ONBOARD_CONF_HEADER
 *
 * Use TagOnboardingConfig.h file to get manufacturer, model and setup ID.
 *
 * Used in: conf/TagOnboardingConfig.h
 *
 * The manufacturer, model and setup ID should be stored at NV (Non Volatile)
 * area (e.g. flash memory).
 * This option helps developer to get manufacturer, model setup ID from code area
 * which is specified by conf/TagOnboardingConfig.h file. to ease integration test.
 *
 * \warning This option must be disabled for production release
 */
#define TAG_CONFIG_USE_ONBOARD_CONF_HEADER

/**
 * \def CONFIG_USE_KEY_NV_ITEM_ENCRYPTION
 *
 * The SDK stores security critical information as encrypt form to storage.
 *
 * Used in:
 *      TagSecurityAuthentication.c
 *      port/xxx/PortEncryption.c
 *
 * The SDK generates security critical information at runtime.
 * This option make the SDK to store these information as encrypted form
 * and load and use after decryption with using PortKeyEncrypt(), PortKeyDecrypt() APIs.
 *
 * \note This option doesn't have encryption/decryption functionality itself.
 * It should be implemented at PortEncryption.c file.
 * If the hardware supports storage encryption, This option isn't needed.
 */
//#define CONFIG_USE_KEY_NV_ITEM_ENCRYPTION

/**
 * \def TAG_CONFIG_FACTORY_RESET_IN_SDK
 *
 * There is sample factory reset scenario in SDK.
 * It checks button status in TagStart function.
 * And if button is pressed, it checks button status every MANUAL_FACTORY_RESET_BUTTON_CHECK_PERIOD
 * to see if user is still holding button.
 * And when user holds button over MANUAL_FACTORY_RESET_BUTTON_TIMEOUT, it starts factory-reset.
 * You can enable or disable the feature by this config.
 */
#define TAG_CONFIG_FACTORY_RESET_IN_SDK

/**
 * \def TAG_CONFIG_FACTORY_RESET_WITHOUT_REBOOT
 *
 * This option support to proceed with initialization without reboot when receiving a factory reset command.
 *
 */
// #define TAG_CONFIG_FACTORY_RESET_WITHOUT_REBOOT

/**
 * \def TAG_CONFIG_ENABLE_MEM_CHECK
 *
 * This option help to check tag's available memory size.
 *
 */
//#define TAG_CONFIG_ENABLE_MEM_CHECK

/**
 * \def TAG_CONFIG_ENABLE_OOB_DEEP_SLEEP
 *
 * During OOB advertising, it goes to Normal advertising after timeout(T_OOB_ADVERTISE_FAST_INTERVAL_TIMEOUT_SEC).
 * When it goes to Normal advertising mode, it changes advertising interval to T_OOB_ADVERTISE_INTERVAL_NORMAL.
 * you can opt to Deep Sleep mode instead of Normal advertising mode by enabling this config.
 * In deep sleep mode, it doesn't advertising to save battery until a user does any actions(pushs button, changes battery etc).
 */
// #define TAG_CONFIG_ENABLE_OOB_DEEP_SLEEP

/**
 * \def TAG_CONFIG_USE_MBEDTLS_IN_SDK
 *
 * This option is to use the Mbed TLS library contained in the SDK.
 */
 #define TAG_CONFIG_USE_MBEDTLS_IN_SDK

//#include "mbedtls_config.h"

#endif /* EXAMPLE_PROJECTCONFIG_H_ */
