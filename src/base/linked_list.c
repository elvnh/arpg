#include "linked_list.h"
#include "utils.h"

void List_Init(List *list)
{
    list->head.previous = 0;
    list->head.next = &list->tail;

    list->tail.previous = &list->head;
    list->tail.next = 0;
}

void List_PushFront(List *list, ListNode *node)
{
    List_InsertBefore(node, List_Begin(list));
}

void List_PushBack(List *list, ListNode *node)
{
    List_InsertBefore(node, List_End(list));
}

void List_InsertAfter(ListNode *node, ListNode *head)
{
    Assert(node);
    Assert(node != head);
    Assert((head->previous && head->next) || ((head->next == 0) || head->previous->previous == 0));

    node->previous = head;
    node->next = head->next;
    head->next->previous = node;
    head->next = node;
}

void List_InsertBefore(ListNode *node, ListNode *head)
{
    Assert(node);
    Assert(node != head);
    Assert((head->previous && head->next) || ((head->next == 0) || head->previous->previous == 0));

    node->next = head;
    node->previous = head->previous;
    head->previous->next = node;
    head->previous = node;
}

void List_Remove(ListNode *node)
{
    node->previous->next = node->next;
    node->next->previous = node->previous;

    node->next = 0;
    node->previous = 0;
}

void List_PopFront(List *list)
{
    List_Remove(List_Front(list));
}

void List_PopBack(List *list)
{
    List_Remove(List_Back(list));
}

ListNode *List_Begin(List *list)
{
    return list->head.next;
}

ListNode *List_End(List *list)
{
    return &list->tail;
}

ListNode *List_Front(List *list)
{
    if (List_IsEmpty(list)) {
        return 0;
    }

    return List_Begin(list);
}

ListNode *List_Back(List *list)
{
    if (List_IsEmpty(list)) {
        return 0;
    }

    return list->tail.previous;
}

ListNode *List_Next(ListNode *node)
{
    return node->next;
}

ListNode *List_Prev(ListNode *node)
{
    return node->previous;
}

bool List_IsEmpty(List *list)
{
    return List_Begin(list) == List_End(list);
}
