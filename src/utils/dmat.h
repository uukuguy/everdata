/**
 * @file   dmat.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-30 14:17:38
 * 
 * @brief  
 * 
 * 
 */

#ifndef __DMAT_H__
#define __DMAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct dmat_t {
    uint32_t numRows;
    uint32_t numCols;
    double *values;

} dmat_t;

dmat_t *dmat_new(uint32_t numRows, uint32_t numCols);
void dmat_init(dmat_t *matrix, uint32_t numRows, uint32_t numCols);
void dmat_free(dmat_t *matrix);
void dmat_clear(dmat_t *matrix);

uint32_t dmat_get_rows(dmat_t *matrix);
uint32_t dmat_get_cols(dmat_t *matrix);
double *dmat_get_values(dmat_t *matrix);
void dmat_fill(dmat_t *matrix, double value);

int dmat_save_to_csv(dmat_t *matrix, const char *filename);
dmat_t *dmat_load_from_csv(const char *filename, uint32_t numRows, uint32_t numCols);

#ifdef __cplusplus
}
#endif

#endif // __DMAT_H__

