/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      document.h
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:30:24
 *
 * Licence: MIT
 *
 */

#ifndef __TE_DOCUMENT_H__
#define __TE_DOCUMENT_H__

#include <stdint.h>
#include "term.h"

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>
#include <set>


class Lexicon;
class Docset;

class DocTerm {
public:
    DocTerm(Term *pTerm) : m_pTerm(pTerm), m_count(1), m_tf(0.0), m_idf(0.0), m_tfidf(0.0) {};
    ~DocTerm(){};

    Term *m_pTerm;
    uint32_t m_count;
    double m_tf;
    double m_idf;
    double m_tfidf;
};

class Document {

public:
    Document(Docset *pDocset, uint32_t id, const std::string& title);
    virtual ~Document();
    void clear();

    int do_segment();
    int save_segment_file(const std::string& segment_rootdir);
    int load_segment_file(const std::string& segment_rootdir);

    Docset* get_docset() const;
    size_t get_total_words() const;
    size_t get_total_terms() const;

    uint32_t m_id;
    std::string m_title;
    std::string m_filepath;
    std::string m_text;

    uint32_t get_word_termid_by_index(size_t idx) const;
    uint32_t get_termid_by_index(size_t idx) const;
    uint32_t get_term_count_by_index(size_t idx) const;
    typedef std::vector<DocTerm*> Words;
    Words m_words;

    typedef std::vector<DocTerm*> VecTerms;
    VecTerms m_vec_terms;
    typedef std::map<Term*, DocTerm*> Terms;
    Terms m_terms;

    static void segment_peek_token(const char *token, uint32_t token_len, void *user_data);

private:
    Docset *m_pDocset;
    std::string get_segment_filename(const std::string& segment_rootdir);
    void peek_term(const char *buf);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct doc_t {
        void *pDocument;
    } doc_t;

    doc_t *doc_new(uint32_t id, const char *title);
    void doc_free(doc_t *doc);
    void doc_clear(doc_t *doc);
    doc_t *doc_attach(void *pDocument);
    void doc_detach(doc_t *doc);
    const char *doc_get_title(doc_t *doc);
    uint32_t doc_get_id(doc_t *doc);
    size_t doc_get_total_words(doc_t *doc);
    size_t doc_get_total_terms(doc_t *doc);
    uint32_t doc_get_word_termid_by_index(doc_t *doc, size_t idx);
    uint32_t doc_get_termid_by_index(doc_t *doc, size_t idx);
    uint32_t doc_get_term_count_by_index(doc_t *doc, size_t idx);

#ifdef __cplusplus
}
#endif

#endif /* __TE_DOCUMENT_H__ */

