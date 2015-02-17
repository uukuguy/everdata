/**
 * @file  ldac_estimate.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-22 12:37:36
 * 
 * @brief  
 * 
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <limits.h>
#include "string.h"
#include "ldac.h"
#include "docset.h"
#include "document.h"

/* ---------------- utils ---------------- */

int myclock ()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

char *rtime (double t)
{
	int hour, min, sec;
	static char buf[BUFSIZ];

	hour = (int)floor((int)t / 60 / 60);
	min  = (int)floor(((int)t % (60 * 60)) / 60);
	sec  = (int)floor((int)t % 60);
	sprintf(buf, "%2d:%02d:%02d", hour, min, sec);
	
	return buf;
}

int doublecmp (double *x, double *y)
{
	return (*x == *y) ? 0 : ((*x < *y) ? 1 : -1);
}

int converged (double *u, double *v, int n, double threshold)
{
	/* return 1 if |a - b|/|a| < threshold */
	double us = 0;
	double ds = 0;
	double d;
	int i;
	
	for (i = 0; i < n; i++)
		us += u[i] * u[i];

	for (i = 0; i < n; i++) {
		d = u[i] - v[i];
		ds += d * d;
	}

	if (sqrt(ds / us) < threshold)
		return 1;
	else
		return 0;

}

void normalize_matrix_col (double **dst, double **src, int rows, int cols)
{
	/* column-wise normalize from src -> dst */
	double z;
	int i, j;

	for (j = 0; j < cols; j++) {
		for (i = 0, z = 0; i < rows; i++)
			z += src[i][j];
		for (i = 0; i < rows; i++)
			dst[i][j] = src[i][j] / z;
	}
}

void normalize_matrix_row (double **dst, double **src, int rows, int cols)
{
	/* row-wise normalize from src -> dst */
	int i, j;
	double z;

	for (i = 0; i < rows; i++) {
		for (j = 0, z = 0; j < cols; j++)
			z += src[i][j];
		for (j = 0; j < cols; j++)
			dst[i][j] = src[i][j] / z;
	}
}


/* ---------------- dmatrix ---------------- */

double ** dmatrix (int rows, int cols)
{
	double **matrix;
	int i;

	matrix = (double **)calloc(rows, sizeof(double *));
	if (matrix == NULL)
		return NULL;
	for (i = 0; i < rows; i++) {
		matrix[i] = (double *)calloc(cols, sizeof(double));
		if (matrix[i] == NULL)
			return NULL;
	}
	
	return matrix;
}

void free_dmatrix (double **matrix, int rows)
{
	int i;
	for (i = 0; i < rows; i++)
		free(matrix[i]);
	free(matrix);
}

/* ---------------- gamma ---------------- */

/*
    gamma.c
    digamma and trigamma function by Thomas Minka
    $Id: gamma.c,v 1.3 2004/11/10 04:23:06 dmochiha Exp $

*/

#define psi(x)	digamma(x)
#define ppsi(x)	trigamma(x)

/* The digamma function is the derivative of gammaln.

   Reference:
    J Bernardo,
    Psi ( Digamma ) Function,
    Algorithm AS 103,
    Applied Statistics,
    Volume 25, Number 3, pages 315-317, 1976.

    From http://www.psc.edu/~burkardt/src/dirichlet/dirichlet.f
    (with modifications for negative numbers and extra precision)
*/
double digamma(double x)
{
  double result;
  static const double
	  neginf = -1.0/0.0,
	  c = 12,
	  s = 1e-6,
	  d1 = -0.57721566490153286,
	  d2 = 1.6449340668482264365, /* pi^2/6 */
	  s3 = 1./12,
	  s4 = 1./120,
	  s5 = 1./252,
	  s6 = 1./240,
	  s7 = 1./132;
	  /*s8 = 691/32760,*/
	  /*s9 = 1/12,*/
	  /*s10 = 3617/8160;*/
  /* Illegal arguments */
  if((x == neginf) || isnan(x)) {
    return 0.0/0.0;
  }
  /* Singularities */
  if((x <= 0) && (floor(x) == x)) {
    return neginf;
  }
  /* Negative values */
  /* Use the reflection formula (Jeffrey 11.1.6):
   * digamma(-x) = digamma(x+1) + pi*cot(pi*x)
   *
   * This is related to the identity
   * digamma(-x) = digamma(x+1) - digamma(z) + digamma(1-z)
   * where z is the fractional part of x
   * For example:
   * digamma(-3.1) = 1/3.1 + 1/2.1 + 1/1.1 + 1/0.1 + digamma(1-0.1)
   *               = digamma(4.1) - digamma(0.1) + digamma(1-0.1)
   * Then we use
   * digamma(1-z) - digamma(z) = pi*cot(pi*z)
   */
  if(x < 0) {
    return digamma(1-x) + M_PI/tan(-M_PI*x);
  }
  /* Use Taylor series if argument <= S */
  if(x <= s) return d1 - 1/x + d2*x;
  /* Reduce to digamma(X + N) where (X + N) >= C */
  result = 0;
  while(x < c) {
    result -= 1/x;
    x++;
  }
  /* Use de Moivre's expansion if argument >= C */
  /* This expansion can be computed in Maple via asympt(Psi(x),x) */
  if(x >= c) {
    double r = 1/x;
    result += log(x) - 0.5*r;
    r *= r;
    result -= r * (s3 - r * (s4 - r * (s5 - r * (s6 - r * s7))));
  }
  return result;
}

/* The trigamma function is the derivative of the digamma function.

   Reference:

    B Schneider,
    Trigamma Function,
    Algorithm AS 121,
    Applied Statistics, 
    Volume 27, Number 1, page 97-99, 1978.

    From http://www.psc.edu/~burkardt/src/dirichlet/dirichlet.f
    (with modification for negative arguments and extra precision)
*/
double trigamma(double x)
{
  double result;
  double neginf = -1.0/0.0,
	  small = 1e-4,
	  large = 8,
	  c = 1.6449340668482264365, /* pi^2/6 = Zeta(2) */
	  c1 = -2.404113806319188570799476,  /* -2 Zeta(3) */
	  b2 =  1./6,
	  b4 = -1./30,
	  b6 =  1./42,
	  b8 = -1./30,
	  b10 = 5./66;
  /* Illegal arguments */
  if((x == neginf) || isnan(x)) {
    return 0.0/0.0;
  }
  /* Singularities */
  if((x <= 0) && (floor(x) == x)) {
    return -neginf;
  }
  /* Negative values */
  /* Use the derivative of the digamma reflection formula:
   * -trigamma(-x) = trigamma(x+1) - (pi*csc(pi*x))^2
   */
  if(x < 0) {
    result = M_PI/sin(-M_PI*x);
    return -trigamma(1-x) + result*result;
  }
  /* Use Taylor series if argument <= small */
  if(x <= small) {
    return 1/(x*x) + c + c1*x;
  }
  result = 0;
  /* Reduce to trigamma-1(x+n) where ( X + N ) >= B */
  while(x < large) {
    result += 1/(x*x);
    x++;
  }
  /* Apply asymptotic formula when X >= B */
  /* This expansion can be computed in Maple via asympt(Psi(1,x),x) */
  if(x >= large) {
    double r = 1/(x*x);
    result += 0.5*r + (1 + r*(b2 + r*(b4 + r*(b6 + r*(b8 + r*b10)))))/x;
  }
  return result;
}

/* ---------------- feature ---------------- */
typedef struct {
	int    len;
	int    *id;
        double *cnt;
} document;
 
/*void free_feature_matrix (document *matrix)*/
/*{*/
	/*document *dp;*/
	/*for (dp = matrix; (dp->len) != -1; dp++)*/
	/*{*/
		/*free(dp->id);*/
		/*free(dp->cnt);*/
	/*}*/
	/*free(matrix);*/
/*}*/

/* ---------------- likelihood ---------------- */

double ldac_lik (docset_t *docset, double **beta, double **gammas, int m, uint32_t nclass)
{
	double **egammas;
	double z, lik;
	/*document *dp;*/
	uint32_t i, j, k;
	lik = 0;
	
	if ((egammas = dmatrix(m, nclass)) == NULL) {
		fprintf(stderr, "ldac_likelihood:: cannot allocate egammas.\n");
		exit(1);
	}
	normalize_matrix_row(egammas, gammas, m, nclass);
	
	/*for (dp = data, i = 0; (dp->len) != -1; dp++, i++)*/
    size_t total_docs = docset_get_total_docs(docset);
    for ( i = 0 ; i < total_docs ; i++ )
	{
		/*n = dp->len;*/
        doc_t *doc = docset_get_document_by_index(docset, i);
        size_t num_terms = doc_get_total_terms(doc);
		for (j = 0; j < num_terms; j++) {
            uint32_t term_id = doc_get_word_termid_by_index(doc, j);
			for (k = 0, z = 0; k < nclass; k++)
				/*z += beta[dp->id[j]][k] * egammas[i][k];*/
				/*z += beta[ldac_doc->terms[j]][k] * egammas[i][k];*/
				z += beta[term_id][k] * egammas[i][k];
			/*lik += dp->cnt[j] * log(z);*/
            uint32_t term_count = doc_get_term_count_by_index(doc, j);
			lik += term_count * log(z);
			/*lik += ldac_doc->counts[j] * log(z);*/
		}
	}

	free_dmatrix(egammas, m);
	return lik;

}

/*double ldac_ppl (document *data, double **beta, double **gammas, int m, int nclass)*/
double ldac_ppl (docset_t *docset, double **beta, double **gammas, int m, uint32_t nclass)
{
	/*document *dp;*/
	double n = 0;
	size_t i, j;

	/*for (dp = data; (dp->len) != -1; dp++)*/
    size_t total_docs = docset_get_total_docs(docset);
    for ( i = 0 ; i < total_docs ; i++ ){
        doc_t *doc = docset_get_document_by_index(docset, i);
        size_t num_terms = doc_get_total_terms(doc);
		for (j = 0; j < num_terms; j++){
			/*n += ldac_doc->counts[j];*/
            uint32_t term_count = doc_get_term_count_by_index(doc, j);
            n += term_count;
        }
        doc_detach(doc);
    }

	return exp (- ldac_lik(docset, beta, gammas, m, nclass) / n);
}

/* ---------------- newton ---------------- */

#define MAX_RECURSION_LIMIT  20
#define MAX_NEWTON_ITERATION 20
void newton_alpha (double *alpha, double **gammas, int M, int K, int level)
{
	int i, j, t;
	double *g, *h, *pg, *palpha;
	double z, sh, hgz;
	double psg, spg, gs;
	double alpha0, palpha0;

	/* allocate arrays */
	if ((g = (double *)calloc(K, sizeof(double))) == NULL) {
		fprintf(stderr, "newton:: cannot allocate g.\n");
		return;
	}
	if ((h = (double *)calloc(K, sizeof(double))) == NULL) {
		fprintf(stderr, "newton:: cannot allocate h.\n");
		return;
	}
	if ((pg = (double *)calloc(K, sizeof(double))) == NULL) {
		fprintf(stderr, "newton:: cannot allocate pg.\n");
		return;
	}
	if ((palpha = (double *)calloc(K, sizeof(double))) == NULL) {
		fprintf(stderr, "newton:: cannot allocate palpha.\n");
		return;
	}

	/* initialize */
	if (level == 0)
	{
		for (i = 0; i < K; i++) {
			for (j = 0, z = 0; j < M; j++)
				z += gammas[j][i];
			alpha[i] = z / (M * K);
		}
	} else {
		for (i = 0; i < K; i++) {
			for (j = 0, z = 0; j < M; j++)
				z += gammas[j][i];
			alpha[i] = z / (M * K * pow(10, level));
		}
	}

	psg = 0;
	for (i = 0; i < M; i++) {
		for (j = 0, gs = 0; j < K; j++)
			gs += gammas[i][j];
		psg += psi(gs);
	}
	for (i = 0; i < K; i++) {
		for (j = 0, spg = 0; j < M; j++)
			spg += psi(gammas[j][i]);
		pg[i] = spg - psg;
	}

	/* main iteration */
	for (t = 0; t < MAX_NEWTON_ITERATION; t++)
	{
		for (i = 0, alpha0 = 0; i < K; i++)
			alpha0 += alpha[i];
		palpha0 = psi(alpha0);
		
		for (i = 0; i < K; i++)
			g[i] = M * (palpha0 - psi(alpha[i])) + pg[i];
		for (i = 0; i < K; i++)
			h[i] = - 1 / ppsi(alpha[i]);
		for (i = 0, sh = 0; i < K; i++)
			sh += h[i];
		
		for (i = 0, hgz = 0; i < K; i++)
			hgz += g[i] * h[i];
		hgz /= (1 / ppsi(alpha0) + sh);

		for (i = 0; i < K; i++)
			alpha[i] = alpha[i] - h[i] * (g[i] - hgz) / M;
		
		for (i = 0; i < K; i++)
			if (alpha[i] < 0) {
				if (level >= MAX_RECURSION_LIMIT) {
					fprintf(stderr, "newton:: maximum recursion limit reached.\n");
					exit(1);
				} else {
					free(g);
					free(h);
					free(pg);
					free(palpha);
					return newton_alpha(alpha, gammas, M, K, 1 + level);
				}
			}

		if ((t > 0) && converged(alpha, palpha, K, 1.0e-4)) {
			free(g);
			free(h);
			free(pg);
			free(palpha);
			return;
		} else
			for (i = 0; i < K; i++)
				palpha[i] = alpha[i];
		
	}
	fprintf(stderr, "newton:: maximum iteration reached. t = %d\n", t);
	
	return;

}

/* ---------------- vbem ---------------- */
void vbem (doc_t *doc, double *gamma, double **q,
      double *nt, double *pnt, double *ap,
      const double *alpha, const double **beta,
      int L, int K, int emmax)
{
	int j, k, l;
	double z;

	for (k = 0; k < K; k++)
		nt[k] = (double) L / K;
	
	for (j = 0; j < emmax; j++)
	{
		/* vb-estep */
		for (k = 0; k < K; k++)
			ap[k] = exp(psi(alpha[k] + nt[k]));
		/* accumulate q */
		for (l = 0; l < L; l++){
            uint32_t term_id = doc_get_word_termid_by_index(doc, l);
			for (k = 0; k < K; k++){
				/*q[l][k] = beta[d->id[l]][k] * ap[k];*/

				q[l][k] = beta[term_id][k] * ap[k];
				/*q[l][k] = beta[doc->terms[l]][k] * ap[k];*/
            }
        }
		/* normalize q */
		for (l = 0; l < L; l++) {
			z = 0;
			for (k = 0; k < K; k++)
				z += q[l][k];
			for (k = 0; k < K; k++)
				q[l][k] /= z;
		}
		/* vb-mstep */
		for (k = 0; k < K; k++) {
			z = 0;
			for (l = 0; l < L; l++){
				/*z += q[l][k] * d->cnt[l];*/

                uint32_t term_count = doc_get_term_count_by_index(doc, l);
                z += q[l][k] * term_count;
				/*z += q[l][k] * doc->counts[l];*/

            }
			nt[k] = z;
		}
		/* converge? */
		if ((j > 0) && converged(nt, pnt, K, 1.0e-2))
			break;
		for (k = 0; k < K; k++)
			pnt[k] = nt[k];
	}
	for (k = 0; k < K; k++)
		gamma[k] = alpha[k] + nt[k];
	
	return;
}

/* ---------------- ldac_learn ---------------- */
void accum_gammas (double **gammas, double *gamma, int n, int K)
{
	/* gammas(n,:) = gamma for Newton-Raphson of alpha */
	int k;
	for (k = 0; k < K; k++)
		gammas[n][k] = gamma[k];
	return;
}

void accum_betas (double **betas, double **q, int K, doc_t *doc)
{
	size_t i, k;
	size_t num_terms = doc_get_total_terms(doc);

	for (i = 0; i < num_terms; i++){
        uint32_t term_id = doc_get_termid_by_index(doc, i);
        uint32_t term_count = doc_get_term_count_by_index(doc, i);
		for (k = 0; k < K; k++){
			betas[term_id][k] += q[i][k] * term_count;
			/*betas[dp->terms[i]][k] += q[i][k] * dp->counts[i];*/
        }
    }
}


#define CONVERGENCE 1.0e-2
#define RANDOM ((double)rand()/(double)RAND_MAX)

void ldac_learn(docset_t *docset, double *alpha, double **beta,
	   uint32_t nclass, uint32_t nlex, uint32_t dlenmax,
	   uint32_t emmax, uint32_t demmax, double epsilon)
{
	double *gamma, **q, **gammas, **betas, *nt, *pnt, *ap;
	double ppl, pppl = 0;
	double z;
	uint32_t i, j, t, n;
	int start, elapsed;
	
	/*
	 *  randomize a seed
	 *
	 */
	srand(time(NULL));
	
	/*
	 *    count data length
	 *
	 */
    n = docset_get_total_docs(docset);
    size_t total_docs = docset_get_total_docs(docset);
	
	/*
	 *  initialize parameters
	 *
	 */
	for (i = 0; i < nclass; i++)
		alpha[i] = RANDOM;
	for (i = 0, z = 0; i < nclass; i++)
		z += alpha[i];
	for (i = 0; i < nclass; i++)
		alpha[i] = alpha[i] / z;
	qsort(alpha, nclass, sizeof(double), // sort alpha initially
	      (int (*)(const void *, const void *))doublecmp);

	for (j = 0; j < nclass; j++) {
		for (i = 0, z = 0; i < nlex; i++) {
			beta[i][j] = RANDOM * 10;
			z += beta[i][j];
		}
		for (i = 0; i < nlex; i++) {
			beta[i][j] = beta[i][j] / z;
		}
	}

	/*
	 *  initialize posteriors
	 *
	 */
	if ((gammas = dmatrix(n, nclass)) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate gammas.\n");
		return;
	}
	if ((betas = dmatrix(nlex, nclass)) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate betas.\n");
		return;
	}
	/*
	 *  initialize buffers
	 *
	 */
	if ((q = dmatrix(dlenmax, nclass)) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate q.\n");
		return;
	}
	if ((gamma = (double *)calloc(nclass, sizeof(double))) == NULL)
	{
		fprintf(stderr, "ldac_learn:: cannot allocate gamma.\n");
		return;
	}
	if ((ap = (double *)calloc(nclass, sizeof(double))) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate ap.\n");
		return;
	}
	if ((nt = (double *)calloc(nclass, sizeof(double))) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate nt.\n");
		return;
	}
	if ((pnt = (double*)calloc(nclass, sizeof(double))) == NULL) {
		fprintf(stderr, "ldac_learn:: cannot allocate pnt.\n");
		return;
	}

	printf("Number of documents          = %lu\n", total_docs);
	printf("Number of words              = %d\n", nlex);
	printf("Number of latent classes     = %d\n", nclass);
	printf("Number of outer EM iteration = %d\n", emmax);
	printf("Number of inner EM iteration = %d\n", demmax);
	printf("Convergence threshold        = %g\n", epsilon);

	/*
	 *  learn main
	 *
	 */
	start = myclock();
	for (t = 0; t < emmax; t++)
	{
		printf("iteration %2d/%3d..\t", t + 1, emmax);
		fflush(stdout);
		/*
		 *  VB-E step
		 *
		 */
        size_t total_docs = docset_get_total_docs(docset);
        for ( size_t i = 0 ; i < total_docs ; i++ ) {
            doc_t *doc = docset_get_document_by_index(docset, i);
            size_t num_terms = doc_get_total_terms(doc);
			vbem(doc, gamma, q, nt, pnt, ap, alpha, (const double **)beta, num_terms, nclass, demmax);
			accum_gammas(gammas, gamma, i, nclass);
			accum_betas(betas, q, nclass, doc);

            doc_detach(doc);
		}
		/*
		 *  VB-M step
		 *
		 */
		/* Newton-Raphson for alpha */
		newton_alpha(alpha, gammas, total_docs, nclass, 0);
		/* MLE for beta */
		normalize_matrix_col(beta, betas, nlex, nclass);
		/* clean buffer */
		for (i = 0; i < nlex; i++)
			for (j = 0; j < nclass; j++)
				betas[i][j] = 0;
		/*
		 *  converge?
		 *
		 */
		ppl = ldac_ppl (docset, beta, gammas, n, nclass);

		elapsed = myclock() - start;
		printf("PPL = %.04f\t", ppl); fflush(stdout);
		if ((t > 1) && (fabs((ppl - pppl)/pppl) < epsilon)) {
			if (t < 5) {
				free_dmatrix(gammas, n);
				free_dmatrix(betas, nlex);
				free_dmatrix(q, dlenmax);
				free(gamma);
				free(ap);
				free(nt);
				free(pnt);
				printf("\nearly convergence. restarting..\n");
				ldac_learn (docset, alpha, beta, nclass, nlex,
					   dlenmax, emmax, demmax, epsilon);
				return;
			} else {
				printf("\nconverged. [%s]\n", rtime(elapsed));
				break;
			}
		}
		pppl = ppl;
		/* 
		 * ETA
		 *
		 */
		printf("ETA:%s (%d sec/step)\r",
		       rtime(elapsed * ((double) emmax / (t + 1) - 1)),
		       (int)((double) elapsed / (t + 1) + 0.5));
	}
	
	free_dmatrix(gammas, n);
	free_dmatrix(betas, nlex);
	free_dmatrix(q, dlenmax);
	free(gamma);
	free(ap);
	free(nt);
	free(pnt);
	
	return;
}

size_t ldac_docset_max_terms_in_doc(docset_t *docset)
{
    size_t max_terms = 0;
    size_t n = 0;
    size_t total_docs = docset_get_total_docs(docset);
    for ( n = 0 ; n < total_docs ; n++ ){
        doc_t *doc = docset_get_document_by_index(docset, n);
        size_t num_terms = doc_get_total_terms(doc);
        if ( max_terms < num_terms )
            max_terms = num_terms;
        doc_detach(doc);
    }
    return max_terms;
}

void write_vector (FILE *fp, double *vector, uint32_t n)
{
	uint32_t i;
	for (i = 0; i < n; i++)
		fprintf(fp, "%.7e%s", vector[i], (i == n - 1) ? "\n" : "   ");
}

void write_matrix (FILE *fp, double **matrix, uint32_t rows, uint32_t cols)
{
	uint32_t i, j;
	/*for (i = 0; i < rows; i++)*/
		/*for (j = 0; j < cols; j++)*/
			/*fprintf(fp, "%.7e%s", matrix[i][j],*/
				/*(j == cols - 1) ? "\n" : "   ");*/

    for (j = 0; j < cols; j++)
        for (i = 0; i < rows; i++)
			fprintf(fp, "%.7e%s", matrix[i][j],
				(i == rows - 1) ? "\n" : "   ");
}

void ldac_write (FILE *ap, FILE *bp, double *alpha, double **beta,
	   uint32_t nclass, uint32_t nlex)
{
	printf("writing model..\n"); fflush(stdout);
	write_vector(ap, alpha, nclass);
	write_matrix(bp, beta, nlex, nclass);
	printf("done.\n"); fflush(stdout);
}

	
#define CLASS_DEFAULT		50
#define EMMAX_DEFAULT		100
#define DEMMAX_DEFAULT		20
#define EPSILON_DEFAULT		1.0e-4

int ldac_blei_estimate(ldac_t *ldac, docset_t *docset)
{
    /*const char *directory = ldac->model_dir;*/
    size_t dlenmax = ldac_docset_max_terms_in_doc(docset);

    uint32_t nclass = ldac->NTOPICS;
    uint32_t nlex = docset_get_total_terms(docset); 

	int emmax      = EMMAX_DEFAULT;		// default in lda.h
	int demmax     = DEMMAX_DEFAULT;	// default in lda.h
	double epsilon = EPSILON_DEFAULT;	// default in lda.h

	double *alpha;
	double **beta;
	FILE *ap, *bp;		// for alpha, beta

	/* allocate parameters */
	if ((alpha = (double *)calloc(nclass, sizeof(double))) == NULL) {
		fprintf(stderr, "lda:: cannot allocate alpha.\n");
		exit(1);
	}
	if ((beta = dmatrix(nlex, nclass)) == NULL) {
		fprintf(stderr, "lda:: cannot allocate beta.\n");
		exit(1);
	}
	/* open model outputs */
    char alpha_file[1024];
    char beta_file[1024];
    sprintf(alpha_file, "%s.alpha", ldac->model_root);
    sprintf(beta_file, "%s.beta", ldac->model_root);
	if (((ap = fopen(alpha_file, "w")) == NULL)
     || ((bp = fopen(beta_file, "w"))  == NULL))
	{
		fprintf(stderr, "lda:: cannot open model outputs.\n");
		exit(1);
	}

	ldac_learn (docset, alpha, beta, nclass, nlex, dlenmax,
		   emmax, demmax, epsilon);
    ldac_write (ap, bp, alpha, beta, nclass, nlex);

	free_dmatrix(beta, nlex);
	free(alpha);
	
	fclose(ap);
	fclose(bp);

    return 0;
}

