#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "typedefs.h"

#define list_get_item(node, item_type, memb_name) ((item_type *)byte_offset(&(node)->next, \
            -offsetof(item_type, memb_name.next)))

// TODO: make this work even if there are no dummy nodes

typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *previous;
} ListNode;

typedef struct {
    ListNode head;
    ListNode tail;
} List;

void        list_init(List *list);
void        list_push_front(List *list, ListNode *node);
void        list_push_back(List *list, ListNode *node);
void        list_insert_after(ListNode *node, ListNode *head);
void        list_insert_before(ListNode *node, ListNode *head);
void        list_remove(ListNode *node);
void        list_pop_front(List *list);
void        list_pop_back(List *list);
ListNode   *list_begin(List *list);
ListNode   *list_end(List *list);
ListNode   *list_front(List *list);
ListNode   *list_back(List *list);
ListNode   *list_next(ListNode *node);
ListNode   *list_prev(ListNode *node);
bool        list_is_empty(List *list);

#endif //LINKED_LIST_H
