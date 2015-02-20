/**
 * @file   channel.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-21 23:38:33
 *
 * @brief
 *
 *
 */

#include <czmq.h>
#include "common.h"
#include "logger.h"
#include "everdata.h"

/*#include "container.h"*/
#include "datanode.h"
#include "bucket.h"
#include "channel.h"

/* ================ channel_connect_to_broker() ================ */
zsock_t *channel_connect_to_broker(channel_t *channel)
{
    notice_log("channel_connect_to_broker");
    bucket_t *bucket = channel->bucket;
    /*container_t *container = bucket->container;*/
    datanode_t *datanode = bucket->datanode;

    zsock_t *broker_sock = zsock_new_dealer(channel->broker_endpoint);

    if ( broker_sock != NULL ){
        zmsg_t *msg_worker_ready = create_status_message(MSG_STATUS_WORKER_READY);
        /*zmsg_addmem(msg_worker_ready, &channel->bucket->container->id, sizeof(uint32_t));*/
        zmsg_addmem(msg_worker_ready, &channel->bucket->datanode->id, sizeof(uint32_t));
        zmsg_addmem(msg_worker_ready, &channel->bucket->id, sizeof(uint32_t));
        zmsg_addmem(msg_worker_ready, &channel->id, sizeof(uint32_t));
        zmsg_send(&msg_worker_ready, broker_sock);
    } else {
        /*warning_log("Channel(%d) int bucket(%d) container(%d) connect to broker failed. endpoint:%s", channel->id, bucket->id, container->id, channel->broker_endpoint);*/
        warning_log("Channel(%d) int bucket(%d) datanode(%d) connect to broker failed. endpoint:%s", channel->id, bucket->id, datanode->id, channel->broker_endpoint);
    }

    return broker_sock;
}

/* ================ channel_thread_main() ================ */
void channel_thread_main(zsock_t *pipe, void *user_data)
{
    channel_t *channel = (channel_t*)user_data;

    bucket_t *bucket = channel->bucket;
    /*container_t *container = bucket->container;*/
    datanode_t *datanode = bucket->datanode;

    /*trace_log("Channel %d in bucket(%d) container(%d) Ready.", channel->id, bucket->id, container->id);*/
    trace_log("Channel %d in bucket(%d) datanode(%d) Ready.", channel->id, bucket->id, datanode->id);

    zsock_signal(pipe, 0);
    message_send_status(pipe, MSG_STATUS_ACTOR_READY);

    zsock_t *broker_sock = channel_connect_to_broker(channel);
    if ( broker_sock == NULL ){
    }

    uint32_t interval = INTERVAL_INIT;
    uint32_t liveness = HEARTBEAT_LIVENESS * 2;

    zpoller_t *poller = zpoller_new(broker_sock, NULL);
    while ( true ){
        zsock_t *sock = zpoller_wait(poller, HEARTBEAT_INTERVAL);

        if ( zclock_time() > channel->heartbeat_at ){
            /*trace_log("--> Channel(%d) Bucket(%d) Container(%d) Send worker heartbeat.", channel->id, bucket->id, container->id);*/
            trace_log("--> Channel(%d) Bucket(%d) Datanode(%d) Send worker heartbeat.", channel->id, bucket->id, datanode->id);
            channel->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

            message_send_heartbeat(broker_sock, MSG_HEARTBEAT_WORKER);
        }

        if ( sock != NULL ){
            zmsg_t *msg = zmsg_recv(sock);
            if ( msg == NULL ){
                break;
            }
            /*zmsg_print(msg);*/

            if ( message_check_heartbeat(msg, MSG_HEARTBEAT_BROKER) == 0 ){
            /*trace_log("<-- Channel(%d) Bucket(%d) Container(%d) Receive broker heartbeat.", channel->id, bucket->id, container->id);*/
            trace_log("<-- Channel(%d) Bucket(%d) Datanode(%d) Receive broker heartbeat.", channel->id, bucket->id, datanode->id);
                liveness = HEARTBEAT_LIVENESS;
                zmsg_destroy(&msg);
            } else {
                bucket_handle_message(bucket, sock, msg);
            }
        } else {
            if ( --liveness == 0 ){
                /*zclock_sleep(interval);*/
                if ( interval < INTERVAL_MAX ){
                    interval *= 2;
                }

                /*warning_log("Channel(%d) Bucket(%d) Container(%d) timeout. Try reconnect...", channel->id, bucket->id, container->id);*/
                warning_log("Channel(%d) Bucket(%d) Datanode(%d) timeout. Try reconnect...", channel->id, bucket->id, datanode->id);
                zsock_destroy(&broker_sock);

                broker_sock = channel_connect_to_broker(channel);
                zpoller_destroy(&poller);
                poller = zpoller_new(broker_sock, NULL);

                liveness = HEARTBEAT_LIVENESS;
            }
        }

    }
    zpoller_destroy(&poller);

    message_send_status(pipe, MSG_STATUS_ACTOR_OVER);

    zsock_destroy(&broker_sock);

    /*trace_log("Channel(%d) Bucket(%d) Container(%d) Exit.", channel->id, channel->bucket->id, channel->bucket->container->id);*/
    trace_log("Channel(%d) Bucket(%d) Datanode(%d) Exit.", channel->id, channel->bucket->id, channel->bucket->datanode->id);

}

/* ================ channel_new() ================ */
channel_t *channel_new(bucket_t *bucket, uint32_t channel_id)
{
    notice_log("channel_new()");
    channel_t *channel = (channel_t*)malloc(sizeof(channel_t));
    memset(channel, 0, sizeof(channel_t));

    channel->id = channel_id;
    channel->bucket = bucket;
    channel->broker_endpoint = bucket->broker_endpoint;
    channel->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

    /* --------channel->actor -------- */
    ZPIPE_ACTOR_NEW(channel, channel_thread_main);

    return channel;
}

/* ================ channel_free() ================ */
void channel_free(channel_t *channel)
{
    ZPIPE_ACTOR_FREE(channel);

    free(channel);
}

