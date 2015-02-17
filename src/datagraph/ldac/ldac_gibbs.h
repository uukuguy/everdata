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

#ifndef	__LDAC_GIBBS_H__
#define	__LDAC_GIBBS_H__

#include <map>
#include <string>
#include <stdint.h>

typedef struct docset_t docset_t;

using namespace std;
// map of words/terms [string => uint32_t]
typedef map<string, uint32_t> mapword2id;
// map of words/terms [uint32_t => string]
typedef map<uint32_t, string> mapid2word;

class dataset;

typedef struct ldac_t ldac_t;

#define	MODEL_STATUS_UNKNOWN	0
#define	MODEL_STATUS_EST	1
#define	MODEL_STATUS_ESTC	2
#define	MODEL_STATUS_INF	3

// LDA model
class model {
public:
    // fixed options
    string wordmapfile;		// file that contains word map [string -> integer id]
    string trainlogfile;	// training log file
    string tassign_suffix;	// suffix for topic assignment file
    string theta_suffix;	// suffix for theta file
    string phi_suffix;		// suffix for phi file
    string others_suffix;	// suffix for file containing other parameters
    string twords_suffix;	// suffix for file containing words-per-topics

    string dir;			// model directory
    string dfile;		// data file    
    string model_name;		// model name
    int model_status;		// model status:
				// MODEL_STATUS_UNKNOWN: unknown status
				// MODEL_STATUS_EST: estimating from scratch
				// MODEL_STATUS_ESTC: continue to estimate the model from a previous one
				// MODEL_STATUS_INF: do inference

    dataset * ptrndata;	// pointer to training dataset object
    dataset * pnewdata; // pointer to new dataset object

    mapid2word id2word; // word map [int => string]
    
    // --- model parameters and variables ---    
    uint32_t M; // dataset size (i.e., number of docs)
    uint32_t V; // vocabulary size
    uint32_t K; // number of topics
    double alpha, beta; // LDA hyperparameters 
    uint32_t niters; // number of Gibbs sampling iterations
    uint32_t liter; // the iteration at which the model was saved
    uint32_t savestep; // saving period
    uint32_t twords; // print out top words per each topic
    uint32_t withrawstrs;

    double * p; // temp variable for sampling
    uint32_t ** z; // topic assignments for words, size M x doc.size()
    uint32_t ** nw; // cwt[i][j]: number of instances of word/term i assigned to topic j, size V x K
    uint32_t ** nd; // na[i][j]: number of words in document i assigned to topic j, size M x K
    uint32_t * nwsum; // nwsum[j]: total number of words assigned to topic j, size K
    uint32_t * ndsum; // nasum[i]: total number of words in document i, size M
    double ** theta; // theta: document-topic distributions, size M x K
    double ** phi; // phi: topic-word distributions, size K x V
    
    // for inference only
    uint32_t inf_liter;
    uint32_t newM;
    uint32_t newV;
    uint32_t ** newz;
    uint32_t ** newnw;
    uint32_t ** newnd;
    uint32_t * newnwsum;
    uint32_t * newndsum;
    double ** newtheta;
    double ** newphi;
    // --------------------------------------
    
    model() {
	set_default_values();
    }
          
    ~model();
    
    // set default values for variables
    void set_default_values();   

    // parse command line to get options
    int parse_args(int argc, char ** argv);
    
    // initialize the model
    int init(int argc, char ** argv);
    
    // load LDA model to continue estimating or to do inference
    int load_model(string model_name);
    
    // save LDA model to files
    // model_name.tassign: topic assignments for words in docs
    // model_name.theta: document-topic distributions
    // model_name.phi: topic-word distributions
    // model_name.others: containing other parameters of the model (alpha, beta, M, V, K)
    int save_model(string model_name);
    int save_model_tassign(string filename);
    int save_model_theta(string filename);
    int save_model_phi(string filename);
    int save_model_others(string filename);
    int save_model_twords(string filename);
    int save_model_tdocs(string filename, docset_t *docset);
    int save_model_datagraph(string filename, docset_t *docset);
    
    // saving inference outputs
    int save_inf_model(string model_name);
    int save_inf_model_tassign(string filename);
    int save_inf_model_newtheta(string filename);
    int save_inf_model_newphi(string filename);
    int save_inf_model_others(string filename);
    int save_inf_model_twords(string filename);
    
    int init_est_from_docset(docset_t *docset, uint32_t NTOPICS, uint32_t EM_MAX_ITER);
    // init for estimation
    int init_est();
    int init_estc();
	
    // estimate LDA model using Gibbs sampling
    void estimate();
    int sampling(uint32_t m, uint32_t n);
    void compute_theta();
    void compute_phi();
    
    // init for inference
    int init_inf();
    // inference for new (unseen) data based on the estimated LDA model
    void inference();
    int inf_sampling(uint32_t m, uint32_t n);
    void compute_newtheta();
    void compute_newphi();
};

#endif // __LDAC_GIBBS_H__

