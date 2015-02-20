/**
 * @file   bucket.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-21 15:09:27
 *
 * @brief
 *
 *
 */

#include <czmq.h>
#include "common.h"
#include "filesystem.h"
#include "logger.h"
#include "md5.h"
#include "everdata.h"

#include "container.h"
#include "bucket.h"
#include "channel.h"
#include "object.h"
#include "bucketdb.h"

/* ================ bucket_del_data() ================ */
zmsg_t *bucket_del_data(bucket_t *bucket, zsock_t *sock, zframe_t *identity, zmsg_t *msg)
{
    zmsg_t *sendback_msg = NULL;

    if ( sendback_msg == NULL ){
        sendback_msg = create_status_message(MSG_STATUS_WORKER_ERROR);

        zmsg_wrap(sendback_msg, identity);
        zmsg_send(&sendback_msg, sock);
    }

    return sendback_msg;
}


/* ================ bucket_get_data() ================ */
zmsg_t *bucket_get_data(bucket_t *bucket, zsock_t *sock, zframe_t *identity, zmsg_t *msg)
{
    bucketdb_t *bucketdb = bucket->bucketdb;

    zmsg_t *sendback_msg = NULL;

    zframe_t *frame_msgtype = zmsg_first(msg);
    if ( frame_msgtype != NULL ){
        zframe_t *frame_action = zmsg_next(msg);
        if ( frame_action != NULL ){

            zframe_t *frame_key = zmsg_next(msg);
            if ( frame_key != NULL ){

                const char *key = (const char *)zframe_data(frame_key);
                uint32_t key_len = zframe_size(frame_key);

                md5_value_t key_md5;
                md5(&key_md5, (uint8_t *)key, key_len);

                uint32_t slice_idx = 0;
                slice_t *slice = bucketdb_read_from_storage(bucketdb, key_md5, slice_idx);
                if ( slice != NULL ){

                    sendback_msg = create_key_data_message(key, slice->data, slice->size);

                    slice_free(slice);
                } else {
                    sendback_msg = create_status_message(MSG_STATUS_WORKER_NOTFOUND);
                } // slice != NULL
            } // frame_key != NULL
        } // frame_action != NULL
    } // frame_msgtype != NULL

    if ( sendback_msg == NULL ){
        sendback_msg = create_status_message(MSG_STATUS_WORKER_ERROR);
    }

    return sendback_msg;
}

/* ================ bucket_put_data() ================ */
zmsg_t *bucket_put_data(bucket_t *bucket, zsock_t *sock, zframe_t *identity, zmsg_t *msg)
{
    bucketdb_t *bucketdb = bucket->bucketdb;
    zmsg_t *sendback_msg = NULL;

    UNUSED zframe_t *frame_msgtype = zmsg_first(msg);
    if ( frame_msgtype != NULL ){
        UNUSED zframe_t *frame_action = zmsg_next(msg);
        if ( frame_action != NULL ) {
            zframe_t *frame_key = zmsg_next(msg);
            if ( frame_key != NULL ) {
                const char *key = (const char *)zframe_data(frame_key);
                UNUSED uint32_t key_len = zframe_size(frame_key);

                zframe_t *frame = zmsg_next(msg);

                if ( frame != NULL ){
                    const char *data = (const char *)zframe_data(frame);
                    uint32_t data_size = zframe_size(frame);

                    md5_value_t key_md5;
                    md5(&key_md5, (uint8_t *)key, key_len);

                    uint32_t slice_idx = 0;
                    slice_t *slice = slice_new(key_md5, slice_idx, data, data_size);

                    bucketdb_write_to_storage(bucketdb, slice);

                    slice_free(slice);

                    sendback_msg = create_status_message(MSG_STATUS_WORKER_ACK);
                }
            }
        }
    }
    if ( sendback_msg == NULL ){
        sendback_msg = create_status_message(MSG_STATUS_WORKER_ERROR);
    }

    return sendback_msg;
}

/* ================ bucket_handle_message() ================ */
int bucket_handle_message(bucket_t *bucket, zsock_t *sock, zmsg_t *msg)
{
    /*zmsg_print(msg);*/

    zframe_t *identity = zmsg_unwrap(msg);

    zmsg_t *sendback_msg = NULL;

    /*sendback_msg = create_status_message(MSG_STATUS_WORKER_ACK);*/
    if ( message_check_action(msg, MSG_ACTION_PUT) == 0 ){
        sendback_msg = bucket_put_data(bucket, sock, identity, msg);
    } else if (message_check_action(msg, MSG_ACTION_GET) == 0 ) {
        sendback_msg = bucket_get_data(bucket, sock, identity, msg);
    } else if (message_check_action(msg, MSG_ACTION_DEL) == 0 ) {
        sendback_msg = bucket_del_data(bucket, sock, identity, msg);
    }

    zmsg_destroy(&msg);

    if (sendback_msg != NULL) {
        zmsg_wrap(sendback_msg, identity);
        zmsg_send(&sendback_msg, sock);
    }

    return 0;
}

/* ================ bucket_thread_main() ================ */
void bucket_thread_main(zsock_t *pipe, void *user_data)
{
    bucket_t *bucket = (bucket_t*)user_data;
    container_t *container = bucket->container;

    trace_log("Bucket %d in worker(%d) Ready.", bucket->id, container->id);

    ZPIPE_ACTOR_THREAD_BEGIN(pipe);
    {

        ZPIPE_NEW_BEGIN(bucket, bucket->total_channels);

        channel_t *channel = channel_new(bucket, i);

        ZPIPE_NEW_END(bucket, channel);

        ZPIPE_LOOP(bucket);

    }

    ZPIPE_ACTOR_THREAD_END(pipe);

    trace_log("Bucket(%d) Container(%d) Exit.", bucket->id, container->id);
}

/* ================ bucket_new() ================ */
bucket_t *bucket_new(container_t *container, uint32_t bucket_id)
{
    bucket_t *bucket = (bucket_t*)malloc(sizeof(bucket_t));
    memset(bucket, 0, sizeof(bucket_t));

    bucket->id = bucket_id;
    bucket->container = container;
    bucket->total_channels = container->total_channels;
    bucket->storage_type = container->storage_type;
    bucket->broker_endpoint = container->broker_endpoint;
    bucket->verbose = container->verbose;

    /* -------- bucket->bucketdb -------- */
    bucket->bucketdb = bucketdb_new(container->data_dir, bucket_id, bucket->storage_type);

    bucket->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

    /* -------- bucket->actor -------- */
    ZPIPE_ACTOR_NEW(bucket, bucket_thread_main);

    return bucket;
}

/* ================ bucket_free() ================ */
void bucket_free(bucket_t *bucket)
{
    ZPIPE_FREE(bucket, channel_free);

    if ( bucket->bucketdb != NULL ){
        bucketdb_free(bucket->bucketdb);
        bucket->bucketdb = NULL;
    }

    ZPIPE_ACTOR_FREE(bucket);

    free(bucket);
}

