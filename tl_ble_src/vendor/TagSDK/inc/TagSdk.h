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

#ifndef TAGSDK_INC_TAGSDK_H_
#define TAGSDK_INC_TAGSDK_H_

typedef enum {
    TAG_RESULT_SUCCESS = 0,        /**< Operation successes */
    TAG_RESULT_UNDEFINED_FAIL,     /**< Fail due to undefined error */
    TAG_RESULT_MEMORY_FAIL,        /**< Fail due to memory allocation fail */
    TAG_RESULT_NV_LOAD_FAIL,       /**< Fail due to NV Load or invalid NV data*/
    TAG_RESULT_INVALID_ARG,        /**< Fail due to invalid arguments */
    TAG_RESULT_NOT_SUPPORTED,      /**< Requested operation is not supported */
} TagResult_t;

/** @brief Initialize tag context
 *
 * @detail In this function, it operates NV loading, setting RTC,
 *         initializing crypto etc. You shall initialize BSP before call this function.
 *
 * @retval TAG_RESULT_SUCCESS Tag initialization successes
 * @retval others Please refer @ref TagResult_t
 */
TagResult_t TagInit(void);

/** @brief Start tag services
 *
 * @detail It initialize BLE GATT DB and services and starts BLE advertising.
 *         You shall initialize BLE stack before call this function.
 *
 * @retval TAG_RESULT_SUCCESS Tag starts services successfully
 * @retval others Please refer @ref TagResult_t
 */
TagResult_t TagStart(void);

/** @brief Factory reset tag
 *
 * @detail This function reset NV area as factory default set and reboot the device.
 *         It restart from OOB state.
 *
 * @retval TAG_RESULT_SUCCESS Factory reset successfully
 * @retval others Please refer @ref TagResult_t
 */
TagResult_t TagCleanup(void);

#endif /* TAGSDK_INC_TAGSDK_H_ */
