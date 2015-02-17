/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      lexicon.h
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:38:56
 *
 * Licence: MIT
 *
 */

#ifndef __TE_LEXICON_H__
#define __TE_LEXICON_H__

#include <stdint.h>
#include <sys/types.h>
#include "term.h"

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>

class Term;

class Lexicon {
public:
    typedef std::map<std::string, Term*> TermsByText;
    typedef TermsByText Terms;
    TermsByText terms_by_text;

    typedef std::map<uint32_t, Term*> TermsById;
    TermsById terms_by_id;

    typedef std::vector<Term*> TermsByIndex;
    TermsByIndex terms_by_index;

public:
    std::string m_name;

public:
    Lexicon();
    Lexicon(const std::string& lexicon_name);
    virtual ~Lexicon();

    size_t size() const;
    void clear();

    Term *add_term(const std::string& term);
    Term *get_term_by_id(uint32_t term_id) const;
    Term *get_term_by_text(const std::string &term_text) const;
    Term *get_term_by_index(uint32_t idx) const;

    int write_to_file(const std::string &filename) const;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct term_t term_t;

    typedef struct lexicon_t {
        void *pLexicon;
    } lexicon_t;

    lexicon_t *lexicon_new(const char *lexicon_name);
    void lexicon_free(lexicon_t *lexicon);

    lexicon_t *lexicon_attach(void *pLexicon);
    void lexicon_detach(lexicon_t *lexicon);

    size_t lexicon_get_size(lexicon_t *lexicon);
    term_t *lexicon_get_term_by_index(lexicon_t *lexicon, size_t idx);
    term_t *lexicon_get_term_by_id(lexicon_t *lexicon, uint32_t term_id);
    term_t *lexicon_get_term_by_text(lexicon_t *lexicon, const char *term_text);

#ifdef __cplusplus
}
#endif

#endif /* __TE_LEXICON_H__ */

