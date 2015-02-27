/**
 * @file   message.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-02-23 21:52:13
 *
 * @brief
 *
 *
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MSGTYPE_UNKNOWN 0x00FF
#define MSGTYPE_STATUS 0x00FE
#define MSGTYPE_DATA    0x00FD
#define MSGTYPE_HEARTBEAT 0x00FC
#define MSGTYPE_ACTION 0x00FB

#define MSG_STATUS_ACTOR_READY  "\x01\x01"
#define MSG_STATUS_ACTOR_OVER   "\x01\xFF"

typedef struct _zsock_t zsock_t;
typedef struct _zmsg_t zmsg_t;

int16_t message_get_msgtype(zmsg_t *msg);

int message_check_msgid(zmsg_t *msg, int16_t the_msgtype, const char *id);
int message_check_status(zmsg_t *msg, const char *status);
int message_check_heartbeat(zmsg_t *msg, const char *heartbeat);
int message_check_action(zmsg_t *msg, const char *action);

void message_add_status(zmsg_t *msg, const char *status);
void message_add_heartbeat(zmsg_t *msg, const char *heartbeat);
void message_add_key_data(zmsg_t *msg, const char *key, const char *data, uint32_t data_size);
int message_send_status(zsock_t *sock, const char *status);
int message_send_heartbeat(zsock_t *sock, const char *heartbeat);

zmsg_t *create_base_message(int16_t msgtype);
zmsg_t *create_status_message(const char *status);
zmsg_t *create_heartbeat_message(const char *heartbeat);
zmsg_t *create_action_message(const char *action);
zmsg_t *create_data_message(const char *data, uint32_t data_size);
zmsg_t *create_key_data_message(const char *key, const char *data, uint32_t data_size);
zmsg_t *create_sendback_message(zmsg_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* __MESSAGE_H__ */

