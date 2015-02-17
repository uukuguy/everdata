// (C) Copyright 2004, David M. Blei (blei [at] cs [dot] cmu [dot] edu)

// This file is part of LDA-C.

// LDA-C is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your
// option) any later version.

// LDA-C is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

#include "lda-estimate.h"

/*
 * perform inference on a document and update sufficient statistics
 *
 */

double doc_e_step(ldac_t *ldac, lda_document_t* doc, double* gamma, double** phi,
                  lda_model* model, lda_suffstats* ss)
{
    double likelihood;
    uint32_t k;

    // posterior inference

    likelihood = lda_inference(ldac, doc, model, gamma, phi);

    // update sufficient statistics

    double gamma_sum = 0;
    for (k = 0; k < model->num_topics; k++)
    {
        gamma_sum += gamma[k];
        ss->alpha_suffstats += digamma(gamma[k]);
    }
    ss->alpha_suffstats -= model->num_topics * digamma(gamma_sum);

    uint32_t n ;
    for (n = 0; n < doc->length; n++)
    {
        for (k = 0; k < model->num_topics; k++)
        {
            ss->class_word[k][doc->words[n]] += doc->counts[n]*phi[n][k];
            ss->class_total[k] += doc->counts[n]*phi[n][k];
        }
    }

    ss->num_docs = ss->num_docs + 1;

    return(likelihood);
}


/*
 * writes the word assignments line for a document to a file
 *
 */

void write_word_assignment(FILE* f, lda_document_t* doc, double** phi, lda_model* model)
{
    uint32_t n;

    fprintf(f, "%03d", doc->length);
    for (n = 0; n < doc->length; n++)
    {
        fprintf(f, " %04d:%02d",
                doc->words[n], argmax(phi[n], model->num_topics));
    }
    fprintf(f, "\n");
    fflush(f);
}


/*
 * saves the gamma parameters of the current dataset
 *
 */

void save_gamma(char* filename, double** gamma, int num_docs, int num_topics)
{
    FILE* fileptr;
    int d, k;
    fileptr = fopen(filename, "w");

    for (d = 0; d < num_docs; d++)
    {
	fprintf(fileptr, "%5.10f", gamma[d][0]);
	for (k = 1; k < num_topics; k++)
	{
	    fprintf(fileptr, " %5.10f", gamma[d][k]);
	}
	fprintf(fileptr, "\n");
    }
    fclose(fileptr);
}



/*
 * read settings.
 *
 */

void read_settings(char* filename)
{
    /*FILE* fileptr;*/
    /*char alpha_action[100];*/
    /*fileptr = fopen(filename, "r");*/
    /*fscanf(fileptr, "var max iter %d\n", &VAR_MAX_ITER);*/
    /*fscanf(fileptr, "var convergence %f\n", &VAR_CONVERGED);*/
    /*fscanf(fileptr, "em max iter %d\n", &EM_MAX_ITER);*/
    /*fscanf(fileptr, "em convergence %f\n", &EM_CONVERGED);*/
    /*fscanf(fileptr, "alpha %s", alpha_action);*/
    /*if (strcmp(alpha_action, "fixed")==0)*/
    /*{*/
	/*ESTIMATE_ALPHA = 0;*/
    /*}*/
    /*else*/
    /*{*/
	/*ESTIMATE_ALPHA = 1;*/
    /*}*/
    /*fclose(fileptr);*/
}




