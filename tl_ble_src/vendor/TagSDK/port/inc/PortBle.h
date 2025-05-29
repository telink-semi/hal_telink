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

#ifndef TAGSDK_PORT_INC_PORTBLE_H_
#define TAGSDK_PORT_INC_PORTBLE_H_


#include "PortBleDataType.h"

#include "TagCore.h"
#include "TagBleCallback.h"

/** @struct PortBleAdvStruct
 * @brief BLE Advertising data struct
 */
typedef struct
{
    uint8_t         length;     /**< Total length of the [adType + aData] fields. Equal to 1 + lengthOf(aData). */
    uint32_t        adType;     /**< AD Type of this AD Structure. */
    uint8_t*        aData;      /**< Data contained in this AD Structure; length of this array is equal to (length - 1). */
} PortBleAdvStruct;

/** @struct PortBleAdvData
 * @brief BLE Advertising data struct array
 */
typedef struct {
    uint8_t             cNumAdStructures;   /**< Number of AD Structures. */
    PortBleAdvStruct*   aAdStructures;      /**< Array of AD Structures. */
} PortBleAdvData;

/** @enum PortBleAdvType
 */
typedef enum {
    PortBleAdvConnectableUndirected,     /**< Connectable and scannable undirected advertising events */
    PortBleAdvNonConnectable,            /**< Non-connectable scannable undirected advertising events*/
} PortBleAdvType;

/** @enum PortBleAdvOwnAddressType
 */
typedef enum {
    PortBleAdvAddrTypePublic,     /**< Static Public Address */
    PortBleAdvAddrTypeRandom,     /**< Resolvable Private Address */
} PortBleAdvOwnAddressType;

/** @struct PortBleAdvParams
 * @brief BLE advertising parameters configure
 */
typedef struct {
    uint16_t                            minInterval;            /**< Minimum desired advertising interval. Default: 1.28 s. */
    uint16_t                            maxInterval;            /**< Maximum desired advertising interval. Default: 1.28 s. */
    PortBleAdvType                      advertisingType;        /**< Advertising type, See @ref PortBleAdvType */
    PortBleAdvOwnAddressType            ownAddressType;         /**< Indicates whether the advertising address is the public address or the random address */
    uint8_t                             needAddrChange;         /**< indicator if it need to change random address */
    int8_t txPower;                          /**< Advertising tx power */
} PortBleAdvParams;

/** @enum PortBleTxPowerScenarioType
 * @brief BLE Tx power change scenario type
 */
typedef enum {
    PortBleTxPowerBootingAdvStrong,       /**< Sent when TagStart function called under OOB state. Tag should set advertising Tx power weak */
    PortBleTxPowerBootingAdvWeak,         /**< Sent when right after onboarding process is done. Tag should set advertising Tx power strong */
    PortBleTxPowerOnboardedAdvStrong,      /**< Sent when TagStart function called under not OOB state  */
    PortBleTxPowerManual /**< Sent when manual tx power needed  */
} PortBleTxPowerScenarioType;


/**
 * @brief       Initialize BLE stack
 *
 * @details     Prepare chipset ble stack before tag initialzie GATT DB and ble advertising.
 *
 * @retval      TAG_ERROR_NONE for Success
 */
TagError_t PortBleInit(void);

/**
 * @brief       add a service in Gatt DB
 *
 * @details     This function add a service in Gatt DB
 *
 * @param[in]   service_type value of service to add.
 *                 Tag has 3 type of services, which are AUTH_SERVICE, CONTROL_SERVICE and ONBOARDING_SERVICE.
 *
 * @retval         TAG_ERROR_NONE for Success
 * @retval         TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval         TAG_ERROR_BLE_GATT_DB_REGISTER_FAIL failed to add service in Gatt DB
 */
TagError_t PortBleAddGattDbService(ServiceType service_type);

/**
 * @brief       add characteristics in Gatt DB
 *
 * @details     This function add characteristics in Gatt DB
 *
 * @param[in]   service_type value of service to add characteristics.
 *                 Tag has 3 type of services, which are AUTH_SERVICE, CONTROL_SERVICE and ONBOARDING_SERVICE.
 *                And, specially control service need more characteristic for overmature status.
 *
 * @retval         TAG_ERROR_NONE for Success
 * @retval         TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval         TAG_ERROR_BLE_GATT_DB_UNREGISTER_FAIL failed to add characteristic in Gatt DB
 */
TagError_t PortBleAddGattDbCharacteristic(ServiceType service_type);

/**
 * @brief       remove a service in Gatt DB
 *
 * @details     This function remove a service in Gatt DB
 *
 * @param[in]   service_type value of service to remove.
 *                Tag has 3 type of services, which are AUTH_SERVICE, CONTROL_SERVICE and ONBOARDING_SERVICE.
 *
 * @retval         TAG_ERROR_NONE for Success
 * @retval         TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval         TAG_ERROR_BLE_GATT_DB_REGISTER_FAIL failed to remove service in Gatt DB
 * @retval        TAG_ERROR_NOT_SUPPORTED remove feature in gatt db is not supported
 */
TagError_t PortBleRemoveGattDbService(ServiceType service_type);

/**
 * @brief       remove characteristics in Gatt DB
 *
 * @details     This function remove characteristics in Gatt DB
 *
 * @param[in]   service_type value of service to remove characteristics.
 *                 Tag has 3 type of services, which are AUTH_SERVICE, CONTROL_SERVICE and ONBOARDING_SERVICE.
 *                And, specially control service need more characteristic for overmature status.
 *
 * @retval         TAG_ERROR_NONE for Success
 * @retval         TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval         TAG_ERROR_BLE_GATT_DB_REGISTER_FAIL failed to remove service in Gatt DB
 * @retval        TAG_ERROR_NOT_SUPPORTED remove feature in gatt db is not supported
 */
TagError_t PortBleRemoveGattDbCharacteristic(ServiceType service_type);

/** @brief Stop ble advertising
 *
 * @retval TAG_ERROR_NONE Successfully stop advertising
 * @retval TAG_ERROR_BLE_BASE Ble Operation failed
 */
TagError_t PortBleStopAdv(void);

/** @brief Start ble advertising
 *
 * @details Start ble advertising with advData and params configure.
 *          If there is advertising going on, refresh advertising.
 *
 * @param[in] advData Contains BLE advertising data structs. See @ref PortBleAdvData
 * @param[in] params BLE advertisig parameters like interval or address type etc. See @ref PortBleAdvParams
 *
 * @retval TAG_ERROR_NONE Successfully start advertising
 * @retval TAG_ERROR_MEME_ALLOC Fail to allocate internal memory
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval TAG_ERROR_BLE_BASE Ble Operation failed
 */
TagError_t PortBleStartAdv(PortBleAdvData *advData, PortBleAdvParams *params);

/** @brief Get MTU size for given connection
 *
 * @param[in] connInfo not-null PortBleConnInfo pointer having connection handle info
 * @param[out] outMtu MTU size for given connection
 *
 * @retval TAG_ERROR_NONE Successfully start advertising
 * @retval TAG_ERROR_BLE_BASE Ble Operation failed
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 */
TagError_t PortBleGetMtu(PortBleConnInfo *connInfo, uint16_t *outMtu);

/** @brief Set Tx power
 *
 * @details Set advertising and connection Ble Tx power according to type
 *          See @ref PortBleTxPowerScenarioType
 *
 * @param[in] type BLE Tx change scenario type
 *
 * @retval TAG_ERROR_NONE Successfully changed Tx power
 * @retval TAG_ERROR_BLE_BASE Ble Operation failed
 * @retval TAG_ERROR_INVALID_ARG given type is not specified.
 */
TagError_t PortBleSetTxPower(PortBleTxPowerScenarioType type, int8_t txPower);

/** @brief Request BLE connection parameters
 *
 * @details Request BLE connection parameters
 *
 * @param[in] connInfo not-null PortBleConnInfo pointer having connection handle info
 * @param[in] intervalMin Minimum Connection Interval in 1.25 ms units
 * @param[in] intervalMax Maximum Connection Interval in 1.25 ms units
 * @param[in] slaveLatency Slave Latency in number of connection events
 * @param[in] timeoutMultiplier Connection Supervision Timeout in 10 ms units
 *
 * @retval TAG_ERROR_NONE Successfully request connection parameters
 * @retval TAG_ERROR_BLE_BASE Ble Operation failed
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 */
TagError_t PortBleRequestConnectionParameters(PortBleConnInfo *connInfo, uint16_t intervalMin, uint16_t intervalMax, uint16_t slaveLatency, uint16_t timeoutMultiplier);

/**
 * @brief       disconnect a connection
 *
 * @details     This function disconnect a connection
 *
 * @param[in]   connInfo pointer of PortBleConnInfo containing connection information needed for each solution
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGapDisconnect(PortBleConnInfo *connInfo);

/**
 * @brief       send reply to client about attribute written status
 *
 * @details     This function send reply to client about attribute written status
 *
 * @param[in]   event pointer of BleGattData contatining write request connection information, characteristic handle, read data and length
 * @param[in]   status written status value to send to client
 *
 * @retval      TAG_ERROR_NONE for success
 * @retval        TAG_ERROR_BLE_GATT_WRITE_CONFIRM_FAIL failed to send written status
 */
TagError_t PortBleGattsSendAttrWrittenStatus(BleGattData *gattData, TagBleError_t status);

/**
 * @brief       send reply to client about attribute read status
 *
 * @details     This function send reply to client about attribute read status
 *
 * @param[in]   event pointer of BleGattData contatining read request connection information, characteristic handle, read data and length
 * @param[in]   status read status value to send to client
 *
 * @retval      TAG_ERROR_NONE for success
 * @retval        TAG_ERROR_BLE_GATT_READ_CONFIRM_FAIL failed to send read status
 */
TagError_t PortBleGattsSendAttrReadStatus(BleGattData *gattData, TagBleError_t status);

/**
 * @brief       Get attribute value
 *
 * @details     This function get attribute value stored in GATT DB or similar.
 *
 * @param[in]   attrInfo pointer of PortBleAttrInfo containing attribute information needed for each solution
 * @param[out]  valueBuf pointer of value buffer to get attribute value
 * @param[in]   bufSize size of value buffer
 * @param[out]  outLen written length of output
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGattGetAttrValue(PortBleAttrInfo* attrInfo, unsigned char *valueBuf, size_t bufSize, size_t *outLen);
/**
 * @brief       Set attribute value
 *
 * @details     This function set attribute value to GATT DB or similar.
 *
 * @param[in]   attrInfo pointer of PortBleAttrInfo containing attribute information needed for each solution
 * @param[in]   attrValue attribute value to set
 * @param[in]   attrValueLen attribute value length to set
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGattSetAttrValue(PortBleAttrInfo *attrInfo, unsigned char *attrValue, size_t attrValueLen);
/**
 * @brief       send indication
 *
 * @details     This function send indication with input value
 *
 * @param[in]   connInfo pointer of PortBleConnInfo containing connection information needed for each solution
 * @param[in]   attrInfo pointer of PortBleAttrInfo containing attribute information needed for each solution
 * @param[in]   attrValue attribute value to send
 * @param[in]   attrValueLen attribute value length to send
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGattSendIndication(PortBleConnInfo *connInfo, PortBleAttrInfo* attrInfo, unsigned char *attrValue, size_t attrValueLen);
/**
 * @brief       send notification
 *
 * @details     This function send notification with input value
 *
 * @param[in]   connInfo pointer of PortBleConnInfo containing connection information needed for each solution
 * @param[in]   attrInfo pointer of PortBleAttrInfo containing attribute information needed for each solution
 * @param[in]   attrValue attribute value to send
 * @param[in]   attrValueLen attribute value length to send
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGattSendNotification(PortBleConnInfo *connInfo, PortBleAttrInfo* attrInfo, unsigned char *attrValue, size_t attrValueLen);

/** @brief Copy ble connection handle
 *
 * @details Copy src ble connection handle info to dest
 *
 * @param[out] dest not-null, memory-allocated empty PortBleConnInfo struct pointer
 * @param[in] src not-null PortBleConnInfo pointer having connection handle info
 *
 * @retval TAG_ERROR_NONE Successfully copied
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval TAG_ERROR_MEME_ALLOC Fail to allocate internal connection handle info
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 */
TagError_t PortBleCopyConnHandle(PortBleConnInfo* dest, PortBleConnInfo* src);

/** @brief Destroy ble connection handle
 *
 * @details Free or deinit connection handle struct if there was memroy allocation in the struct
 *
 * @param[in] connInfo not-null PortBleConnInfo pointer having connection handle info
 *
 * @retval TAG_ERROR_NONE Successfully destroyed
 * @retval TAG_ERROR_INVALID_ARG Invalid arguments passed
 * @retval TAG_ERROR_OPERATION_FAILURE Operation failed
 */
TagError_t PortBleDestroyConnHandle(PortBleConnInfo* connInfo);

/** @brief Check if two connection handles are equal
 *
 * @details Check if handle1 and handle2 indicate same ble device
 *
 * @param[in] handle1 not-null PortBleConnInfo pointer having connection handle info
 * @param[in] handle2 not-null PortBleConnInfo pointer having connection handle info
 *
 * @retval true Two connections are eqaul
 * @retval false Two connections are not equal
 */
bool PortBleIsEqualConnHandle(PortBleConnInfo* handle1, PortBleConnInfo* handle2);

/**
 * @brief       change attrInfo by using index
 *
 * @details     This function use a characteristic index to find right attribute value and changes the attrInfo
 *
 * @param[out]  attrInfo pointer of PortBleAttrInfo to contain attribute information needed for each solution
 * @param[in]   characteristicIndex index of characteristic
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleChangeAttrInfoByIndex(PortBleAttrInfo* attrInfo, int characteristicIndex);

/**
 * @brief       Reply bondig request.
 *
 * @details     Reply bondig request

 * @param[in]   connInfo pointer of PortBleConnInfo containing connection information needed for each solution
 * @param[in]   reply indication if accpet bonding request or not. true - accept bonding request, false - reject bonding request.
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGapBondingReply(PortBleConnInfo *connInfo, bool reply);

/**
 * @brief       Remove other bonding information except the connInfo
 *
 * @details     Remove other bonding information except the connInfo,
 *              If connInfo is NULL, delete all bonding information.
 *
 * @param[in]   connInfo pointer of PortBleConnInfo containing connection information needed for each solution.
 *
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 */
TagError_t PortBleGapRemoveOtherBondings(PortBleConnInfo *connInfo);
#endif /* TAGSDK_PORT_INC_PORTBLE_H_ */
