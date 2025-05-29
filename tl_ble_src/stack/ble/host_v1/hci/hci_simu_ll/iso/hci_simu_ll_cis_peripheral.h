
#pragma once

/**
 * @brief      this function is used by the Peripheral's Host to inform the Controller to accept the request for the CIS
 *             that is identified by the Connection_Handle
 * @param[in]  cisHandle - Connection handle of the CIS
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t ble_hci_ll_acceptCisRequest(u16 cisHandle);


/**
 * @brief      this function  is used by the Peripheral's Host to inform the Controller to reject the request for the CIS
 *             that is identified by the Connection_Handle.
 * @param[in]  cisHandle - Connection handle of the CIS to be rejected
 * @param[in]  reason - Reason the CIS request was rejected
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t ble_hci_ll_rejectCisReq(u16 cisHandle, u8 reason);
