/**
 * @file   object.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-28 20:53:03
 *
 * @brief
 *
 *
 */

#include "object.h"
#include "zmalloc.h"
#include "kvdb.h"
#include "logger.h"

/* ==================== slice_new() ==================== */
slice_t *slice_new(md5_value_t key_md5, uint32_t slice_idx, const char *data, uint32_t data_size)
{
    slice_t *slice = (slice_t*)zmalloc(sizeof(slice_t));
    memset(slice, 0, sizeof(slice_t));

    slice->slice_key.key_md5 = key_md5;
    slice->slice_key.slice_idx = slice_idx;
    if ( data != NULL ) {
        slice->data = (char*)zmalloc(data_size);
        memcpy(slice->data, data, data_size);
    }
    slice->size = data_size;

    return slice;
}

/* ==================== slice_free() ==================== */
void slice_free(slice_t *slice)
{
    if ( slice != NULL ){
        if ( slice->data != NULL ){
            zfree(slice->data);
            slice->data = NULL;
        }
        zfree(slice);
    }
}

void slice_attach_data(slice_t *slice, char *data, uint32_t data_size)
{
    if ( slice->data != NULL ){
        zfree(slice->data);
        slice->data = NULL;
        slice->size = 0;
    }
    slice->data = data;
    slice->size = data_size;
}

/* ==================== slice_write_to_kvdb() ==================== */
int slice_write_to_kvdb(kvdb_t *kvdb, slice_t *slice)
{
    uint32_t slice_idx = slice->slice_key.slice_idx;
    char *slice_key = (char *)&slice->slice_key;
    uint32_t slice_key_len = sizeof(slice_key_t);

    int rc = kvdb_put(kvdb, slice_key, slice_key_len, slice->data, slice->size);
    if ( rc != 0 ) {
        error_log("Storage save by kvdb_put() failed. slice_key=%s slice_idx=%d buf_size=%d", slice_key, slice_idx, slice->size);
        return -1;
    } else {
        trace_log("Storage save by kvdb_put() OK. slice_key=%s slice_idx=%d buf_size=%d", slice_key, slice_idx, slice->size);
        return 0;
    }
}

/* ==================== slice_read_from_kvdb() ==================== */
slice_t *slice_read_from_kvdb(kvdb_t *kvdb, md5_value_t key_md5, uint32_t slice_idx)
{
    slice_t *slice = NULL;

    slice_key_t slice_key;
    slice_key.key_md5 = key_md5;
    slice_key.slice_idx = slice_idx;

    uint32_t slice_key_len = sizeof(slice_key_t);

    char *buf = NULL;
    uint32_t buf_size = 0;
    int rc = kvdb_get(kvdb, (char*)&slice_key, slice_key_len, (void**)&buf, &buf_size);
    if ( rc == 0 && buf != NULL && buf_size > 0 ){
        slice = slice_new(key_md5, slice_idx, NULL, 0);
        slice_attach_data(slice, buf, buf_size);
    }

    return slice;
}

/* ==================== slice_delete_from_kvdb() ==================== */
int slice_delete_from_kvdb(kvdb_t *kvdb, md5_value_t key_md5, uint32_t slice_idx)
{
    slice_key_t slice_key;
    slice_key.key_md5 = key_md5;
    slice_key.slice_idx = slice_idx;

    uint32_t slice_key_len = sizeof(slice_key_t);

    int ret = 0;
    int rc = kvdb_del(kvdb, (char*)&slice_key, slice_key_len);
    if ( rc == 0 ){
    } else {
        error_log("kvdb_del() failed. slice_key:%s, slice_idx=%d", (char*)&slice_key, slice_idx);
        ret = -1;
    }

    return ret;
}

/*static void _slice_init(const void *cpv_input, void *pv_output)*/
/*{*/
    /*memset(cpv_input, 0, sizeof(slice_t));*/
    /**(bool_t*)pv_output = true;*/
/*}*/

/*static void _slice_destroy(const void *cpv_input, void *pv_output)*/
/*{*/
    /**(bool_t*)pv_output = true;*/
/*}*/

/*static void _slice_copy(const void *cpv_first, const void *cpv_second, void *pv_output)*/
/*{*/
    /*slice_t *slice_first = (slice_t*)cpv_first;*/
    /*slice_t *slice_second = (slice_t*)cpv_second;*/
    /*memcpy(slice_second, slice_first, sizeof(slice_t));*/
    /**(bool_t*)pv_output = true;*/
/*}*/

/*static void _slice_less(const void *cpv_first, const void *cpv_second, void *pv_output)*/
/*{*/
    /*slice_t *slice_first = (slice_t*)cpv_first;*/
    /*slice_t *slice_second = (slice_t*)cpv_second;*/
    /*uint32_t idx_first = slice_first->slice_key.slice_idx;*/
    /*uint32_t idx_second = slice_second->slice_key.slice_idx;*/
    /**(bool_t*)pv_output = idx_first < idx_second ? true : false;*/
/*}*/

/*void slice_constructor( void )*/
	/*__attribute__ ((no_instrument_function, constructor));*/

/*void slice_constructor(void)*/
/*{*/
    /*type_register(slice_t, _user_init, _user_copy, _user_less, _user_destroy);*/
/*}*/

/* ==================== object_new() ==================== */
object_t *object_new(const char *key, uint32_t keylen)
{
    object_t *object = (object_t*)zmalloc(sizeof(object_t));
    memset(object, 0, sizeof(object_t));

    if ( key != NULL ){
        object->key = (char *)zmalloc(keylen);
        object->keylen = keylen;
        memcpy(object->key, key, keylen);

        md5(&object->key_md5, (uint8_t *)object->key, object->keylen);
    }

    object->slices = g_list_new();

    return object;
}


/* ==================== object_free() ==================== */
void object_free(object_t *object)
{
    if ( object->key != NULL ){
        zfree(object->key);
        object->key = NULL;
    }

    if ( object->slices != NULL ) {
        g_list_t *slices = object->slices;
        g_iterator_t *it = g_list_begin(slices);
        g_iterator_t *itend = g_list_end(slices);
        while ( g_iterator_compare(it, itend) != 0 ){
            slice_t *slice = (slice_t*)g_iterator_get(it);
            if ( slice != NULL ){
                slice_free(slice);
            }
            g_iterator_next(it);
        }
        g_iterator_free(it);
        g_iterator_free(itend);

        g_list_free(object->slices);
        object->slices = NULL;
    }
    zfree(object);
}

