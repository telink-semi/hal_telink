/********************************************************************************************************
 * @file    tl_queue.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef COMMON_TL_QUEUE_H_
#define COMMON_TL_QUEUE_H_

#include "types.h"

/**
 *  @brief Definition for priority calculate function
 */
typedef u32 (*priCmpCbFunc_t)(u32);

/**
 *  @brief Structure of an item in the Queue
 */
typedef struct queue_item
{
    struct queue_item *next; //!<  Pointer to the next element
} queue_item_t;

/**
 *  @brief Definition for the Queue structure
 */
typedef struct queue
{
    queue_item_t  *head;     //!<  Pointer to the head item of the queue
    queue_item_t  *tail;     //!<  Pointer to the tail item of the queue
    priCmpCbFunc_t priCmpCb; //!<  Priority function, NULL means not use priority
    u32            curNum;   //!<  The total number of enqueued items in the queue
} queue_t;

/**
 *  @brief Definition for the status of the Queue Module
 */
typedef enum
{
    QUEUE_SUCCESS,       //!< Queue operation success
    QUEUE_PARAM_INVALID, //!< Queue input parameter invalid
    QUEUE_EMPTY,         //!< Queue is empty
    QUEUE_NOT_FOUND,     //!< Queue is not found
    QUEUE_OVERFLOWED,    //!< Queue is overflowed
} queue_sts_t;

/*********************************************************************
 * @fn      queue_init
 * @brief   Initialize the queue
 * @param   pQueue  - The queue need to use
 * @param   priFunc - Pointer to the function that calculates a priority
 *                    NULL means not use the priority feature
 * @return  Status
 */
queue_sts_t queue_init(queue_t *pQueue, priCmpCbFunc_t priFunc);

/*********************************************************************
  * @fn          queue_enq
  * @brief       Enqueue an element to the tail of the specified queue
  * @param[in]   pQueue - The queue that a new element need to push to
  * @param[in]   pItem  - The payload of the new element
  * @return      Status
  */
queue_sts_t queue_enq(queue_t *pQueue, queue_item_t *pItem);

/*********************************************************************
 * @fn          queue_deq
 * @brief       Dequeue and element from the head of the specified queue.
 * @param[in]   pQueue - The specified queue
 * @return      Pointer to first element in the queue
 */
queue_item_t *queue_deq(queue_t *pQueue);

/*********************************************************************
  * @fn          queue_insert
  * @brief       Insert an element into the specified queue, usually
  *              used when iterating over elements in a Queue
  * @param[in]   pQueue - The queue that a new element need to push to
  * @param[in]   pItem  - The new element to be inserted
  * @param[in]   pPrev  - The element in the queue before the new element to be inserted.
  *                       Note: if the pPrev is NULL, the pItem is the 1st element in queue.
  * @return      Status
  */
queue_sts_t queue_insert(queue_t *pQueue, void *pItem, void *pPrev);

/*********************************************************************
  * @fn          queue_insert
  * @brief       Remove an element from the specified queue, usually
  *              used when iterating over elements in a Queue
  * @param[in]   pQueue - The queue that a target element need to be removed
  * @param[in]   pItem  - The target element to be removed
  * @param[in]   pPrev  - The element in the queue before the target element
  *                       to be removed. Note: if the pPrev is NULL, which
  *                       means the pItem is the 1st element in queue
  * @return      Status
  */
queue_sts_t queue_remove(queue_t *pQueue, void *pItem, void *pPrev);

/*********************************************************************
 * @fn          queue_delete
 * @brief       Delete an element from the queue (Do not use iterator)
 * @param[in]   pQueue - The specified queue
 * @param[in]   pItem  - Pointer to the target element to be removed
 * @return  Status
 */
queue_sts_t queue_delete(queue_t *pQueue, queue_item_t *pItem);

/*********************************************************************
 * @fn          queue_isempty
 * @brief       Check if the specified queue is empty
 * @param[in]   pQueue - The specified queue
 * @return      Return TRUE, if the specified queue is empty
 */
bool queue_isempty(queue_t *pQueue);

/*********************************************************************
 * @fn          queue_count
 * @brief       Count the number of elements in the queue
 * @param[in]   pQueue - The specified queue
 * @return      Number of elements in queue
 */
u32 queue_count(queue_t *pQueue);


#endif /* COMMON_TL_QUEUE_H_ */
