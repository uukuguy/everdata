/**
 * @file   svd_octave.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 18:10:04
 * 
 * @brief  
 * 
 * 
 */

#include "docset.h"
#include "document.h"
#include "term.h"
#include "dmat.h"
#include "smat.h"
#include "logger.h"
#include "lexicon.h"
#include "svd.h"

#include <unistd.h>
#include <fcntl.h>
#include <oct.h>

//typedef struct svd_data_t{
    //Document *pDocument/
    //Matrix &A;
//} svd_data_t;

//void lexicon_term_loop_octavve(term_t *term, void *user_data)
//{
    //svd_data_t *svd_data = (svd_data_t*)user_data;
    //doc_t *doc = svd_data->doc;
    //Matrix &A = svd_data->A;

    //if ( doc_has_term(doc, term) == 1 ) {
        //uint32_t row = term_get_id(term);
        //uint32_t col = doc_get_id(doc);
        ////uint32_t row = doc_get_id(doc);
        ////uint32_t col = term_get_id(term);

        //double term_tfidf = doc_get_term_tfidf(doc, term);
        //A(row, col) = term_tfidf;
    //} 
//}

typedef struct lsv_t {
    Matrix &U;
    int hfile;
    uint32_t dimensions;
} lsv_t;

void save_terms_title(term_t *term, void *user_data)
{
    int *pHfile = (int*)user_data;
    int hfile = *pHfile;

    char buf[1024];
    sprintf(buf, ", %s", term_get_text(term));
    size_t nBytes __attribute__((unused)) = write(hfile, buf, strlen(buf));
}

//void save_left_singular_value(term_t *term, void *user_data)
//{
    //lsv_t *lsv = (lsv_t*)user_data;
    //Matrix &U = lsv->U;
    //int hfile = lsv->hfile;
    //uint32_t dimensions = lsv->dimensions;

    //char buf[1024];
    //sprintf(buf, "%s", term_get_text(term));
    //size_t nBytes __attribute__((unused)) = write(hfile, buf, strlen(buf)); 

    //uint32_t row = term_get_id(term);
    //for ( uint32_t col = 0 ; col < dimensions ; col++ ){
        //double value = U(row, col);
        //sprintf(buf, ", %.6f", value);
        //size_t nBytes2 __attribute__((unused)) = write(hfile, buf, strlen(buf));
    //}
    //size_t nBytes1 __attribute__((unused)) = write(hfile, "\n", 1);
    
//}

//void docset_save_svd_result(docset_t *docset, Matrix &U)
//{
    //Docset *pDocset = (Docset*)(docset->pDocset);

    //char filename[NAME_MAX];
    //sprintf(filename, "%s.corrmat", pDocset->m_name.c_str());

    ////uint32_t numTerms = docset_get_terms_size(docset);
    //int hfile = open(filename, O_CREAT | O_WRONLY, 0640);

    ////write(hfile, "    ", 4);
    ////lexicon_apply_each_term(docset->lexicon, save_terms_title, &hfile);
    ////write(hfile, "\n", 1);

    //uint32_t numDocs = docset_get_docs_size(docset);
    //lsv_t lsv = {
        //.U = U,
        //.hfile = hfile,
        //.dimensions = numDocs,
    //};
    //lexicon_apply_each_term(docset->lexicon, save_left_singular_value, &lsv);

    //close(hfile);

//}

#include <sstream>
#include <vector>

double vector_angle_cosine_octave(ColumnVector &v1, ColumnVector &v2, uint32_t size)
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

int docset_query_octave(docset_t *docset, const char *query_string, uint32_t dimensions)
{
    if ( query_string == NULL || strlen(query_string) == 0 )
        return -1;

    Docset *pDocset = (Docset*)(docset->pDocset);
    const Lexicon &lexicon = pDocset->get_lexicon();

    uint32_t numTerms = pDocset->get_total_terms();
    uint32_t numDocs = pDocset->get_total_docs();
    ColumnVector q_vct(numTerms, 0.0);

    std::istringstream query_stream(query_string);
    std::string word;
	while (query_stream >> word) {
       Term *pTerm = lexicon.get_term_by_text(word); 
        if ( pTerm != NULL ){
            uint32_t term_id = pTerm->m_id;
            q_vct(term_id) = q_vct(term_id) + 1;
        }
    };

    //DiagMatrix &s = *(DiagMatrix*)docset->svd_s;
    //Matrix &U = *(Matrix*)docset->svd_U;
    //Matrix &V = *(Matrix*)docset->svd_V;
    DiagMatrix s;
    Matrix U;
    Matrix V;

    ColumnVector d_vct(dimensions, 0.0);
	// Dq = Xq' T S^-1
	for (uint32_t i = 0; i < dimensions; i++) {
		double sum = 0;
		for (uint32_t j = 0; j < numTerms; j++) {
            sum += q_vct(j) * U(j,i);
		}
        d_vct(i) = sum * ( 1 / s(i, i));
	}

	//compare each document with Dq
	for ( uint32_t n = 0 ; n < numDocs ; n++ ) {
        ColumnVector t_vct(dimensions, 0.0);
		// fill temp document vector
		for ( uint32_t m = 0 ; m < dimensions ; m++) {
            t_vct(m) = V(n,m) * s(m, m);
		}
        AddCorrelation(n, vector_angle_cosine_octave(d_vct, t_vct, dimensions));	
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

dmat_t *docset_save_matrix_octave(Matrix& C, const char *filename)
{
    uint32_t numRows = C.rows();
    uint32_t numCols = C.cols();

    dmat_t *corrmat = dmat_new(numRows, numCols);
    double *values = dmat_get_values(corrmat);
    for ( uint32_t row = 0 ; row < numRows ; row++ ){
        for ( uint32_t col = 0 ; col < numCols ; col++ ){
            double v = C(row, col);
            *values = v;
            values++;
        }
    }

    return corrmat;
}

#include <fstream>
#include <iomanip>
int save_singular_value_octave(DiagMatrix &s, const char *filename)
{
    std::fstream out(filename, std::ios::out);
    out << std::setprecision(6);
    for ( int i = 0 ; i < s.rows() ; i++ ){
        out << s(i, i) << std::endl;
    }
    return 0;
}

/* ==================== docset_do_svd_octave() ==================== */
void docset_do_svd_octave(docset_t *docset, uint32_t dimensions)
{
    GET_TIME_MILLIS(msec0);

    Docset *pDocset = (Docset*)(docset->pDocset);

    uint32_t numRows = pDocset->get_total_terms();
    uint32_t numCols = pDocset->get_total_docs();
    //uint32_t totalNonZeroValues = pDocset->calculate_nonzerovalues();

    smat_t *tfm = pDocset->calculate_tfmatrix();

    Matrix A(numRows, numCols, 0.0);

    SMAT_LOOP_BEGIN(tfm, numRows, numCols);
    A(row, col) = value;
    SMAT_LOOP_END();
    //uint32_t v = 0;
    //for ( uint32_t col = 0 ; col < numCols ; col++ ){
        ////uint32_t numNonZeroValues = tfm->pointr[col + 1] - tfm->pointr[col];
        //for ( ; v < tfm->pointr[col + 1]; v++) {
            //uint32_t row = tfm->rowind[v];
            //double value = tfm->values[v];
            //A(row, col) = value;
        //}

    //}

    smat_free(tfm);

    GET_TIME_MILLIS(msec1);
    notice_log("svd prepare: %llu.%03llu sec.", (msec1 - msec0) / 1000, (msec1 - msec0) % 1000);

    //SVD::type type_computed = SVD::std;
    SVD::type type_computed = SVD::economy;
    //SVD::type type_computed = SVD::sigma_only;
    SVD::driver svd_driver = SVD::GESVD;
    //SVD::driver svd_driver = SVD::GESDD;
    
    SVD svd(A, type_computed, svd_driver);

    DiagMatrix s;
    Matrix U; 
    Matrix V;

    s = svd.singular_values();
    U = svd.left_singular_matrix();
    V = svd.right_singular_matrix();

    printf("Singular Values (%d,%d)\n", s.rows(), s.cols());
    for ( int n = 0 ; n < s.rows() ; n++ ){
        if ( n < 20 ) {
            printf("%.6f ", s(n, n));
        } else if ( n == 20 ){
            printf("\n......\n");
        } else if ( n > s.rows() - 20) {
            printf("%.6f ", s(n, n));
        }
    }
    printf("\nU(%d,%d) s(%d,%d) V(%d, %d)\n", U.rows(), U.cols(), s.rows(), s.cols(), V.rows(), V.cols());

    GET_TIME_MILLIS(msec2);

    save_singular_value_octave(s, "test-s");

    // Reduce dimensions
    for ( size_t i = dimensions ; i < (size_t)s.rows() ; i++ ){
        s(i, i) = 0.0;
    }
    //docset->dimensions = dimensions <= (uint32_t)s.rows() ? dimensions : s.rows();

    printf("Building CorrMatrix...\n");
    //Matrix C = U * s * V;
    
    GET_TIME_MILLIS(msec21);

    printf("Saving CorrMatrix...\n");
    //docset_save_matrix_octave(docset, C, "test.corrmat");

    GET_TIME_MILLIS(msec3);
    notice_log("svd do: %llu.%03llu sec.", (msec2 - msec1) / 1000, (msec2 - msec1) % 1000);
    notice_log("Build CorrMatrix do: %llu.%03llu sec.", (msec21 - msec2) / 1000, (msec21 - msec2) % 1000);
    notice_log("Save CorrMatrix do: %llu.%03llu sec.", (msec3 - msec21) / 1000, (msec3 - msec21) % 1000);

    notice_log("svd total: %llu.%03llu sec.", (msec3 - msec0) / 1000, (msec3 - msec0) % 1000);
}

