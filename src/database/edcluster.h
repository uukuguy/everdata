/**
 * @file   edcluster.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-03-03 13:16:25
 *
 * @brief
 *
 *
 */

#ifndef __EDCLUSTER_H__
#define __EDCLUSTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct edcluster_t edcluster_t;

edcluster_t *edcluster_new(void);
void edcluster_free(edcluster_t *cluster);

int edcluster_add_type(int type_id, const char *type_name);
const char *edcluster_get_type_name(edcluster_t *cluster, int type_id);
int edcluster_get_type_id(edcluster_t *cluster, const char *type_name);

#ifdef __cplusplus
}
#endif

#endif // __EDCLUSTER_H__


