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

#ifndef TAGSDK_PORT_INC_PORTENCRYPTIION_H_
#define TAGSDK_PORT_INC_PORTENCRYPTIION_H_

#include <stddef.h>

/**
 * @brief Initialize encryption
 *
 * @details This function initilaize encryption
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 */
TagError_t PortEncryptionInit(void);

/**
 * @brief Encrypt the key
 *
 * @details This function encrypt the key to protect it.
 *
 * @param[in] inputBuf pointer of input data
 * @param[in] inputLen length of input data
 * @param[out] outputBuf pointer of output data
 * @param[out] outputLen pointer of output length
 *
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 */
TagError_t PortKeyEncrypt(unsigned char *inputBuf, size_t inputLen, unsigned char **outputBuf, size_t *outputLen);
/**
 * @brief Decrypt the key
 *
 * @details This function decrypt the encrypted key using PortKeyEncrypt
 *
 * @param[in] inputBuf pointer of input data
 * @param[in] inputLen length of input data
 * @param[out] outputBuf pointer of output data
 * @param[out] outputLen pointer of output length
 *
 * @return return TAG_ERROR_NONE for success. negative values for failure.
 */
TagError_t PortKeyDecrypt(unsigned char *inputBuf, size_t inputLen, unsigned char **outputBuf, size_t *outputLen);

#endif /* TAGSDK_PORT_INC_PORTENCRYPTIION_H_ */
