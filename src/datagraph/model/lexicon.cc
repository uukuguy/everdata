/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      lexicon.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:44:44
 *
 * Licence: MIT
 *
 */

#include "lexicon.h"
#include "term.h"
#include <stdlib.h>

lexicon_t *lexicon_new(const char *lexicon_name)
{
    lexicon_t *lexicon = (lexicon_t*)malloc(sizeof(lexicon_t));
    memset(lexicon, 0, sizeof(lexicon_t));

    Lexicon *pLexicon = new Lexicon(lexicon_name);
    lexicon->pLexicon = pLexicon;

    return lexicon;
}

void lexicon_free(lexicon_t *lexicon)
{
    Lexicon *pLexicon = (Lexicon*)lexicon->pLexicon;
    delete pLexicon;
    free(lexicon);
}

lexicon_t *lexicon_attach(void *pLexicon)
{
    lexicon_t *lexicon = (lexicon_t*)malloc(sizeof(lexicon_t));
    memset(lexicon, 0, sizeof(lexicon_t));
    lexicon->pLexicon = pLexicon;
    return lexicon;
}

void lexicon_detach(lexicon_t *lexicon)
{
    free(lexicon);
}

size_t lexicon_get_size(lexicon_t *lexicon)
{
    Lexicon *pLexicon = (Lexicon*)lexicon->pLexicon;
    return pLexicon->size();
}

term_t *lexicon_get_term_by_index(lexicon_t *lexicon, size_t idx)
{
    Lexicon *pLexicon = (Lexicon*)lexicon->pLexicon;
    Term *pTerm = pLexicon->get_term_by_index(idx);

    term_t *term = term_attach(pTerm);
    return term;
}

term_t *lexicon_get_term_by_id(lexicon_t *lexicon, uint32_t term_id)
{
    Lexicon *pLexicon = (Lexicon*)lexicon->pLexicon;
    Term *pTerm = pLexicon->get_term_by_id(term_id);

    term_t *term = term_attach(pTerm);
    return term;
}

term_t *lexicon_get_term_by_text(lexicon_t *lexicon, const char *term_text)
{
    Lexicon *pLexicon = (Lexicon*)lexicon->pLexicon;
    Term *pTerm = pLexicon->get_term_by_text(term_text);

    term_t *term = term_attach(pTerm);
    return term;
}

// ================ class Lexicon ================

Lexicon::Lexicon()
{
}

Lexicon::Lexicon(const std::string& name)
    : m_name(name)
{
}

Lexicon::~Lexicon()
{
}

/* ==================== Lexicon::clear() ==================== */ 
void Lexicon::clear()
{
    this->terms_by_text.clear();
    this->terms_by_id.clear();
    for ( Lexicon::TermsByIndex::iterator it = this->terms_by_index.begin() ; 
            it != this->terms_by_index.end() ; it++ ){
        Term *term = *it;
        delete term;
    }
    this->terms_by_index.clear();
}

/* ==================== Lexicon::size() ==================== */ 
size_t Lexicon::size() const
{
    return this->terms_by_index.size();
}

/* ==================== Lexicon::add_term() ==================== */ 
Term *Lexicon::add_term(const std::string& term_text)
{
    Term *term = NULL;

    Lexicon::Terms::iterator it = this->terms_by_text.find(term_text);
    if ( it != this->terms_by_text.end() ){
        term = (*it).second;
    } else {
        uint32_t term_id = this->terms_by_index.size();
        uint8_t wordsize = term_text.length() / 3;
        term = new Term(term_id, wordsize, term_text.c_str());
        this->terms_by_text.insert(Lexicon::TermsByText::value_type(term_text, term));

        this->terms_by_id[term_id] = term;
        this->terms_by_index.push_back(term);
    }

    term->m_count++;

    return term;
}

/* ==================== Lexicon::get_term_by_id() ==================== */ 
Term *Lexicon::get_term_by_id(uint32_t term_id) const
{
    Lexicon::TermsById::const_iterator it = this->terms_by_id.find(term_id);
    if ( it != this->terms_by_id.end() ){
        return (*it).second;
    } else {
        return NULL;
    }
}

/* ==================== Lexicon::get_term_by_text() ==================== */ 
Term *Lexicon::get_term_by_text(const std::string &term_text) const
{
    Lexicon::Terms::const_iterator it = this->terms_by_text.find(term_text);
    if ( it != this->terms_by_text.end() ){
        return (*it).second;
    } else {
        return NULL;
    }
}

/* ==================== Lexicon::get_term_by_index() ==================== */ 
Term *Lexicon::get_term_by_index(uint32_t idx) const
{
    return this->terms_by_index[idx];
}

#include <fstream>
int Lexicon::write_to_file(const std::string &filename) const
{
    std::fstream out(filename.c_str(), std::ios::out);

    Lexicon::TermsById::const_iterator it = this->terms_by_id.begin();
    for ( ; it != this->terms_by_id.end() ; it++ ){
        Term *term = it->second;
        out << term->m_text << std::endl;
    }

    return 0;
}

