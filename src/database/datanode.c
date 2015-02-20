/**
 * @file   datanode.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-23 20:45:10
 *
 * @brief
 *
 *
 */

#include "common.h"
#include "logger.h"
#include "filesystem.h"
#include "everdata.h"

#include "bucket.h"
#include "datanode.h"

datanode_t *datanode_new(uint32_t total_buckets, uint32_t total_channels, int storage_type, const char *broker_endpoint, int verbose)
{
    datanode_t *datanode = (datanode_t*)malloc(sizeof(datanode_t));
    memset(datanode, 0, sizeof(datanode_t));

    datanode->id = 0;
    datanode->total_buckets = total_buckets;
    datanode->total_channels = total_channels;
    datanode->storage_type = storage_type;
    datanode->broker_endpoint = broker_endpoint;
    datanode->verbose = verbose;

    const char *data_dir = "./data";
    if ( mkdir_if_not_exist(data_dir) != 0 ){
        error_log("mkdir %s failed.", data_dir);
        abort();
    }

    sprintf(datanode->data_dir, "%s/storage", data_dir);
    if ( mkdir_if_not_exist(datanode->data_dir) != 0 ){
        error_log("mkdir %s failed.", datanode->data_dir);
        abort();
    }

    return datanode;
}

/* ================ datanode_free() ================ */
void datanode_free(datanode_t *datanode)
{
    ZPIPE_FREE(datanode, bucket_free);

    free(datanode);
}

/* ================ datanode_loop() ================ */
void datanode_loop(datanode_t *datanode)
{

    ZPIPE_NEW_BEGIN(datanode, datanode->total_buckets);

    bucket_t *bucket = bucket_new(datanode, i);

    ZPIPE_NEW_END(datanode, bucket);

    ZPIPE_LOOP(datanode);
}

