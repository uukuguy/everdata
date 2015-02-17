/*
 * Copyright (C) 2007 by
 * 
 * 	Xuan-Hieu Phan
 *	hieuxuan@ecei.tohoku.ac.jp or pxhieu@gmail.com
 * 	Graduate School of Information Sciences
 * 	Tohoku University
 *
 * GibbsLDA++ is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GibbsLDA++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GibbsLDA++; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

/* 
 * References:
 * + The Java code of Gregor Heinrich (gregor@arbylon.net)
 *   http://www.arbylon.net/projects/LdaGibbsSampler.java
 * + "Parameter estimation for text analysis" by Gregor Heinrich
 *   http://www.arbylon.net/publications/text-est.pdf
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "ldac_gibbs.h"
#include "ldac.h"
#include "docset.h"
#include "document.h"
#include "term.h"

#define	BUFF_SIZE_LONG	1000000
#define	BUFF_SIZE_SHORT	512

#include <vector>
using namespace std;

// --------------- strtokenizer ---------------

class strtokenizer {
    protected:
        vector<string> tokens;
        size_t idx;

    public:
        strtokenizer(string str, string seperators = " ");    

        void parse(string str, string seperators);

        size_t count_tokens();
        string next_token();   
        void start_scan();

        string token(size_t i);
};

strtokenizer::strtokenizer(string str, string seperators) {
    parse(str, seperators);
}

void strtokenizer::parse(string str, string seperators) {
    int n = str.length();
    int start, stop;

    start = str.find_first_not_of(seperators);
    while (start >= 0 && start < n) {
        stop = str.find_first_of(seperators, start);
        if (stop < 0 || stop > n) {
            stop = n;
        }

        tokens.push_back(str.substr(start, stop - start));	
        start = str.find_first_not_of(seperators, stop + 1);
    }

    start_scan();
}

size_t strtokenizer::count_tokens() {
    return tokens.size();
}

void strtokenizer::start_scan() {
    idx = 0;
}

string strtokenizer::next_token() {    
    if (idx < tokens.size()) {
        return tokens[idx++];
    } else {
        return "";
    }
}
 
string strtokenizer::token(size_t i) {
    if (i < tokens.size()) {
        return tokens[i];
    } else {
        return "";
    }
}

// ---------------- dataset ----------------

// map of words/terms [string => uint32_t]
typedef map<string, uint32_t> mapword2id;
// map of words/terms [uint32_t => string]
typedef map<uint32_t, string> mapid2word;

class document {
    public:
        uint32_t * words;
        string rawstr;
        uint32_t length;

        document() {
            words = NULL;
            rawstr = "";
            length = 0;	
        }

        document(uint32_t length) {
            this->length = length;
            rawstr = "";
            words = new uint32_t[length];	
        }

        document(uint32_t length, uint32_t * words) {
            this->length = length;
            rawstr = "";
            this->words = new uint32_t[length];
            for (uint32_t i = 0; i < length; i++) {
                this->words[i] = words[i];
            }
        }

        document(uint32_t length, uint32_t * words, string rawstr) {
            this->length = length;
            this->rawstr = rawstr;
            this->words = new uint32_t[length];
            for (uint32_t i = 0; i < length; i++) {
                this->words[i] = words[i];
            }
        }

        document(vector<uint32_t> & doc) {
            this->length = doc.size();
            rawstr = "";
            this->words = new uint32_t[length];
            for (uint32_t i = 0; i < length; i++) {
                this->words[i] = doc[i];
            }
        }

        document(vector<uint32_t> & doc, string rawstr) {
            this->length = doc.size();
            this->rawstr = rawstr;
            this->words = new uint32_t[length];
            for (uint32_t i = 0; i < length; i++) {
                this->words[i] = doc[i];
            }
        }

        ~document() {
            if (words) {
                delete words;
            }
        }
};

class dataset {
    public:
        document ** docs;
        document ** _docs; // used only for inference
        map<uint32_t, uint32_t> _id2id; // also used only for inference
        uint32_t M; // number of documents
        uint32_t V; // number of words

        dataset() {
            docs = NULL;
            _docs = NULL;
            M = 0;
            V = 0;
        }

        dataset(uint32_t M) {
            this->M = M;
            this->V = 0;
            docs = new document*[M];	
            _docs = NULL;
        }   

        ~dataset() {
            if (docs) {
                for (uint32_t i = 0; i < M; i++) {
                    delete docs[i];
                }
            }
            delete docs;

            if (_docs) {
                for (uint32_t i = 0; i < M; i++) {
                    delete _docs[i];		
                }
            }
            delete _docs;	
        }

        void deallocate() {
            if (docs) {
                for (uint32_t i = 0; i < M; i++) {
                    delete docs[i];
                }
            }
            delete docs;
            docs = NULL;

            if (_docs) {
                for (uint32_t i = 0; i < M; i++) {
                    delete _docs[i];
                }
            }
            delete _docs;
            _docs = NULL;
        }

        void add_doc(document * doc, uint32_t idx) {
            if (idx < M) {
                docs[idx] = doc;
            }
        }   

        void _add_doc(document * doc, uint32_t idx) {
            if (idx < M) {
                _docs[idx] = doc;
            }
        }       

        static int write_wordmap(string wordmapfile, mapword2id * pword2id);
        static int read_wordmap(string wordmapfile, mapword2id * pword2id);
        static int read_wordmap(string wordmapfile, mapid2word * pid2word);

        int read_trndata(string dfile, string wordmapfile);
        int read_newdata(string dfile, string wordmapfile);
        int read_newdata_withrawstrs(string dfile, string wordmapfile);

        int load_from_docset(docset_t *docset);
};

int dataset::write_wordmap(string wordmapfile, mapword2id * pword2id) {
    FILE * fout = fopen(wordmapfile.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to write!\n", wordmapfile.c_str());
        return 1;
    }    

    mapword2id::iterator it;
    fprintf(fout, "%zu\n", pword2id->size());
    for (it = pword2id->begin(); it != pword2id->end(); it++) {
        fprintf(fout, "%s %d\n", (it->first).c_str(), it->second);
    }

    fclose(fout);

    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapword2id * pword2id) {
    pword2id->clear();

    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to read!\n", wordmapfile.c_str());
        return 1;
    }    

    char buff[BUFF_SIZE_SHORT];
    string line;

    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);

    for (int i = 0; i < nwords; i++) {
        fgets(buff, BUFF_SIZE_SHORT - 1, fin);
        line = buff;

        strtokenizer strtok(line, " \t\r\n");
        if (strtok.count_tokens() != 2) {
            continue;
        }

        pword2id->insert(pair<string, uint32_t>(strtok.token(0), atoi(strtok.token(1).c_str())));
    }

    fclose(fin);

    return 0;
}

int dataset::read_wordmap(string wordmapfile, mapid2word * pid2word) {
    pid2word->clear();

    FILE * fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to read!\n", wordmapfile.c_str());
        return 1;
    }    

    char buff[BUFF_SIZE_SHORT];
    string line;

    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);

    for (int i = 0; i < nwords; i++) {
        fgets(buff, BUFF_SIZE_SHORT - 1, fin);
        line = buff;

        strtokenizer strtok(line, " \t\r\n");
        if (strtok.count_tokens() != 2) {
            continue;
        }

        pid2word->insert(pair<uint32_t, string>(atoi(strtok.token(1).c_str()), strtok.token(0)));
    }

    fclose(fin);

    return 0;
}


int dataset::load_from_docset(docset_t *docset)
{
    Docset *pDocset = (Docset*)(docset->pDocset);
    this->M = pDocset->get_total_docs();
    this->V = pDocset->get_total_terms();
    this->docs = new document*[this->M];

    for (uint32_t i = 0; i < this->M; i++) {
        Document *pDocument = pDocset->get_document_by_index(i);
        size_t num_words = pDocument->get_total_words();

        document * pdoc = new document(num_words);
        for (size_t j = 0; j < num_words; j++) {
            uint32_t term_id = pDocument->get_word_termid_by_index(j); 

            pdoc->words[j] = term_id;
        }
        this->add_doc(pdoc, i);
    }

    return 0;
}

int dataset::read_trndata(string dfile, string wordmapfile) {
    mapword2id word2id;

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to read!\n", dfile.c_str());
        return 1;
    }   

    mapword2id::iterator it;    
    char buff[BUFF_SIZE_LONG];
    string line;

    // get the number of documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
        printf("No document available!\n");
        return 1;
    }

    // allocate memory for corpus
    if (docs) {
        deallocate();
    } else {
        docs = new document*[M];
    }

    // set number of words to zero
    V = 0;

    for (uint32_t i = 0; i < M; i++) {
        fgets(buff, BUFF_SIZE_LONG - 1, fin);
        line = buff;
        strtokenizer strtok(line, " \t\r\n");
        uint32_t length = strtok.count_tokens();

        if (length <= 0) {
            printf("Invalid (empty) document!\n");
            deallocate();
            M = V = 0;
            return 1;
        }

        // allocate new document
        document * pdoc = new document(length);

        for (uint32_t j = 0; j < length; j++) {
            it = word2id.find(strtok.token(j));
            if (it == word2id.end()) {
                // word not found, i.e., new word
                pdoc->words[j] = word2id.size();
                word2id.insert(pair<string, uint32_t>(strtok.token(j), word2id.size()));
            } else {
                pdoc->words[j] = it->second;
            }
        }

        // add new doc to the corpus
        add_doc(pdoc, i);
    }

    fclose(fin);

    // write word map to file
    if (write_wordmap(wordmapfile, &word2id)) {
        return 1;
    }

    // update number of words
    V = word2id.size();

    return 0;
}

int dataset::read_newdata(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<uint32_t, uint32_t> id2_id;

    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
        printf("No word map available!\n");
        return 1;
    }

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to read!\n", dfile.c_str());
        return 1;
    }   

    mapword2id::iterator it;
    map<uint32_t, uint32_t>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;

    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
        printf("No document available!\n");
        return 1;
    }

    // allocate memory for corpus
    if (docs) {
        deallocate();
    } else {
        docs = new document*[M];
    }
    _docs = new document*[M];

    // set number of words to zero
    V = 0;

    for (uint32_t i = 0; i < M; i++) {
        fgets(buff, BUFF_SIZE_LONG - 1, fin);
        line = buff;
        strtokenizer strtok(line, " \t\r\n");
        uint32_t length = strtok.count_tokens();

        vector<uint32_t> doc;
        vector<uint32_t> _doc;
        for (uint32_t j = 0; j < length; j++) {
            it = word2id.find(strtok.token(j));
            if (it == word2id.end()) {
                // word not found, i.e., word unseen in training data
                // do anything? (future decision)
            } else {
                uint32_t _id;
                _it = id2_id.find(it->second);
                if (_it == id2_id.end()) {
                    _id = id2_id.size();
                    id2_id.insert(pair<uint32_t, uint32_t>(it->second, _id));
                    _id2id.insert(pair<uint32_t, uint32_t>(_id, it->second));
                } else {
                    _id = _it->second;
                }

                doc.push_back(it->second);
                _doc.push_back(_id);
            }
        }

        // allocate memory for new doc
        document * pdoc = new document(doc);
        document * _pdoc = new document(_doc);

        // add new doc
        add_doc(pdoc, i);
        _add_doc(_pdoc, i);
    }

    fclose(fin);

    // update number of new words
    V = id2_id.size();

    return 0;
}

int dataset::read_newdata_withrawstrs(string dfile, string wordmapfile) {
    mapword2id word2id;
    map<uint32_t, uint32_t> id2_id;

    read_wordmap(wordmapfile, &word2id);
    if (word2id.size() <= 0) {
        printf("No word map available!\n");
        return 1;
    }

    FILE * fin = fopen(dfile.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to read!\n", dfile.c_str());
        return 1;
    }   

    mapword2id::iterator it;
    map<uint32_t, uint32_t>::iterator _it;
    char buff[BUFF_SIZE_LONG];
    string line;

    // get number of new documents
    fgets(buff, BUFF_SIZE_LONG - 1, fin);
    M = atoi(buff);
    if (M <= 0) {
        printf("No document available!\n");
        return 1;
    }

    // allocate memory for corpus
    if (docs) {
        deallocate();
    } else {
        docs = new document*[M];
    }
    _docs = new document*[M];

    // set number of words to zero
    V = 0;

    for (uint32_t i = 0; i < M; i++) {
        fgets(buff, BUFF_SIZE_LONG - 1, fin);
        line = buff;
        strtokenizer strtok(line, " \t\r\n");
        uint32_t length = strtok.count_tokens();

        vector<uint32_t> doc;
        vector<uint32_t> _doc;
        for (uint32_t j = 0; j < length - 1; j++) {
            it = word2id.find(strtok.token(j));
            if (it == word2id.end()) {
                // word not found, i.e., word unseen in training data
                // do anything? (future decision)
            } else {
                uint32_t _id;
                _it = id2_id.find(it->second);
                if (_it == id2_id.end()) {
                    _id = id2_id.size();
                    id2_id.insert(pair<uint32_t, uint32_t>(it->second, _id));
                    _id2id.insert(pair<uint32_t, uint32_t>(_id, it->second));
                } else {
                    _id = _it->second;
                }

                doc.push_back(it->second);
                _doc.push_back(_id);
            }
        }

        // allocate memory for new doc
        document * pdoc = new document(doc, line);
        document * _pdoc = new document(_doc, line);

        // add new doc
        add_doc(pdoc, i);
        _add_doc(_pdoc, i);
    }

    fclose(fin);

    // update number of new words
    V = id2_id.size();

    return 0;
}

// ---------------------------------------------------

string generate_model_name(int iter) {
    string model_name = "model-";

    char buff[BUFF_SIZE_SHORT];

    if (0 <= iter && iter < 10) {
        sprintf(buff, "0000%d", iter);
    } else if (10 <= iter && iter < 100) {
        sprintf(buff, "000%d", iter);
    } else if (100 <= iter && iter < 1000) {
        sprintf(buff, "00%d", iter);
    } else if (1000 <= iter && iter < 10000) {
        sprintf(buff, "0%d", iter);
    } else {
        sprintf(buff, "%d", iter);
    }

    if (iter >= 0) {
        model_name += buff;
    } else {
        model_name += "final";
    }

    return model_name;
}

void quicksort(vector<pair<uint64_t, double> > & vect, uint64_t left, uint64_t right) {
    uint64_t l_hold, r_hold;
    pair<uint64_t, double> pivot;

    l_hold = left;
    r_hold = right;    
    uint64_t pivotidx = left;
    pivot = vect[pivotidx];

    while (left < right) {
        while (vect[right].second <= pivot.second && left < right) {
            right--;
        }
        if (left != right) {
            vect[left] = vect[right];
            left++;
        }
        while (vect[left].second >= pivot.second && left < right) {
            left++;
        }
        if (left != right) {
            vect[right] = vect[left];
            right--;
        }
    }

    vect[left] = pivot;
    pivotidx = left;
    left = l_hold;
    right = r_hold;

    if (left < pivotidx) {
        quicksort(vect, left, pivotidx - 1);
    }
    if (right > pivotidx) {
        quicksort(vect, pivotidx + 1, right);
    }    
}

model::~model() {
    if (p) {
        delete p;
    }

    if (ptrndata) {
        delete ptrndata;
    }

    if (pnewdata) {
        delete pnewdata;
    }

    if (z) {
        for (uint32_t m = 0; m < M; m++) {
            if (z[m]) {
                delete z[m];
            }
        }
    }

    if (nw) {
        for (uint32_t w = 0; w < V; w++) {
            if (nw[w]) {
                delete nw[w];
            }
        }
    }

    if (nd) {
        for (uint32_t m = 0; m < M; m++) {
            if (nd[m]) {
                delete nd[m];
            }
        }
    } 

    if (nwsum) {
        delete nwsum;
    }   

    if (ndsum) {
        delete ndsum;
    }

    if (theta) {
        for (uint32_t m = 0; m < M; m++) {
            if (theta[m]) {
                delete theta[m];
            }
        }
    }

    if (phi) {
        for (uint32_t k = 0; k < K; k++) {
            if (phi[k]) {
                delete phi[k];
            }
        }
    }

    // only for inference
    if (newz) {
        for (uint32_t m = 0; m < newM; m++) {
            if (newz[m]) {
                delete newz[m];
            }
        }
    }

    if (newnw) {
        for (uint32_t w = 0; w < newV; w++) {
            if (newnw[w]) {
                delete newnw[w];
            }
        }
    }

    if (newnd) {
        for (uint32_t m = 0; m < newM; m++) {
            if (newnd[m]) {
                delete newnd[m];
            }
        }
    } 

    if (newnwsum) {
        delete newnwsum;
    }   

    if (newndsum) {
        delete newndsum;
    }

    if (newtheta) {
        for (uint32_t m = 0; m < newM; m++) {
            if (newtheta[m]) {
                delete newtheta[m];
            }
        }
    }

    if (newphi) {
        for (uint32_t k = 0; k < K; k++) {
            if (newphi[k]) {
                delete newphi[k];
            }
        }
    }
}

void model::set_default_values() {
    wordmapfile = "wordmap.txt";
    trainlogfile = "trainlog.txt";
    tassign_suffix = ".tassign";
    theta_suffix = ".theta";
    phi_suffix = ".phi";
    others_suffix = ".others";
    twords_suffix = ".twords";
    
    dir = "./";
    dfile = "trndocs.dat";
    model_name = "model-final";    
    model_status = MODEL_STATUS_UNKNOWN;
    
    ptrndata = NULL;
    pnewdata = NULL;
    
    M = 0;
    V = 0;
    K = 100;
    alpha = 50.0 / K;
    beta = 0.1;
    niters = 2000;
    liter = 0;
    savestep = 200;    
    twords = 0;
    withrawstrs = 0;
    
    p = NULL;
    z = NULL;
    nw = NULL;
    nd = NULL;
    nwsum = NULL;
    ndsum = NULL;
    theta = NULL;
    phi = NULL;
    
    newM = 0;
    newV = 0;
    newz = NULL;
    newnw = NULL;
    newnd = NULL;
    newnwsum = NULL;
    newndsum = NULL;
    newtheta = NULL;
    newphi = NULL;
}

int model::parse_args(int argc, char ** argv) {
    return 0;
    //return utils::parse_args(argc, argv, this);
}

int model::init(int argc, char ** argv) {
    // call parse_args
    if (parse_args(argc, argv)) {
        return 1;
    }

    if (model_status == MODEL_STATUS_EST) {
        // estimating the model from scratch
        if (init_est()) {
            return 1;
        }

    } else if (model_status == MODEL_STATUS_ESTC) {
        // estimating the model from a previously estimated one
        if (init_estc()) {
            return 1;
        }

    } else if (model_status == MODEL_STATUS_INF) {
        // do inference
        if (init_inf()) {
            return 1;
        }
    }

    return 0;
}

int model::load_model(string model_name) {
    uint32_t i, j;

    string filename = dir + model_name + tassign_suffix;
    FILE * fin = fopen(filename.c_str(), "r");
    if (!fin) {
        printf("Cannot open file %s to load model!\n", filename.c_str());
        return 1;
    }

    char buff[BUFF_SIZE_LONG];
    string line;

    // allocate memory for z and ptrndata
    z = new uint32_t*[M];
    ptrndata = new dataset(M);
    ptrndata->V = V;
 
    for (i = 0; i < M; i++) {
        char * pointer = fgets(buff, BUFF_SIZE_LONG, fin);
        if (!pointer) {
            printf("Invalid word-topic assignment file, check the number of docs!\n");
            return 1;
        }

        line = buff;
        strtokenizer strtok(line, " \t\r\n");
        uint32_t length = strtok.count_tokens();

        vector<uint32_t> words;
        vector<uint32_t> topics;
        for (j = 0; j < length; j++) {
            string token = strtok.token(j);

            strtokenizer tok(token, ":");
            if (tok.count_tokens() != 2) {
                printf("Invalid word-topic assignment line!\n");
                return 1;
            }

            words.push_back(atoi(tok.token(0).c_str()));
            topics.push_back(atoi(tok.token(1).c_str()));
        }

        // allocate and add new document to the corpus
        document * pdoc = new document(words);
        ptrndata->add_doc(pdoc, i);

        // assign values for z
        z[i] = new uint32_t[topics.size()];
        for (j = 0; j < topics.size(); j++) {
            z[i][j] = topics[j];
        }
    }   

    fclose(fin);

    return 0;
}

int model::save_model(string model_name) {
    if (save_model_tassign(dir + model_name + tassign_suffix)) {
        return 1;
    }

    if (save_model_others(dir + model_name + others_suffix)) {
        return 1;
    }

    if (save_model_theta(dir + model_name + theta_suffix)) {
        return 1;
    }

    if (save_model_phi(dir + model_name + phi_suffix)) {
        return 1;
    }

    if (twords > 0) {
        if (save_model_twords(dir + model_name + twords_suffix)) {
            return 1;
        }
    }

    return 0;
}

int model::save_model_tassign(string filename) {
    uint32_t i, j;

    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    // wirte docs with topic assignments for words
    for (i = 0; i < ptrndata->M; i++) {    
        for (j = 0; j < ptrndata->docs[i]->length; j++) {
            fprintf(fout, "%d:%d ", ptrndata->docs[i]->words[j], z[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);

    return 0;
}

int model::save_model_theta(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    for (uint32_t i = 0; i < M; i++) {
        for (uint32_t j = 0; j < K; j++) {
            fprintf(fout, "%f ", theta[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);

    return 0;
}

int model::save_model_phi(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    for (uint32_t i = 0; i < K; i++) {
        for (uint32_t j = 0; j < V; j++) {
            fprintf(fout, "%f ", phi[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);    

    return 0;
}

int model::save_model_others(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    fprintf(fout, "alpha=%f\n", alpha);
    fprintf(fout, "beta=%f\n", beta);
    fprintf(fout, "ntopics=%d\n", K);
    fprintf(fout, "ndocs=%d\n", M);
    fprintf(fout, "nwords=%d\n", V);
    fprintf(fout, "liter=%d\n", liter);

    fclose(fout);    

    return 0;
}

int model::save_inf_model(string model_name) {
    if (save_inf_model_tassign(dir + model_name + tassign_suffix)) {
        return 1;
    }

    if (save_inf_model_others(dir + model_name + others_suffix)) {
        return 1;
    }

    if (save_inf_model_newtheta(dir + model_name + theta_suffix)) {
        return 1;
    }

    if (save_inf_model_newphi(dir + model_name + phi_suffix)) {
        return 1;
    }

    if (twords > 0) {
        if (save_inf_model_twords(dir + model_name + twords_suffix)) {
            return 1;
        }
    }

    return 0;
}

int model::save_inf_model_tassign(string filename) {
    uint32_t i, j;

    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    // wirte docs with topic assignments for words
    for (i = 0; i < pnewdata->M; i++) {    
        for (j = 0; j < pnewdata->docs[i]->length; j++) {
            fprintf(fout, "%d:%d ", pnewdata->docs[i]->words[j], newz[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);

    return 0;
}

int model::save_inf_model_newtheta(string filename) {
    uint32_t i, j;

    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    for (i = 0; i < newM; i++) {
        for (j = 0; j < K; j++) {
            fprintf(fout, "%f ", newtheta[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);

    return 0;
}

int model::save_inf_model_newphi(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    for (uint32_t i = 0; i < K; i++) {
        for (uint32_t j = 0; j < newV; j++) {
            fprintf(fout, "%f ", newphi[i][j]);
        }
        fprintf(fout, "\n");
    }

    fclose(fout);    

    return 0;
}

int model::save_inf_model_others(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    fprintf(fout, "alpha=%f\n", alpha);
    fprintf(fout, "beta=%f\n", beta);
    fprintf(fout, "ntopics=%d\n", K);
    fprintf(fout, "ndocs=%d\n", newM);
    fprintf(fout, "nwords=%d\n", newV);
    fprintf(fout, "liter=%d\n", inf_liter);

    fclose(fout);    

    return 0;
}

int model::save_inf_model_twords(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    if (twords > newV) {
        twords = newV;
    }
    mapid2word::iterator it;
    map<uint32_t, uint32_t>::iterator _it;

    for (uint32_t k = 0; k < K; k++) {
        vector<pair<uint64_t, double> > words_probs;
        pair<uint64_t, double> word_prob;
        for (uint64_t w = 0; w < newV; w++) {
            word_prob.first = w;
            word_prob.second = newphi[k][w];
            words_probs.push_back(word_prob);
        }

        // quick sort to sort word-topic probability
        quicksort(words_probs, 0, words_probs.size() - 1);

        fprintf(fout, "Topic %dth:\n", k);
        for (uint32_t i = 0; i < twords; i++) {
            _it = pnewdata->_id2id.find(words_probs[i].first);
            if (_it == pnewdata->_id2id.end()) {
                continue;
            }
            it = id2word.find(_it->second);
            if (it != id2word.end()) {
                fprintf(fout, "\t%s   %f\n", (it->second).c_str(), words_probs[i].second);
            }
        }
    }

    fclose(fout);    

    return 0;    
}


int model::init_est() {
    uint32_t m, n, w, k;

    p = new double[K];

    // + read training data
    ptrndata = new dataset;
    //ptrndata->load_from_corpus(corpus);
    if (ptrndata->read_trndata(dir + dfile, dir + wordmapfile)) {
        printf("Fail to read training data!\n");
        return 1;
    }

    // + allocate memory and assign values for variables
    M = ptrndata->M;
    V = ptrndata->V;
    // K: from command line or default value
    // alpha, beta: from command line or default values
    // niters, savestep: from command line or default values

    nw = new uint32_t*[V];
    for (w = 0; w < V; w++) {
        nw[w] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nw[w][k] = 0;
        }
    }

    nd = new uint32_t*[M];
    for (m = 0; m < M; m++) {
        nd[m] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nd[m][k] = 0;
        }
    }

    nwsum = new uint32_t[K];
    for (k = 0; k < K; k++) {
        nwsum[k] = 0;
    }

    ndsum = new uint32_t[M];
    for (m = 0; m < M; m++) {
        ndsum[m] = 0;
    }

    srandom(time(0)); // initialize for random number generation
    z = new uint32_t*[M];
    for (m = 0; m < ptrndata->M; m++) {
        uint32_t N = ptrndata->docs[m]->length;
        z[m] = new uint32_t[N];

        // initialize for z
        for (n = 0; n < N; n++) {
            uint32_t topic = (uint32_t)(((double)random() / RAND_MAX) * K);
            z[m][n] = topic;

            // number of instances of word i assigned to topic j
            nw[ptrndata->docs[m]->words[n]][topic] += 1;
            // number of words in document i assigned to topic j
            nd[m][topic] += 1;
            // total number of words assigned to topic j
            nwsum[topic] += 1;
        } 
        // total number of words in document i
        ndsum[m] = N;      
    }

    theta = new double*[M];
    for (m = 0; m < M; m++) {
        theta[m] = new double[K];
    }

    phi = new double*[K];
    for (k = 0; k < K; k++) {
        phi[k] = new double[V];
    }    

    return 0;
}

int model::init_estc() {
    // estimating the model from a previously estimated one
    uint32_t m, n, w, k;

    p = new double[K];

    // load moel, i.e., read z and ptrndata
    if (load_model(model_name)) {
	printf("Fail to load word-topic assignmetn file of the model!\n");
	return 1;
    }

    nw = new uint32_t*[V];
    for (w = 0; w < V; w++) {
        nw[w] = new uint32_t[K];
        for (k = 0; k < K; k++) {
    	    nw[w][k] = 0;
        }
    }
	
    nd = new uint32_t*[M];
    for (m = 0; m < M; m++) {
        nd[m] = new uint32_t[K];
        for (k = 0; k < K; k++) {
    	    nd[m][k] = 0;
        }
    }
	
    nwsum = new uint32_t[K];
    for (k = 0; k < K; k++) {
	nwsum[k] = 0;
    }
    
    ndsum = new uint32_t[M];
    for (m = 0; m < M; m++) {
	ndsum[m] = 0;
    }

    for (m = 0; m < ptrndata->M; m++) {
	uint32_t N = ptrndata->docs[m]->length;

	// assign values for nw, nd, nwsum, and ndsum	
        for (n = 0; n < N; n++) {
    	    uint32_t w = ptrndata->docs[m]->words[n];
    	    uint32_t topic = z[m][n];
    	    
    	    // number of instances of word i assigned to topic j
    	    nw[w][topic] += 1;
    	    // number of words in document i assigned to topic j
    	    nd[m][topic] += 1;
    	    // total number of words assigned to topic j
    	    nwsum[topic] += 1;
        } 
        // total number of words in document i
        ndsum[m] = N;      
    }
	
    theta = new double*[M];
    for (m = 0; m < M; m++) {
        theta[m] = new double[K];
    }
	
    phi = new double*[K];
    for (k = 0; k < K; k++) {
        phi[k] = new double[V];
    }    

    return 0;        
}

void model::estimate() {
    //if (twords > 0) {
        //// print out top words per topic
        //dataset::read_wordmap(dir + wordmapfile, &id2word);
    //}

    printf("Sampling %d iterations!\n", niters); 

    uint32_t last_iter = liter;
    for (liter = last_iter + 1; liter <= niters + last_iter; liter++) {
        printf("Iteration %d ...\r", liter); fflush(stdout);

        // for all z_i
        for (uint32_t m = 0; m < M; m++) {
            for (uint32_t n = 0; n < ptrndata->docs[m]->length; n++) {
                // (z_i = z[m][n])
                // sample from p(z_i|z_-i, w)
                uint32_t topic = sampling(m, n);
                z[m][n] = topic;
            }
        }

        //if (savestep > 0) {
            //if (liter % savestep == 0) {
                //// saving the model
                //printf("Saving the model at iteration %d ...\n", liter); 
                //compute_theta();
                //compute_phi();
                //save_model(generate_model_name(liter));
            //}
        //}
    }

    printf("Gibbs sampling completed!\n"); 
    printf("Saving the final model!\n"); 
    compute_theta();
    compute_phi();
    liter--;
    save_model(generate_model_name(-1));
}

int model::sampling(uint32_t m, uint32_t n) {
    // remove z_i from the count variables
    uint32_t topic = z[m][n];
    uint32_t w = ptrndata->docs[m]->words[n];
    nw[w][topic] -= 1;
    nd[m][topic] -= 1;
    nwsum[topic] -= 1;
    ndsum[m] -= 1;

    double Vbeta = V * beta;
    double Kalpha = K * alpha;    
    // do multinomial sampling via cumulative method
    for (uint32_t k = 0; k < K; k++) {
        p[k] = (nw[w][k] + beta) / (nwsum[k] + Vbeta) *
            (nd[m][k] + alpha) / (ndsum[m] + Kalpha);
    }
    // cumulate multinomial parameters
    for (uint32_t k = 1; k < K; k++) {
        p[k] += p[k - 1];
    }
    // scaled sample because of unnormalized p[]
    double u = ((double)random() / RAND_MAX) * p[K - 1];

    for (topic = 0; topic < K; topic++) {
        if (p[topic] > u) {
            break;
        }
    }

    // add newly estimated z_i to count variables
    nw[w][topic] += 1;
    nd[m][topic] += 1;
    nwsum[topic] += 1;
    ndsum[m] += 1;    

    return topic;
}

void model::compute_theta() {
    for (uint32_t m = 0; m < M; m++) {
        for (uint32_t k = 0; k < K; k++) {
            theta[m][k] = (nd[m][k] + alpha) / (ndsum[m] + K * alpha);
        }
    }
}

void model::compute_phi() {
    for (uint32_t k = 0; k < K; k++) {
        for (uint32_t w = 0; w < V; w++) {
            phi[k][w] = (nw[w][k] + beta) / (nwsum[k] + V * beta);
        }
    }
}

int model::init_inf() {
    // estimating the model from a previously estimated one
    uint32_t m, n, w, k;

    p = new double[K];

    // load moel, i.e., read z and ptrndata
    if (load_model(model_name)) {
        printf("Fail to load word-topic assignmetn file of the model!\n");
        return 1;
    }

    nw = new uint32_t*[V];
    for (w = 0; w < V; w++) {
        nw[w] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nw[w][k] = 0;
        }
    }

    nd = new uint32_t*[M];
    for (m = 0; m < M; m++) {
        nd[m] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nd[m][k] = 0;
        }
    }

    nwsum = new uint32_t[K];
    for (k = 0; k < K; k++) {
        nwsum[k] = 0;
    }

    ndsum = new uint32_t[M];
    for (m = 0; m < M; m++) {
        ndsum[m] = 0;
    }

    for (m = 0; m < ptrndata->M; m++) {
        uint32_t N = ptrndata->docs[m]->length;

        // assign values for nw, nd, nwsum, and ndsum	
        for (n = 0; n < N; n++) {
            uint32_t w = ptrndata->docs[m]->words[n];
            uint32_t topic = z[m][n];

            // number of instances of word i assigned to topic j
            nw[w][topic] += 1;
            // number of words in document i assigned to topic j
            nd[m][topic] += 1;
            // total number of words assigned to topic j
            nwsum[topic] += 1;
        } 
        // total number of words in document i
        ndsum[m] = N;      
    }

    // read new data for inference
    pnewdata = new dataset;
    if (withrawstrs) {
        if (pnewdata->read_newdata_withrawstrs(dir + dfile, dir + wordmapfile)) {
            printf("Fail to read new data!\n");
            return 1;
        }    
    } else {
        if (pnewdata->read_newdata(dir + dfile, dir + wordmapfile)) {
            printf("Fail to read new data!\n");
            return 1;
        }    
    }

    newM = pnewdata->M;
    newV = pnewdata->V;

    newnw = new uint32_t*[newV];
    for (w = 0; w < newV; w++) {
        newnw[w] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            newnw[w][k] = 0;
        }
    }

    newnd = new uint32_t*[newM];
    for (m = 0; m < newM; m++) {
        newnd[m] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            newnd[m][k] = 0;
        }
    }

    newnwsum = new uint32_t[K];
    for (k = 0; k < K; k++) {
        newnwsum[k] = 0;
    }

    newndsum = new uint32_t[newM];
    for (m = 0; m < newM; m++) {
        newndsum[m] = 0;
    }

    srandom(time(0)); // initialize for random number generation
    newz = new uint32_t*[newM];
    for (m = 0; m < pnewdata->M; m++) {
        uint32_t N = pnewdata->docs[m]->length;
        newz[m] = new uint32_t[N];

        // assign values for nw, nd, nwsum, and ndsum	
        for (n = 0; n < N; n++) {
            //uint32_t w = pnewdata->docs[m]->words[n];
            uint32_t _w = pnewdata->_docs[m]->words[n];
            uint32_t topic = (uint32_t)(((double)random() / RAND_MAX) * K);
            newz[m][n] = topic;

            // number of instances of word i assigned to topic j
            newnw[_w][topic] += 1;
            // number of words in document i assigned to topic j
            newnd[m][topic] += 1;
            // total number of words assigned to topic j
            newnwsum[topic] += 1;
        } 
        // total number of words in document i
        newndsum[m] = N;      
    }    

    newtheta = new double*[newM];
    for (m = 0; m < newM; m++) {
        newtheta[m] = new double[K];
    }

    newphi = new double*[K];
    for (k = 0; k < K; k++) {
        newphi[k] = new double[newV];
    }    

    return 0;        
}

void model::inference() {
    if (twords > 0) {
        // print out top words per topic
        dataset::read_wordmap(dir + wordmapfile, &id2word);
    }

    printf("Sampling %d iterations for inference!\n", niters); 

    for (inf_liter = 1; inf_liter <= niters; inf_liter++) {
        printf("Iteration %d ...\r", inf_liter); fflush(stdout);

        // for all newz_i
        for (uint32_t m = 0; m < newM; m++) {
            for (uint32_t n = 0; n < pnewdata->docs[m]->length; n++) {
                // (newz_i = newz[m][n])
                // sample from p(z_i|z_-i, w)
                uint32_t topic = inf_sampling(m, n);
                newz[m][n] = topic;
            }
        }
    }

    printf("Gibbs sampling for inference completed!\n");
    printf("Saving the inference outputs!\n");
    compute_newtheta();
    compute_newphi();
    inf_liter--;
    save_inf_model(dfile);
}

int model::inf_sampling(uint32_t m, uint32_t n) {
    // remove z_i from the count variables
    uint32_t topic = newz[m][n];
    uint32_t w = pnewdata->docs[m]->words[n];
    uint32_t _w = pnewdata->_docs[m]->words[n];
    newnw[_w][topic] -= 1;
    newnd[m][topic] -= 1;
    newnwsum[topic] -= 1;
    newndsum[m] -= 1;

    double Vbeta = V * beta;
    double Kalpha = K * alpha;
    // do multinomial sampling via cumulative method
    for (uint32_t k = 0; k < K; k++) {
        p[k] = (nw[w][k] + newnw[_w][k] + beta) / (nwsum[k] + newnwsum[k] + Vbeta) *
            (newnd[m][k] + alpha) / (newndsum[m] + Kalpha);
    }
    // cumulate multinomial parameters
    for (uint32_t k = 1; k < K; k++) {
        p[k] += p[k - 1];
    }
    // scaled sample because of unnormalized p[]
    double u = ((double)random() / RAND_MAX) * p[K - 1];

    for (topic = 0; topic < K; topic++) {
        if (p[topic] > u) {
            break;
        }
    }

    // add newly estimated z_i to count variables
    newnw[_w][topic] += 1;
    newnd[m][topic] += 1;
    newnwsum[topic] += 1;
    newndsum[m] += 1;    

    return topic;
}

void model::compute_newtheta() {
    for (uint32_t m = 0; m < newM; m++) {
        for (uint32_t k = 0; k < K; k++) {
            newtheta[m][k] = (newnd[m][k] + alpha) / (newndsum[m] + K * alpha);
        }
    }
}

void model::compute_newphi() {
    map<uint32_t, uint32_t>::iterator it;
    for (uint32_t k = 0; k < K; k++) {
        for (uint32_t w = 0; w < newV; w++) {
            it = pnewdata->_id2id.find(w);
            if (it != pnewdata->_id2id.end()) {
                newphi[k][w] = (nw[it->second][k] + newnw[w][k] + beta) / (nwsum[k] + newnwsum[k] + V * beta);
            }
        }
    }
}

#include "lexicon.h"
int model::init_est_from_docset(docset_t *docset, uint32_t NTOPICS, uint32_t EM_MAX_ITER) 
{

    M = docset_get_total_docs(docset);
    V = docset_get_total_terms(docset);
    K = NTOPICS;
    niters = EM_MAX_ITER;

    // id2word
    lexicon_t *lexicon = docset_get_lexicon(docset);
    for ( size_t i = 0 ; i < lexicon_get_size(lexicon) ; i++ ) {
        term_t *term = lexicon_get_term_by_index(lexicon, i);
        uint32_t term_id = term_get_id(term);
        const char *term_text = term_get_text(term);

        this->id2word.insert(map<uint32_t, string>::value_type(term_id, term_text));

        term_detach(term);
    }
    lexicon_detach(lexicon);


    uint32_t m, n, w, k;

    p = new double[K];

    // + read training data
    ptrndata = new dataset;
    ptrndata->load_from_docset(docset);

    // + allocate memory and assign values for variables
    M = ptrndata->M;
    V = ptrndata->V;
    // K: from command line or default value
    // alpha, beta: from command line or default values
    // niters, savestep: from command line or default values

    nw = new uint32_t*[V];
    for (w = 0; w < V; w++) {
        nw[w] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nw[w][k] = 0;
        }
    }

    nd = new uint32_t*[M];
    for (m = 0; m < M; m++) {
        nd[m] = new uint32_t[K];
        for (k = 0; k < K; k++) {
            nd[m][k] = 0;
        }
    }

    nwsum = new uint32_t[K];
    for (k = 0; k < K; k++) {
        nwsum[k] = 0;
    }

    ndsum = new uint32_t[M];
    for (m = 0; m < M; m++) {
        ndsum[m] = 0;
    }

    srandom(time(0)); // initialize for random number generation
    z = new uint32_t*[M];
    for (m = 0; m < ptrndata->M; m++) {
        uint32_t N = ptrndata->docs[m]->length;
        z[m] = new uint32_t[N];

        // initialize for z
        for (n = 0; n < N; n++) {
            uint32_t topic = (uint32_t)(((double)random() / RAND_MAX) * K);
            z[m][n] = topic;

            // number of instances of word i assigned to topic j
            nw[ptrndata->docs[m]->words[n]][topic] += 1;
            // number of words in document i assigned to topic j
            nd[m][topic] += 1;
            // total number of words assigned to topic j
            nwsum[topic] += 1;
        } 
        // total number of words in document i
        ndsum[m] = N;      
    }

    theta = new double*[M];
    for (m = 0; m < M; m++) {
        theta[m] = new double[K];
    }

    phi = new double*[K];
    for (k = 0; k < K; k++) {
        phi[k] = new double[V];
    }    

    return 0;
}


#include "docset.h"
#include "document.h"
#include <string.h>

int model::save_model_twords(string filename) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    if (twords > V) {
        twords = V;
    }
    mapid2word::iterator it;

    for (uint32_t k = 0; k < K; k++) {
        vector<pair<uint64_t, double> > words_probs;
        pair<uint64_t, double> word_prob;
        for (uint32_t w = 0; w < V; w++) {
            word_prob.first = w;
            word_prob.second = phi[k][w];
            words_probs.push_back(word_prob);
        }

        // quick sort to sort word-topic probability
        quicksort(words_probs, 0, words_probs.size() - 1);

        fprintf(fout, "Topic %dth:\n", k);
        for (uint32_t i = 0; i < twords; i++) {
            it = id2word.find(words_probs[i].first);
            if (it != id2word.end()) {
                fprintf(fout, "\t%s   %f\n", (it->second).c_str(), words_probs[i].second);
            }
        }
    }

    fclose(fout);    

    return 0;    
}

int model::save_model_tdocs(string filename, docset_t *docset) {
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    mapid2word::iterator it;
    for (uint32_t k = 0; k < K; k++) {

        vector<pair<uint64_t, double> > words_probs;
        pair<uint64_t, double> word_prob;
        for (uint32_t w = 0; w < V; w++) {
            word_prob.first = w;
            word_prob.second = phi[k][w];
            words_probs.push_back(word_prob);
        }

        // quick sort to sort word-topic probability
        quicksort(words_probs, 0, words_probs.size() - 1);

        fprintf(fout, "Topic %dth: \n", k);
        for (uint32_t i = 0; i < twords; i++) {
            it = id2word.find(words_probs[i].first);
            if (it != id2word.end()) {
                fprintf(fout, "%s(%.3f)\t", (it->second).c_str(), words_probs[i].second);
            }
        }
        fprintf(fout, "\n\n");

        vector<pair<uint64_t, double> > docs_probs;
        pair<uint64_t, double> doc_prob;
        for (uint32_t m = 0; m < M; m++) {
            double p = theta[m][k];
            if ( p > 0.5 ) {
                doc_t *doc = docset_get_document_by_index(docset, m);
                const char *doc_title = doc_get_title(doc);
                doc_prob.first = (uint64_t)doc_title;
                doc_prob.second = p;
                docs_probs.push_back(doc_prob);
                //fprintf(fout, "(%.3f)[%d] %s\n", p, m, dname); 
                doc_detach(doc);
            }
        }

        if ( docs_probs.size() > 1 ) {
            quicksort(docs_probs, 0, docs_probs.size() - 1);
        }

        for ( uint32_t k = 0 ; k < docs_probs.size() ; k++ ){
            //fprintf(fout, "(%.3f)[%d] %s\n", docs_probs[k].second, k, docs_probs[k].first); 
            fprintf(fout, "(%.3f) %s\n", docs_probs[k].second, (const char *)docs_probs[k].first); 
        }

        fprintf(fout, "\n");
    }

    fclose(fout);

    return 0;
}

typedef struct model_doc_t{
    uint32_t id;
    uint32_t doc_id;
    std::string title;
    double topic_corr;
} model_doc_t;

typedef struct model_topic_t{
    uint32_t id;
    std::string title;
    uint32_t num_docs;
    model_doc_t **docs;
} model_topic_t;

static uint32_t max_docs = 100;

int save_datagraph(string filename, model_topic_t **topics, uint32_t num_topics)
{
    FILE * fout = fopen(filename.c_str(), "w");
    if (!fout) {
        printf("Cannot open file %s to save!\n", filename.c_str());
        return 1;
    }

    std::string head = " window['datagraph_data'] =\n \
 {'d3': {\n \
    'options':{'radius':'10','fontSize':'12','labelFontSize':'7','nodeResize':'count','nodeLabel':'label','outerRadius':'480'},\n \
    'data':{\n \
        'nodes':[\n"; 

    fprintf(fout, "%s", head.c_str());
    for (uint32_t k = 0 ; k < num_topics ; k++){
        model_topic_t *topic = topics[k];
        uint32_t num_docs = topic->num_docs;
        const char *topic_title = topic->title.c_str();
        fprintf(fout, "{\"name\":\"T%d\", \"count\":%d, \"group\":\"T%d\", \"label\":\"(%d)%s\"},\n", k, num_docs, k, k, topic_title);
    }

    for (uint32_t k = 0 ; k < num_topics ; k++ ){
        model_topic_t *topic = topics[k];
        uint32_t num_docs = topic->num_docs;
        for (uint32_t n = 0 ; n < num_docs && n < max_docs ; n++){
            model_doc_t *mdoc = topic->docs[n];
            uint32_t doc_cnt = mdoc->id;
            //uint32_t doc_id = mdoc->doc_id;
            //const char *doc_title = mdoc->title.c_str();
            uint32_t doc_links = 1;
            fprintf(fout, "{\"name\":\"D%d\", \"count\":%d, \"group\":\"T%d\", \"label\":\"\"},\n", doc_cnt + num_topics, doc_links, k);
            //fprintf(fout, "{\"name\":\"D%d\", \"count\":%d, \"group\":\"T%d\", \"label\":\"(%d)\"},\n", doc_cnt + num_topics, doc_links, k, doc_id);
            //fprintf(fout, "{\"name\":\"D%d\", \"count\":%d, \"group\":\"T%d\", \"label\":\"(%d)%s\"},\n", doc_cnt + num_topics, doc_links, k, doc_id, doc_title);
        }

    }

    // links

    std::string middle = "], \n\
        'links':[ \n";
    fprintf(fout, "%s", middle.c_str());

    //{"source":46,"target":47,"depth":6,"count":1}

    for (uint32_t k = 0 ; k < num_topics ; k++ ){
        model_topic_t *topic = topics[k];
        uint32_t topic_id = topic->id;
        uint32_t num_docs = topic->num_docs;
        for (uint32_t n = 0 ; n < num_docs && n < max_docs; n++){
            model_doc_t *mdoc = topic->docs[n];
            uint32_t doc_cnt = mdoc->id;
            uint32_t depth = 1;
            uint32_t doc_count = 1;
            fprintf(fout, "{\"source\":%d, \"target\":%d, \"depth\":%d, \"count\":%d},", 
                    topic_id, doc_cnt + num_topics, depth, doc_count);

        }

    }


    std::string tail = " ] \n\
            } \n\
        }};\n";

    fprintf(fout, "%s", tail.c_str());

    fclose(fout);

    printf("Save datagraph done.\n");

    return 0;
}

int model::save_model_datagraph(string filename, docset_t *docset) {
    uint32_t num_topics = K;
    model_topic_t **topics = (model_topic_t**)malloc(sizeof(model_topic_t*) * num_topics);
    memset(topics, 0, sizeof(model_topic_t*) * num_topics);

    uint32_t doc_cnt = 0;
    mapid2word::iterator it;
    for (uint32_t k = 0; k < K; k++) {

        vector<pair<uint64_t, double> > words_probs;
        pair<uint64_t, double> word_prob;
        for (uint32_t w = 0; w < V; w++) {
            word_prob.first = w;
            word_prob.second = phi[k][w];
            words_probs.push_back(word_prob);
        }

        // quick sort to sort word-topic probability
        quicksort(words_probs, 0, words_probs.size() - 1);

        model_topic_t *topic = (model_topic_t*)malloc(sizeof(model_topic_t));
        topics[k] = topic;
        topic->id = k;

        for (uint32_t i = 0; i < 5; i++) {
            it = id2word.find(words_probs[i].first);
            if (it != id2word.end()) {
                char buf[128];
                sprintf(buf, "%s(%.3f) ", (it->second).c_str(), words_probs[i].second);
                topic->title += std::string(buf);
            }
        }

        vector<pair<uint64_t, double> > docs_probs;
        pair<uint64_t, double> doc_prob;
        for (uint32_t m = 0; m < M; m++) {
            double p = theta[m][k];
            if ( p > 0.5 ) {
                doc_t *doc = docset_get_document_by_index(docset, m);
                doc_prob.first = (uint64_t)doc;
                doc_prob.second = p;
                docs_probs.push_back(doc_prob);
            }
        }

        size_t num_docs = docs_probs.size();
        topic->num_docs = num_docs;
        if ( num_docs > 1 ) {
            quicksort(docs_probs, 0, num_docs - 1);
        }

        if ( num_docs > 0 ){
            topic->docs = (model_doc_t**)malloc(sizeof(model_doc_t*) * num_docs);
            memset(topic->docs, 0, sizeof(model_doc_t*) *num_docs);
            for ( uint32_t n = 0 ; n < num_docs && n < max_docs ; n++ ){
                //fprintf(fout, "(%.3f)[%d] %s\n", docs_probs[k].second, k, docs_probs[k].first); 
                doc_t *doc = (doc_t*)docs_probs[n].first;
                const char *doc_title = doc_get_title(doc);
                double topic_corr = docs_probs[n].second;
                uint32_t doc_id = doc_get_id(doc);
                doc_detach(doc);

                model_doc_t *mdoc = (model_doc_t*)malloc(sizeof(model_doc_t));
                topic->docs[n] = mdoc;
                mdoc->id = doc_cnt++;
                mdoc->doc_id = doc_id;

                if (strlen(doc_title) < 20 )
                    mdoc->title = doc_title;
                else {
                    char tbuf[64];
                    memcpy(tbuf, doc_title, 20);
                    tbuf[20] = '\0';
                    mdoc->title = tbuf;
                }

                mdoc->topic_corr = topic_corr;
            }
        }
    }

    save_datagraph(filename, topics, num_topics);

    return 0;
}
#include "docset.h"

extern "C" {
    
    int ldac_gibbs_estimate(ldac_t *ldac, docset_t *docset, double alpha, double beta)
    {
        model *lda_mode = new model();
        lda_mode->init_est_from_docset(docset, ldac->NTOPICS, ldac->EM_MAX_ITER);

        lda_mode->alpha = alpha;
        lda_mode->beta = beta;
        lda_mode->twords = 20;

        lda_mode->estimate();


        lda_mode->save_model_tdocs(lda_mode->dir + lda_mode->model_name + ".tdocs", docset);
        lda_mode->save_model_datagraph(lda_mode->dir + "/html/" + lda_mode->model_name + "_data.js", docset);

        delete lda_mode;

        return 0;
    }

    int ldac_gibbs_inference(ldac_t *ldac)
    {
        return 0;
    }
}

