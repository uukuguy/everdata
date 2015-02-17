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
#include <malloc.h>
#include <memory.h>

ldac_t *ldac_new(void)
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

    ldac->corpus = NULL;

    return ldac;
}

void ldac_free(ldac_t *ldac)
{
    free(ldac);
}


#include "lda-estimate.h"

void ldac_init(void)
{
    long t1;
    (void) time(&t1);
    seedMT(t1);
    // seedMT(4357U);
}

int ldac_load_corpus_from_blei_file(ldac_t *ldac, const char *filename)
{
    ldac->corpus = read_data(filename);
    return 0;
}

void write_word_assignment(FILE* f, lda_document_t* doc, double** phi, lda_model* model);
static int LAG = 5;

int ldac_do_estimate(ldac_t *ldac)
{
    const char *directory = ldac->model_dir;
    lda_corpus_t *corpus = ldac->corpus;

    uint32_t d, n;
    lda_model *model = NULL;
    double **var_gamma, **phi;

    // allocate variational parameters

    var_gamma = malloc(sizeof(double*)*(corpus->num_docs));
    for (d = 0; d < corpus->num_docs; d++)
	var_gamma[d] = malloc(sizeof(double) * ldac->NTOPICS);

    uint32_t max_length = max_corpus_length(corpus);
    phi = malloc(sizeof(double*)*max_length);
    for (n = 0; n < max_length; n++)
	phi[n] = malloc(sizeof(double) * ldac->NTOPICS);

    // initialize model

    char filename[100];

    lda_suffstats* ss = NULL;
    if (strcmp(ldac->model_root, "seeded")==0)
    {
        model = new_lda_model(corpus->num_terms, ldac->NTOPICS);
        ss = new_lda_suffstats(model);
        corpus_initialize_ss(ss, model, corpus);
        lda_mle(model, ss, 0);
        model->alpha = ldac->INITIAL_ALPHA;
    }
    else if (strcmp(ldac->model_root, "random")==0)
    {
        model = new_lda_model(corpus->num_terms, ldac->NTOPICS);
        ss = new_lda_suffstats(model);
        random_initialize_ss(ss, model);
        lda_mle(model, ss, 0);
        model->alpha = ldac->INITIAL_ALPHA;
    }
    else
    {
        model = load_lda_model(ldac->model_root);
        ss = new_lda_suffstats(model);
    }

    sprintf(filename,"%s/000",directory);
    save_lda_model(model, filename);

    // run expectation maximization

    uint32_t i = 0;
    double likelihood, likelihood_old = 0, converged = 1;
    sprintf(filename, "%s/likelihood.dat", directory);
    FILE* likelihood_file = fopen(filename, "w");

    while (((converged < 0) || (converged > ldac->EM_CONVERGED) || (i <= 2)) && (i <= ldac->EM_MAX_ITER))
    {
        i++; printf("**** em iteration %d ****\n", i);
        likelihood = 0;
        zero_initialize_ss(ss, model);

        // e-step

        for (d = 0; d < corpus->num_docs; d++)
        {
            if ((d % 1000) == 0) printf("document %d\n",d);
            likelihood += doc_e_step(ldac, 
                                     &(corpus->docs[d]),
                                     var_gamma[d],
                                     phi,
                                     model,
                                     ss);
        }

        // m-step

        lda_mle(model, ss, ldac->ESTIMATE_ALPHA);

        // check for convergence

        converged = (likelihood_old - likelihood) / (likelihood_old);
        if (converged < 0) ldac->VAR_MAX_ITER = ldac->VAR_MAX_ITER * 2;
        likelihood_old = likelihood;

        // output model and likelihood

        fprintf(likelihood_file, "%10.10f\t%5.5e\n", likelihood, converged);
        fflush(likelihood_file);
        if ((i % LAG) == 0)
        {
            sprintf(filename,"%s/%03d",directory, i);
            save_lda_model(model, filename);
            sprintf(filename,"%s/%03d.gamma",directory, i);
            save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);
        }
    }

    // output the final model

    sprintf(filename,"%s/final",directory);
    save_lda_model(model, filename);
    sprintf(filename,"%s/final.gamma",directory);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);

    // output the word assignments (for visualization)

    sprintf(filename, "%s/word-assignments.dat", directory);
    FILE* w_asgn_file = fopen(filename, "w");
    for (d = 0; d < corpus->num_docs; d++)
    {
        if ((d % 100) == 0) printf("final e step document %d\n",d);
        likelihood += lda_inference(ldac, &(corpus->docs[d]), model, var_gamma[d], phi);
        write_word_assignment(w_asgn_file, &(corpus->docs[d]), phi, model);
    }
    fclose(w_asgn_file);
    fclose(likelihood_file);

    return 0;
}

/*
 * inference only
 *
 */

int ldac_do_inference(ldac_t *ldac)
{
    const char *model_root = ldac->model_root;
    const char *save = ldac->model_dir;
    lda_corpus_t *corpus = ldac->corpus;

    FILE* fileptr;
    char filename[100];
    uint32_t i, d, n;
    lda_model *model;
    double **var_gamma, likelihood, **phi;
    lda_document_t* doc;

    model = load_lda_model(model_root);
    var_gamma = malloc(sizeof(double*)*(corpus->num_docs));
    for (i = 0; i < corpus->num_docs; i++)
	var_gamma[i] = malloc(sizeof(double)*model->num_topics);
    sprintf(filename, "%s-lda-lhood.dat", save);
    fileptr = fopen(filename, "w");
    for (d = 0; d < corpus->num_docs; d++)
    {
	if (((d % 100) == 0) && (d>0)) printf("document %d\n",d);

	doc = &(corpus->docs[d]);
	phi = (double**) malloc(sizeof(double*) * doc->length);
	for (n = 0; n < doc->length; n++)
	    phi[n] = (double*) malloc(sizeof(double) * model->num_topics);
	likelihood = lda_inference(ldac, doc, model, var_gamma[d], phi);

	fprintf(fileptr, "%5.5f\n", likelihood);
    }
    fclose(fileptr);
    sprintf(filename, "%s-gamma.dat", save);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);

    return 0;
}


/*
 * update sufficient statistics
 *
 */

/*
 * main
 *
 */


int lda_main(int argc, char* argv[])
{
    // (est / inf) alpha k settings data (random / seed/ model) (directory / out)

    ldac_t *ldac = ldac_new();

    if (argc > 1)
    {
        if (strcmp(argv[1], "est")==0) {
            ldac->INITIAL_ALPHA = atof(argv[2]);
            ldac->NTOPICS = atoi(argv[3]);

            ldac_load_corpus_from_blei_file(ldac, argv[5]);
            make_directory(argv[7]);

            ldac->model_root = argv[6];
            ldac->model_dir = argv[7];

            ldac_do_estimate(ldac);
        }
        if (strcmp(argv[1], "inf")==0) {
            ldac_load_corpus_from_blei_file(ldac, argv[4]);

            ldac->model_root = argv[3];
            ldac->model_dir = argv[5];

            ldac_do_inference(ldac);
        }
    }
    else
    {
        printf("usage : lda est [initial alpha] [k] [settings] [data] [random/seeded/*] [directory]\n");
        printf("        lda inf [settings] [model] [data] [name]\n");
    }

    ldac_free(ldac);

    return(0);
}


