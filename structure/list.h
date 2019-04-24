#ifndef _DLIST_HEADER_
#define _DLIST_HEADER_

typedef struct _ef_list_entry_t ef_list_entry_t;

struct _ef_list_entry_t {
    ef_list_entry_t *prev;
    ef_list_entry_t *next;
};

#define CAST_PARENT_PTR(ptr, parent_type, field_name) \
((parent_type*)((char*)ptr-(char*)&((parent_type*)0)->field_name))

inline int ef_list_empty(ef_list_entry_t *head) __attribute__((always_inline));
inline void ef_list_init(ef_list_entry_t *head) __attribute__((always_inline));
inline void ef_list_remove(ef_list_entry_t *entry) __attribute__((always_inline));
inline void ef_list_insert_before(ef_list_entry_t *current, ef_list_entry_t *entry) __attribute__((always_inline));
inline void ef_list_insert_after(ef_list_entry_t *current, ef_list_entry_t *entry) __attribute__((always_inline));
inline ef_list_entry_t *ef_list_entry_before(ef_list_entry_t *current) __attribute__((always_inline));
inline ef_list_entry_t *ef_list_entry_after(ef_list_entry_t *current) __attribute__((always_inline));
inline ef_list_entry_t *ef_list_remove_before(ef_list_entry_t *current) __attribute__((always_inline));
inline ef_list_entry_t *ef_list_remove_after(ef_list_entry_t *current) __attribute__((always_inline));

inline int ef_list_empty(ef_list_entry_t *head)
{
    return head->next == head;
}

inline void ef_list_init(ef_list_entry_t *head)
{
    head->prev = head;
    head->next = head;
}

inline void ef_list_remove(ef_list_entry_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

inline void ef_list_insert_before(ef_list_entry_t *current, ef_list_entry_t *entry)
{
    entry->next = current;
    current->prev->next = entry;
    entry->prev = current->prev;
    current->prev = entry;
}

inline void ef_list_insert_after(ef_list_entry_t *current, ef_list_entry_t *entry)
{
    entry->next = current->next;
    current->next = entry;
    entry->prev = current;
    entry->next->prev = entry;
}

inline ef_list_entry_t *ef_list_entry_before(ef_list_entry_t *current)
{
    return current->prev;
}

inline ef_list_entry_t *ef_list_entry_after(ef_list_entry_t *current)
{
    return current->next;
}

inline ef_list_entry_t *ef_list_remove_before(ef_list_entry_t *current)
{
    ef_list_entry_t *entry;

    if (ef_list_empty(current)) {
        return NULL;
    }

    entry = current->prev;
    current->prev = entry->prev;
    current->prev->next = current;
    return entry;
}

inline ef_list_entry_t *ef_list_remove_after(ef_list_entry_t *current)
{
    ef_list_entry_t *entry;

    if (ef_list_empty(current)) {
        return NULL;
    }

    entry = current->next;
    current->next = entry->next;
    current->next->prev = current;
    return entry;
}

#endif
