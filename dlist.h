#ifndef _DLIST_HEADER_
#define _DLIST_HEADER_

typedef struct _dlist_entry_t {
    struct _dlist_entry_t *prev;
    struct _dlist_entry_t *next;
} dlist_entry_t;

#define CAST_PARENT_PTR(ptr, parent_type, field_name) \
((parent_type*)((char*)ptr-(char*)&((parent_type*)0)->field_name))

inline void list_init(dlist_entry_t *head) __attribute__((always_inline));
inline int list_empty(dlist_entry_t *head) __attribute__((always_inline));
inline void list_insert_after(dlist_entry_t *current, dlist_entry_t *entry) __attribute__((always_inline));
inline dlist_entry_t *list_entry_after(dlist_entry_t *current) __attribute__((always_inline));
inline dlist_entry_t *list_remove_after(dlist_entry_t *current) __attribute__((always_inline));
inline void list_insert_before(dlist_entry_t *current, dlist_entry_t *entry) __attribute__((always_inline));
inline dlist_entry_t *list_entry_before(dlist_entry_t *current) __attribute__((always_inline));
inline dlist_entry_t *list_remove_before(dlist_entry_t *current) __attribute__((always_inline));
inline void list_remove(dlist_entry_t *entry) __attribute__((always_inline));

inline void list_init(dlist_entry_t *head)
{
    head->prev = head;
    head->next = head;
}

inline int list_empty(dlist_entry_t *head)
{
    return head->next == head;
}

inline void list_insert_after(dlist_entry_t *current, dlist_entry_t *entry)
{
    entry->next = current->next;
    current->next = entry;
    entry->prev = current;
    entry->next->prev = entry;
}

inline dlist_entry_t *list_entry_after(dlist_entry_t *current)
{
    return current->next;
}

inline dlist_entry_t *list_remove_after(dlist_entry_t *current)
{
    if(list_empty(current))
    {
        return NULL;
    }
    dlist_entry_t *entry = current->next;
    current->next = entry->next;
    current->next->prev = current;
    return entry;
}

inline void list_insert_before(dlist_entry_t *current, dlist_entry_t *entry)
{
    entry->next = current;
    current->prev->next = entry;
    entry->prev = current->prev;
    current->prev = entry;
}

inline dlist_entry_t *list_entry_before(dlist_entry_t *current)
{
    return current->prev;
}

inline dlist_entry_t *list_remove_before(dlist_entry_t *current)
{
    if(list_empty(current))
    {
        return NULL;
    }
    dlist_entry_t *entry = current->prev;
    current->prev = entry->prev;
    current->prev->next = current;
    return entry;
}

inline void list_remove(dlist_entry_t *entry)
{
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

#endif
