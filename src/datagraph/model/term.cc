/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      term.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:33:56
 *
 * Licence: MIT
 *
 */

#include "term.h"
#include <stdlib.h>

Term::Term(uint32_t id, uint8_t wordsize, const std::string& text)
    : m_id(id), m_wordsize(wordsize), m_text(text), m_count(0)
{
}

Term::~Term()
{
}

#ifdef __cplusplus
extern "C" {
#endif

term_t *term_new(uint32_t id, uint8_t wordsize, const char *text)
{
    term_t *term = (term_t*)malloc(sizeof(term_t));
    term->pTerm = new Term(id, wordsize, text);
    return term;
}

void term_free(term_t *term)
{
    Term *pTerm = (Term*)(term->pTerm);
    delete(pTerm);
    free(term);
}

term_t *term_attach(void *pTerm)
{
    term_t *term = (term_t*)malloc(sizeof(term_t));
    term->pTerm = pTerm;
    return term;
}

void term_detach(term_t *term)
{
    free(term);
}

uint32_t term_get_id(term_t *term)
{
    Term *pTerm = (Term*)(term->pTerm);
    return pTerm->m_id;
}

const char *term_get_text(term_t *term)
{
    Term *pTerm = (Term*)(term->pTerm);
    return pTerm->m_text.c_str();
}

uint32_t term_get_count(term_t *term)
{
    Term *pTerm = (Term*)(term->pTerm);
    return pTerm->m_count;
}

#ifdef __cplusplus
}
#endif

