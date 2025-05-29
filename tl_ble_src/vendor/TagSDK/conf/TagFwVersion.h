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

#ifndef TAGSDK_TAGFWVERSION_H_
#define TAGSDK_TAGFWVERSION_H_

/** @brief Major version of firmware
 * @details Major version of firmware in range from 0 to 999.
 * @note can't keep backward compatibility for Tag Spec.
 */
#define TAG_FWVER_MAJOR 0

/** @brief Minor version of firmware
 * @details Minor version of firmware in range from 0 to 999.
 * @note feature added. keep backward compatibility for Tag Spec.
 */
#define TAG_FWVER_MINOR 0

/** @brief Minor version of firmware
 * @details Minor version of firmware in range from 0 to 999.
 * @note bug fix
 */
#define TAG_FWVER_PATCH 0

#endif /* TAGSDK_TAGFWVERSION_H_ */
