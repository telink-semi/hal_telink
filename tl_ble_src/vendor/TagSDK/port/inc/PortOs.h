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

#ifndef TAGSDK_PORT_INC_PORTOS_H_
#define TAGSDK_PORT_INC_PORTOS_H_

#include <stdbool.h>
#include <stddef.h>

#include "TagErrorType.h"

#include "PortOsDataType.h"

/**
 * @brief       Allocate memory
 *
 * @param[in]   size    size of memory to allocate in bytes.
 * @details     This function allocate memory at heap space.
 * @return      returns allocated memory pointer if success, null for failure.
 *
 */
void* TagMalloc(size_t size);

/**
 * @brief       Free memory
 *
 * @param[in]   ptr pointer of memory to free.
 * @details     This function frees allocated memory at heap space.
 *
 */
void TagFree(void *ptr);

/**
 * @brief       Timer callback function prototype
 * @see @ref    PortTimerCreate
 */
typedef void (*PortTimerCallbackFunction_t)( PortTimerHandle_t timer );

/**
 * @brief       Software timer create
 *
 * @param[in]   timerName   Human readable text name for the timer.
 *                          This isn't impact timer's operation. This is only for debugging purpose.
 * @param[in]   period      The period of timer. The period is specified in ticks and the macro
 *                          CONV_MS_TO_TICKS() can be used to convert a time in milliseconds to ticks.
 * @param[in]   autoReload  If autoReload is set to true, then the timer will expire repeatedly
 *                          with a frequency set by period parameter. If autoReload is set to false, then
 *                          the timer will be a one-shot and enter the dormant state after it expires.
 * @param[in]   timerId     An identifier that is assigned to the timer being created. Typically this
 *                          would be used int the timer callback function to identify which timer expired
 *                          when the same callback function is assigned to more than one timer, or together
 *                          with the PortTimerGetTimerId() and PortTimerSetTimerId() functions to save a value
 *                          between calls to the timer's callback function.
 * @param[in]   callbackFunc    The function to call when the timer expires. Callback functions must have the
 *                              prototype defined by PortTimerCallbackFunction_t.
 * @details     Creates a new software timer instance and returns a handle by which the timer can be referenced.
 * @return      returns     Newly allocated handle pointer for success. NULL for failure.
 *
 */
PortTimerHandle_t PortTimerCreate(const char* timerName, uint32_t period, bool autoReload,
                                  void* const timerId, PortTimerCallbackFunction_t callbackFunc);

/**
 * @brief       Get timer id
 *
 * @param[in]   timer    The timer handle being queried.
 * @details     This function returns the ID assigned to the software timer.
 * @return      returns The ID assigned to the timer being queried.
 *
 */
void *PortTimerGetTimerId(PortTimerHandle_t timer);

/**
 * @brief       Set timer id
 *
 * @param[in]   timer    The timer handle being updated.
 * @param[in]   newId    The handle to which the timer's identifier will be set.
 * @details     This function changes an timer ID which assigned to a software timer.
 *
 */
void PortTimerSetTimerId(PortTimerHandle_t timer, void *newId);

/**
 * @brief       Change the period of a timer
 *
 * @param[in]   timer       The timer handle that is having its period changed.
 * @param[in]   newPeriod   The new period for timer, in ticks. CONV_MS_TO_TICKS() can be
 *                          used to convert a time to ticks
 * @param[in]   blockTime   The time in ticks which the calling task should be held in the blocked state
 *                          to wait for the change period command to be successfully sent to the timer command queue
 * @details     This function changes the period of a timer that was previously created with the PortTimerCreate() API.
 * @retval      TAG_ERROR_NONE              The command was successfully sent to the timer command queue.
 * @retval      TAG_ERROR_OPERATION_FAILURE The command could not be sent to the timer command queue.
 *
 */
TagError_t PortTimerChangePeriod(PortTimerHandle_t timer, uint32_t newPeriod, uint32_t blockTime);

/**
 * @brief       Start a timer
 *
 * @param[in]   timer       The timer handle being started/restarted.
 * @param[in]   blockTime   The time in ticks which the calling task should be held in the blocked state
 *                          to wait for the change period command to be successfully sent to the timer command queue
 * @details     This function starts a timer that was previously created with PortTimerCreate() API. If the timer had
 *              already been started and was already in the active state, then PortTimerStart() has equivalent
 *              functionality to the PortTimerReset() API.
 * @retval      TAG_ERROR_NONE              The command was successfully sent to the timer command queue.
 * @retval      TAG_ERROR_OPERATION_FAILURE The command could not be sent to the timer command queue
 *
 */
TagError_t PortTimerStart(PortTimerHandle_t timer, uint32_t blockTime);

/**
 * @brief       Re-start a timer
 *
 * @param[in]   timer       The timer handle being reset/started/restarted.
 * @param[in]   blockTime   The time in ticks which the calling task should be held in the blocked state
 *                          to wait for the change period command to be successfully sent to the timer command queue
 * @details     This function re-starts a timer that was previously created using the PortTimerCreate() API.
 *              If the timer had already been started and was already in the active state, then the PortTimerReset()
 *              will cause the timer to re-evaluate its expiry time so that it is relative to when PortTimerReset()
 *              was called. If the timer was in the dormant state then PortTimerReset() has equivalent functionality
 *              to the PortTimerStart() API
 * @retval      TAG_ERROR_NONE              The command was successfully sent to the timer command queue.
 * @retval      TAG_ERROR_OPERATION_FAILURE The command could not be sent to the timer command queue
 *
 */
TagError_t PortTimerReset(PortTimerHandle_t timer, uint32_t blockTime);

/**
 * @brief       Stop a timer
 *
 * @param[in]   timer       The timer handle being stopped.
 * @param[in]   blockTime   The time in ticks which the calling task should be held in the blocked state
 *                          to wait for the change period command to be successfully sent to the timer command queue
 * @details     This function stops a timer that was previously started using either of the PortTimerStart(),
 *              PortTimerReset() and PortTimerChangePeriod().
 * @retval      TAG_ERROR_NONE              The command was successfully sent to the timer command queue.
 * @retval      TAG_ERROR_OPERATION_FAILURE The command could not be sent to the timer command queue
 *
 */
TagError_t PortTimerStop(PortTimerHandle_t timer, uint32_t blockTime);

/**
 * @brief       Delete a timer
 *
 * @param[in]   timer       The timer handle being deleted.
 * @param[in]   blockTime   The time in ticks which the calling task should be held in the blocked state
 *                          to wait for the change period command to be successfully sent to the timer command queue
 * @details     This function deletes a timer that was previously created using the PortTimerCreate() API
 * @retval      TAG_ERROR_NONE              The command was successfully sent to the timer command queue.
 * @retval      TAG_ERROR_OPERATION_FAILURE The command could not be sent to the timer command queue
 *
 */
TagError_t PortTimerDelete(PortTimerHandle_t timer, uint32_t blockTime);

/**
 * @brief       Queries a software timer to see if it is active or dormant.
 *
 * @param[in]   timer       The timer handle being queried.
 * @details     This function queries a software timer to see if it is active or dormant.
 * @return      true if timer is active, otherwise false.
 *
 */
bool PortTimerIsTimerActive(PortTimerHandle_t timer);

/**
 * @brief       Create a new queue and returns a handle.
 *
 * @param[in]   queueLength     The maximum number of items the queue can hold at any one time.
 * @param[in]   itemSize        The size, in bytes, required to hold each item in the queue.
 * @details     This function creates a new queue and returns a handle.
 * @return      Newly allocated handle pointer for success. NULL for failure.
 *
 */
PortQueueHandle_t PortQueueCreate(unsigned long queueLength, size_t itemSize);

/**
 * @brief       Resets a queue.
 *
 * @param[in]   queue   The handle of queue being reset
 * @details     This function resets a queue to its original empty state.
 * @return      TAG_ERROR_NONE for success, otherwise failure.
 *
 */
TagError_t PortQueueReset(PortQueueHandle_t queue);

/**
 * @brief       Post an item on a queue.
 *
 * @param[in]   queue       The handle of queue on which the item is to be posted.
 * @param[in]   itemToQueue A pointer to the item that is to be placed on the queue.
 * @param[in]   ticksToWait The maximum amount of time the task should block waiting
 *                          for space to become available on the queue.
 * @details     This function post an item on a queue. The item is queued by copy, not by reference.
 * @retval      TAG_ERROR_NONE  the item was successfully posted.
 *              TAG_ERROR_OPERATION_FAILURE failed to post the item.
 *
 */
TagError_t PortQueueSend(PortQueueHandle_t queue, const void *itemToQueue, uint32_t ticksToWait);

/**
 * @brief       Return the number of messages stored in a queue.
 *
 * @param[in]   queue   The handle of queue being queried.
 * @details     This function returns the number of messages stored in a queue.
 * @return      The number of messages available in the queue.
 *
 */
unsigned long PortQueueMessagesWaiting(PortQueueHandle_t queue);

/**
 * @brief       Receive an item from a queue.
 *
 * @param[in]   queue       The handle of queue which the item is to be received.
 * @param[out]  buffer      Pointer to the buffer into which the received item will be copied.
 * @param[in]   ticksToWait The maximum amount of time the task should block waiting
 *                          for an item to receive should the queue be empty at the time of the call.
 * @details     This function receives an item from a queue.
 *              The item is received by copy so a buffer of adequate size must be provided.
 * @retval      TAG_ERROR_NONE  the item was successfully received from the queue.
 *              TAG_ERROR_OPERATION_FAILURE failed to receive item from the queue.
 *
 */
TagError_t PortQueueReceive(PortQueueHandle_t queue, void *buffer, uint32_t ticksToWait);

/**
 * @brief       Return the number of free spaces in a queue.
 *
 * @param[in]   queue   The handle of queue being queried.
 * @details     This function returns the number of free spaces in a queue.
 * @return      The number of free spaces available in the queue.
 *
 */
unsigned long PortQueueSpacesAvailable(PortQueueHandle_t queue);

/**
 * @brief       Delete a queue
 *
 * @param[in]   queue   The handle of queue to be deleted.
 * @details     This function delete a queue - freeing all the memory allocated
 *              for storing of items placed on the queue
 *
 */
void PortQueueDelete(PortQueueHandle_t queue);

/**
 * @brief       Create an event group
 *
 * @details     This function creates a new event group, and returns a handle by which
 *              the newly created event group can be referenced.
 * @return      Newly created event group is returned for success, NULL for failed.
 *
 */
PortEventGroupHandle_t PortEventGroupCreate(void);

/**
 * @brief       Set bits within an event group.
 *
 * @param[in]   eventGroup      The event group in which the bits are to be set.
 * @param[in]   bitsToSet       A bitwise value that the bit or bits to set in the event group.
 * @details     This function set bits within an event group. Setting bits in an event group will automatically
 *              unblock tasks taht are blocked waiting for the bits.
 * @return      The value of the event group at the time the call to PortEventGroupSetBits() returns.
 *
 */
uint32_t PortEventGroupSetBits(PortEventGroupHandle_t eventGroup, uint32_t bitsToSet);

/**
 * @brief       Read bits within an event group.
 *
 * @param[in]   eventGroup      The event group in which the bits are being tested.
 * @param[in]   bitsToWaitFor   A bitwise value that indicates the bit or bits to test inside the event group.
 *                              This value must not be 0.
 * @param[in]   clearOnExit     If this value is set to true then any bits set in the value passed as the bitsToWaitFor
 *                              parameter will be cleared in the event group before PortEventGroupWaitBits() returns if
 *                              PortEventGroupWaitBits() returns for any reason other than a timeout.
 * @param[in]   waitForAllBits  This is used to create either a logical AND test or logical OR test. If this value is
 *                              true then PortEventGroupWaitBits() will return when either all the bits set in the
 *                              value passed as the bitsToWaitFor parameter are set in the event group or the specified
 *                              block time expires. If this is set as false then PortEventGroupWaitBits() will return
 *                              when any of the bits set in the value passed as the bitsToWaitFor parameter are set in
 *                              the event group or the specified block time expires.
 * @param[in]   ticksToWait     The maximum amount of time to wait for the bits specified by bitsToWaitFor to become set
 *                              in ticks.
 * @details     This function read bits within an event group, optionally entering the blocked state with timeout.
 * @return      The value of the event group at the time either the event bits being waited for became set,
 *              or the block time expired.
 *
 */
uint32_t PortEventGroupWaitBits(PortEventGroupHandle_t eventGroup, uint32_t bitsToWaitFor,
                                bool clearOnExit, bool waitForAllBits, uint32_t ticksToWait);

/**
 * @brief       Delete an event group
 *
 * @param[in]   eventGroup  The event group being deleted.
 * @details     This function delete an event group that was previously created using a call to PortEventGroupCreate().
 *
 */
void PortEventGroupDelete(PortEventGroupHandle_t eventGroup);

/**
 * @brief       Task callback function prototype
 * @see @ref    PortTaskCreate
 */
typedef void (*PortTaskFunction_t)( void * );

/**
 * @brief       Create a new task and make it ready to run.
 *
 * @param[in]   taskCode    Pointer to the task entry function
 * @param[in]   name        A descriptive name for the task.
 * @param[in]   stackDepth  The number of words (not bytes) to allocate for use as task's stack.
 * @param[in]   parameters  A value that is passed as the parameter to the created task.
 * @param[in]   priority    The priority at which the created task will execute.
 * @param[out]  handle      Used to pass a handle to the created task out of the PortTaskCreate().
 *                          This is optional and can be set to NULL.
 * @details     This function creates a new task and make it ready to run.
 * @retval      TAG_ERROR_NONE      The task was created successfully.
 * @retval      TAG_ERROR_MEM_ALLOC Failed to create the task.
 *
 */
TagError_t PortTaskCreate(PortTaskFunction_t taskCode, const char * name, uint16_t stackDepth,
                          void *parameters, unsigned long priority, PortTaskHandle_t* handle);

/**
 * @brief       Remove a task.
 *
 * @param[in]   task    The handle of the task to be deleted. Passing NULL will cause the calling task to be deleted.
 * @details     This function removes task.
 *
 */
void PortTaskDelete(PortTaskHandle_t task);

/**
 * @brief       Delay a task for a given number of ticks.
 *
 * @param[in]   ticksToDelay    The amount of time, in tick periods, that the calling task should block.
 * @details     This function delays a task for a given number of ticks. CONV_MS_TO_TICKS() can be used to
 *              calculate delay ticks from time.
 *
 */
void PortTaskDelay(uint32_t ticksToDelay);

/**
 * @brief       Prepare for Tag Main Task, if any.
 *
 * @details     This function will be called at the very beginning of Tag Main Task.
 *              One way is to allocate secure context for Tag Main Task.
 * 
 */
void PortPrepareMainTask(void);

#endif /* TAGSDK_PORT_INC_PORTOS_H_ */
