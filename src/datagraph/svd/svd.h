/**
 * @file   svd.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-01-28 17:53:00
 *
 * @brief
 *
 *
 */
#ifndef __SVD_H__
#define __SVD_H__

typedef struct doc_cor
{
	int doc_id;
	double correlation; //calculate by CompareVector()
} doc_cor;

void AddCorrelation(uint32_t docid, double corrlation);

extern std::vector<struct doc_cor> g_cor;

#endif // __SVD_H__

