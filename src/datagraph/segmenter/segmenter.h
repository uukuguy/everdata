/**
 * @file  segmenter.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 17:45:50
 * 
 * @brief  
 * 
 * 
 */

#ifndef __SEGMENTER_H__
#define __SEGMENTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

typedef struct segmenter_t segmenter_t;

typedef void(*segment_peek_token_fn)(const char *token, uint32_t token_len, void *user_data);

segmenter_t *segmenter_new(const char *segclass, const char *dictpath);
void segmenter_free(segmenter_t *segmenter);
int segmenter_segment_buffer(segmenter_t *segmenter, const char *buf, size_t buf_size, segment_peek_token_fn segment_peek_token, void *user_data);

#ifdef __cplusplus
}
#endif

#endif // __SEGMENTER_H__


