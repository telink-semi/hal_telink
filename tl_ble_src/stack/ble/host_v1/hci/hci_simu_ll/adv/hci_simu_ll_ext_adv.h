#pragma once

/**
 * @brief      This function is used to set the advertising parameters
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  adv_evt_prop - describes the type of advertising event that is being configured and its basic properties
 * @param[in]  pri_advInter_min - Minimum advertising interval for undirected and low duty cycle directed advertising.
 * @param[in]  pri_advInter_max - Maximum advertising interval for undirected and low duty cycle directed advertising.
 * @param[in]  pri_advChnMap - primary advertisement channel
 * @param[in]  ownAddrType - Own Address Type
 * @param[in]  peerAddrType - Peer Address Type
 * @param[in]  peerAddr - Peer Address
 * @param[in]  advFilterPolicy - Advertising Filter Policy
 * @param[in]  adv_tx_pow - Advertising TX Power
 * @param[in]  pri_adv_phy - primary advertisement PHY
 * @param[in]  sec_adv_max_skip - Maximum advertising events the Controller can skip
 * @param[in]  sec_adv_phy - Secondary advertisement PHY
 * @param[in]  adv_sid - Value of the Advertising SID subfield in the ADI field of the PDU
 * @param[in]  scan_req_notify_en - Scan Request Notification Enable
 * @return     Status - 0x00: command succeeded;
 *                      0x12:  1. adv_handle out of range;
 *                             2. pri_advChnMap out of range
 *                      0x0C:  advertising is enabled for the specified advertising set
 */
ble_sts_t ble_hci_ll_setExtAdvParam(u8 adv_handle, advEvtProp_type_t adv_evt_prop, u32 pri_advInter_min, u32 pri_advInter_max, adv_chn_map_t pri_advChnMap, own_addr_type_t ownAddrType, u8 peerAddrType, u8 *peerAddr, adv_fp_type_t advFilterPolicy, tx_power_t adv_tx_pow, le_phy_type_t pri_adv_phy, u8 sec_adv_max_skip, le_phy_type_t sec_adv_phy, u8 adv_sid, u8 scan_req_notify_en);


/**
 * @brief      This function is used to set the data used in advertising PDU that have a data field
 *             notice that: setting legacy ADV data also use this API, data length can not exceed 31
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  advData_len - The number of octets in the Advertising Data parameter
 * @param[in]  *advData - Advertising data
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_setExtAdvData(u8 adv_handle, int advData_len, const u8 *advData);


/**
 * @brief      This function is used to provide scan response data used in scanning response PDUs.
 *             notice that: setting legacy scan response data also use this API, data length can not exceed 31
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  scanRspData_len - The number of octets in the Scan_Response Data parameter
 * @param[in]  *scanRspData - Scan response data
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_setExtScanRspData(u8 adv_handle, int scanRspData_len, const u8 *scanRspData);


/**
 * @brief      This function is used to request the Controller to enable or disable one or more advertising sets using the
               advertising sets identified by the adv_handle
 * @param[in]  enable -
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  duration -   the duration for which that advertising set is enabled
 *                          Range: 0x0001 to 0xFFFF, Time = N * 10 ms, Time Range: 10 ms to 655,350 ms
 * @param[in]  max_extAdvEvt - Maximum number of extended advertising events the Controller shall
 *                             attempt to send prior to terminating the extended advertising
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_setExtAdvEnable(adv_en_t enable, u8 adv_handle, u16 duration, u8 max_extAdvEvt);


/**
 * @brief      This function is used by the Host to set the random device address specified by the Random_Address
               parameter
 * @param[in]  adv_handle - Used to identify an advertising set
 * @param[in]  *rand_addr - Random Device Address
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_setAdvRandomAddr(u8 adv_handle, u8 *rand_addr);


/**
 * @brief      This function is is used to remove an advertising set from the Controller.
 * @param[in]  adv_handle - Used to identify an advertising set
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_removeAdvSet(u8 adv_handle);


/**
 * @brief      This function is used to remove all existing advertising sets from the Controller.
 * @param[in]  none.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t ble_hci_ll_clearAdvSets(void);


/**
 * @brief      If one connection Peripheral is established by Extended ADV, user can use this API to locate the Extended ADV set
 * @param[in]  connHandle - connection handle of ACL connection.
 * @return     Extended ADV handle:
 *                  0xFF: invalid ADV handle, e.g. connection is not Peripheral role, or Peripheral is established by legacy ADV
 *                  Others: Extended ADV handle
 */
u8 ble_hci_ll_getExtendedAdvHandleForAclConnection(u16 connHandle);
