/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      document.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:31:20
 *
 * Licence: MIT
 *
 */

#include "common.h"
#include "util.h"
#include "logger.h"
#include "document.h"
#include "docset.h"
#include "segmenter.h"
#include "lexicon.h"
#include "corpus.h"
#include <iostream>
#include <stdlib.h>

doc_t *doc_new(uint32_t id, const char *title)
{
    doc_t *doc = (doc_t*)malloc(sizeof(doc_t));
    memset(doc, 0, sizeof(doc_t));

    Document *pDocument = new Document(NULL, id, title);
    doc->pDocument = pDocument;

    return doc;
}

void doc_free(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    delete pDocument;
    free(doc);
}

doc_t *doc_attach(void *pDocument)
{
    doc_t *doc = (doc_t*)malloc(sizeof(doc_t));
    memset(doc, 0, sizeof(doc_t));
    doc->pDocument = pDocument;
    return doc;
}

void doc_detach(doc_t *doc)
{
    free(doc);
}

void doc_clear(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    pDocument->clear();
}

const char *doc_get_title(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->m_title.c_str();
}

uint32_t doc_get_id(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->m_id;
}

size_t doc_get_total_words(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->get_total_words();
}

size_t doc_get_total_terms(doc_t *doc)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->get_total_terms();
}

uint32_t doc_get_word_termid_by_index(doc_t *doc, size_t idx)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->get_word_termid_by_index(idx);
}

uint32_t doc_get_termid_by_index(doc_t *doc, size_t idx)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->get_termid_by_index(idx);
}

uint32_t doc_get_term_count_by_index(doc_t *doc, size_t idx)
{
    Document *pDocument = (Document*)doc->pDocument;
    return pDocument->get_term_count_by_index(idx);
}

// ================ class Document ================

Document::Document(Docset *pDocset, uint32_t id, const std::string& title)
    : m_id(id), m_title(title), m_pDocset(pDocset)
{
}

Document::~Document()
{
    clear();
}

void Document::clear()
{
    for ( Document::Terms::iterator it = m_terms.begin() ; it != m_terms.end() ; it++ ){
        DocTerm *pDocTerm = it->second;
        delete pDocTerm;
    }

    m_words.clear();
    m_vec_terms.clear();
    m_terms.clear();
}

Docset* Document::get_docset() const
{
    return m_pDocset;
}

size_t Document::get_total_words() const
{
    return m_words.size();
}

size_t Document::get_total_terms() const
{
    return m_terms.size();
}

/* ==================== peek_term() ==================== */ 
void Document::peek_term(const char *buf)
{
    Docset *pDocset = get_docset();
    Lexicon *pLexicon = &pDocset->get_lexicon();
    Term *term = pLexicon->get_term_by_text(buf);
    if ( term == NULL ){
        term = pLexicon->add_term(buf);
    }
    assert(term != NULL);

    DocTerm *docterm = NULL;
    Document::Terms::iterator it = m_terms.find(term);
    if ( it != m_terms.end() ){
        docterm = it->second;
        docterm->m_count++;
    } else {
        docterm = new DocTerm(term);
        m_terms.insert(Document::Terms::value_type(term, docterm));
        m_vec_terms.push_back(docterm);
    }
    assert(docterm != NULL);
    m_words.push_back(docterm);


}

/* ==================== segment_peek_token() ==================== */ 
void Document::segment_peek_token(const char *token, uint32_t token_len, void *user_data)
{
    Document *document = (Document*)user_data;

    if ( token_len >= 6 && token_len < 64){
        char buf[65];
        memcpy(buf, token, token_len);
        buf[token_len] = '\0';
        //printf("%s(%d) ", buf, token_len);

        document->peek_term(buf);
    }

}

/* ==================== doc_do_segment() ==================== */ 
// Modify m_words and m_terms.
int Document::do_segment()
{
    std::cout << "Document::do_segment() title: " << m_title << std::endl;

    int rc = 0;

    GET_TIME_MILLIS(msec0);

    const char * filename = m_filepath.c_str();
    int file_handle = open(filename, O_RDONLY, 0640);
    if ( file_handle == -1 ) {
        error_log("Open file %s failue.", filename);
        return -1;
    }
    size_t file_size = lseek(file_handle, 0, SEEK_END);
    lseek(file_handle, 0, SEEK_SET);
    char *buf = (char*)malloc(file_size);
    size_t nBytes __attribute__((unused)) = read(file_handle, buf, file_size);
    close(file_handle);

    segmenter_t *segmenter = segmenter_new("mmseg", "./share/mmseg/data");
    segmenter_segment_buffer(segmenter, buf, file_size, Document::segment_peek_token, (void*)this);
    segmenter_free(segmenter);

    free(buf);

    GET_TIME_MILLIS(msec1);


    printf("Term Split took: %zu ms.\n", (size_t)(msec1 - msec0));
    printf("Total words: %zu\n", m_words.size());
    printf("Keep terms: %zu\n\n", m_terms.size());

    return rc;
}

std::string Document::get_segment_filename(const std::string& segment_rootdir)
{
    char seg_filename[NAME_MAX * 10];
    const char *title = m_title.c_str();
    sprintf(seg_filename, "%s/%s.segment", segment_rootdir.c_str(), title);
    return seg_filename;
}

#include <boost/tokenizer.hpp>
#include <string>
/* ==================== load_segment_file() ==================== */ 
int Document::load_segment_file(const std::string& segment_rootdir)
{
    typedef boost::tokenizer<boost::char_separator<char> > Token;
    boost::char_separator<char> sep(" \t\n");

    std::string seg_filename = get_segment_filename(segment_rootdir);

    int hfile = open(seg_filename.c_str(), O_RDONLY, 0640);
    if ( hfile >= 0 ){
        size_t file_size = lseek(hfile, 0, SEEK_END);
        lseek(hfile, 0, SEEK_SET);
        char *buf = (char*)malloc(file_size+1);
        size_t nBytes __attribute__((unused)) = read(hfile, buf, file_size);
        buf[file_size] = '\0';
        close(hfile);

        std::string strFile(buf);
        Token tok(strFile, sep);
        for ( Token::iterator it = tok.begin() ; it != tok.end() ; it++ ){
            peek_term(it->c_str());
        }

        free(buf);
        return 0;
    }
    return -1;
}

int Document::save_segment_file(const std::string& segment_rootdir)
{
    std::string seg_filename = get_segment_filename(segment_rootdir);

    int hfile = open(seg_filename.c_str(), O_CREAT | O_WRONLY, 0640);
    for ( Document::Words::const_iterator it = m_words.begin() ; it != m_words.end() ; it++ ){
        DocTerm *docterm = *it;
        const std::string& term_text = docterm->m_pTerm->m_text;
        size_t nBytes __attribute__((unused)) = write(hfile, term_text.c_str(), term_text.length());
    }
    close(hfile);

    return 0;
}

uint32_t Document::get_word_termid_by_index(size_t idx) const
{
    DocTerm* pDocterm = m_words[idx];
    return pDocterm->m_pTerm->m_id;
}

uint32_t Document::get_termid_by_index(size_t idx) const
{
    DocTerm *pDocterm = m_vec_terms[idx];
    return pDocterm->m_pTerm->m_id;
}

uint32_t Document::get_term_count_by_index(size_t idx) const
{
    DocTerm *pDocterm = m_vec_terms[idx];
    return pDocterm->m_count;
}

