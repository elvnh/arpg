#ifndef SL_LIST_H
#define SL_LIST_H

#include "base/list.h"

#define sl_list_push_front_x(list, node, name)  \
    do {                                        \
        (node)->name = (list)->head;            \
        (list)->head = (node);                  \
                                                \
        if (!(list)->tail) {                    \
            (list)->tail = (node);              \
        }                                       \
    } while (0)

#define sl_list_push_back_x(list, node, name)   \
    do {                                        \
        if ((list)->tail) {                     \
            (list)->tail->name = (node);        \
        } else {                                \
            (list)->head = (node);              \
        }                                       \
                                                \
        (list)->tail = (node);                  \
    } while (0)

#define sl_list_pop(list)                       \
    do {                                        \
        ASSERT(!list_is_empty(list));        \
        (list)->head = (list)->head->next;      \
        if (!(list)->head) {                    \
            (list)->tail = 0;                   \
        }                                       \
    } while (0);

#define sl_list_push_front(list, node) sl_list_push_front_x(list, node, next)
#define sl_list_push_back(list, node) sl_list_push_back_x(list, node, next)

#endif //SL_LIST_H
