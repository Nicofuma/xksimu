#ifndef XKAAPI_SIMU_LIST_H__
#define XKAAPI_SIMU_LIST_H__

// ----- LIST -----

#define xksimu_list_foreach(__list,__cursor, __data) \
        (__data) = (__list).head == NULL ? NULL : (__list).head->data; \
        for ((__cursor) = (__list).head ; \
                (__cursor) != NULL ; \
                (__cursor) = (__cursor)->next, \
                (__data) = (__cursor) == NULL ? NULL : (__cursor)->data)

struct xksimu_list_el_t {
        struct xksimu_list_el_t * next;
        struct xksimu_list_el_t * prev;
        void * free;
        void * data;
};

struct xksimu_list_t {
        struct xksimu_list_el_t * head;
        struct xksimu_list_el_t * queue;
        int size;
};

void xksimu_list_push_front(struct xksimu_list_t * list, void * data);
void xksimu_list_push_back(struct xksimu_list_t * list, void * data);
void * xksimu_list_pop_front(struct xksimu_list_t * list);
void * xksimu_list_pop_back(struct xksimu_list_t * list);
void xksimu_list_free(struct xksimu_list_t * list);
void * xksimu_list_remove(struct xksimu_list_t * list, void * data);
int xksimu_list_contains(struct xksimu_list_t list, void * data);
int xksimu_list_empty(const struct xksimu_list_t const * list);
void * xksimu_list_get(struct xksimu_list_t list, int index);

typedef int xksimu_comp_void_f(void *, void *);
int xksimu_comp_void_equals(void * d1, void * d2);
int xksimu_list_containsf(struct xksimu_list_t list, void * data, xksimu_comp_void_f comp);


#endif
