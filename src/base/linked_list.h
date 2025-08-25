#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "typedefs.h"

// TODO: use ByteOffset
#define List_GetItem(node, item_type, memb_name) ((item_type *)((byte *)&(node)->next \
            - offsetof(item_type, memb_name.next)))

// TODO: test
// TODO: make this work even if there are no dummy nodes

typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *previous;
} ListNode;

typedef struct {
    ListNode head;
    ListNode tail;
} List;

void        List_Init(List *list);
void        List_PushFront(List *list, ListNode *node);
void        List_PushBack(List *list, ListNode *node);
void        List_InsertAfter(ListNode *node, ListNode *head);
void        List_InsertBefore(ListNode *node, ListNode *head);
void        List_Remove(ListNode *node);
void        List_PopFront(List *list);
void        List_PopBack(List *list);
ListNode   *List_Begin(List *list);
ListNode   *List_End(List *list);
ListNode   *List_Front(List *list);
ListNode   *List_Back(List *list);
ListNode   *List_Next(ListNode *node);
ListNode   *List_Prev(ListNode *node);
bool        List_IsEmpty(List *list);

#endif //LINKED_LIST_H
