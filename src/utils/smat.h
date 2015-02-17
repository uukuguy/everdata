/**
 * @file   smat.h
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 16:58:47
 * 
 * @brief  
 * 
 * 
 */

#ifndef __SMAT_H__
#define __SMAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Harwell-Boeing sparse matrix. */
typedef struct smat_t {
  uint32_t numRows;
  uint32_t numCols;
  uint32_t totalNonZeroValues;     /* Total non-zero entries. */
  uint32_t *pointr;  /* For each col (plus 1), index of first non-zero entry. */
  uint32_t *rowind;  /* For each nz entry, the row index. */
  double *values; /* For each nz entry, the value. */
} smat_t;

smat_t *smat_new(uint32_t numRows, uint32_t numCols, uint32_t totalNonZeroValues);
void smat_free(smat_t *matrix);
void smat_clear(smat_t *matrix);

uint32_t smat_get_rows(smat_t *smat);
uint32_t smat_get_cols(smat_t *smat);
uint32_t smat_get_totalNonZeroValues(smat_t *smat);

int smat_save_to_file(smat_t *matrix, const char *filename);
smat_t *smat_load_from_file(const char *filename);

#define SMAT_LOOP_FOREACH_COL(matrix, numCols) \
    { \
    uint32_t v = 0; \
    for ( uint32_t col = 0 ; col < numCols ; col++ ){ \

#define SMAT_LOOP_FOREACH_ROW(matrix, numRows) \
        for ( ; v < matrix->pointr[col + 1]; v++) { \
            uint32_t row = matrix->rowind[v]; \
            double value = matrix->values[v]; \

#define SMAT_LOOP_END_ROW() \
        }

#define SMAT_LOOP_END_COL() \
    } \
    }

#define SMAT_LOOP_BEGIN(matrix, numRows, numCols) \
    { \
    uint32_t v = 0; \
    for ( uint32_t col = 0 ; col < numCols ; col++ ){ \
        for ( ; v < matrix->pointr[col + 1]; v++) { \
            uint32_t row = matrix->rowind[v]; \
            double value = matrix->values[v]; \

#define SMAT_LOOP_END() \
        } \
    } \
    } \

#ifdef __cplusplus
}
#endif

#endif // __SMAT_H__

