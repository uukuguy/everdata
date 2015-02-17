/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      term.h
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-11 21:32:53
 *
 * Licence: MIT
 *
 */

#ifndef __TE_TERM_H__
#define __TE_TERM_H__

#include <stdint.h>

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>

class Term {

public:
    Term(uint32_t id, uint8_t wordsize, const std::string& text);
    ~Term();

    uint32_t m_id;
    uint8_t m_wordsize;
    std::string m_text;
    uint32_t m_count;
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct term_t {
        void *pTerm;
    } term_t;

    term_t *term_new(uint32_t id, uint8_t wordsize, const char *text);
    void term_free(term_t *term);

    term_t *term_attach(void *pTerm);
    void term_detach(term_t *term);

    uint32_t term_get_id(term_t *term);
    const char *term_get_text(term_t *term);
    uint32_t term_get_count(term_t *term);

#ifdef __cplusplus
}
#endif

#endif /* __TE_TERM_H__ */

