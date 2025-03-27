/********************************************************************************************************
 * @file    tlk_list.c
 *
 * @brief   This is the source file for BLE SDK
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

#include "tlk_list_stack.h"

///////////////////////////////////////////////////////////////////////////////
//  Single Linked List structure
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief       Initialize the single linked list.
 * @param[in]   pList  - The single linked list need to use.
 * @return  None.
 */
void sList_initial(struct single_list* pList)
{
    SLIST_INIT(pList);
}

/**
 * @brief       single linked list insert a node into head.
 * @param[in]   pList  - The single linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void sList_headInsertNode(struct single_list* pList, struct single_list_node* pNode)
{
    if(sList_checkNodeExist(pList, pNode))  return ;


    SLIST_NEXT(pNode, next) = NULL;
    SLIST_INSERT_HEAD(pList, pNode, next);
}

/**
 * @brief       single linked list insert a node into tail.
 * @param[in]   pList  - The single linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void sList_tailInsertNode(struct single_list* pList, struct single_list_node* pNode)
{
    struct single_list_node *cur;
    struct single_list_node *prev = NULL;
    SLIST_FOREACH(cur, pList, next) {
        if(cur == pNode)    return ;
        prev = cur;
    }
    if(prev == NULL) {
        SLIST_NEXT(pNode, next) = NULL;
        SLIST_INSERT_HEAD(pList, pNode, next);
    } else {
        SLIST_INSERT_AFTER(prev, pNode, next);
    }
}

/**
 * @brief       single linked list insert a node into previous node.
 * @param[in]   pNode  - The pointer of the new node.
 * @param[in]   pRrev  - The pointer of the previous node.
 * @return  None.
 */
void sList_insertNode(struct single_list_node* pNode, struct single_list_node* pPrev)
{
    SLIST_NEXT(pNode, next) = NULL;
    SLIST_INSERT_AFTER(pPrev, pNode, next);
}

/**
 * @brief       single linked list delete head node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_list_node* sList_deleteHeadNode(struct single_list* pList)
{
    struct single_list_node* head = SLIST_FIRST(pList);
    SLIST_REMOVE_HEAD(pList, next);
    return head;
}

/**
 * @brief       single linked list delete tail node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_list_node* sList_deleteTailNode(struct single_list* pList)
{
    if(SLIST_EMPTY(pList))      return NULL;

    struct single_list_node *cur;
    struct single_list_node *prev = NULL;
    SLIST_FOREACH(cur, pList, next) {
        prev = cur;
    }

    SLIST_REMOVE(pList, prev, single_list_node, next);

    return prev;
}

/**
 * @brief       single linked list delete specific node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None..
 */
void sList_deleteSpecificNode(struct single_list* pList, struct single_list_node* pNode)
{
    struct single_list_node *cur = NULL;
    SLIST_FOREACH(cur, pList, next) {
        if(cur == pNode) {
            SLIST_REMOVE(pList, pNode, single_list_node, next);
            return ;
        }
    }
}

/**
 * @brief       single linked list check a node exist or not.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None.
 */
bool sList_checkNodeExist(struct single_list* pList, struct single_list_node* pNode)
{
    struct single_list_node *cur = NULL;
    SLIST_FOREACH(cur, pList, next) {
        if(cur == pNode) {
            return true;
        }
    }
    return false;
}

/**
 * @brief       single linked list traverse all node.
 * @param[in]   pList  - The single linked list that delete a node.
 * @param[in]   cb  - The callback function for traverse node.
 * @return  None.
 */
void sList_traverseAllNode(struct single_list* pList, traverseNodeCb_t cb)
{
    struct single_list_node *cur = NULL;
    SLIST_FOREACH(cur, pList, next) {
        if(cb(cur))     return ;
    }
}

///////////////////////////////////////////////////////////////////////////////
//  Single Priority Linked List structure
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief       Initial the single priority linked list.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   cb      - the get priority value callback function.
 * @return None.
 */
void spList_initial(struct single_priority_list* pList, getNodePriorityCb_t cb)
{
    sList_initial(&pList->list);
    pList->cb = cb;
}

/**
 * @brief       single priority linked list insert a node.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   pNode   - The pointer of the new node.
 * @return None.
 */
void spList_insertNode(struct single_priority_list* pList, struct single_list_node* pNode)
{
    if(pList->cb == NULL)   return ;

    struct single_list* sList = &pList->list;

    struct single_list_node *cur;
    struct single_list_node *prev = NULL;

    int newPriorityVal = pList->cb(pNode);

    SLIST_FOREACH(cur, sList, next) {
        if(cur == pNode)
        {
            return ;
        }

        if(pList->cb(cur) > newPriorityVal) {
            break;
        }

        prev = cur;
    }

    if(prev == NULL) {
        SLIST_NEXT(pNode, next) = NULL;
        SLIST_INSERT_HEAD(sList, pNode, next);
    } else {
        SLIST_INSERT_AFTER(prev, pNode, next);
    }

}

/**
 * @brief       single priority linked list delete a node.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   pNode   - The pointer of the had node.
 * @return None.
 */
void spList_deleteNode(struct single_priority_list* pList, struct single_list_node* pNode)
{
    return sList_deleteSpecificNode(&pList->list, pNode);
}

/**
 * @brief       single priority linked list delete all node of the same priority.
 * @param[in]   pList   - The single priority linked list need to use.
 * @param[in]   priorityValue - the priority value.
 * @return None.
 */
void spList_deletNodeUsePriorityValue(struct single_priority_list* pList, int priorityValue)
{
    struct single_list* sList = &pList->list;

    if(pList->cb == NULL || SLIST_EMPTY(sList))
    {
        return ;
    }

    struct single_list_node *prev = SLIST_FIRST(sList);
    struct single_list_node *cur = SLIST_NEXT(prev, next);

    for(; cur; cur=SLIST_NEXT(cur, next)) {
        if(pList->cb(cur) == priorityValue)
        {
            prev->next = cur->next;
        }
        else {
            prev = cur;
        }
    }

    if(pList->cb(SLIST_FIRST(sList)) == priorityValue)
    {
        SLIST_REMOVE_HEAD(sList, next);
    }
}


///////////////////////////////////////////////////////////////////////////////
//  single tail Linked List structure
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief       Initialize the single tail linked list.
 * @param[in]   pList  - The single tail linked list need to use.
 * @return  None.
 */
void stList_initial(struct single_tail_list* pList)
{
    STAILQ_INIT(pList);
}

/**
 * @brief       single tail linked list insert a node into head.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void stList_headInsertNode(struct single_tail_list* pList, struct single_tail_list_node* pNode)
{
    if(sList_checkNodeExist((struct single_list*)pList, (struct single_list_node*)pNode)) {
        return ;
    }
    
    STAILQ_NEXT(pNode, next) = NULL;
    STAILQ_INSERT_HEAD(pList, pNode, next);
}

/**
 * @brief       single tail linked list insert a node into tail.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @return  None.
 */
void stList_tailInsertNode(struct single_tail_list* pList, struct single_tail_list_node* pNode)
{
    if(sList_checkNodeExist((struct single_list*)pList, (struct single_list_node*)pNode)) {
        return ;
    }
    
    if(STAILQ_EMPTY(pList))
    {
        STAILQ_INSERT_HEAD(pList, pNode, next);
    }
    else
    {
        STAILQ_INSERT_TAIL(pList, pNode, next);
    }
}

/**
 * @brief       single tail linked list insert a node into previous node.
 * @param[in]   pList  - The single tail linked list that a new node to insert.
 * @param[in]   pNode  - The pointer of the new node.
 * @param[in]   pRrev  - The pointer of the previous node.
 * @return  None.
 */
void stList_insertNode(struct single_tail_list* pList, struct single_tail_list_node* pNode, struct single_tail_list_node* pPrev)
{
    STAILQ_NEXT(pNode, next) = NULL;
    STAILQ_INSERT_AFTER(pList, pPrev, pNode, next);
}

/**
 * @brief       single tail linked list delete head node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_tail_list_node* stList_deleteHeadNode(struct single_tail_list* pList)
{
    struct single_tail_list_node* pHead = STAILQ_FIRST(pList);

    if(!pHead) {
        return NULL;
    }

    STAILQ_REMOVE_HEAD(pList, next);
    return pHead;
}

/**
 * @brief       single tail linked list delete tail node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @return  the pointer of delete node.
 */
struct single_tail_list_node* stList_deleteTailNode(struct single_tail_list* pList)
{
    struct single_tail_list_node* pTail = STAILQ_LAST(pList, single_tail_list_node, next);
    
    if(STAILQ_EMPTY(pList)) {
        return NULL;
    }

    STAILQ_REMOVE(pList, pTail, single_tail_list_node, next);
    return pTail;
}

/**
 * @brief       single tail linked list delete specific node.
 * @param[in]   pList  - The single tail linked list that delete a node.
 * @param[in]   pNode  - The pointer of want delete the node.
 * @return  None..
 */
void stList_deleteSpecificNode(struct single_tail_list* pList, struct single_tail_list_node* pNode)
{
    STAILQ_REMOVE(pList, pNode, single_tail_list_node, next);
}


