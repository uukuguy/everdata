/**
 * @file  ldac.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-19 16:07:54
 * 
 * @brief  
 * 
 * 
 */

#include "ldac.h"
#include "corpus.h"
#include <stdlib.h>
#include <memory.h>

ldac_t *ldac_new()
{
    ldac_t *ldac = (ldac_t*)malloc(sizeof(ldac_t));
    memset(ldac, 0, sizeof(ldac_t));

    ldac->VAR_MAX_ITER = 20;
    ldac->VAR_CONVERGED = 0.000001;

    ldac->EM_MAX_ITER = 100;
    ldac->EM_CONVERGED = 0.0001;
    ldac->ESTIMATE_ALPHA = 1; 

    ldac->INITIAL_ALPHA = 1.0;
    ldac->NTOPICS = 10;

    ldac->model_root = "random";
    ldac->model_dir = "./ldac-model";

    return ldac;
}

void ldac_free(ldac_t *ldac)
{
    free(ldac);
}

int ldac_blei_estimate(ldac_t *ldac, docset_t *docset);
int ldac_gibbs_estimate(ldac_t *ldac, docset_t *docset, double alpha, double beta);

int ldac_estimate(ldac_t *ldac, docset_t *docset)
{
    /*return ldac_blei_estimate(ldac, docset);*/
    return ldac_gibbs_estimate(ldac, docset, 5.0, 0.01);
}

