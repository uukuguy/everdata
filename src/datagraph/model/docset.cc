/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      docset.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified:
 * Created:   2015-01-23 20:49:19
 *
 * Licence: MIT
 *
 */

#include "docset.h"
#include "document.h"
#include "corpus.h"
#include "utils.h"
#include "logger.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>
#include <math.h>

docset_t *docset_new(corpus_t *corpus, uint32_t id, const char *name)
{
    docset_t *docset = (docset_t*)malloc(sizeof(docset_t));
    memset(docset, 0, sizeof(docset_t));

    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    Docset *pDocset = new Docset(pCorpus, id, name);
    docset->pDocset = pDocset;

    return docset;
}

void docset_free(docset_t *docset)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    delete pDocset;
    free(docset);
}

docset_t *docset_attach(void *pDocset)
{
    docset_t *docset = (docset_t*)malloc(sizeof(docset_t));
    memset(docset, 0, sizeof(docset_t));
    docset->pDocset = pDocset;
    return docset;
}

void docset_detach(docset_t *docset)
{
    free(docset);
}

corpus_t *docset_get_corpus(docset_t *docset)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    corpus_t *corpus = corpus_attach(pDocset->m_pCorpus);
    return corpus;
}

size_t docset_get_size(docset_t *docset)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    return pDocset->size();
}

size_t docset_get_total_docs(docset_t *docset)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    return pDocset->get_total_docs();
}

size_t docset_get_total_terms(docset_t *docset)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    return pDocset->get_total_terms();
}

doc_t *docset_get_document_by_index(docset_t *docset, size_t idx)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    Document *pDocument = pDocset->get_document_by_index(idx);
    doc_t *doc = doc_attach(pDocument);
    return doc;
}

doc_t *docset_get_document_by_id(docset_t *docset, uint32_t doc_id)
{
    Docset *pDocset = (Docset*)docset->pDocset;
    Document *pDocument = pDocset->get_document_by_id(doc_id);
    doc_t *doc = doc_attach(pDocument);
    return doc;
}

int docset_load_from_files(docset_t *docset, const char *files_dir)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    return pDocset->load_from_files(files_dir);
}

int docset_segment_files(docset_t *docset)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    return pDocset->segment_files();
}

int docset_load_segment_files(docset_t *docset, const char *segment_rootdir)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    return pDocset->load_segment_files(segment_rootdir);
}

int docset_save_segment_files(docset_t *docset, const char *segment_rootdir)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    return pDocset->save_segment_files(segment_rootdir);
}

lexicon_t *docset_get_lexicon(docset_t *docset)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    Lexicon *pLexicon = &pDocset->get_lexicon();
    lexicon_t *lexicon = lexicon_attach(pLexicon);
    return lexicon;
}

void docset_calculate_tfidf(docset_t *docset)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    pDocset->calculate_tfidf();
}

smat_t *docset_calculate_tfmatrix(docset_t *docset)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    return pDocset->calculate_tfmatrix();
}

// ================ class Docset ================

Docset::Docset(Corpus *pCorpus, uint32_t id, const std::string& name)
    :m_pCorpus(pCorpus), m_id(id), m_name(name), m_status(DOCSET_STATUS_NONE)
{
}

Docset::~Docset()
{
}

size_t Docset::size() const
{
    return m_docs.size();
}

void Docset::clear()
{
    for ( Docset::Documents::iterator it = m_docs.begin() ; it != m_docs.end() ; it++ ){
        Document *pDocument = it->second;
        delete pDocument;
    }
    m_docs.clear();
    m_vec_docs.clear();

    m_lexicon.clear();
}

Corpus* Docset::get_corpus() const
{
    return m_pCorpus;
}

Document* Docset::get_document_by_index(size_t idx) const
{
    return m_vec_docs[idx];
}

Document* Docset::get_document_by_id(uint32_t doc_id) const
{
    Docset::Documents::const_iterator it = m_docs.find(doc_id);
    if ( it != m_docs.end() )
        return it->second;
    else
        return NULL;
}

int Docset::load_from_files(const std::string& files_dir)
{
    int ret = 0;

    boost::filesystem::path bp(files_dir);
    boost::filesystem::directory_iterator end_iter;
    for ( boost::filesystem::directory_iterator file_iter(bp) ; file_iter != end_iter ; ++file_iter) {
        boost::filesystem::path filepath = boost::filesystem::system_complete(*file_iter);
        if ( filepath.extension() == ".textract" ) {
            std::string strFilePath = filepath.generic_string();

            const char *title = strFilePath.c_str();
            const char *doc_name = title;
            uint32_t namelen = strlen(doc_name);
            for ( uint32_t i = namelen - 1 ; i >= 1 ; i-- ){
                if ( doc_name[i] == '/' ){
                    title = &doc_name[i+1];
                    break;
                }
            }
            uint32_t doc_id = m_docs.size();
            Document *document = new Document(this, doc_id, title);
            document->m_filepath = strFilePath;

            m_docs.insert(Docset::Documents::value_type(doc_id, document));
            m_vec_docs.push_back(document);
        }
    }

    if ( ret == 0 ){
        m_status = DOCSET_STATUS_DOCUMENTS_READY;
    }

    return ret;
}

int Docset::segment_files()
{
    int ret = 0;

    for ( Docset::Documents::iterator it = m_docs.begin() ; it != m_docs.end() ; it++ ){
        Document *document = it->second;
        document->do_segment();
    }

    if ( ret == 0 ) {
        calculate_tfidf();
        m_status = DOCSET_STATUS_SEGMENTED;
    }

    return ret;
}

int Docset::load_segment_files(const std::string& segment_rootdir)
{
    int ret = 0;

    for ( Docset::Documents::iterator it = m_docs.begin() ; it != m_docs.end() ; it++ ){
        Document *document = it->second;
        if ( document->load_segment_file(segment_rootdir) != 0 ){
            error_log("load_segment_file from %s failed. Document title: %s", segment_rootdir.c_str(), document->m_title.c_str());
            return -1;
        }
    }

    if ( ret == 0 ) {
        calculate_tfidf();
        m_status = DOCSET_STATUS_SEGMENTED;
    }

    return ret;
}

int Docset::save_segment_files(const std::string& segment_rootdir) const
{
    int ret = 0;

    for ( Docset::Documents::const_iterator it = m_docs.begin() ; it != m_docs.end() ; it++ ){
        Document *document = it->second;
        if ( document->save_segment_file(segment_rootdir) != 0 ){
            error_log("save_segment_file to %s failed. Document title: %s", segment_rootdir.c_str(), document->m_title.c_str());
            return -1;
        }
    }

    return ret;
}


size_t Docset::get_total_docs() const
{
    return m_docs.size();
}

size_t Docset::get_total_terms() const
{
    return m_lexicon.size();
}

void Docset::calculate_tfidf()
{
    size_t total_docs = get_total_docs();
    for ( size_t i = 0 ; i < total_docs ; i++ ){
        Document *pDocument = get_document_by_index(i);
        uint32_t total_words = pDocument->get_total_words();
        for ( Document::VecTerms::iterator it = pDocument->m_vec_terms.begin() ; it != pDocument->m_vec_terms.end() ; it++ ){
            DocTerm *pDocterm = *it;
            Term *pTerm = pDocterm->m_pTerm;
            double tf = (double)pDocterm->m_count / (double)total_words;
            double idf = log((double)total_docs / (double)(pTerm->m_count + 1) );
            double tfidf = tf * idf;
            pDocterm->m_tf = tf;
            pDocterm->m_idf = idf;
            pDocterm->m_tfidf = tfidf;
        }
    }
}

/* ==================== calculate_nonzerovalues() ==================== */
uint32_t Docset::calculate_nonzerovalues() const
{
    uint32_t totalNonZeroValues = 0;

    Docset::Documents::const_iterator it = m_docs.begin();
    for ( ; it != m_docs.end() ; it++ ){
        Document *pDocument = it->second;
        uint32_t numNonZeroValues = pDocument->get_total_terms();
        totalNonZeroValues += numNonZeroValues;
    }

    return totalNonZeroValues;
}

smat_t* Docset::calculate_tfmatrix()
{
    uint32_t numRows = get_total_terms();
    uint32_t numCols = get_total_docs();
    uint32_t totalNonZeroValues = calculate_nonzerovalues();

    smat_t *tfm = smat_new(numRows, numCols, totalNonZeroValues);

    uint32_t v = 0;
    uint32_t col = 0;

    Docset::Documents::const_iterator it = m_docs.begin();
    for ( ; it != m_docs.end() ; it++ , col++){
        Document *pDocument = it->second;
        tfm->pointr[col] = v;
        for ( uint32_t row = 0 ; row < numRows ; row++ ){
            Term *pTerm = m_lexicon.get_term_by_index(row);
            Document::Terms::iterator it_term = pDocument->m_terms.find(pTerm);
            if ( it_term != pDocument->m_terms.end()) {
                DocTerm *pDocterm = it_term->second;
                double tfidf = pDocterm->m_tfidf;
                *(tfm->rowind + v) = row;
                *(tfm->values + v) = tfidf;
                v++;
            }
        }
    }
    tfm->pointr[numCols] = totalNonZeroValues;

    return tfm;

}
