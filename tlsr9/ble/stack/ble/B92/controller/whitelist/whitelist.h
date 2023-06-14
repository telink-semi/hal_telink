/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#ifndef LL_WHITELIST_H_
#define LL_WHITELIST_H_



/**
 * @brief      This function is used to clear WhiteList
 * @param[in]  none
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		blc_ll_clearWhiteList(void);


/**
 * @brief      This function is used to add a device form WhiteList
 * @param[in]  adr_type - device address type
 * @param[in]  addr - device address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		blc_ll_addDeivceToWhiteList(u8 adr_type, u8 *addr);


/**
 * @brief      This function is used to delete a device from WhiteList
 * @param[in]  adr_type - device address type
 * @param[in]  addr - device address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t		blc_ll_removeDeivceFromWhiteList(u8 adr_type, u8 *addr);




#define	ll_whiteList_reset		blc_ll_clearWhiteList
#define	ll_whiteList_add		blc_ll_addDeivceToWhiteList
#define	ll_whiteList_delete		blc_ll_removeDeivceFromWhiteList


#endif /* LL_WHITELIST_H_ */
