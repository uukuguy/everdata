/**
 * @file   bucketdb.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-28 17:27:07
 * 
 * @brief  
 * 
 * 
 */

#include "bucketdb.h"
#include "zmalloc.h"
#include "logger.h"
#include "filesystem.h"
#include "kvdb.h"
#include "object.h"

/* ================ slicedb_new() ================= */
slicedb_t *slicedb_new(uint32_t id, kvdb_t *kvdb, uint64_t max_dbsize)
{
    slicedb_t *slicedb = (slicedb_t*)zmalloc(sizeof(slicedb_t));

    slicedb_init(slicedb, id, kvdb, max_dbsize);

    return slicedb;
}

/* ================ slicedb_init() ================= */
void slicedb_init(slicedb_t *slicedb, uint32_t id, kvdb_t *kvdb, uint64_t max_dbsize)
{
    memset(slicedb, 0, sizeof(slicedb_t));
    slicedb->id = id;
    slicedb->kvdb = kvdb,
    slicedb->max_dbsize = max_dbsize;
}

/* ================ slicedb_free() ================= */
void slicedb_free(slicedb_t *slicedb)
{
    if ( slicedb->kvdb != NULL ){
        kvdb_close(slicedb->kvdb);
        slicedb->kvdb = NULL;
    }
    zfree(slicedb);
}

/* ================ create_kvenv() ================= */
kvenv_t *create_kvenv(const char *dbpath, eBucketDBType storage_type, uint64_t max_dbsize, uint32_t max_dbs)
{
    kvenv_t *kvenv = NULL;

    if ( storage_type == BUCKETDB_KVDB ){
        kvenv = kvenv_new("lmdb", dbpath, max_dbsize, max_dbs);
    } else if ( storage_type == BUCKETDB_KVDB_LMDB ){
        kvenv = kvenv_new("lmdb", dbpath, max_dbsize, max_dbs);
    } else if ( storage_type == BUCKETDB_KVDB_LSM ){
        kvenv = kvenv_new("lsm", dbpath, max_dbsize, max_dbs);
    } else if ( storage_type == BUCKETDB_KVDB_ROCKSDB ){
        kvenv = kvenv_new("rocksdb", dbpath, max_dbsize, max_dbs);
    } else if ( storage_type == BUCKETDB_KVDB_LEVELDB ){
        kvenv = kvenv_new("leveldb", dbpath, max_dbsize, max_dbs);
    } else if ( storage_type == BUCKETDB_KVDB_EBLOB ){
        kvenv = kvenv_new("eblob", dbpath, max_dbsize, max_dbs);
    }

    if ( kvenv == NULL ){
        error_log("kvenv_new() failed. dbpath:%s", dbpath);
    }

    return kvenv;
}

/* ================ open_kvdb() ================= */
kvdb_t *open_kvdb(const char *dbname, eBucketDBType storage_type, const char *root_dir, uint64_t max_dbsize, uint32_t max_dbs)
{
    char dbpath[NAME_MAX];
    sprintf(dbpath, "%s/%s", root_dir, dbname);

    kvdb_t *kvdb = NULL;
    kvenv_t *kvenv = create_kvenv(dbpath, storage_type, max_dbsize, max_dbs);
    if ( kvenv != NULL ){
        kvdb = kvdb_open(kvenv, dbname);
        if ( kvdb == NULL ){
            error_log("kvdb_init() failed. dbname:%s dbpath:%s", dbname, dbpath);
        }
    }

    return kvdb;
}

/* ================ bucketdb_open_slicedb() ================= */
slicedb_t *bucketdb_open_slicedb(bucketdb_t *bucketdb, uint32_t db_id)
{
    slicedb_t *slicedb = NULL;

    char dbname[NAME_MAX];
    sprintf(dbname, "slice-%03d", db_id);

    uint64_t max_dbsize = bucketdb->max_dbsize;
    uint32_t max_dbs = 4;
    kvdb_t *kvdb = open_kvdb(dbname, bucketdb->storage_type, bucketdb->root_dir, max_dbsize, max_dbs);

    if ( kvdb != NULL ){
        slicedb = slicedb_new(db_id, kvdb, bucketdb->max_dbsize);
        bucketdb->slicedbs[db_id] = slicedb;
    } else {
        error_log("SliceDB create failed. dbname:%s", dbname);
    }

    return slicedb;
}

/* ================ bucketdb_new() ================= */
bucketdb_t *bucketdb_new(const char *root_dir, uint32_t id, enum eBucketDBType storage_type)
{
    bucketdb_t *bucketdb = (bucketdb_t*)zmalloc(sizeof(bucketdb_t));
    memset(bucketdb, 0, sizeof(bucketdb_t));

    bucketdb->id = id;
    bucketdb->storage_type = storage_type;
    bucketdb->max_dbsize = 1024L * 1024L * 800L;

    /* Create bucketdbn root dir */
    sprintf(bucketdb->root_dir, "%s/%04d", root_dir, id);
    if ( mkdir_if_not_exist(bucketdb->root_dir) != 0 ){
        error_log("Cann't create bucketdb(%d) dir:%s", id, bucketdb->root_dir);
        zfree(bucketdb);
        return NULL;
    }

    // Create Metadata DB.
    const char *metadata_dbname = "metadata";
    uint64_t max_dbsize = bucketdb->max_dbsize;
    uint32_t max_dbs = 4;
    kvdb_t *kvdb_metadata = open_kvdb(metadata_dbname, BUCKETDB_KVDB_LMDB, bucketdb->root_dir, max_dbsize, max_dbs);
    if ( kvdb_metadata == NULL ){
        error_log("MetadataDB create failed. dbname:%s", metadata_dbname);
        zfree(bucketdb);
        return NULL;
    }
    bucketdb->kvdb_metadata = kvdb_metadata;

    /* Slices DB */
    if ( bucketdb->storage_type >= BUCKETDB_KVDB ){
        uint32_t active_slicedb_id = 0;
        if ( kvdb_get_uint32(kvdb_metadata, "active_slicedb_id", &active_slicedb_id) != 0 ){
            active_slicedb_id = 0;
        }
        trace_log("bucketdb active_slicedb_id:%d", active_slicedb_id);

        // Create Slice DB.
        for ( int db_id = 0 ; db_id <= active_slicedb_id ; db_id++ ){
            slicedb_t *slicedb = bucketdb_open_slicedb(bucketdb, db_id);
            if ( slicedb != NULL ){
            } else {
                for ( int n = 0 ; n < db_id ; n++ ){
                    slicedb_free(bucketdb->slicedbs[n]);
                    bucketdb->slicedbs[n] = NULL;
                }
                zfree(bucketdb);
                return NULL;
            }
        }
        bucketdb->active_slicedb = bucketdb->slicedbs[active_slicedb_id];
    }

    /*bucketdb->caching_objects = object_queue_new(object_compare_md5_func);*/

    return bucketdb;
}

/* ================ bucketdb_free() ================= */
void bucketdb_free(bucketdb_t *bucketdb)
{

    if ( bucketdb->active_slicedb != NULL ){
        uint32_t active_slicedb_id = bucketdb->active_slicedb->id;
        for ( int db_id = 0 ; db_id < active_slicedb_id ; db_id++ ){
            if ( bucketdb->slicedbs[db_id] != NULL ){
                slicedb_free(bucketdb->slicedbs[db_id]);
                bucketdb->slicedbs[db_id] = NULL;
            }
        }
    }

    if ( bucketdb->kvdb_metadata != NULL ){
        kvdb_close(bucketdb->kvdb_metadata);
        bucketdb->kvdb_metadata = NULL;
    }

    /*if ( bucketdb->caching_objects != NULL ){*/
        /*object_queue_free(bucketdb->caching_objects);*/
        /*bucketdb->caching_objects = NULL;*/
    /*}*/

    zfree(bucketdb);
}

/* ================ bucketdb_put_metadata() ================= */
int bucketdb_put_metadata(bucketdb_t *bucketdb, const char *key, const char *data, uint32_t data_size)
{
    assert(bucketdb->kvdb_metadata != NULL);

    return kvdb_put(bucketdb->kvdb_metadata, key, strlen(key), (void*)data, data_size);
}

/* ================ bucketdb_get_metadata() ================= */
int bucketdb_get_metadata(bucketdb_t *bucketdb, const char *key, char **data, uint32_t *data_size)
{
    assert(bucketdb->kvdb_metadata != NULL);

    return kvdb_get(bucketdb->kvdb_metadata, key, strlen(key), (void**)data, data_size);
}


/* ==================== bucketdb_write_to_file() ==================== */ 
int bucketdb_write_to_file(bucketdb_t *bucketdb, object_t *object)
{
    /*int logFile = bucketdb->logFile;*/
    /*if ( logFile == 0 ) {*/
        /*char log_filename[NAME_MAX];*/
        /*sprintf(log_filename, "%s/bucketdb.log", bucketdb->root_dir);*/
        /*logFile = open(log_filename, O_APPEND | O_CREAT | O_WRONLY, 0640);*/
        /*bucketdb->logFile = logFile;*/
    /*}*/

    /*object_put_into_file(logFile, object);*/

    return 0;
}

/* ==================== bucketdb_write_to_kvdb() ==================== */ 
int bucketdb_write_to_kvdb(bucketdb_t *bucketdb, object_t *object)
{
    /*object_put_into_kvdb(bucketdb->active_slicedb->kvdb, object);*/

    return 0;
}

typedef struct slice_metadata_t{
    uint32_t version;
    uint32_t slicedb_id;
} slice_metadata_t;

/* ==================== bucketdb_get_slicedb_id() ==================== */ 
int bucketdb_get_slicedb_id(bucketdb_t *bucketdb, slice_key_t *slice_key, uint32_t *p_slicedb_id)
{
    int ret = 0;

    slice_metadata_t *slice_metadata = NULL;
    uint32_t value_len = 0;
    if ( kvdb_get(bucketdb->kvdb_metadata, (const char*)slice_key, sizeof(slice_key_t), (void**)&slice_metadata, &value_len) == 0 ){
        if ( slice_metadata != NULL && value_len == sizeof(slice_metadata_t) ){
            *p_slicedb_id = slice_metadata->slicedb_id;
            ret = 1;
        } 
    }

    return ret;
}

/* ==================== bucketdb_write_to_storage() ==================== */ 
int bucketdb_write_to_storage(bucketdb_t *bucketdb, slice_t *slice)
{
    int ret = 0;

    if ( bucketdb->storage_type >= BUCKETDB_KVDB ){
        uint32_t active_slicedb_id = bucketdb->active_slicedb->id;
        uint32_t slicedb_id = active_slicedb_id;
        int old_slice = bucketdb_get_slicedb_id(bucketdb, &slice->slice_key, &slicedb_id);

        int try_to_write_full_db = 0;
        if ( kvenv_get_dbsize(bucketdb->active_slicedb->kvdb->kvenv) > 0.9 * bucketdb->active_slicedb->max_dbsize ){
            if ( old_slice  && active_slicedb_id == slicedb_id ){
                try_to_write_full_db = 1;
            }
            slicedb_t *slicedb = bucketdb_open_slicedb(bucketdb, active_slicedb_id + 1);
            bucketdb->active_slicedb = slicedb;

            if ( kvdb_put_uint32(bucketdb->kvdb_metadata, "active_slicedb_id", active_slicedb_id) != 0 ){
                error_log("Save active_slicedb_id failed. bucketdb->id:%d active_slicedb_id:%d", bucketdb->id, active_slicedb_id + 1);
            }
        }

        slicedb_t *active_slicedb = bucketdb->active_slicedb;

        if ( old_slice ) {
            if ( try_to_write_full_db ){
                ret = slice_delete_from_kvdb(bucketdb->slicedbs[slicedb_id]->kvdb, slice->slice_key.key_md5, slice->slice_key.slice_idx);
            } else {
                active_slicedb = bucketdb->slicedbs[slicedb_id];
            }
        }

        slice_metadata_t slice_metadata;
        memset(&slice_metadata, 0, sizeof(slice_metadata_t));
        slice_metadata.version = 0;
        slice_metadata.slicedb_id = active_slicedb->id;
        ret = kvdb_put(bucketdb->kvdb_metadata, (const char *)&slice->slice_key, sizeof(slice_key_t), (void*)&slice_metadata, sizeof(slice_metadata_t)); 
        if ( ret == 0 ){
            ret = slice_write_to_kvdb(active_slicedb->kvdb, slice);
        } else {
            error_log("Write metadata failed. bucketdb->id:%d slice_idx:%d", bucketdb->id, slice->slice_key.slice_idx);
        }
    } else if ( bucketdb->storage_type == BUCKETDB_LOGFILE ){
        /*ret = bucketdb_write_to_file(bucketdb, object);*/
    }

    if ( ret == 0 ) {
        /*object_queue_remove(bucketdb->caching_objects, object);*/
    }

    return ret;
}

/* ==================== bucketdb_read_from_storage() ==================== */ 
slice_t *bucketdb_read_from_storage(bucketdb_t *bucketdb, md5_value_t key_md5, uint32_t slice_idx)
{
    slice_key_t slice_key;
    slice_key.key_md5 = key_md5;
    slice_key.slice_idx = slice_idx;

    slice_t *slice = NULL;
    
    if ( bucketdb->storage_type >= BUCKETDB_KVDB ){

        uint32_t active_slicedb_id = bucketdb->active_slicedb->id;
        uint32_t slicedb_id = active_slicedb_id;
        int old_slice = bucketdb_get_slicedb_id(bucketdb, &slice_key, &slicedb_id);

        if ( old_slice ){
            slice = slice_read_from_kvdb(bucketdb->slicedbs[slicedb_id]->kvdb, key_md5, slice_idx);
        }
    } else if ( bucketdb->storage_type == BUCKETDB_LOGFILE ){
    } 

    return slice;
}

/* ==================== bucketdb_delete_from_storage() ==================== */ 
int bucketdb_delete_from_storage(bucketdb_t *bucketdb, md5_value_t key_md5, uint32_t slice_idx)
{
    int rc = -1;

    slice_key_t slice_key;
    slice_key.key_md5 = key_md5;
    slice_key.slice_idx = slice_idx;

    if ( bucketdb->storage_type >= BUCKETDB_KVDB ){
        uint32_t active_slicedb_id = bucketdb->active_slicedb->id;
        uint32_t slicedb_id = active_slicedb_id;
        int old_slice = bucketdb_get_slicedb_id(bucketdb, &slice_key, &slicedb_id);

        if ( old_slice ){
            rc = slice_delete_from_kvdb(bucketdb->slicedbs[slicedb_id]->kvdb, key_md5, slice_idx);
        }
    } else if ( bucketdb->storage_type == BUCKETDB_LOGFILE ){
    } 

    return rc;
}

