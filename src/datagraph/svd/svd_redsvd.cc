/**
 * @file   svd_redsvd.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-16 21:41:18
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

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/SVD>
#include <eigen3/Eigen/Sparse>

#include "redsvd/redsvd.hpp"

#include <iostream>

using namespace Eigen;

typedef struct svd_data_t{
    Document *pDocument;
    std::vector<Triplet<float> > &triple;
} svd_data_t;

//void lexicon_term_loop_redsvd(term_t *term, void *user_data)
//{
    //svd_data_t *svd_data = (svd_data_t*)user_data;
    //doc_t *doc = svd_data->doc;
    //std::vector<Triplet<float> > &triple = svd_data->triple;

    //if ( doc_has_term(doc, term) == 1 ) {
        //uint32_t row = term_get_id(term);
        //uint32_t col = doc_get_id(doc);
        //double term_tfidf = doc_get_term_tfidf(doc, term);
        //triple.push_back(Triplet<float>(row, col, term_tfidf));
    //} 
//}

/* ==================== docset_do_svd_redsvd() ==================== */
void docset_do_svd_redsvd(docset_t *docset, uint32_t dimensions)
{
    GET_TIME_MILLIS(msec0);

    Docset *pDocset = (Docset*)(docset->pDocset);

    uint32_t numRows = pDocset->get_total_terms();
    uint32_t numCols = pDocset->get_total_docs();
    //uint32_t totalNonZeroValues = pDocset->calculate_nonzerovalues();

    smat_t *tfm = pDocset->calculate_tfmatrix();
    //MatrixXf A(numRows, numCols);
    //A.setZero();
    
    //Eigen::SparseMatrix<float> A(numRows, numCols);
    //SMatrixXf A(numRows, numCols);
    Eigen::SparseMatrix<float, Eigen::RowMajor> A(numRows, numCols);

    std::vector<Triplet<float> > triple;

    SMAT_LOOP_BEGIN(tfm, numRows, numCols);
    triple.push_back(Triplet<float>(row, col, value));
    SMAT_LOOP_END();

    //docset_t::Docs::const_iterator it = docset->docs->begin();
    //for ( ; it != docset->docs->end() ; it++ ){
        //doc_t *doc = *it;

        //svd_data_t svd_data = {
            //.doc = doc,
            //.triple = triple,
        //};
        //lexicon_apply_each_term(docset->lexicon, lexicon_term_loop_redsvd, &svd_data);
    //}

    A.setFromTriplets(triple.begin(), triple.end());

    smat_free(tfm);

    GET_TIME_MILLIS(msec1);
    notice_log("svd prepare: %llu.%03llu sec.", (msec1 - msec0) / 1000, (msec1 - msec0) % 1000);

    // *****************************************
    
    //MatrixXf A = MatrixXf::Random(3,2);

    REDSVD::RedSVD svdOfA(A, dimensions);
    const Eigen::VectorXf& s = svdOfA.singularValues();
    std::cout << "Its singular values are:" << std::endl << s << std::endl;

    //unsigned int computationOptions = ComputeThinU | ComputeThinV;
    //JacobiSVD<MatrixXf> svd(numRows, numCols, computationOptions);

    //svd.compute(A);

    //std::cout << "Its singular values are:" << std::endl << svd.singularValues() << std::endl;
    //std::cout << "Its left singular vectors are the columns of the thin U matrix:" << std::endl << svd.matrixU() << std::endl;
    //std::cout << "Its right singular vectors are the columns of the thin V matrix:" << std::endl << svd.matrixV() << std::endl;

    //Vector3f rhs(1, 0, 0);
    //std::cout << "Now consider this rhs vector:" << std::endl << rhs << std::endl;
    //std::cout << "A least-squares solution of m*x = rhs is:" << std::endl << svd.solve(rhs) << std::endl;
    
    GET_TIME_MILLIS(msec3);
    notice_log("svd do: %llu.%03llu sec.", (msec3 - msec1) / 1000, (msec3 - msec1) % 1000);

    notice_log("svd total: %llu.%03llu sec.", (msec3 - msec0) / 1000, (msec3 - msec0) % 1000);
}

