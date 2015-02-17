/*
 * Copyright (c) 2015 lastz.org
 *
 * File:      main.cc
 * Project:   datagraph
 * Author:    Jason Su <uukuguy@gmail.com>
 *
 * Modified:
 * Created:   2015-01-10 15:15:16
 *
 * Licence: MIT
 *
 */

#include <stdio.h>
#include <iostream>
#include <string>
#include <getopt.h>
#include "corpus.h"
#include "logger.h"
#include "filesystem.h"
#include "ldac.h"
#include "docset.h"

const char *program_name = "datagraph";

static struct option const long_options[] = {
    {"corpus_name", required_argument, NULL, 'n'},
    {"corpus_rootdir", required_argument, NULL, 'r'},
    {"test", no_argument, NULL, 'z'},
	{"verbose", no_argument, NULL, 'v'},
	{"trace", no_argument, NULL, 't'},
	{"help", no_argument, NULL, 'h'},

	{NULL, 0, NULL, 0},
};
static const char *short_options = "n:r:zvth";

/* ==================== usage() ==================== */
static void usage(int status)
{
    if ( status )
        printf("Try `%s --help' for more information.\n", program_name);
    else {
        printf("Usage: %s [OPTION] [PATH]\n", program_name);
        printf("Data Graph\n\
                -n, --corpus_name       Corpus name.\n\
                -r, --corpus_rootdir    Corpus root directory.\n\
                -z, --test              Test.\n\
                -v, --verbose           print debug messages\n\
                -t, --trace             print trace messages\n\
                -h, --help              display this help and exit\n\
                \n");
    }
    exit(status);
}

extern void docset_do_svd_svdlibc(docset_t *docset, uint32_t dimensions);
extern void docset_do_svd_armadillo(docset_t *docset, uint32_t dimensions);
//extern void docset_do_svd_octave(docset_t *docset, uint32_t dimensions);
extern void docset_do_svd_eigen(docset_t *docset, uint32_t dimensions);
extern void docset_do_svd_redsvd(docset_t *docset, uint32_t dimensions);

int main(int argc, char *argv[])
{
    int log_level = LOG_INFO;

    std::string corpus_name = "default";
    std::string corpus_rootdir = "./corpus";
    int is_test = 0;

    /* -------- Init logger -------- */
    char root_dir[NAME_MAX];
    get_instance_parent_full_path(root_dir, NAME_MAX);

    char log_dir[NAME_MAX];
    sprintf(log_dir, "%s/log", root_dir);
    mkdir_if_not_exist(log_dir);

    char logfile[PATH_MAX];
    sprintf(logfile, "%s/%s.log", log_dir, program_name);
    if (log_init(program_name, LOG_SPACE_SIZE, 0, log_level, logfile))
        return -1;


    /* -------- Program Options -------- */
	int ch, longindex;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &longindex)) >= 0) {
        switch (ch) {
            case 'n':
                corpus_name = optarg;
                break;
            case 'r':
                corpus_rootdir = optarg;
                break;
            case 'z':
                is_test = 1;
                break;
            case 'v':
                log_level = LOG_DEBUG;
                break;
            case 't':
                log_level = LOG_TRACE;
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(1);
                break;
        }
    }

	if (optind != argc){
		//po.docset_path = argv[optind];
    }

    /* -------- Main -------- */
    Corpus *corpus = new Corpus(corpus_name, corpus_rootdir);

    GET_TIME_MILLIS(msec0);

    notice_log("Corpus load from files...");
    if ( corpus->load_from_files() != 0 ) {
        error_log("Corpus %s load_from_files() failed.", corpus_name.c_str());
        delete corpus;
        return -1;
    }

    GET_TIME_MILLIS(msec_loaded);

    notice_log("Do segment files...");
    if ( corpus->segment_files() != 0 ){
        error_log("Corpus %s segment_files() failed.", corpus_name.c_str());
        delete corpus;
        return -1;
    }

    GET_TIME_MILLIS(msec_segmented);

    notice_log("Saving segment files...");
    if ( corpus->save_segment_files() != 0 ){
        error_log("Corpus %s save_segment_files() failed.", corpus_name.c_str());
        delete corpus;
        return -1;
    }

    GET_TIME_MILLIS(msec_saved);


    Docset *pDocset = corpus->m_pRootDocset;
    docset_t *docset = docset_attach(pDocset);

    //notice_log("Do LDA...");

    //ldac_t *ldac = ldac_new();
    //ldac_estimate(ldac, docset);
    //ldac_free(ldac);

    GET_TIME_MILLIS(msec_lda);


    notice_log("Do SVD ...");

    uint32_t dimensions = 100;
    docset_do_svd_svdlibc(docset, dimensions);
    //docset_do_svd_armadillo(docset, dimensions);
    //docset_do_svd_octave(docset, dimensions);
    //docset_do_svd_eigen(docset, dimensions);
    //docset_do_svd_redsvd(docset, dimensions);

    GET_TIME_MILLIS(msec_svd);

    docset_detach(docset);
    delete corpus;

    GET_TIME_MILLIS(msec_end);

    notice_log("Corpus load from files: %zu.%03zu sec.", (size_t)(msec_loaded - msec0) / 1000, (size_t)(msec_loaded - msec0) % 1000);
    notice_log("Do segment files: %zu.%03zu sec.", (size_t)(msec_segmented - msec_loaded) / 1000, (size_t)(msec_segmented - msec_loaded) % 1000);
    notice_log("Saving segment files: %zu.%03zu sec.", (size_t)(msec_saved - msec_segmented) / 1000, (size_t)(msec_saved - msec_segmented) % 1000);
    notice_log("Do LDA: %zu.%03zu sec.", (size_t)(msec_lda - msec_saved) / 1000, (size_t)(msec_lda - msec_saved) % 1000);
    notice_log("Do SVD: %zu.%03zu sec.", (size_t)(msec_svd - msec_lda) / 1000, (size_t)(msec_svd - msec_lda) % 1000);

    notice_log("========> Total Time: %zu.%03zu sec.<========", (size_t)(msec_end - msec0) / 1000, (size_t)(msec_end - msec0) % 1000);
    notice_log("Done.");

    return 0;
}

