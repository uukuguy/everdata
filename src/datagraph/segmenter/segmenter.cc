/**
 * @file  segmenter.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 17:49:35
 * 
 * @brief  
 * 
 * 
 */

#include "segmenter.h"
#include <stdlib.h>
#include <string.h>

#include "UnigramCorpusReader.h"
#include "UnigramDict.h"
#include "SynonymsDict.h"
#include "ThesaurusDict.h"
#include "SegmenterManager.h"
#include "Segmenter.h"
#include "csr_utils.h"

using namespace css;

typedef struct segmenter_methods_t {
    void (*segmenter_free)(segmenter_t *);
    int (*segmenter_segment_buffer)(segmenter_t *segmenter, const char *buf, size_t buf_size, segment_peek_token_fn segment_peek_token, void *user_data);
} segmenter_methods_t;

typedef struct segmenter_t {
    const char *segclass;
    const segmenter_methods_t *segmenter_methods;
} segmenter_t;

typedef struct mmseg_segmenter_t{
    segmenter_t segmenter;
    const char *dictpath;
    SegmenterManager* mgr;
    //Segmenter* seg;
} mmseg_segmenter_t;

segmenter_t *mmseg_segmenter_new(const char *dictpath);

typedef struct segmenter_classes_t {
    const char *segclass;
    segmenter_t *(*segmenter_new)(const char *dictpath);
} segmenter_classes_t;

static segmenter_classes_t segmenter_classes[] ={
    {"mmseg", mmseg_segmenter_new},
};

segmenter_t *segmenter_new(const char *segclass, const char *dictpath)
{
    segmenter_t *segmenter = NULL;

    for ( size_t i = 0 ; i < sizeof(segmenter_classes) / sizeof(segmenter_classes_t) ; i++ ){
        if ( strcmp(segclass, segmenter_classes[i].segclass) == 0 ){
            if ( segmenter_classes[i].segmenter_new != NULL ){
                segmenter = segmenter_classes[i].segmenter_new(dictpath);
            }
            break;
        }
    }

    return segmenter;
}

void segmenter_free(segmenter_t *segmenter)
{
    if ( segmenter->segmenter_methods->segmenter_free != NULL ){
        segmenter->segmenter_methods->segmenter_free(segmenter);
    }
}

int segmenter_segment_buffer(segmenter_t *segmenter, const char *buf, size_t buf_size, segment_peek_token_fn segment_peek_token, void *user_data)
{
    if ( segmenter->segmenter_methods->segmenter_free != NULL ){
        return segmenter->segmenter_methods->segmenter_segment_buffer(segmenter, buf, buf_size, segment_peek_token, user_data);
    } else {
        return -1;
    }
}

void mmseg_segmenter_free(segmenter_t *segmenter);
int mmseg_segmenter_segment_buffer(segmenter_t *segmenter, const char *buf, size_t buf_size, segment_peek_token_fn segment_peek_token, void *user_data);

static const segmenter_methods_t mmseg_segmenter_methods = {
    mmseg_segmenter_free,
    mmseg_segmenter_segment_buffer
};

segmenter_t *mmseg_segmenter_new(const char *dictpath)
{
    mmseg_segmenter_t *mmseg_segmenter = (mmseg_segmenter_t*)malloc(sizeof(mmseg_segmenter_t));
    memset(mmseg_segmenter, 0, sizeof(mmseg_segmenter_t));

    mmseg_segmenter->segmenter.segclass = "mmseg";
    mmseg_segmenter->segmenter.segmenter_methods = &mmseg_segmenter_methods;
    mmseg_segmenter->dictpath = dictpath;

    mmseg_segmenter->mgr = new SegmenterManager();
    mmseg_segmenter->mgr->init(dictpath);

    //mmseg_segmenter->seg = mmseg_segmenter->mgr->getSegmenter();

    return (segmenter_t*)mmseg_segmenter;
}

void mmseg_segmenter_free(segmenter_t *segmenter)
{
    mmseg_segmenter_t *mmseg_segmenter = (mmseg_segmenter_t*)segmenter;

    delete mmseg_segmenter->mgr;

    free(mmseg_segmenter);
}

int segment_buffer(const char* buffer, size_t length, Segmenter* seg, segment_peek_token_fn segment_peek_token, void *user_data);
int mmseg_segmenter_segment_buffer(segmenter_t *segmenter, const char *buf, size_t buf_size, segment_peek_token_fn segment_peek_token, void *user_data)
{
    mmseg_segmenter_t *mmseg_segmenter = (mmseg_segmenter_t*)segmenter;

    Segmenter* seg = mmseg_segmenter->mgr->getSegmenter();
    segment_buffer(buf, buf_size, seg, segment_peek_token, user_data);

    return 0;
}

#include <stdint.h>
#include <map>
#include <assert.h>

int segment_buffer(const char* buffer, size_t length, Segmenter* seg, segment_peek_token_fn segment_peek_token, void *user_data)
{
    std::map<std::string, int> tokens;
    std::multimap<int, std::string> count_tokens;
    //uint32_t total_tokens = 0;

	//unsigned long srch,str;
	//str = currentTimeMillis();

	//begin seg
	seg->setBuffer((u1*)buffer,length);
	u2 len = 0, symlen = 0;
	u2 kwlen = 0, kwsymlen = 0;
	//check 1st token.
	unsigned char txtHead[3] = {239,187,191};
	char* tok = (char*)seg->peekToken(len, symlen);
	seg->popToken(len);
	if(seg->isSentenceEnd()){
		do {
			char* kwtok = (char*)seg->peekToken(kwlen , kwsymlen,1);
			if(kwsymlen)
				printf("[kw]%*.*s/x ",kwsymlen,kwsymlen,kwtok);
		}while(kwsymlen);
	}

	if(len == 3 && memcmp(tok,txtHead,sizeof(char)*3) == 0){
		//check is 0xFEFF
		//do nothing
	}else{
		//printf("%*.*s| ",symlen,symlen,tok);
	}
	while(1){
		len = 0;
		char* tok = (char*)seg->peekToken(len,symlen);
		if(!tok || !*tok || !len)
			break;
		seg->popToken(len);
		if(seg->isSentenceEnd()){
			do {
				char* kwtok = (char*)seg->peekToken(kwlen , kwsymlen,1);
				if(kwsymlen)
					printf("[kw]%*.*s/x ",kwsymlen,kwsymlen,kwtok);
			}while(kwsymlen);
		}

        if(*tok == '\r')
            continue;
        if(*tok == '\n'){
            //printf("\n");
            continue;
        }

        if ( segment_peek_token != NULL ){
            segment_peek_token(tok, symlen, user_data);
        }
		//check thesaurus
        if ( 0 )
		{
			const char* thesaurus_ptr = seg->thesaurus(tok, symlen);
			while(thesaurus_ptr && *thesaurus_ptr) {
				len = strlen(thesaurus_ptr);
				printf("%*.*s/s ",len,len,thesaurus_ptr);
				thesaurus_ptr += len + 1; //move next
			}
		}
		//printf("%s",tok);
	}
	return 0;
}

