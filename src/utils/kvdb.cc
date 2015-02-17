/**
 * @file  kvdb.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 20:28:04
 *
 * @brief
 *
 *
 */

#include "kvdb.h"

#include <stdlib.h>
#include <memory.h>
#include <leveldb/c.h>
#include <map>
#include <string>

typedef struct kvdb_t{
    int id;
    const char *dbname;
    leveldb_t *db;
    leveldb_options_t *pOpt;
    leveldb_writeoptions_t *pWriteOpt;
    leveldb_readoptions_t *pReadOpt;
} kvdb_t;

void kvdb_free(kvdb_t *kvdb);

/* ==================== kvdb_new() ==================== */
kvdb_t *kvdb_new(int db_id, const char *dbname, const char *dbpath)
{
    kvdb_t *kvdb = (kvdb_t*)malloc(sizeof(kvdb_t));
    memset(kvdb, 0, sizeof(kvdb_t));

    kvdb->pOpt = leveldb_options_create();
    leveldb_options_set_create_if_missing(kvdb->pOpt, 1);
    kvdb->pWriteOpt = leveldb_writeoptions_create();
    kvdb->pReadOpt = leveldb_readoptions_create();

    char *szErr = NULL;
    kvdb->db = leveldb_open(kvdb->pOpt, dbpath, &szErr);

    if( szErr ){
        kvdb_free(kvdb);
        kvdb = NULL;
    }

    return kvdb;
}

/* ==================== kvdb_free() ==================== */
void kvdb_free(kvdb_t *kvdb)
{
    if ( kvdb != NULL ) {
        leveldb_close(kvdb->db);
        leveldb_writeoptions_destroy(kvdb->pWriteOpt);
        leveldb_readoptions_destroy(kvdb->pReadOpt);
        leveldb_options_destroy(kvdb->pOpt);

        free(kvdb);
    }
}

/* ==================== kvdb_put() ==================== */
int kvdb_put(kvdb_t *kvdb, const char *key, uint32_t klen, const char *value, uint32_t vlen)
{
    char *szErr = NULL;
    leveldb_put(kvdb->db, kvdb->pWriteOpt, key, klen, value, vlen, &szErr);
    if ( szErr ) {
        return -1;
    } else {
        return 0;
    }
}

/* ==================== kvdb_put() ==================== */
int kvdb_get(kvdb_t *kvdb, const char *key, uint32_t klen, char **ppVal, uint32_t *pnVal)
{
  char *szErr = NULL;
  size_t nVal = 0;

  char *pVal = leveldb_get(kvdb->db, kvdb->pReadOpt, (const char *)key, klen, &nVal, &szErr);

  if( pVal == NULL ){
    *pnVal = 0;
    return -1;
  }else{
    *pnVal = nVal;
  }

  if ( szErr ) {
      return -1;
  } else {
      char *result = (char*)malloc(nVal);
      memcpy(result, pVal, nVal);
      *ppVal = result;
      return 0;
  }
}

/* ==================== kvdb_put() ==================== */
void kvdb_list(kvdb_t *kvdb)
{
    printf("token,count\n");

    std::multimap<uint64_t, std::string> mapWC;

    leveldb_iterator_t *it = leveldb_create_iterator(kvdb->db, kvdb->pReadOpt);

    leveldb_iter_seek_to_first(it);

    while ( leveldb_iter_valid(it) ){
        size_t klen = 0;
        const char *key = leveldb_iter_key(it, &klen);
        char *token = (char*)malloc(klen + 1);
        memcpy(token, key, klen);
        token[klen] = 0;

        size_t vlen = 0;
        const char *value = leveldb_iter_value(it, &vlen);

        uint64_t cnt = *(uint64_t*)value;
        mapWC.insert(std::multimap<uint64_t , std::string>::value_type(cnt, std::string(token)));

        free(token);

        leveldb_iter_next(it);
    }

    leveldb_iter_destroy(it);

    for ( std::map<uint64_t, std::string>::reverse_iterator iter = mapWC.rbegin() ; iter != mapWC.rend() ; iter++ ){
        uint64_t cnt = iter->first;
        std::string key = iter->second;
        printf("%s,%zu\n", key.c_str(), (size_t)cnt);
    }
}

