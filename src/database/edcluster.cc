/**
 * @file   edcluster.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-03-03 13:16:48
 *
 * @brief
 *
 *
 */

#include "edcluster.h"
#include <stdlib.h>
#include <memory.h>

typedef struct edcluster_t {
    int version;
} edcluster_t;

edcluster_t *edcluster_new(void)
{
    edcluster_t *cluster = (edcluster_t*)malloc(sizeof(edcluster_t));
    memset(cluster, 0, sizeof(edcluster_t));

    return cluster;
}

void edcluster_free(edcluster_t *cluster)
{
    free(cluster);
}
