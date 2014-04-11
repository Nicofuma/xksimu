#include <malloc.h>
#include "list.h"

// ----- list->-----

void xksimu_list_push_front(struct xksimu_list_t * list, void * data) {
        struct xksimu_list_el_t * el = calloc(1, sizeof(struct xksimu_list_el_t));
        el->data = data;
        el->prev = NULL;
        if (list->head != NULL) {
                list->head->prev = el;
        }
        if (list->queue == NULL) {
                list->queue = el;
        }
        el->next = list->head;
        list->head = el;
        list->size++;
}

void xksimu_list_push_back(struct xksimu_list_t * list, void * data) {
        struct xksimu_list_el_t * el = calloc(1, sizeof(struct xksimu_list_el_t));
        el->data = data;
        el->next = NULL;
        if (list->queue != NULL) {
                list->queue->next = el;
        }
        if (list->head == NULL) {
                list->head = el;
        }
        el->prev = list->queue;
        list->queue = el;
        list->size++;
}

void * xksimu_list_pop_front(struct xksimu_list_t * list) {
        struct xksimu_list_el_t * el = list->head;
        
        if (el == NULL) {
                return NULL;
        }

        void * data = el->data;

        if (list->queue != list->head) {
                list->head = list->head->next;
                list->head->prev = NULL;
        } else {
                list->queue = NULL;
                list->head = NULL;
        }

        el->data = NULL;
        el->prev = NULL;
        el->next = NULL;
        el->free = NULL;
        free(el);
        list->size--;

        return data;
}

void * xksimu_list_pop_back(struct xksimu_list_t * list) {
        struct xksimu_list_el_t * el = list->queue;

        if (el == NULL) {
                return NULL;
        }
        
        void * data = el->data;

        if (list->queue != list->head) {
                list->queue = list->queue->prev;
                list->queue->next = NULL;
        } else {
                list->queue = NULL;
                list->head = NULL;
        }

        el->data = NULL;
        el->prev = NULL;
        el->next = NULL;
        el->free = NULL;
        free(el);
        list->size--;

        return data;
}

void xksimu_list_free(struct xksimu_list_t * list) {
        struct xksimu_list_el_t * temp = list->head;
        
        while (temp != NULL) {
                struct xksimu_list_el_t * to_destr = temp;
                temp = temp->next;
                
                to_destr->next = NULL;
                to_destr->prev = NULL;
                        
                //free(to_destr->data); // TODO : Utiliser un foncteur libérer à la place.
                free(to_destr);
        }

        list->head = NULL;
        list->queue = NULL;
}

void * xksimu_list_remove(struct xksimu_list_t * list, void * data) {
        struct xksimu_list_el_t * curr_el = list->head;

        if (curr_el != NULL) {
                if (curr_el->data != data) {
                        while (curr_el->next != NULL && curr_el->next->data != data) {
                                curr_el = curr_el->next;
                        }

                        if (curr_el->next != NULL) {
                                struct xksimu_list_el_t * to_destr = curr_el->next;
                                curr_el->next = to_destr->next;

                                if (to_destr->next != NULL) {
                                        to_destr->next->prev = curr_el;
                                } else {
                                        list->queue = curr_el;
                                }
                                //curr_el->next->prev = curr_el;
                                //curr_el->next = curr_el->next->next;
                                
                                free(to_destr);
                                list->size--;

                                return data;
                        }
                } else {
                        list->head = list->head->next;
                        
                        if (list->head != NULL) {
                                list->head->prev = NULL;
                        } else {
                                list->queue = NULL;
                        }

                        curr_el->next = NULL;
                        free(curr_el);
                        list->size--;
                        
                        return data;
                }
        }

        return NULL;
}

int xksimu_comp_void_equals(void * d1, void * d2) {
        return d1 == d2;
}

int xksimu_list_containsf(struct xksimu_list_t list, void * data, xksimu_comp_void_f comp) {
        struct xksimu_list_el_t * el;
        void * d;

        xksimu_list_foreach(list, el, d) {
                if(comp(d,data)) {
                        return 1;
                }
        }

        return 0;
}

int xksimu_list_contains(struct xksimu_list_t list, void * data) {
        return xksimu_list_containsf(list, data, xksimu_comp_void_equals);
}

void * xksimu_list_get(struct xksimu_list_t list, int index) {
        struct xksimu_list_el_t * el;
        void * d;

        int n = 1;
        xksimu_list_foreach(list, el, d) {
                if (n == index) {
                        return d;
                }
                n++;
        }

        return NULL;
}

int xksimu_list_empty(const struct xksimu_list_t const * list) {
        return list->head == NULL;
}
