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

#ifndef TAGSDK_PORT_INC_PORTNV_H_
#define TAGSDK_PORT_INC_PORTNV_H_

#include "TagErrorType.h"
#include "TagNVItem.h"

/**
 * @brief Port-layer specific parameters or vendor specific data handle
 */
typedef void *PortNVHandle_t;

/**
 * @brief Contains a enumeration values for types of each NV opening-mode
 */
typedef enum {
    PORT_NV_READONLY,
    PORT_NV_READWRITE,
    PORT_NV_EXIST,
} PortNVOpenMode_t;

/**
 * @brief Contains each supported NV item information of the NV port-layer
 */
typedef struct {
    TagNVItem_t lastSupportedRW;
    TagNVItem_t lastSupportedRO;
    TagNVItem_t lastSupportedVendor;
} PortNVSupportedInfo_t;

/**
 * @brief Initialization of port-layer NV implementation
 *
 * @details This function will be triggered from TagCore to initialize port-layer NV implementation.
 *          This function will be called from TagNVInit() to prepare for usage of
 *          target specific NV(non-volatile memory) implementation.
 * @param[out] nvSupportedInfo Actual supported NV item information by the NV port-layer
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVInit(PortNVSupportedInfo_t *nvSupportedInfo);

/**
 * @brief Deinitialization of port-layer NV implementation
 *
 * @details This function will be triggered from TagCore to deinitialize port-layer NV implementation.
 *          This function will be called from TagNVDeinit() to release for usage of
 *          target specific NV(non-volatile memory) implementation.
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVDeinit(void);

/**
 * @brief Open NV item with its opening-mode
 *
 * @details This function tries to open with its opening-mode
 *          If your target needs to use lock-mechanism for the multiple instance based NV usage,
 *          please implement that in this function
 * @param[in] item Each type of TagNVItem_t to open it
 * @param[in] mode The NV opening-mode like as read-only to load, read/write to store
 * @return returns Allocated NV-handle poninter for success, NULL for failure
 *
 */
PortNVHandle_t* PortNVOpen(TagNVItem_t item, PortNVOpenMode_t mode);

/**
 * @brief Read data from the allocated NV-handle
 *
 * @details This function tries to read data from the allocated NV-handle
 * @param[in] handle A pointer of allocated NV-handle by PortNVOpen()
 * @param[in] dataBuf A pointer of memory buffer to read actual data from NV port-layer
 * @param[in] bufSz Actual size of 'dataBuf' (memory buffer size)
 * @param[out] readSz Actual read size of data from NV port-layer
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVRead(PortNVHandle_t *handle, void *dataBuf, size_t bufSz, size_t *readSz);

/**
 * @brief Write data to the allocated NV-handle
 *
 * @details This function tries to write data to the allocated NV-handle
 * @param[in] handle A pointer of allocated NV-handle by PortNVOpen()
 * @param[in] data A pointer of actual data space to write into NV port-layer
 * @param[in] dataSz Actual size of 'data' (actual data size)
 * @param[out] writtenSz Actual written size of data into NV port-layer
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVWrite(PortNVHandle_t *handle, const void *data, size_t dataSz, size_t *writtenSz);

/**
 * @brief Close the allocated NV-handle
 *
 * @details This function tries to close allocated NV-handle.
 * @param[in] handle Allocated NV-handle to close
 *
 */
void PortNVClose(PortNVHandle_t *handle);

/**
 * @brief Remove each NV item from NV port-layer
 *
 * @details This function tries to remove each NV item from NV port-layer
 *          TagCore use this API for the factory-reset also
 * @param[in] item Each type of TagNVItem_t to remove it
 *                 TagCore first calls this API with 'TAG_NV_RW_ITEM_ALL' for the factory-reset
 *                 (to remove all RW items), and then, if it has failed, TagCore calls again one by one
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVRemove(TagNVItem_t item);

/**
 * @brief Check each NV item's ability
 *
 * @details This function tries to check each NV item can do or not
 *          TagCore use this API for the checking of its existence mainly
 * @param[in] item Each type of TagNVItem_t to check it
 * @param[in] mode The NV ability(mode) for checking like as read-only, read or write & exist
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVAccess(TagNVItem_t item, PortNVOpenMode_t mode);

/**
 * @brief Set Tag's Aging-count value into NV port-layer
 *
 * @details This function tries to store Tag's latest Aging-count value into NV port-layer
 *          TagCore calls this API every 15 minutes to store changed Tag's Aging-count value
 *          So you have to consider wear-leveling implementation to prevent unwanted physical flash broken
 * @param[in] count Aging-count value to save
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVSetAgingCnt(unsigned int count);

/**
 * @brief Get Tag's Aging-count value from NV port-layer
 *
 * @details This function tries to load Tag's last saved Aging-count value from NV port-layer
 *          This API has to load last saved Aging-count from NV port-layer if it is available
 * @param[out] count Aging-count value to load
 * @return returns TAG_ERROR_NONE for success, otherwise for failure
 *
 */
TagError_t PortNVGetAgingCnt(unsigned int *count);

#endif /* TAGSDK_PORT_INC_PORTNV_H_ */
