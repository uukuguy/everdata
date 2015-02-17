/**
 * @file  ldac_corpus.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-22 14:26:58
 * 
 * @brief  
 * 
 * 
 */

#ifndef __LDAC_CORPUS_H__
#define __LDAC_CORPUS_H__

#include <stdint.h>

typedef struct ldac_document_t {
    uint32_t *terms;
    //int *counts;
    double *counts;
    uint32_t num_terms;

    uint32_t *words;
    uint32_t num_words;

    uint32_t total;
} ldac_document_t;


typedef struct ldac_corpus_t {
    ldac_document_t* docs;
    uint32_t num_terms;
    uint32_t num_docs;
} ldac_corpus_t;

#endif // __LDAC_CORPUS_H__


