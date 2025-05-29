#pragma once

// This file all api for ATT server deal with request, can not called by user.

/*****************core v5.4 Vol 3 Part F 3.4.2.1 ATT_EXCHANGE_MTU_REQ******************/
/**
 *  @brief L2CAP ATT deal with exchange MTU request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or exchange MTU response.
*/
uint16_t ble_host_att_deal_exchange_mtu_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.3.1 ATT_FIND_INFORMATION_REQ*************/
/**
 *  @brief L2CAP ATT or EATT deal with find information request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or find information response.
*/
uint16_t ble_host_att_deal_find_information_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.3.3 ATT_FIND_BY_TYPE_VALUE_REQ************/
/**
 *  @brief L2CAP ATT or EATT deal with find by type value request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or find by type value response.
*/
uint16_t ble_host_att_deal_find_by_type_value_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.4.1 ATT_READ_BY_TYPE_REQ*****************/
/**
 *  @brief L2CAP ATT or EATT deal with read by type request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read by type response.
*/
uint16_t ble_host_att_deal_read_by_type_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.4.3 ATT_READ_REQ************************/
/**
 *  @brief L2CAP ATT or EATT deal with read request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read response.
*/
uint16_t ble_host_att_deal_read_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.4.5 ATT_READ_BLOB_REQ********************/
/**
 *  @brief L2CAP ATT or EATT deal with read blob request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read blob response.
*/
uint16_t ble_host_att_deal_read_blob_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.4.7 ATT_READ_MULTIPLE_REQ****************/
/**
 *  @brief L2CAP ATT or EATT deal with read multiple request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read multiple response.
 */
uint16_t ble_host_att_deal_read_multiple_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/******************core v5.4 Vol 3 Part F 3.4.4.9 ATT_READ_BY_GROUP_TYPE_REQ***********/
/**
 *  @brief L2CAP ATT or EATT deal with read by group type request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read by group type response.
*/
uint16_t ble_host_att_deal_read_by_group_type_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.4.11 ATT_READ_MULTIPLE_VARIABLE_REQ*******/
/**
 *  @brief L2CAP ATT or EATT deal with read multiple variable request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read multiple variable response.
*/
uint16_t ble_host_att_deal_read_multiple_variable_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.5.1 ATT_WRITE_REQ*************************/
/**
 *  @brief L2CAP ATT or EATT deal with read request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or read response.
*/
uint16_t ble_host_att_deal_write_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.5.3 ATT_WRITE_CMD*************************/
/**
 *  @brief L2CAP ATT or EATT deal with write command.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, 0 no response needed.
*/
uint16_t ble_host_att_deal_write_cmd(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.5.4 ATT_SIGNED_WRITE_CMD******************/
/**
 *  @brief L2CAP ATT or EATT deal with signed write command.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, 0 no response needed.
*/
uint16_t ble_host_att_deal_signed_write_cmd(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.6.1 ATT_PREPARE_WRITE_REQ*****************/
/**
 *  @brief L2CAP ATT or EATT deal with prepare write request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or prepare write response.
 */
uint16_t ble_host_att_deal_prepare_write_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.6.3 ATT_EXECUTE_WRITE_REQ*****************/
/**
 *  @brief L2CAP ATT or EATT deal with execute write request.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, replay error response or execute write response.
 */
uint16_t ble_host_att_deal_execute_write_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);

/*****************core v5.4 Vol 3 Part F 3.4.7.3 ATT_HANDLE_VALUE_CFM******************/
/**
 *  @brief L2CAP ATT or EATT deal with handle value confirmation.
 *
 *  @param[in] att_deal_info ATT connection information.
 *  @param[in] pdu ATT PDU format.
 *  @param[in] pdu_len ATT PDU length.
 *
 *  @return Length of response data, 0 no response needed.
 */
uint16_t ble_host_att_deal_handle_value_cfm(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len);
