/**
 * @file   smat.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-12 17:04:19
 * 
 * @brief  
 * 
 * 
 */

#include "smat.h"
#include <stdlib.h>
#include <memory.h>

/* ---------------- smat ---------------- */
smat_t *smat_new(uint32_t numRows, uint32_t numCols, uint32_t totalNonZeroValues)
{
    smat_t *matrix = (smat_t*)malloc(sizeof(smat_t));
    memset(matrix, 0, sizeof(smat_t));

    matrix->numRows = numRows;
    matrix->numCols = numCols;
    matrix->totalNonZeroValues = totalNonZeroValues;

    matrix->pointr = (uint32_t*)malloc(sizeof(uint32_t) * (numCols + 1));
    matrix->rowind = (uint32_t*)malloc(sizeof(uint32_t) * totalNonZeroValues);
    matrix->values = (double*)malloc(sizeof(double) * totalNonZeroValues);

    return matrix;
}

void smat_free(smat_t *matrix)
{
    smat_clear(matrix);
    free(matrix);
}

void smat_clear(smat_t *matrix)
{
    free(matrix->pointr);
    free(matrix->rowind);
    free(matrix->values);

    matrix->numRows = 0;
    matrix->numCols = 0;
    matrix->totalNonZeroValues = 0;
}

uint32_t smat_get_rows(smat_t *smat)
{
    return smat->numRows;
}

uint32_t smat_get_cols(smat_t *smat)
{
    return smat->numCols;
}

uint32_t smat_get_totalNonZeroValues(smat_t *smat)
{
    return smat->totalNonZeroValues;
}

#include <fstream>
int smat_save_to_file(smat_t *matrix, const char *filename) 
{
    uint32_t numRows = smat_get_rows(matrix);
    uint32_t numCols = smat_get_cols(matrix);
    uint32_t totalNonZeroValues = smat_get_totalNonZeroValues(matrix);

    std::fstream out(filename, std::ios::out);
    out << numRows << " " 
        << numCols << " "
        << totalNonZeroValues 
        << std::endl;

    uint32_t v = 0;
    for ( uint32_t col = 0 ; col < numCols ; col++ ){
        uint32_t numNonZeroValues = matrix->pointr[col + 1] - matrix->pointr[col];
        out << numNonZeroValues << std::endl;
        for ( ; v < matrix->pointr[col + 1]; v++) {
            out << matrix->rowind[v] << " " << matrix->values[v] << " ";
        }
        out << std::endl;
    }
    return true;
}

smat_t *smat_load_from_file(const char *filename)
{
    uint32_t numRows = 0;
    uint32_t numCols = 0;
    uint32_t totalNonZeroValues = 0;

    std::fstream in(filename, std::ios::in);
    in >> numRows >> numCols >> totalNonZeroValues;

    smat_t *matrix = smat_new(numRows, numCols, totalNonZeroValues);
    uint32_t v = 0;
    for ( uint32_t c = 0 ; c < numCols ; c++ ){
        uint32_t numNonZeroValues = 0;
        in >> numNonZeroValues;

        matrix->pointr[c] = v;
        for ( uint32_t n = 0 ; n < numNonZeroValues ; n++, v++){
            uint32_t rowIndex = 0;
            double value = 0.0;
            in >> rowIndex >> value;

            *(matrix->rowind + v) = rowIndex;
            *(matrix->values + v) = value;
        }
    }
    matrix->pointr[numCols] = totalNonZeroValues;

    return matrix;
}


