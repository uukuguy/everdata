/**
 * @file   bucketdb.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-28 17:18:35
 * 
 * @brief  
 * 
 * 
 */

#ifndef __BUCKETDB_H__
#define __BUCKETDB_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"
#include "md5.h"

typedef struct kvdb_t kvdb_t;
typedef struct slice_t slice_t;

typedef enum eBucketDBType {
    BUCKETDB_NONE = 0,
    BUCKETDB_LOGFILE,

    BUCKETDB_KVDB,
    BUCKETDB_KVDB_LMDB,
    BUCKETDB_KVDB_EBLOB,
    BUCKETDB_KVDB_LEVELDB,
    BUCKETDB_KVDB_ROCKSDB,
    BUCKETDB_KVDB_LSM
} eBucketDBType;

/* ---------- struct slicedb_t ---------- */
typedef struct slicedb_t {
    uint32_t id;
    kvdb_t *kvdb;
    uint64_t max_dbsize;
} slicedb_t;

slicedb_t *slicedb_new(uint32_t id, kvdb_t *kvdb, uint64_t max_dbsize);
void slicedb_init(slicedb_t *slicedb, uint32_t id, kvdb_t *kvdb, uint64_t max_dbsize);
void slicedb_free(slicedb_t *slicedb);

/* ---------- struct bucketdb_t ---------- */
typedef struct bucketdb_t {
    uint32_t id;
    char root_dir[NAME_MAX];
    enum eBucketDBType storage_type;

    kvdb_t *kvdb_metadata;

    slicedb_t *active_slicedb;
    slicedb_t *slicedbs[1024];
    uint64_t max_dbsize;

} bucketdb_t;

bucketdb_t *bucketdb_new(const char *root_dir, uint32_t id, enum eBucketDBType storage_type);
void bucketdb_free(bucketdb_t *bucketdb);

int bucketdb_put_metadata(bucketdb_t *bucketdb, const char *key, const char *data, uint32_t data_size);
int bucketdb_get_metadata(bucketdb_t *bucketdb, const char *key, char **data, uint32_t *data_size);

int bucketdb_write_to_storage(bucketdb_t *bucketdb, slice_t *slice);
slice_t *bucketdb_read_from_storage(bucketdb_t *bucketdb, md5_value_t key_md5, uint32_t slice_idx);
int bucketdb_delete_from_storage(bucketdb_t *bucketdb, md5_value_t key_md5, uint32_t slice_idx);

#ifdef __cplusplus
}
#endif

#endif // __BUCKETDB_H__

