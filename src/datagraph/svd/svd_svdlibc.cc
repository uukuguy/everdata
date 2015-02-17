/**
 * @file   svd_svdlibc.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 01:08:44
 * 
 * @brief  
 * 
 * 
 */

#include "docset.h"
#include "document.h"
#include "term.h"
#include "logger.h"
#include "lexicon.h"
#include "smat.h"
#include <stdio.h>
#include <fcntl.h> /* O_DIRECT */
#include <unistd.h> /* write(), close() */
#include <string>
#include <iostream>
#include <sstream>
#include <limits.h>
#include <list>
#include <map>

extern "C" {
#include "svdlib.h"
}

typedef struct svd_data_t{
    doc_t *doc;
    SMat A;
    uint32_t *pPos;
} svd_data_t;

//void lexicon_term_loop_svdlibc(term_t *term, void *user_data)
//{
    //svd_data_t *svd_data = (svd_data_t*)user_data;
    //doc_t *doc = svd_data->doc;
    //SMat A = svd_data->A;
    //uint32_t *pPos = svd_data->pPos;
    //uint32_t pos = *pPos;

    //if ( doc_has_term(doc, term) == 1 ) {
        //uint32_t row = term_get_id(term);
        ////uint32_t col = doc_get_id(doc);
        //double term_tfidf = doc_get_term_tfidf(doc, term);
        //*(A->rowind + pos) = row;
        //*(A->value + pos) = term_tfidf;
        //*pPos = pos + 1;
    //} 
//}

/* ==================== docset_do_svd_svdlibc() ==================== */
void docset_do_svd_svdlibc(docset_t *docset, uint32_t dimensions)
{
    GET_TIME_MILLIS(msec0);

    Docset *pDocset = (Docset*)(docset->pDocset);
    uint32_t numRows = pDocset->get_total_terms();
    uint32_t numCols = pDocset->get_total_docs(); 
    uint32_t totalNonZeroValues = pDocset->calculate_nonzerovalues();
    smat_t *tfm = pDocset->calculate_tfmatrix();

    SMat A = svdNewSMat(numRows, numCols, totalNonZeroValues);

    SMAT_LOOP_FOREACH_COL(tfm, numCols);
        A->pointr[col] = v;
    SMAT_LOOP_FOREACH_ROW(tfm, numRos);
        *(A->rowind + v) = row;
        *(A->value + v) = value;
    SMAT_LOOP_END_ROW();
    SMAT_LOOP_END_COL();
    A->pointr[numCols] = totalNonZeroValues;

    smat_free(tfm);

    //uint32_t v = 0;
    //for ( uint32_t col = 0 ; col < numCols ; col++ ){
        //A->pointr[col] = v;
        //for ( ; v < tfm->pointr[col + 1]; v++) {
            //uint32_t row = tfm->rowind[v];
            //double tfidf = tfm->values[v];
            //*(A->rowind + v) = row;
            //*(A->value + v) = tfidf;
        //}
    //}
    //A->pointr[numCols] = totalNonZeroValues;

    GET_TIME_MILLIS(msec1);
    notice_log("svd prepare: %zu.%03zu sec.", (size_t)(msec1 - msec0) / 1000, (size_t)(msec1 - msec0) % 1000);

    int iterations = 0;
    double las2end[2] = {-1.0e-30, 1.0e-30};
    double kappa = 1e-6;
    SVDRec R = svdLAS2(A, dimensions, iterations, las2end, kappa);

    GET_TIME_MILLIS(msec3);

    //docset->v_mtx = v_mtx;
    //docset->s_vct = s_vct;

    if ( R != NULL ){
        printf("\ns_vct: ");
        for ( int i = 0 ; i < R->d ; i++ ){
            printf("%.6f ", R->S[i]);
        }
        printf("\n");
    }

    svdFreeSVDRec(R);
    svdFreeSMat(A);

    notice_log("svd do: %zu.%03zu sec.", (size_t)(msec3 - msec1) / 1000, (size_t)(msec3 - msec1) % 1000);

    notice_log("svd total: %zu.%03zu sec.", (size_t)(msec3 - msec0) / 1000, (size_t)(msec3 - msec0) % 1000);
}

