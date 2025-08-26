#include "linked_list.h"
#include "utils.h"

void list_init(List *list)
{
    list->head.previous = 0;
    list->head.next = &list->tail;

    list->tail.previous = &list->head;
    list->tail.next = 0;
}

void list_push_front(List *list, ListNode *node)
{
    list_insert_before(node, list_begin(list));
}

void list_push_back(List *list, ListNode *node)
{
    list_insert_before(node, list_end(list));
}

void list_insert_after(ListNode *node, ListNode *head)
{
    ASSERT(node);
    ASSERT(node != head);
    ASSERT((head->previous && head->next) || ((head->next == 0) || head->previous->previous == 0));

    node->previous = head;
    node->next = head->next;
    head->next->previous = node;
    head->next = node;
}

void list_insert_before(ListNode *node, ListNode *head)
{
    ASSERT(node);
    ASSERT(node != head);
    ASSERT((head->previous && head->next) || ((head->next == 0) || head->previous->previous == 0));

    node->next = head;
    node->previous = head->previous;
    head->previous->next = node;
    head->previous = node;
}

void list_remove(ListNode *node)
{
    node->previous->next = node->next;
    node->next->previous = node->previous;

    node->next = 0;
    node->previous = 0;
}

void list_pop_front(List *list)
{
    list_remove(list_front(list));
}

void list_pop_back(List *list)
{
    list_remove(list_back(list));
}

ListNode *list_begin(List *list)
{
    return list->head.next;
}

ListNode *list_end(List *list)
{
    return &list->tail;
}

ListNode *list_front(List *list)
{
    if (list_is_empty(list)) {
        return 0;
    }

    return list_begin(list);
}

ListNode *list_back(List *list)
{
    if (list_is_empty(list)) {
        return 0;
    }

    return list->tail.previous;
}

ListNode *list_next(ListNode *node)
{
    return node->next;
}

ListNode *list_prev(ListNode *node)
{
    return node->previous;
}

bool list_is_empty(List *list)
{
    return list_begin(list) == list_end(list);
}
