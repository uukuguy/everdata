/**
 * @file  cboost.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-18 14:34:21
 * 
 * @brief  
 * 
 * 
 */

#ifndef __CBOOST_H__
#define __CBOOST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/* ================ iterator ================ */
typedef struct g_iterator_t g_iterator_t;

extern g_iterator_t *g_iterator_next(g_iterator_t *iter);
extern g_iterator_t *g_iterator_prev(g_iterator_t *iter);
extern void *g_iterator_get(g_iterator_t *iter);
extern void g_iterator_set(g_iterator_t *iter, void *element);
extern void g_iterator_erase(g_iterator_t *iter);
extern void g_iterator_free(g_iterator_t *iter);
extern int g_iterator_compare(g_iterator_t *iter_1, g_iterator_t *itesr_2);

/* ================ vector ================ */
typedef struct g_vector_t  g_vector_t;

extern g_vector_t *g_vector_new(void);
extern void g_vector_free(g_vector_t *vec);
extern void g_vector_clear(g_vector_t *vec);
extern size_t g_vector_size(g_vector_t *vec);
extern int g_vector_empty(g_vector_t *vec);
extern void g_vector_push_back(g_vector_t *vec, void *element);
extern void *g_vector_pop_front(g_vector_t *vec);
extern size_t g_vector_erase(g_vector_t *vec, size_t element_idx);
extern void *g_vector_get_element(g_vector_t *vec, size_t element_idx);
extern void *g_vector_get_first(g_vector_t *vec);
extern void *g_vector_get_last(g_vector_t *vec);
extern g_iterator_t *g_vector_begin(g_vector_t *vector);
extern g_iterator_t *g_vector_end(g_vector_t *vector);


/* ================ list ================ */
typedef struct g_list_t g_list_t;

extern g_list_t *g_list_new(void);
extern void g_list_free(g_list_t *list);
extern void g_list_clear(g_list_t *list);
extern size_t g_list_size(g_list_t *list);
extern int g_list_empty(g_list_t *list);
extern void g_list_push_back(g_list_t *list, void *element);
extern void g_list_push_front(g_list_t *list, void *element);
extern void g_list_pop_back(g_list_t *list);
extern void g_list_pop_front(g_list_t *list);
extern g_iterator_t *g_list_begin(g_list_t *list);
extern g_iterator_t *g_list_end(g_list_t *list);
extern g_iterator_t *g_list_erase(g_list_t *list, g_iterator_t *iter);


/* ================ queue ================ */


/* ================ stack ================ */


/* ================ stringmap ================ */
typedef struct g_stringmap_t g_stringmap_t;

extern g_stringmap_t *g_stringmap_new(void);
extern void g_stringmap_free(g_stringmap_t *map);
extern void g_stringmap_clear(g_stringmap_t *map);
extern size_t g_stringmap_size(g_stringmap_t *map);
extern int g_stringmap_empty(g_stringmap_t *map);
extern void g_stringmap_insert(g_stringmap_t *map, const char *key, void *value);
extern g_iterator_t *g_stringmap_begin(g_stringmap_t *map);
extern g_iterator_t *g_stringmap_end(g_stringmap_t *map);
extern void g_map_erase(g_stringmap_t *map, g_iterator_t *iter);
extern g_iterator_t *g_stringmap_find(g_stringmap_t *map, const char *key);

/* ================ intmap ================ */
typedef struct g_intmap_t g_intmap_t;

extern g_intmap_t *g_intmap_new(void);
extern void g_intmap_free(g_intmap_t *map);
extern void g_intmap_clear(g_intmap_t *map);
extern size_t g_intmap_size(g_intmap_t *map);
extern int g_intmap_empty(g_intmap_t *map);
extern void g_intmap_insert(g_intmap_t *map, int key, void *value);
extern g_iterator_t *g_map_begin(g_intmap_t *map);
extern g_iterator_t *g_map_end(g_intmap_t *map);
extern void g_intmap_erase(g_intmap_t *map, g_iterator_t *iter);
extern g_iterator_t *g_intmap_find(g_intmap_t *map, int key);

#ifdef __cplusplus
}
#endif

#endif /* __CBOOST_H__ */

