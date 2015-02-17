/**
 * @file  ldac.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-19 16:06:18
 * 
 * @brief  
 * 
 * 
 */

#ifndef __LDAC_H__
#define __LDAC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct docset_t docset_t;

typedef struct ldac_t{
    /* inference */
    uint32_t VAR_MAX_ITER;
    float VAR_CONVERGED;

    /* estimate */
    uint32_t EM_MAX_ITER;
    float EM_CONVERGED;
    uint32_t ESTIMATE_ALPHA; 

    double INITIAL_ALPHA;
    uint32_t NTOPICS;

    const char *model_dir;
    const char *model_root;

} ldac_t;

ldac_t *ldac_new();
void ldac_free(ldac_t *ldac);
int ldac_estimate(ldac_t *ldac, docset_t *docset);

#ifdef __cplusplus
}
#endif

#endif // __LDAC_H__

