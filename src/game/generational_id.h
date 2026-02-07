#ifndef GENERATIONAL_ID_H
#define GENERATIONAL_ID_H

// NOTE: only signed types are allowed

#define DEFINE_GENERATIONAL_ID_NODE_FIELDS(gen_type, index_type)        \
    gen_type generation; index_type prev_free_id_index; index_type next_free_id_index

#define DEFINE_GENERATIONAL_ID_LIST_HEAD(index_type) index_type first_free_id_index

#define initialize_generational_id(id_node, index, arr_size)    \
    do {                                                        \
        (id_node)->generation = 1;                              \
        (id_node)->prev_free_id_index = index - 1;              \
        if ((index) == ((arr_size) - 1)) {                      \
            (id_node)->next_free_id_index = -1;                 \
        } else {                                                \
            (id_node)->next_free_id_index = (index) + 1;        \
        }                                                       \
    } while (0)

#define bump_generation_counter(id, max)                                             \
    do {                                                                             \
        ASSERT((id)->generation >= 1);                                               \
        ((id)->generation == (max) ? ((id)->generation = 1) : ++((id)->generation)); \
    } while (0)

// NOTE: these functions don't do any bounds checking, the user has to do that themselves

#define remove_generational_id_from_list(list, arr_name, id)                \
    do {                                                                    \
        (list)->first_free_id_index = (id)->next_free_id_index;             \
        if (((id)->prev_free_id_index != -1)) {                             \
            (list)->arr_name[(id)->prev_free_id_index].next_free_id_index   \
                = (id)->next_free_id_index;                                 \
        }                                                                   \
        if (((id)->next_free_id_index != -1)) {                             \
            (list)->arr_name[(id)->next_free_id_index].prev_free_id_index = \
                (id)->prev_free_id_index;                                   \
        }                                                                   \
        (id)->next_free_id_index = (id)->prev_free_id_index = -1;           \
    } while (0)

#define push_generational_id_to_free_list(list, arr_name, index)                    \
    do {                                                                            \
        (list)->arr_name[(index)].next_free_id_index = (list)->first_free_id_index; \
        (list)->first_free_id_index = (index);                                      \
    } while (0)

#endif //GENERATIONAL_ID_H
