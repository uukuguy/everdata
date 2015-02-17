/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      corpus.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified:
 * Created:   2015-01-10 15:31:13
 *
 * Licence: MIT
 *
 */

#include "corpus.h"
#include "docset.h"
#include "document.h"
#include "logger.h"
#include "filesystem.h"
#include <boost/filesystem.hpp>
#include <stdlib.h>

corpus_t *corpus_new(const char *name, const char *rootdir)
{
    corpus_t *corpus = (corpus_t*)malloc(sizeof(corpus_t));
    memset(corpus, 0, sizeof(corpus_t));

    Corpus *pCorpus = new Corpus(name, rootdir);
    corpus->pCorpus = pCorpus;

    return corpus;
}

void corpus_free(corpus_t *corpus)
{
    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    delete pCorpus;
    free(corpus);
}

corpus_t *corpus_attach(void *pCorpus)
{
    corpus_t *corpus = (corpus_t*)malloc(sizeof(corpus_t));
    memset(corpus, 0, sizeof(corpus_t));

    corpus->pCorpus = pCorpus;
    return corpus;
}

void corpus_detach(corpus_t *corpus)
{
    free(corpus);
}

int corpus_load_from_files(corpus_t *corpus)
{
    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    return pCorpus->load_from_files();
}

int corpus_segment_files(corpus_t *corpus)
{
    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    return pCorpus->segment_files();
}

int corpus_load_segment_files(corpus_t *corpus)
{
    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    return pCorpus->load_segment_files();
}

int corpus_save_segment_files(corpus_t *corpus)
{
    Corpus *pCorpus = (Corpus*)(corpus->pCorpus);
    return pCorpus->save_segment_files();
}

// ================ class Corpus ================

Corpus::Corpus(const std::string& name, const std::string& rootdir)
    : m_name(name), m_rootdir(rootdir), m_status(CORPUS_STATUS_NONE)
{
    std::string corpus_dir = rootdir + "/" + name;
    std::string puretext_dir = corpus_dir + "/" + "puretext";
    std::string segment_dir = corpus_dir + "/" + "segment";

    mkdir_if_not_exist(rootdir.c_str());
    mkdir_if_not_exist(corpus_dir.c_str());
    mkdir_if_not_exist(puretext_dir.c_str());
    mkdir_if_not_exist(segment_dir.c_str());

    m_pRootDocset = new Docset(this, 0, "Default");
}

Corpus::~Corpus()
{
    clear();
    delete m_pRootDocset;
}

void Corpus::clear()
{
    m_pRootDocset->clear();
}

int Corpus::add_files(const std::string& files_rootdir, const std::string& file_extension)
{
    int ret = 0;
    //boost::filesystem::path bp(files_rootdir);
    //boost::filesystem::directory_iterator end_iter;
    //for ( boost::filesystem::directory_iterator file_iter(bp) ; file_iter != end_iter ; ++file_iter) {
        //boost::filesystem::path filepath = boost::filesystem::system_complete(*file_iter);
        //if ( filepath.extension() == file_extension ) {
            //std::string strFilePath = filepath.generic_string();
        //}
    //}

    return ret;
}

int Corpus::load_from_files()
{
    int ret = 0;

    std::string puretext_rootdir = m_rootdir + "/" + m_name + "/puretext";
    ret = m_pRootDocset->load_from_files(puretext_rootdir);

    if ( ret == 0 ){
        m_status = CORPUS_STATUS_DOCUMENTS_READY;
    }

    return ret;
}

int Corpus::segment_files()
{
    int ret = m_pRootDocset->segment_files();

    if ( ret == 0 ) {
        m_status = CORPUS_STATUS_SEGMENTED;
    }

    return ret;
}

int Corpus::load_segment_files()
{
    std::string segment_rootdir = m_rootdir + "/" + m_name + "/segment";

    int ret = m_pRootDocset->load_segment_files(segment_rootdir);

    if ( ret == 0 ) {
        m_status = CORPUS_STATUS_SEGMENTED;
    }

    return ret;
}

int Corpus::save_segment_files() const
{
    std::string segment_rootdir = m_rootdir + "/" + m_name + "/segment";

    int ret = m_pRootDocset->save_segment_files(segment_rootdir);

    return ret;
}
