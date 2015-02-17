/**
 * @file   svd.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-01-28 17:53:18
 *
 * @brief
 *
 *
 */

#include <vector>
#include <math.h>
#include "svd.h"

std::vector<struct doc_cor> g_cor;

void AddCorrelation(uint32_t docid, double corrlation)
{
	struct doc_cor d_cor;
	d_cor.doc_id = docid;
	d_cor.correlation = corrlation;
	if (g_cor.size() == 0) {
        g_cor.push_back(d_cor);
	    return;
	}
    std::vector<struct doc_cor>::iterator it;
	int i=0;
	for (it=g_cor.begin();it<g_cor.end();it++,i++) {
	    if (fabs(corrlation) > fabs(g_cor[i].correlation)) {
            g_cor.insert(it,d_cor);
            return;
        }
	}
	if (it==g_cor.end()) {
		g_cor.push_back(d_cor);
	}

}
