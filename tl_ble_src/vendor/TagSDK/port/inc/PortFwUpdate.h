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

#ifndef TAGSDK_PORT_INC_FWUPDATE_H_
#define TAGSDK_PORT_INC_FWUPDATE_H_

#include <stdbool.h>
#include "TagErrorType.h"
#include "TagConfig.h"

#if defined(TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE) && (TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE == 1)

/**
 * @brief Get maximum allowed write without response.
 *
 * @details This function get number of writes without response, which is transmitted continuously from the end user.
 *
 * @return maximum number of allowed write.
 */
uint8_t PortFwUpdateGetMaxWriteWithoutResponse(void);

/**
 * @brief  Get start address for firmware update.
 *
 * @details This function get start address of eeprom to write firmware image.
 *
 * @return start address for eeprom
 *
 */
uint32_t PortFwUpdateGetStartAddress(void);

/**
 * @brief  Places the next image chunk into the FLASH.
 *
 * @details This function write data into the FLASH.
 *
 * @param[in] length  the length of the data chunk
 * @param[in] Addr    eeprom address to write data in flash.
 * @param[in] pData   pointer to the data chunk
 *
 * @return 0 for success, otherwise failure.
 *
 */
TagError_t PortFwUpdateWriteFlash(uint32_t length, uint32_t Addr, uint8_t *pData);

/**
 * @brief  Read data in the FLASH.
 *
 * @details This function read data in the FLASH.
 *
 * @param[in] length the length of the data chunk
 * @param[in] Addr   eeprom address to write data in flash.
 * @param[in] inbuf  read buffer
 *
 */
void PortFwUpdateReadFlash(uint16_t length, uint32_t Addr, uint8_t *inbuf);

/**
 * @brief  Erase data in the FLASH.
 *
 * @details This function erase data in the FLASH.
 *
 * @param[in] length  the size of data
 * @param[in] Addr    eeprom address to erase data in flash.
 *
 * @return 0 for success, otherwise failure.
 *
 */
TagError_t PortFwUpdateEraseFlash(uint32_t length, uint32_t Addr);

/**
 * @brief   this api is called after device rebooting
 *
 * @details if you want to call other api after device rebooting, you can add code in in this api.
 *
 */
void PortFwUpdateInitCb(void);

/**
 * @brief   this api is called before stating s/w update.
 *
 * @details if you want to call other api before stating s/w update, you can add code in in this api.
 *
 */
void PortFwUpdateStartCb(void);

/**
 * @brief   this api is called before ending s/w update.
 *
 * @details if you want to call other api before ending s/w update, you can add code in in this api.
 *
 */
void PortFwUpdateEndCb(void);

/**
 * @brief   this api is called after s/w update success.
 *
 * @details if you want to call other api after s/w update success, you can add code in in this api.
 *
 */
void PortFwUpdateSuccessCb(void *param);

/**
 * @brief   this api is called after s/w update failed.
 *
 * @details if you want to call other api after s/w update failed, you can add code in in this api.
 *
 */
void PortFwUpdateFailedCb(void);

/**
 * @brief   this api is called to check firmware update status.
 *
 * @details if you want to get firmware update status.
 *
 * @return firmware update status
 *
 */
bool PortFwUpdateStatus(void);

#endif // TAG_ACCESSORY_OPTION_FIRMWARE_UPDATE
#endif /* TAGSDK_PORT_INC_FWUPDATE_H_ */
