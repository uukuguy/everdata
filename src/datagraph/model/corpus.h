/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      corpus.h
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified: 
 * Created:   2015-01-10 15:21:55
 *
 * Licence: MIT
 *
 */

#ifndef __TE_CORPUS_H__
#define __TE_CORPUS_H__

#include <stdint.h>

#ifdef __cplusplus

#include <string>
#include <map>
#include <vector>

class Document;
class Docset;

typedef std::vector<uint32_t> DocumentVector;
typedef std::vector<uint32_t> TermVector;

class Corpus {

    enum CORPUS_STATUS{
        CORPUS_STATUS_NONE,
        CORPUS_STATUS_DOCUMENTS_READY,
        CORPUS_STATUS_SEGMENTED
    };

public:
    Corpus(const std::string& name, const std::string& rootdir);
    virtual ~Corpus();
    void clear();

    int add_files(const std::string& files_rootdir, const std::string& file_extension);
    int load_from_files();
    int segment_files();
    int load_segment_files();
    int save_segment_files() const;

    std::string m_name;
    std::string m_rootdir;
    enum CORPUS_STATUS m_status;

    Docset *m_pRootDocset;
    //typedef std::map<uint32_t, Document*> Documents;
    //Documents m_docs;

};

#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct corpus_t {
        void *pCorpus;
    } corpus_t;

    corpus_t *corpus_new(const char *name, const char *rootdir);
    void corpus_free(corpus_t *corpus);

    corpus_t *corpus_attach(void *pCorpus);
    void corpus_detach(corpus_t *corpus);

    int corpus_load_from_files(corpus_t *corpus);
    int corpus_segment_files(corpus_t *corpus);
    int corpus_load_segment_files(corpus_t *corpus);
    int corpus_save_segment_files(corpus_t *corpus);

#ifdef __cplusplus
}
#endif

#endif /* __TE_CORPUS_H__ */

