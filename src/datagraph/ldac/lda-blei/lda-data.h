#ifndef LDA_DATA_H
#define LDA_DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct lda_corpus_t lda_corpus_t;

#define OFFSET 0;                  // offset for reading data

lda_corpus_t* read_data(const char* data_filename);
uint32_t max_corpus_length(lda_corpus_t* c);

#endif
