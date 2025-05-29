#pragma once

/**
 * @brief  This function is used to enable/disable time stamp in SDU reported from controller
 * to host
 * @param[in]      Status - 0x00:  disable, time stamp is invalid in SDU;
 *                        - 0x01:  enable, time stamp is valid in SDU
 */
void ble_hci_iso_enableSduToHostTimestamp(u8 en);


/**
 * @brief      This function is  used to identify and create the isochronous data path between the Host and the Controller for a CIS,
 *             CIS  configuration, or BIS identified by the Connection_Handle parameter. can also be used to configure a codec for each data path.
 * @param[in]  conn_handle - Connection handle of the CIS or BIS
 * @param[in]  dir - Data_Path_Direction
 * @param[in]  id - Data_Path_ID.
 * @param[in]  cid_assignNum - Assigned Numbers for Coding Format
 * @param[in]  cidcompId - Company ID
 * @param[in]  cid_vendorDef - Vendor-defined codec ID.
 * @param[in]  control_dly - Controller delay in microseconds
 * @param[in]  codec_cfg_len - Length of codec configuration
 * @param[in]  codec_cfg1 - Codec-specific configuration data 1
 * @param[in]  codec_cfg2 - Codec-specific configuration data 2
 * @param[in]  codec_cfg3 - Codec-specific configuration data 3
 * @param[in]  codec_cfg4 - Codec-specific configuration data 4
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t ble_hci_ll_setupIsoDataPath(u16 conn_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef, u32 control_dly, u8 codec_cfg_len, u8 codec_cfg1, u8 codec_cfg2, u8 codec_cfg3, u8 codec_cfg4);


/**
 * @brief      This function is used to remove the input and/or output data path(s) associated with a CIS, CIS configuration, or BIS
               identified by the Connection_Handle parameter.
 * @param[in]  conn_handle - Connection handle of the CIS or BIS
 * @param[in]  dir_mask - Data_Path_Direction
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t ble_hci_ll_removeIsoDataPath(u16 conn_handle, dp_dir_msk_t dir_mask);


/**
 * @brief      This function is used to send ISO data.
 * @param[in]  cisHandle or bisHandle
 * @param[in]  pData  point to data to send
 * @param[in]  len  the length to send
 * @return      Status - 0x00:  succeeded;
 *                       other:  failed
 */
ble_sts_t ble_hci_iso_sendData(u16 handle, u8 *pData, u16 len);


void ble_hci_ial_register_sdu_pop_callback(ial_sdu_pop_callback_t callback);
