/**
 * @file   object.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-28 20:51:28
 *
 * @brief
 *
 *
 */

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdint.h>
#include "cboost.h"
#include "md5.h"

typedef struct kvdb_t kvdb_t;

typedef struct slice_key_t {
    md5_value_t key_md5;
    uint32_t slice_idx;
} slice_key_t;
/* -------------------- slice_t -------------------- */
typedef struct slice_t {
    slice_key_t slice_key;
    uint32_t size;
    char *data;
} slice_t;

slice_t *slice_new(md5_value_t key_md5, uint32_t slice_idx, const char *data, uint32_t data_size);
void slice_free(slice_t *slice);
void slice_attach_data(slice_t *slice, char *data, uint32_t data_size);
int slice_write_to_kvdb(kvdb_t *kvdb, slice_t *slice);
slice_t *slice_read_from_kvdb(kvdb_t *kvdb, md5_value_t key_md5, uint32_t slice_idx);
int slice_delete_from_kvdb(kvdb_t *kvdb, md5_value_t key_md5, uint32_t slice_idx);


/* -------------------- object_t -------------------- */
typedef struct object_t
{
    char *key;
    uint32_t keylen;
    md5_value_t key_md5;
    uint32_t object_size;
    uint32_t nslices;

    g_list_t *slices;

} object_t;

object_t *object_new(const char *key, uint32_t keylen);
void object_free(object_t *object);

#endif // __OBJECT_H__

