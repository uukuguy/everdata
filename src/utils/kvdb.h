/**
 * @file  kvdb.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 20:27:48
 *
 * @brief
 *
 *
 */

#ifndef __KVDB_H__
#define __KVDB_H__

#include <stdint.h>

typedef struct kvdb_t kvdb_t;

kvdb_t *kvdb_new(int db_id, const char *dbname, const char *dbpath);
void kvdb_free(kvdb_t *kvdb);
int kvdb_put(kvdb_t *kvdb, const char *key, uint32_t klen, const char *value, uint32_t vlen);
int kvdb_get(kvdb_t *kvdb, const char *key, uint32_t klen, char **ppVal, uint32_t *pnVal);
void kvdb_list(kvdb_t *kvdb);

#endif // __KVDB_H__

