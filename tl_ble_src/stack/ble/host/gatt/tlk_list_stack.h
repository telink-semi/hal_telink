/********************************************************************************************************
 * @file    tlk_list.h
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
#pragma once

#include <sys/queue.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
//  Single Linked List structure
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Definition for traverse the node callback function.
 * @param[in] node - the pointer of the node.
 * @return None.
 */
typedef int (*traverseNodeCb_t)(void *node);

/**
 * @brief Structure of a node in the single linked list.
 */
struct single_list_node
{
    SLIST_ENTRY(single_list_node)
    next; //!< Pointer to the next node.
};

/**
 * @brief Definition for the single linked list structure.
 */
SLIST_HEAD(single_list, single_list_node);

/**
 * @brief Definition a variable single linked list and initial.
 */
#define SLIST_DEF(name)         \
    struct single_list name = { \
        .slh_first = NULL,      \
    }

#define SLIST_INSERT_NODE_HEAD(sList, node) sList_headInsertNode((struct single_list *)(sList), (struct single_list_node *)(node))
#define SLIST_INSERT_TAIL(sList, node)      sList_tailInsertNode((struct single_list *)(sList), (struct single_list_node *)(node))
#define SLIST_INSERT_NODE(prevNode, node)   sList_insertNode((struct single_list_node *)(prevNode), (struct single_list_node *)(node))
#define SLIST_DELETE_HEAD(sList)            (void *)sList_deleteHeadNode((struct single_list *)(sList))
#define SLIST_DELETE_TAIL(sList)            (void *)sList_deleteTailNode((struct single_list *)(sList))
#define SLIST_DELETE_NODE(sList, node)      sList_deleteSpecificNode((struct single_list *)(sList), (struct single_list_node *)(node))


/**
 * @brief       Initialize the single linked list.
 * @param[in]   pList  - The single linked list need to use.
 * @return  None.
 */
void sList_initial(struct single_list *pList);

/**
 * @brief       single linked list insert a node into head.
 * @param[in]   pList  - The single linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void sList_headInsertNode(struct single_list *pList, struct single_list_node *pNode);

/**
 * @brief       single linked list insert a node into tail.
 * @param[in]   pList  - The single linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void sList_tailInsertNode(struct single_list *pList, struct single_list_node *pNode);

/**
 * @brief       single linked list insert a node into previous node.
 * @param[in]   pNode  - The pointer of the new node.
 * @param[in]   pRrev  - The pointer of the previous node.
 * @return  None.
 */
void sList_insertNode(struct single_list_node *pNode, struct single_list_node *pPrev);

/**
 * @brief       single linked list delete head node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_list_node *sList_deleteHeadNode(struct single_list *pList);

/**
 * @brief       single linked list delete tail node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_list_node *sList_deleteTailNode(struct single_list *pList);

/**
 * @brief       single linked list delete specific node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None..
 */
void sList_deleteSpecificNode(struct single_list *pList, struct single_list_node *pNode);

/**
 * @brief       single linked list check a node exist or not.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None.
 */
bool sList_checkNodeExist(struct single_list *pList, struct single_list_node *pNode);

/**
 * @brief       single linked list traverse all node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   cb  - The callback function for traverse node.
 * @return  None.
 */
void sList_traverseAllNode(struct single_list *pList, traverseNodeCb_t cb);

///////////////////////////////////////////////////////////////////////////////
//  Single Priority Linked List structure
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Definition for get priority value of the node callback function.
 * @param[in] node - the pointer of the node.
 * @return priority value.
 */
typedef int (*getNodePriorityCb_t)(void *node);

/**
 * @brief Definition for the single priority linked list structure.
 */
struct single_priority_list
{
    struct single_list  list; //!< include a single linked list.
    getNodePriorityCb_t cb;   //!< Pointer to the get priority value.
} __attribute__((packed));

/**
 * @brief Definition a variable single priority linked list and initial.
 */
#define SPLIST_DEF(name, callback)       \
    struct single_priority_list name = { \
        .list.slh_first = NULL,          \
        .cb             = callback,      \
    }

#define SPLIST_INSERT_NODE(spList, node) spList_insertNode((struct single_priority_list *)(spList), (struct single_list_node *)(node))
#define SPLIST_DELETE_NODE(spList, node) spList_deleteNode((struct single_priority_list *)(spList), (struct single_list_node *)(node))
#define SPLIST_DELETE_HEAD(spList, type) (type *)sList_deleteHeadNode((struct single_list *)(spList))
#define SPLIST_DELETE_PRIO(spList, prio) spList_deletNodeUsePriorityValue((struct single_priority_list *)(spList), prio)
#define SPLIST_TRAVERSE(spList, cb)      sList_traverseAllNode((struct single_list *)(spList), (traverseNodeCb_t)(cb))

/**
 * @brief       Initial the single priority linked list.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   cb      - the get priority value callback function.
 * @return None.
 */
void spList_initial(struct single_priority_list *pList, getNodePriorityCb_t cb);

/**
 * @brief       single priority linked list insert a node.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   pNode   - The pointer of the new node.
 * @return None.
 */
void spList_insertNode(struct single_priority_list *pList, struct single_list_node *pNode);

/**
 * @brief       single priority linked list delete a node.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   pNode   - The pointer of the had node.
 * @return None.
 */
void spList_deleteNode(struct single_priority_list *pList, struct single_list_node *pNode);

/**
 * @brief       single priority linked list delete all node of the same priority.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   priorityValue - the priority value.
 * @return None.
 */
void spList_deletNodeUsePriorityValue(struct single_priority_list *pList, int priorityValue);

///////////////////////////////////////////////////////////////////////////////
//  single tail Linked List structure
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Structure of a node in the single tail linked list.
 */
struct single_tail_list_node
{
    STAILQ_ENTRY(single_tail_list_node)
    next; //!< Pointer to the next node.
};

/**
 * @brief Definition for the single tail linked list structure.
 */
STAILQ_HEAD(single_tail_list, single_tail_list_node);

/**
 * @brief Definition a variable single tail linked list and initial.
 */
#define STLIST_DEF(name)                \
    struct single_tail_list name = {    \
        .stqh_first = NULL,             \
        .stqh_last  = &name.stqh_first, \
    }

#define STLIST_INIT(stList)                        stList_initial((struct single_tail_list *)(stList))
#define STLIST_INSERT_HEAD(stList, node)           stList_headInsertNode((struct single_tail_list *)(stList), (struct single_tail_list_node *)(node))
#define STLIST_INSERT_TAIL(stList, node)           stList_tailInsertNode((struct single_tail_list *)(stList), (struct single_tail_list_node *)(node))
#define STLIST_INSERT_NODE(stList, prevNode, node) stList_insertNode((struct single_tail_list *)(stList),        \
                                                                     (struct single_tail_list_node *)(prevNode), \
                                                                     (struct single_tail_list_node *)(node))
#define STLIST_DELETE_HEAD(stList, type) (type *)stList_deleteHeadNode((struct single_tail_list *)(stList))
#define STLIST_DELETE_TAIL(stList, type) (type *)stList_deleteTailNode((struct single_tail_list *)(stList))
#define STLIST_DELETE_NODE(stList, node) stList_deleteSpecificNode((struct single_tail_list *)(stList), (struct single_tail_list_node *)(node))


/**
 * @brief       Initialize the single tail linked list.
 * @param[in]   pList  - The single tail linked list need to use.
 * @return  None.
 */
void stList_initial(struct single_tail_list *pList);

/**
 * @brief       single tail linked list insert a node into head.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void stList_headInsertNode(struct single_tail_list *pList, struct single_tail_list_node *pNode);

/**
 * @brief       single tail linked list insert a node into tail.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void stList_tailInsertNode(struct single_tail_list *pList, struct single_tail_list_node *pNode);

/**
 * @brief       single tail linked list insert a node into previous node.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @param[in]   pRrev  - The pointer of the previous node.
 * @return  None.
 */
void stList_insertNode(struct single_tail_list *pList, struct single_tail_list_node *pNode, struct single_tail_list_node *pPrev);

/**
 * @brief       single tail linked list delete head node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_tail_list_node *stList_deleteHeadNode(struct single_tail_list *pList);

/**
 * @brief       single tail linked list delete tail node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_tail_list_node *stList_deleteTailNode(struct single_tail_list *pList);

/**
 * @brief       single tail linked list delete specific node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None..
 */
void stList_deleteSpecificNode(struct single_tail_list *pList, struct single_tail_list_node *pNode);


///////////////////////////////////////////////////////////////////////////////
//  queue structure based on single tail Linked List
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Definition for the queue structure.
 */
STAILQ_HEAD(queue_single_tail_list, single_tail_list_node);

#define QUEUE_DEF(name)   STLIST_DEF(name)

#define QUEUE_INIT(queue) STLIST_INIT(queue)
//removes the first element of the queue and returns.
#define QUEUE_POP(queue, type) STLIST_DELETE_HEAD(queue, type)
//return the first element of the queue.
#define QUEUE_FRONT(queue) STAILQ_FIRST((struct single_tail_list *)(queue))
//push the element at tail of the queue.
#define QUEUE_PUSH(queue, node) STLIST_INSERT_TAIL(queue, node)
//return the tail element of the queue.
#define QUEUE_BACK(queue, type) (type *)STAILQ_LAST((struct single_tail_list *)(queue), single_tail_list_node, next)
