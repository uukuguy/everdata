/**
 * @file   edbroker.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-08 03:50:03
 *
 * @brief
 *
 *
 */

#include <czmq.h>
#include <pthread.h>
#include "common.h"
#include "logger.h"
#include "farmhash.h"
#include "everdata.h"

#include "cboost.h"
#include "crush.hpp"

/* -------- struct worker_t -------- */
typedef struct worker_t {
    zframe_t *identity;
    char *id_string;
    int64_t expiry;
} worker_t;

worker_t *worker_new(zframe_t *identity)
{
    worker_t *worker = (worker_t*)malloc(sizeof(worker_t));
    memset(worker, 0, sizeof(worker_t));

    worker->identity = identity;
    worker->id_string = zframe_strhex(identity);
    worker->expiry = zclock_time() + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;

    return worker;
}

void worker_free(worker_t *worker)
{
    if ( worker->identity != NULL )
        zframe_destroy(&worker->identity);
    if ( worker->id_string != NULL ){
        free(worker->id_string);
        worker->id_string = NULL;
    }
    free(worker);
}

/* -------- struct broker_t -------- */
typedef struct broker_t{
    zloop_t *loop;
    zsock_t *sock_local_frontend;
    zsock_t *sock_local_backend;
    int heartbeat_timer_id;
    int64_t heartbeat_at;

    pthread_mutex_t workers_lock;

    g_stringmap_t *backends;
    g_vector_t *select_backends;

    int is_stub;

    CRush m_rush;

} broker_t;

broker_t *broker_new(void)
{
    broker_t *broker = (broker_t*)malloc(sizeof(broker_t));
    memset(broker, 0, sizeof(broker_t));

    pthread_mutex_init(&broker->workers_lock, NULL);

    broker->heartbeat_timer_id = -1;
    broker->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

    broker->backends = g_stringmap_new();
    broker->select_backends = g_vector_new();

    broker->is_stub = 0;

    return broker;
}

void broker_lock_workers(broker_t *broker)
{
    pthread_mutex_lock(&broker->workers_lock);
}

void broker_unlock_workers(broker_t *broker)
{
    pthread_mutex_unlock(&broker->workers_lock);
}

void broker_free(broker_t *broker)
{

    broker_lock_workers(broker);

    g_iterator_t *it = g_stringmap_begin(broker->backends);
    g_iterator_t *itend = g_stringmap_end(broker->backends);
    while ( g_iterator_compare(it, itend) != 0 ){
        worker_t *w = (worker_t*)g_iterator_get(it);
        if ( w != NULL )
            worker_free(w);
        g_iterator_next(it);
    };
    g_iterator_free(it);
    g_iterator_free(itend);
    g_stringmap_free(broker->backends);
    broker->backends = NULL;
    g_vector_free(broker->select_backends);
    broker->select_backends = NULL;

    broker_unlock_workers(broker);

    pthread_mutex_destroy(&broker->workers_lock);


    free(broker);
}

uint32_t broker_get_available_workers(broker_t *broker)
{
    return g_stringmap_size(broker->backends);
}

void broker_end_local_frontend(broker_t *broker)
{
    if ( broker->sock_local_frontend != NULL ){
        zloop_reader_end(broker->loop, broker->sock_local_frontend);
        broker->sock_local_frontend = NULL;
    }
}

void broker_end_local_backend(broker_t *broker)
{
    if ( broker->sock_local_backend != NULL ){
        zloop_reader_end(broker->loop, broker->sock_local_backend);
        broker->sock_local_backend = NULL;
    }
}

void broker_end_timer(broker_t *broker)
{
    if ( broker->heartbeat_timer_id != -1 ){
        zloop_timer_end(broker->loop, broker->heartbeat_timer_id);
        broker->heartbeat_timer_id = -1;
    }
}

void broker_end_loop(broker_t *broker)
{
    broker_end_local_frontend(broker);
    broker_end_local_backend(broker);
    broker_end_timer(broker);
}

void refresh_select_backends(broker_t *broker)
{
    g_vector_t *backends = broker->select_backends;
    g_vector_clear(backends);
    g_iterator_t *it = g_stringmap_begin(broker->backends);
    g_iterator_t *itend = g_stringmap_end(broker->backends);
    while ( g_iterator_compare(it, itend) != 0 ){
        worker_t *w = (worker_t*)g_iterator_get(it);
        g_vector_push_back(backends, w);
        g_iterator_next(it);
    }
    g_iterator_free(it);
    g_iterator_free(itend);
}

/* ================ broker_set_worker_ready() ================ */
worker_t *broker_set_worker_ready(broker_t *broker, zframe_t *worker_identity)
{

    broker_lock_workers(broker);

    char *id_string = zframe_strhex(worker_identity);

    int worker_found = 0;

    g_stringmap_t *backends = broker->backends;
    g_iterator_t *it = g_stringmap_find(backends, id_string);
    g_iterator_t *itend = g_stringmap_end(backends);
    free(id_string);

    worker_t *worker = NULL;
    if ( g_iterator_compare(it, itend) != 0){
        worker_found = 1;
        worker = (worker_t*)g_iterator_get(it);
    } else {
        worker_found = 0;
        worker = worker_new(worker_identity);
    }

    if ( worker_found == 1 ){
        int64_t now = zclock_time();
        worker->expiry = now + HEARTBEAT_INTERVAL * HEARTBEAT_LIVENESS;
        /*notice_log("worker(%s) ready. expiry:%zu", worker->id_string, worker->expiry - now);*/
    } else {
        g_stringmap_insert(backends, worker->id_string, worker);
        refresh_select_backends(broker);
    }

    /*refresh_select_backends(broker);*/

    broker_unlock_workers(broker);

    return worker;
}

/* ================ broker_workers_purge() ================ */
void broker_workers_purge(broker_t *broker)
{

    broker_lock_workers(broker);

    g_stringmap_t *backends = broker->backends;

    g_iterator_t *it = g_stringmap_begin(backends);
    g_iterator_t *itend = g_stringmap_end(backends);
    while ( g_iterator_compare(it, itend) != 0 ){
        worker_t *worker = (worker_t*)g_iterator_get(it);
        if ( worker != NULL ){
            int64_t now = zclock_time();
            int64_t expiry = worker->expiry;
            if ( now > expiry ){
                g_iterator_erase(it);
                warning_log("Worker %s workers(%zu) timeout. Remove from queue. now:%llu worker expiry:%llu(%d)", worker->id_string, g_stringmap_size(backends), now, expiry, (int32_t)(expiry - now));

                zframe_t *identity = worker->identity;
                zframe_destroy(&identity);
                worker->identity = NULL;
                worker_free(worker);

                continue;
            }
        }
        g_iterator_next(it);
    };

    refresh_select_backends(broker);

    broker_unlock_workers(broker);
}

/* ================ broker_choose_worker_identity() ================ */
zframe_t *broker_choose_worker_identity(broker_t *broker, zmsg_t *msg)
{
    zframe_t *worker_identity = NULL;

    UNUSED zframe_t *frame_identity = zmsg_first(msg);
    if ( frame_identity != NULL ){
        UNUSED zframe_t *frame_empty = zmsg_next(msg);
        if ( frame_empty != NULL ){
            UNUSED zframe_t *frame_msgtype = zmsg_next(msg);
            if ( frame_msgtype != NULL ){
                UNUSED zframe_t *frame_action = zmsg_next(msg);
                if ( frame_action != NULL ) {
                    zframe_t *frame_key = zmsg_next(msg);
                    if ( frame_key != NULL ) {
                        const char *key = (const char *)zframe_data(frame_key);
                        UNUSED uint32_t key_len = zframe_size(frame_key);

                        //md5_value_t key_md5;
                        //md5(&key_md5, (uint8_t *)key, key_len);
                        int32_t hash = util::Hash32(key, key_len);

                        /*g_vector_t *backends = g_vector_new();*/
                        /*g_iterator_t *it = g_stringmap_begin(broker->backends);*/
                        /*g_iterator_t *itend = g_stringmap_end(broker->backends);*/
                        /*while ( g_iterator_compare(it, itend) != 0 ){*/
                            /*worker_t *w = g_iterator_get(it);*/
                            /*g_vector_push_back(backends, w);*/
                            /*g_iterator_next(it);*/
                        /*}*/
                        /*g_iterator_free(it);*/
                        /*g_iterator_free(itend);*/
                        g_vector_t *backends = broker->select_backends;
                        size_t total_backends = g_vector_size(backends);
                        if ( total_backends > 0 ){
                            //int idx = key_md5.h1 % total_backends;
                            int idx = hash % total_backends;
                            worker_t *worker = (worker_t*)g_vector_get_element(backends, idx);
                            if ( worker != NULL ){
                                worker_identity = zframe_dup(worker->identity);
                                /*notice_log("Choose worker identity: %s, key: %s", worker->id_string, key);*/
                            }
                        }

                        /*g_vector_free(backends);*/
                    }
                }
            }
        }
    }

    return worker_identity;
}

/* ================ handle_pullin_on_local_frontend() ================ */
int handle_pullin_on_local_frontend(zloop_t *loop, zsock_t *sock, void *user_data)
{
    broker_t *broker = (broker_t*)user_data;

    zmsg_t *msg = zmsg_recv(sock);
    if ( msg == NULL ){
        broker_end_loop(broker);
        return -1;
    }
    /*zmsg_print(msg);*/

    if ( broker->is_stub ) {
        zmsg_t *sendback_msg = create_sendback_message(msg);
        message_add_status(sendback_msg, MSG_STATUS_WORKER_ACK);
        zmsg_send(&sendback_msg, sock);
    } else {
        zframe_t *worker_identity = broker_choose_worker_identity(broker, msg);

        if ( worker_identity != NULL ){
            /* for req */
            /*zmsg_pushmem(msg, "", 0);*/
            zmsg_push(msg, worker_identity);
            zmsg_send(&msg, broker->sock_local_backend);
        } else {
            zmsg_t *sendback_msg = create_sendback_message(msg);
            message_add_status(sendback_msg, MSG_STATUS_WORKER_ERROR);
            zmsg_send(&sendback_msg, sock);
        }
    }

    zmsg_destroy(&msg);

    return 0;
}

/* ================ handle_pullin_on_local_backend() ================ */
int handle_pullin_on_local_backend(zloop_t *loop, zsock_t *sock, void *user_data)
{
    broker_t *broker = (broker_t*)user_data;

    zmsg_t *msg = zmsg_recv(sock);
    if ( msg == NULL ){
        broker_end_loop(broker);
        return -1;
    }
    /*zmsg_print(msg);*/

    zframe_t *worker_identity = zmsg_unwrap(msg);
    assert(zframe_is(worker_identity));

    worker_t *worker = broker_set_worker_ready(broker, worker_identity);

    if ( message_check_heartbeat(msg, MSG_HEARTBEAT_WORKER) == 0 ){
        broker_lock_workers(broker);
        uint32_t available_workers = broker_get_available_workers(broker);
        int64_t now = zclock_time();
        int64_t expiry = 0;

        expiry = worker->expiry;

        broker_unlock_workers(broker);
        trace_log("<-- Receive worker heartbeat. Workers:%d. now:%llu first expiry:%llu(%d)", available_workers, now, expiry, (int32_t)(expiry - now));
        zmsg_destroy(&msg);

    } else if ( message_check_status(msg, MSG_STATUS_WORKER_READY) == 0 ) {
        broker_lock_workers(broker);
        uint32_t available_workers = broker_get_available_workers(broker);
        broker_unlock_workers(broker);
        notice_log("WORKER(%s) READY. Workers:%d", worker->id_string, available_workers);
        zmsg_destroy(&msg);
    }

    if ( msg != NULL ){
        /*zmsg_print(msg);*/
        zmsg_send(&msg, broker->sock_local_frontend);
    }

    return 0;
}

/* ================ handle_heartbeat_timer() ================ */
int handle_heartbeat_timer(zloop_t *loop, int timer_id, void *user_data)
{
    broker_t *broker = (broker_t*)user_data;
    assert(broker != NULL);

    if ( zclock_time() >= broker->heartbeat_at ){

        broker->heartbeat_at = zclock_time() + HEARTBEAT_INTERVAL;

        broker_lock_workers(broker);

        g_stringmap_t *backends = broker->backends;
        g_iterator_t *it = g_stringmap_begin(backends);
        g_iterator_t *itend = g_stringmap_end(backends);
        while ( g_iterator_compare(it, itend) != 0 ){
            worker_t *worker = (worker_t*)g_iterator_get(it);
            {
                uint32_t available_workers = broker_get_available_workers(broker);
                int64_t now = zclock_time();
                int64_t expiry = 0;
                expiry = worker->expiry;
                trace_log("--> Send broker heartbeat to worker %s. Workers:%d. now:%llu first expiry:%llu(%d)", worker->id_string, available_workers, now, expiry, (int32_t)(expiry - now));
            }

            zmsg_t *heartbeat_msg = zmsg_new();
            zmsg_push(heartbeat_msg, zframe_dup(worker->identity));
            message_add_heartbeat(heartbeat_msg, MSG_HEARTBEAT_BROKER);
            zmsg_send(&heartbeat_msg, broker->sock_local_backend);

            g_iterator_next(it);
        };

        broker_unlock_workers(broker);

    }

    broker_workers_purge(broker);

    return 0;
}

/* ================ run_broker() ================ */
int run_broker(const char *frontend, const char *backend, int is_stub, int verbose)
{
    info_log("run_broker() with frontend:%s backend:%s", frontend, backend);

    int rc = 0;
    broker_t *broker = broker_new();
    broker->is_stub = is_stub;

    zsock_t *sock_local_frontend = zsock_new_router(frontend);
    zsock_t *sock_local_backend = zsock_new_router(backend);

    zloop_t *loop = zloop_new();
    zloop_set_verbose(loop, verbose);

    broker->loop = loop;
    broker->sock_local_frontend = sock_local_frontend;
    broker->sock_local_backend = sock_local_backend;

    broker->heartbeat_timer_id = zloop_timer(loop, HEARTBEAT_INTERVAL, -1, handle_heartbeat_timer, broker);

    zloop_reader(loop, sock_local_frontend, handle_pullin_on_local_frontend, broker);
    zloop_reader(loop, sock_local_backend, handle_pullin_on_local_backend, broker);

    zloop_start(loop);

    zsock_destroy(&sock_local_frontend);
    zsock_destroy(&sock_local_backend);

    broker_free(broker);

    return rc;
}


