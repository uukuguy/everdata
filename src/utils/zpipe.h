/**
 * @file   zpipe.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-22 00:41:29
 *
 * @brief
 *
 *
 */

#ifndef __ZPIPE_H__
#define __ZPIPE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _zactor_t zactor_t;
typedef struct _zsock_t zsock_t;
typedef void (zactor_fn)(zsock_t *pipe, void *args);

typedef struct zpipe_t {
    uint32_t total_actors;
    uint32_t total_over_actors;
    int verbose;
    void **actors;

    void *user_data;
} zpipe_t;

typedef struct zpipe_actor_t{
    zactor_t *actor;
    zpipe_t *zpipe;
} zpipe_actor_t;

typedef struct zpipe_actor_args_t {
    zactor_fn *thread_main;
    void *user_data;
} zpipe_actor_args_t;

zpipe_t *zpipe_new(uint32_t total_actors);
void zpipe_init(zpipe_t *zpipe, uint32_t total_actors);
void zpipe_free(zpipe_t *zpipe);
int zpipe_loop(zpipe_t *zpipe);

void zpipe_actor_root_thread_main(zsock_t *pipe, void *user_data);
//void zpipe_actor_thread_begin(zsock_t *pipe);
//void zpipe_actor_thread_end(zsock_t *pipe);

#define ZPIPE \
        zpipe_t *zpipe; \

#define ZPIPE_NEW_BEGIN(master, total_slaves) \
        master->zpipe = zpipe_new(total_slaves); \
        master->zpipe->actors = (void**)malloc(sizeof(void*) * total_slaves); \
        for (int i = 0 ; i < total_slaves ; i++ ){ \

#define ZPIPE_NEW_END(master, slave) \
        master->zpipe->actors[i] = slave; \
    }

#define ZPIPE_FREE(master, slave_free, slave_type) \
    for ( uint32_t i = 0 ; i < master->zpipe->total_actors ; i++ ){ \
        slave_free((slave_type *)master->zpipe->actors[i]); \
        master->zpipe->actors[i] = NULL; \
    } \
    free(master->zpipe->actors);\
    master->zpipe->actors = NULL; \
    zpipe_free(master->zpipe); \
    master->zpipe = NULL;

#define ZPIPE_LOOP(master) \
        zpipe_loop(master->zpipe);

#define ZPIPE_ACTOR \
        zpipe_actor_t zpipe_actor;

#define ZPIPE_ACTOR_NEW(slave, slave_thread_main) \
        { \
        zpipe_actor_args_t _actor_args = {slave_thread_main, slave}; \
        slave->zpipe_actor.actor = zactor_new(zpipe_actor_root_thread_main, (void*)&_actor_args); \
        }

        //slave->zpipe_actor.actor = zactor_new(slave_thread_main, slave);

#define ZPIPE_ACTOR_FREE(slave) \
    zactor_destroy(&slave->zpipe_actor.actor); \
    slave->zpipe_actor.actor = NULL;


//#define ZPIPE_ACTOR_THREAD_BEGIN(pipe) \
        //zpipe_actor_thread_begin(pipe);

//#define ZPIPE_ACTOR_THREAD_END(pipe) \
        //zpipe_actor_thread_end(pipe);

#ifdef __cplusplus
}
#endif

#endif // __ZPIPE_H__

