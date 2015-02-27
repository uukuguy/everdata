/**
 * @file   everdata.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-10 14:41:58
 *
 * @brief
 *
 *
 */
#ifndef __EVERDATA_H__
#define __EVERDATA_H__

#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define BROKER_FRONTEND_PORT 19977
#define BROKER_BACKEND_PORT 19978

#define HEARTBEAT_INTERVAL 1000
#define HEARTBEAT_LIVENESS 5
#define INTERVAL_INIT 1000
#define INTERVAL_MAX 32000

#define MSG_HEARTBEAT_WORKER    "\x00\x0A"
#define MSG_HEARTBEAT_BROKER    "\x00\x0B"
#define MSG_HEARTBEAT_CLIENT    "\x00\x0C"

#define MSG_STATUS_WORKER_READY    "\x0A\x00"
#define MSG_STATUS_WORKER_ACK      "\x0A\x01"
#define MSG_STATUS_WORKER_NOTFOUND "\x0A\x02"
#define MSG_STATUS_WORKER_PENDING  "\x0A\xFE"
#define MSG_STATUS_WORKER_ERROR    "\x0A\xFF"

#define MSG_STATUS_BROKER_READY "\x0B\x00"
#define MSG_STATUS_BROKER_ACK   "\x0B\x01"
#define MSG_STATUS_BROKER_PENDING "\x0B\xFE"
#define MSG_STATUS_BROKER_ERROR "\x0B\xFF"

#define MSG_STATUS_CLIENT_READY "\x0C\x00"
#define MSG_STATUS_CLIENT_ACK   "\x0C\x01"
#define MSG_STATUS_CLIENT_PENDING "\x0C\xFE"
#define MSG_STATUS_CLIENT_ERROR "\x0C\xFF"

#define MSG_ACTION_PUT "\x02\x01"
#define MSG_ACTION_GET "\x02\x02"
#define MSG_ACTION_DEL "\x02\x03"

#ifdef __cplusplus
}
#endif

#endif /* __EVERDATA_H__ */

