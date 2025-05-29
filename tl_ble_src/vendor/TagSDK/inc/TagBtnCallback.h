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

#ifndef TAGSDK_INC_TAGBTNCALLBACK_H_
#define TAGSDK_INC_TAGBTNCALLBACK_H_

typedef enum
{
    EVENT_BUTTON_PUSHED = 1,
    EVENT_BUTTON_DOUBLE_PUSHED,
    EVENT_BUTTON_HOLD,
    EVENT_BUTTON_LONG_HOLD,
} ButtonEventType;

void SystemButtonEventCallback(ButtonEventType eventType);

#endif /* TAGSDK_INC_TAGBTNCALLBACK_H_ */
