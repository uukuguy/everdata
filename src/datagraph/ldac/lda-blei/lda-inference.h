#ifndef LDA_INFERENCE_H
#define LDA_INFERENCE_H

#include <math.h>
#include <float.h>
#include <assert.h>
#include "lda.h"
#include "lda-utils.h"
#include "lda_corpus.h"

/*float VAR_CONVERGED;
int VAR_MAX_ITER;
*/

typedef struct ldac_t ldac_t;

double lda_inference(ldac_t *, lda_document_t*, lda_model*, double*, double**);
double compute_likelihood(lda_document_t*, lda_model*, double**, double*);

#endif
