#ifndef LIST_H
#define LIST_H

#define list_concat(list_a, list_b)                     \
    do {                                                \
        ASSERT(!list_is_empty((list_a)));               \
        if (!list_is_empty((list_b))) {                 \
            (list_a)->tail->next = (list_b)->head;      \
            (list_b)->head->prev = (list_a)->tail;      \
            (list_a)->tail = (list_b)->tail;            \
        }                                               \
    } while (0)

#define list_insert_after(list, node, after)                    \
    do {                                                        \
        if (list_is_empty((list)) || (after) == (list)->tail) { \
            list_push_back((list), (node));                     \
        } else {                                                \
            if ((node)->prev) (node)->prev = (after);           \
            if ((node)->next) (node)->next = (after)->next;     \
            (after)->next = (node);                             \
        }                                                       \
    } while (0)

#define list_push_back(list, node)              \
    do {                                        \
        if (!list_is_empty(list)) {             \
            (list)->tail->next = (node);        \
            (node)->prev = (list)->tail;        \
            (node)->next = 0;                   \
        } else {                                \
            (list)->head = node;                \
        }                                       \
        (list)->tail = node;                    \
    } while (0)

#define list_push_front(list, node)             \
    do {                                        \
        (node)->next = (list)->head;            \
        (node)->prev = 0;                       \
        if (!list_is_empty((list))) {           \
            (list)->head->prev = (node);        \
        } else {                                \
            (list)->tail = (node);              \
        }                                       \
        (list)->head = (node);                  \
    } while (0)

#define list_remove(list, node)                                         \
    do {                                                                \
        if ((node)->prev) (node)->prev->next = (node)->next;            \
        if ((node)->next) (node)->next->prev = (node)->prev;            \
        if (((node) == (list)->head) && ((node) == (list)->tail)) {     \
            (list)->head = (list)->tail = 0;                            \
        } else {                                                        \
            if ((node) == (list)->head) {                               \
                (list)->head = (node)->next;                            \
            } else if ((node) == (list)->tail) {                        \
                (list)->tail = (node)->prev;                            \
            }                                                           \
        }                                                               \
    } while (0)

#define list_pop_head(list) list_remove((list), (list)->head)
#define list_pop_tail(list) list_remove((list), (list)->tail)

#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)
#define list_next(node) ((node)->next)
#define list_prev(node) ((node)->prev)
#define list_is_empty(list) (!((list)->head))

#define LIST_LINKS(type) struct type *next; struct type *prev
#define LIST_HEAD_TAIL(type) struct type *head; struct type *tail
#define DEFINE_LIST(type, name) typedef struct name { type *head; type *tail; } name

#endif //LIST_H
