#ifndef SL_LIST_H
#define SL_LIST_H

#define sl_list_push_front(list, node)          \
    do {                                        \
        (node)->next = (list)->head;            \
        (list)->head = (node);                  \
                                                \
        if (!(list)->tail) {                    \
            (list)->tail = (node);              \
        }                                       \
    } while (0)

#define sl_list_push_back(list, node)           \
    do {                                        \
        if ((list)->tail) {                     \
            (list)->tail->next = (node);        \
        } else {                                \
            (list)->head = (node);              \
        }                                       \
                                                \
        (list)->tail = (node);                  \
    } while (0)

#define sl_list_pop(list)                       \
    do {                                        \
        ASSERT(!sl_list_is_empty(list));        \
        (list)->head = (list)->head->next;      \
        if (!(list)->head) {                    \
            (list)->tail = 0;                   \
        }                                       \
    } while (0);

#define sl_list_head(list) ((list)->head)
#define sl_list_tail(list) ((list)->tail)
#define sl_list_next(node) ((node)->next)
#define sl_list_is_empty(list) (!(list)->head)
#define sl_list_clear(list) ((list)->head = (list)->tail = 0)

#endif //SL_LIST_H
