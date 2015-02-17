/**
 * @file   svd_armadillo.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 01:06:49
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
#include "dmat.h"
#include "smat.h"
#include "svd.h"

//#include <armadillo>
#include <mlpack/methods/quic_svd/quic_svd.hpp>
#include <mlpack/methods/regularized_svd/regularized_svd.hpp>


typedef struct svd_data_t{
    Document *pDocument;
    arma::mat &A;
} svd_data_t;

//void lexicon_term_loop_armadillo(term_t *term, void *user_data)
//{
    //svd_data_t *svd_data = (svd_data_t*)user_data;
    //doc_t *doc = svd_data->doc;
    //arma::mat &A = svd_data->A;

    //if ( doc_has_term(doc, term) == 1 ) {
        //uint32_t row = term_get_id(term);
        //uint32_t col = doc_get_id(doc);
        ////uint32_t row = doc_get_id(doc);
        ////uint32_t col = term_get_id(term);

        //double term_tfidf = doc_get_term_tfidf(doc, term);
        //A(row, col) = term_tfidf;
    //}
//}

dmat_t *docset_save_corrmat_armadillo(docset_t *docset, arma::mat& C, const char *filename)
{
    uint32_t numRows = C.n_rows;
    uint32_t numCols = C.n_cols;
    //dmat_free(docset->corrmat);

    dmat_t *corrmat = dmat_new(numRows, numCols);
    double *values = dmat_get_values(corrmat);
    for ( uint32_t row = 0 ; row < numRows ; row++ ){
        for ( uint32_t col = 0 ; col < numCols ; col++ ){
            double v = C(row, col);
            *values = v;
            values++;
        }
    }

    //docset->corrmat = corrmat;

    //dmat_save_to_csv(matrix, filename);

    return corrmat;
}

#include <fstream>
#include <iomanip>
int save_singular_value_armadillo(arma::vec &s, const char *filename)
{
    std::fstream out(filename, std::ios::out);
    out << std::setprecision(6);
    for ( uint32_t i = 0 ; i < s.size() ; i++ ){
        out << s(i) << std::endl;
    }
    return 0;
}


double vector_angle_cosine_armadillo(arma::vec &v1, arma::vec &v2, uint32_t size)
{
	// A(dot)B = |A||B|Cos(theta)
	// so Cos(theta) = A(dot)B / |A||B|

	double a_dot_b=0;
	for ( uint32_t i = 0 ; i < size ; i++ ) {
		a_dot_b += v1(i) * v2(i);
	}

	double A=0;
	for ( uint32_t j = 0 ; j < size ; j++ ) {
        A += v1(j) * v1(j);
	}
	A = sqrt(A);

	double B=0;
	for ( uint32_t k = 0 ; k < size ; k++ ) {
        B += v2(k) * v2(k);
	}
	B = sqrt(B);

	return a_dot_b / (A * B);
}

int docset_query_armadillo(docset_t *docset, const char *query_string, uint32_t dimensions)
{
    if ( query_string == NULL || strlen(query_string) == 0 )
        return -1;


    Docset *pDocset = (Docset*)(docset->pDocset);
    const Lexicon &lexicon = pDocset->get_lexicon();

    uint32_t numTerms = pDocset->get_total_terms();
    uint32_t numDocs = pDocset->get_total_docs();
    arma::vec q_vct(numTerms, arma::fill::zeros);

    std::istringstream query_stream(query_string);
    std::string word;
	while (query_stream >> word) {
        Term *pTerm = lexicon.get_term_by_text(word);
        if ( pTerm != NULL ){
            uint32_t term_id = pTerm->m_id;
            q_vct(term_id) = q_vct(term_id) + 1;
        }
    };

    //arma::vec &s = *(arma::vec*)docset->svd_s;
    //arma::mat &U = *(arma::mat*)docset->svd_U;
    //arma::mat &V = *(arma::mat*)docset->svd_V;
    arma::vec s;
    arma::mat U;
    arma::mat V;

    arma::vec d_vct(dimensions, arma::fill::zeros);
	// Dq = Xq' T S^-1
	for (uint32_t i = 0; i < dimensions; i++) {
		double sum = 0;
		for (uint32_t j = 0; j < numTerms; j++) {
            sum += q_vct(j) * U(j,i);
		}
        d_vct(i) = sum * ( 1 / s(i));
	}

	//compare each document with Dq
	for ( uint32_t n = 0 ; n < numDocs ; n++ ) {
        arma::vec t_vct(dimensions, arma::fill::zeros);
		// fill temp document vector
		for ( uint32_t m = 0 ; m < dimensions ; m++) {
            t_vct(m) = V(n,m) * s(m);
		}
        AddCorrelation(n, vector_angle_cosine_armadillo(d_vct, t_vct, dimensions));	
	}

    uint32_t n = 0;
    std::vector<struct doc_cor>::const_iterator it;
	for ( it = g_cor.begin() ; it < g_cor.end() ; it++, n++ ) {
        doc_cor cor = *it;
        uint32_t doc_id = cor.doc_id;
        double correlation = cor.correlation;

        Document *pDocument = pDocset->get_document_by_id(doc_id);
        const char *doc_name = "<not found>";
        if ( pDocument != NULL ){
            doc_name = pDocument->m_title.c_str();
        }

        if ( n > g_cor.size() - 10 ) {
            warning_log("%d:<%d,%.3f>%s", n, doc_id, correlation, doc_name);
        } else if ( n < 10 ) {
            notice_log("%d:<%d,%.3f>%s", n, doc_id, correlation, doc_name);
        }

    }
    return 0;
}

int export_vector_to_csv(arma::vec V, const char *filename)
{
    std::fstream out(filename, std::ios::out);

    for ( uint32_t n = 0 ; n < V.size() ; n++ ){
        out << "C" << n;
        if ( n < V.size() - 1 )
            out << ",";
    }
    out << std::endl;

    for ( uint32_t n = 0 ; n < V.size() ; n++ ){
        out << V(n);
        if ( n < V.size() - 1 )
            out << ",";
    }
    out << std::endl;

    return 0;
}

int export_matrix_to_csv(arma::mat& C, const char *filename)
{
    uint32_t numRows = C.n_rows;
    uint32_t numCols = C.n_cols;

    std::fstream out(filename, std::ios::out);

    out << "id,";
    for ( uint32_t col = 0 ; col < numCols ; col++ ){
        out << "C" << col;
        if ( col < numCols - 1 )
            out << ",";
    }
    out << std::endl;

    for ( uint32_t row = 0 ; row < numRows ; row++ ){
        out << "R" << row << ", ";
        for ( uint32_t col = 0 ; col < numCols ; col++ ){
            double v = C(row, col);
            out << v;
            if ( col < numCols - 1 )
                out << ",";
        }
        out << std::endl;
    }
    out << std::endl;

    return 0;
}

/* ==================== docset_do_svd_armadillo() ==================== */
void docset_do_svd_armadillo(docset_t *docset, uint32_t dimensions)
{
    GET_TIME_MILLIS(msec0);

    Docset *pDocset = (Docset*)(docset->pDocset);
    uint32_t numRows = pDocset->get_total_terms();
    uint32_t numCols = pDocset->get_total_docs();
    //uint32_t totalNonZeroValues = pDocset->calculate_nonzerovalues();

    //smat_t *tfm = smat_new(numRows, numCols, totalNonZeroValues);
    smat_t *tfm = pDocset->calculate_tfmatrix();

    arma::mat A(numRows, numCols, arma::fill::zeros);
    //arma::sp_mat A(numRows, numCols);

    SMAT_LOOP_BEGIN(tfm, numRows, numCols);
    A(row, col) = value;
    SMAT_LOOP_END();
    //uint32_t v = 0;
    //for ( uint32_t col = 0 ; col < numCols ; col++ ){
        //for ( ; v < tfm->pointr[col + 1]; v++) {
            //uint32_t row = tfm->rowind[v];
            //double value = tfm->values[v];
            //A(row, col) = value;
        //}

    //}

    smat_free(tfm);

    GET_TIME_MILLIS(msec1);
    notice_log("svd prepare: %llu.%03llu sec.", (msec1 - msec0) / 1000, (msec1 - msec0) % 1000);

    //docset->svd_U = (void*)new arma::mat();
    //docset->svd_s = (void*)new arma::vec();
    //docset->svd_V = (void*)new arma::mat();
    //arma::mat &U = *(arma::mat*)docset->svd_U;
    //arma::vec &s = *(arma::vec*)docset->svd_s;
    //arma::mat &V = *(arma::mat*)docset->svd_V;
    arma::mat U;
    arma::vec s;
    arma::mat V;

    //uint32_t rank = 10;
    //uint32_t iterations = 10;
    //double alpha = 0.01;
    //double lambda = 0.02;
    //mlpack::svd::RegularizedSVD<> svd(A, U, V, rank, iterations, alpha, lambda);

    mlpack::svd::QUIC_SVD svd(A, U, V, s, 0.03, 0.1);
    
    //const char *side = "both";
    ////const char *side = "left";
    ////const char *side = "right";
    ////const char *mode = "d";
    //const char *mode = "s";
    //arma::svd_econ(U, s, V, A, side, mode);

    uint32_t sv_cnt = s.size();
    printf("Singular Values (%d,%d)\n", sv_cnt, sv_cnt);
    for ( uint32_t n = 0 ; n < sv_cnt ; n++ ){
        if ( n < 20 ) {
            printf("%.6f ", s(n));
        } else if ( n == 20 ){
            printf("\n......\n");
        } else if ( n > sv_cnt - 20) {
            printf("%.6f ", s(n));
        }
    }
    printf("\nU(%d,%d) s(%d,%d) V(%d, %d)\n", U.n_rows, U.n_cols, s.n_rows, s.n_cols, V.n_rows, V.n_cols);


    GET_TIME_MILLIS(msec2);

    save_singular_value_armadillo(s, "test-s");

    // Reduce dimensions
    arma::mat S(s.n_rows, s.n_rows, arma::fill::zeros);
    for ( uint32_t i = 0 ; i < s.n_rows ; i++ ){
        if ( i >= dimensions )
            s(i) = 0.0;
        S(i,i) = s(i);
    }

    //printf("Building CorrMatrix...\n");
    //arma::mat C = U * S * V;

    GET_TIME_MILLIS(msec21);

    printf("Saving CorrMatrix...\n");
    //docset_save_corrmat_armadillo(docset, C, "test.corrmat");

    export_matrix_to_csv(U, "./U.csv");
    export_matrix_to_csv(V, "./V.csv");
    export_vector_to_csv(s, "./s.csv");

    GET_TIME_MILLIS(msec3);

    notice_log("svd do: %llu.%03llu sec.", (msec2 - msec1) / 1000, (msec2 - msec1) % 1000);
    notice_log("Build CorrMatrix do: %llu.%03llu sec.", (msec21 - msec2) / 1000, (msec21 - msec2) % 1000);
    notice_log("Save CorrMatrix do: %llu.%03llu sec.", (msec3 - msec21) / 1000, (msec3 - msec21) % 1000);

    notice_log("svd total: %llu.%03llu sec.", (msec3 - msec0) / 1000, (msec3 - msec0) % 1000);

}

