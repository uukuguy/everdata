/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      docset.h
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-23 20:46:55
 *
 * Licence: MIT
 *
 */

#ifndef __DOCSET_H__
#define __DOCSET_H__

#include <stdint.h>
#include "lexicon.h"
#include "smat.h"

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>

class Corpus;
class Document;
class DocTerm;

class Docset {

    enum DOCSET_STATUS{
        DOCSET_STATUS_NONE,
        DOCSET_STATUS_DOCUMENTS_READY,
        DOCSET_STATUS_SEGMENTED
    };

public:
    Corpus *m_pCorpus;
    uint32_t m_id;
    const std::string m_name;

    typedef std::map<uint32_t, Document*> Documents;
    Documents m_docs;
    typedef std::vector<Document*> Vec_Documents;
    Vec_Documents m_vec_docs;

    Docset(Corpus *pCorpus, uint32_t id, const std::string& name);
    ~Docset();

    size_t size() const;
    void clear();

    Corpus* get_corpus() const;

    int load_from_files(const std::string& files_dir);
    int segment_files();
    int load_segment_files(const std::string& segment_rootdir);
    int save_segment_files(const std::string& segment_rootdir) const;

    Document *get_document_by_index(size_t idx) const;
    Document *get_document_by_id(uint32_t doc_id) const;

    enum DOCSET_STATUS m_status;

    size_t get_total_docs() const;
    size_t get_total_terms() const; 

    Lexicon& get_lexicon() {return m_lexicon;};
    const Lexicon& get_lexicon() const {return m_lexicon;};


    uint32_t calculate_nonzerovalues() const;
    void calculate_tfidf();
    smat_t* calculate_tfmatrix();

private:
    Lexicon m_lexicon;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct corpus_t corpus_t;
    typedef struct doc_t doc_t;

    typedef struct docset_t {
        void *pDocset;
    } docset_t;

    docset_t *docset_new(corpus_t *corpus, uint32_t id, const char *name);
    void docset_free(docset_t *docset);

    corpus_t *docset_get_corpus(docset_t *docset);

    docset_t *docset_attach(void *pDocset);
    void docset_detach(docset_t *docset);

    size_t docset_get_size(docset_t *docset);
    size_t docset_get_total_docs(docset_t *docset);
    size_t docset_get_total_terms(docset_t *docset);
    doc_t *docset_get_document_by_index(docset_t *docset, size_t idx);
    doc_t *docset_get_document_by_id(docset_t *docset, uint32_t doc_id);

    int docset_load_from_files(docset_t *docset, const char *files_dir);
    int docset_segment_files(docset_t *docset);
    int docset_load_segment_files(docset_t *docset, const char *segment_rootdir);
    int docset_save_segment_files(docset_t *docset, const char *segment_rootdir);

    lexicon_t *docset_get_lexicon(docset_t *docset);

    void docset_calculate_tfidf(docset_t *docset);
    smat_t *docset_calculate_tfmatrix(docset_t *docset);

#ifdef __cplusplus
}
#endif

#endif // __DOCSET_H__

