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

#ifndef TAGSDK_PORT_INC_PORTSYSTEM_H_
#define TAGSDK_PORT_INC_PORTSYSTEM_H_

//#include "reset.h"
#include "TagCore.h"

/** @brief Reboot this device
 *
 * @note reason can be stored and utilized for any purpose like debugging
 *
 * @param[in] reason reboot request reason
 *
 */
void PortSystemReset(TagBootReason reason);

/**
 * @brief Check whether the device has been booted in Warm Boot or Cold Boot.
 *
 * @details This function will get the device's previous boot state.
 *
 * @return True  If it is Cold Boot
 * @return False If it is Warm Boot.
 *
 */
bool PortSystemIsColdBoot(void);

#endif /* TAGSDK_PORT_INC_PORTSYSTEM_H_ */
