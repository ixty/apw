// ======================================================================== //
// ixty                                                                2018 //
// ======================================================================== //
// Project     : ESP8266 fun                                                //
// Filename    : clist.h                                                    //
// Description : circular linked list utilities                             //
// ======================================================================== //

#ifndef _CLIST_H
#define _CLIST_H

#define clist_front(head) (head)
#define clist_back(head)  (head ? head->prev : NULL)

#define clist_push_back(head, item)                 \
    do {                                            \
        if(*head == NULL)                           \
        {                                           \
            item->prev = item;                      \
            item->next = item;                      \
            *head = item;                           \
        }                                           \
        else                                        \
        {                                           \
            item->next = (*head);                   \
            item->prev = (*head)->prev;             \
            (*head)->prev->next = item;             \
            (*head)->prev = item;                   \
        }                                           \
    } while (0)


#define clist_push_front(head, item)                \
    do {                                            \
        clist_push_back(head, item);                \
        *head = item;                               \
    } while (0)


#define clist_pop_front(head, item)                 \
    do {                                            \
        item = *head;                               \
        /* 1 item */                                \
        if(*head == (*head)->next)                  \
            *head = NULL;                           \
        else                                        \
        {                                           \
            *head = item->next;                     \
            /* 2 items */                           \
            if (item->next->next == item)           \
            {                                       \
                (*head)->prev = (*head);            \
                (*head)->next = (*head);            \
            }                                       \
            else /* 3+ items */                     \
            {                                       \
                item->prev->next = item->next;      \
                item->next->prev = item->prev;      \
            }                                       \
        }                                           \
        item->prev = NULL;                          \
        item->next = NULL;                          \
    } while (0)


#define clist_pop_back(head, item)                  \
    do {                                            \
        *head = (*head)->prev;                      \
        clist_pop_front(head, item);                \
    } while (0)


#define clist_unlink(head, item)                    \
    do {                                            \
        if(*head == item)                           \
            clist_pop_front(head, item);            \
        else                                        \
        {                                           \
            item->prev->next = item->next;          \
            item->next->prev = item->prev;          \
            item->prev = NULL;                      \
            item->next = NULL;                      \
        }                                           \
    } while (0)


#define clist_insert_after(head, it, item)          \
    do {                                            \
        item->next = it->next;                      \
        item->prev = it;                            \
        item->prev->next = item;                    \
        item->next->prev = item;                    \
    } while (0)


#define clist_insert_before(head, it, item)         \
    do {                                            \
        if(it == *head)                             \
            clist_push_front(head, item);           \
        else                                        \
        {                                           \
            item->next = it;                        \
            item->prev = it->prev;                  \
            item->prev->next = item;                \
            item->next->prev = item;                \
        }                                           \
    } while(0)


#endif
