#ifndef LIST_H
#define LIST_H

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
        if (!list_is_empty((list))) {           \
            (list)->head->prev = (node);        \
            (node)->next = (list)->head;        \
            (node)->prev = 0;                   \
        } else {                                \
            (list)->tail = (node);              \
        }                                       \
        (list)->head = (node);                  \
    } while (0)


#define list_remove(list, node)                 \
    do {                                        \
        if ((node)->prev) {                     \
            (node)->prev->next = (node)->next;  \
        } else {                                \
            ASSERT((node) == (list)->head);     \
            (list)->head = (node)->next;        \
        }                                       \
        if ((node)->next) {                     \
            (node)->next->prev = (node)->prev;  \
        } else {                                \
            ASSERT((node) == (list)->tail);     \
            (list)->tail = (node)->prev;        \
        }                                       \
    } while (0)

#define list_head(list) ((list)->head)
#define list_tail(list) ((list)->tail)
#define list_next(node) ((node)->next)
#define list_prev(node) ((node)->prev)
#define list_is_empty(list) (!((list)->head))

#define LIST_LINKS(type) struct type *next; struct type *prev
#define DEFINE_LIST(type, name) typedef struct name { type *head; type *tail; } name

#endif //LIST_H
