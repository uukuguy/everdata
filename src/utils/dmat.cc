/**
 * @file   dmat.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-12-30 14:19:26
 * 
 * @brief  
 * 
 * 
 */

#include "dmat.h"
#include <stdlib.h>
#include <memory.h>

dmat_t *dmat_new(uint32_t numRows, uint32_t numCols)
{
    dmat_t *matrix = (dmat_t*)malloc(sizeof(dmat_t));
    memset(matrix, 0, sizeof(dmat_t));

    dmat_init(matrix, numRows, numCols);

    return matrix;
}

void dmat_init(dmat_t *matrix, uint32_t numRows, uint32_t numCols)
{
    if ( numRows > 0 && numCols > 0 ){
        matrix->numRows = numRows;
        matrix->numCols = numCols;
        matrix->values = (double*)malloc(sizeof(double) * numRows * numCols);
    }
}

void dmat_free(dmat_t *matrix)
{
    dmat_clear(matrix);
    free(matrix);
}

void dmat_clear(dmat_t *matrix)
{
    if ( matrix->values != NULL )
        free(matrix->values);
    memset(matrix, 0, sizeof(dmat_t));
}

double *dmat_get_values(dmat_t *matrix)
{
    return matrix->values;
}

void dmat_fill(dmat_t *matrix, double value)
{
    memset(matrix->values, value, matrix->numRows * matrix->numCols);
}

uint32_t dmat_get_rows(dmat_t *matrix)
{
    return matrix->numRows;
}

uint32_t dmat_get_cols(dmat_t *matrix)
{
    return matrix->numCols;
}

#include <iostream>
#include <iomanip>
#include <fstream>
int dmat_save_to_csv(dmat_t *matrix, const char *filename) 
{
    std::fstream out(filename, std::ios::out);

    out << std::setprecision(6);

    uint32_t numRows = dmat_get_rows(matrix);
    uint32_t numCols = dmat_get_cols(matrix);
    double *values = dmat_get_values(matrix);
    for ( uint32_t row = 0 ; row < numRows ; row++ ){
        for ( uint32_t col = 0 ; col < numCols ; col++ ){
            out << *values;
            if ( col < numCols - 1 )
                out << ",";
        }
        out << std::endl;

        values++;
    }

    return 0;
}

dmat_t *dmat_load_from_csv(const char *filename, uint32_t numRows, uint32_t numCols)
{
    dmat_t *matrix = dmat_new(numRows, numCols);
    double *values = dmat_get_values(matrix);

    std::fstream in(filename, std::ios::in);
    for ( uint32_t row = 0 ; row < numRows ; row++ ){
        for ( uint32_t col = 0 ; col < numCols ; col++ ){
            in >> *values;
            values++;
        }
    }

    return 0;
}

