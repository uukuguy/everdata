#ifndef LDA_ESTIMATE_H
#define LDA_ESTIMATE_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>

#include "lda.h"
#include "lda-data.h"
#include "lda-inference.h"
#include "lda-model.h"
#include "lda-alpha.h"
#include "lda-utils.h"
#include "lda-cokus.h"

/*int LAG = 5;*/

/*float EM_CONVERGED;
int EM_MAX_ITER; 
int ESTIMATE_ALPHA; */
/*double INITIAL_ALPHA;
int NTOPICS;
*/

typedef struct ldac_t ldac_t;

double doc_e_step(ldac_t *ldac, 
                  lda_document_t* doc,
                  double* gamma,
                  double** phi,
                  lda_model* model,
                  lda_suffstats* ss);

void save_gamma(char* filename,
                double** gamma,
                int num_docs,
                int num_topics);

void read_settings(char* filename);
/*
void run_em(char* start,
            char* directory,
            corpus* corpus);

void infer(char* model_root,
           char* save,
           corpus* corpus);
           */

#endif


