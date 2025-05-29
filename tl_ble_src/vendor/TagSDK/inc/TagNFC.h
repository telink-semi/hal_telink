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

#ifndef TAGSDK_INC_TAGNFC_H_
#define TAGSDK_INC_TAGNFC_H_

#include "TagConfig.h"
#include "TagErrorType.h"

#if defined(TAG_ACCESSORY_OPTION_LOST_MESSAGE) && (TAG_ACCESSORY_OPTION_LOST_MESSAGE == 1)
#define LOST_MESSAGE_URL_MAX_LENGTH 220

/** @brief Initialize NFC stack
 *
 * @details Initialize NFC stack
 * @return return TAG_ERROR_NONE for success, return other values for failure.
 *
 */
TagError_t TagNFCInit(void);

/**
 * @brief       Store the URL in NFC
 *
 * @details     This function stores the URL in NFC
 *
 * @param[in]   url pointer of the URL to store
 * @param[in]   urlLen length of the URL to store
 *
 * @return return TAG_ERROR_NONE for success, return other values for failure.
 *
 */
TagError_t TagNFCSetLostMessageURL(char *url, size_t urlLen);

#endif /* TAG_ACCESSORY_OPTION_LOST_MESSAGE */

#endif /* TAGSDK_INC_TAGNFC_H_ */
