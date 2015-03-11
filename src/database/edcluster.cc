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
#include "crush.hpp"
#include <stdlib.h>
#include <memory.h>
#include <vector>
#include <list>
#include <map>
#include <utility>

typedef struct edcluster_t {
    int version;

    CRush *m_rush;

} edcluster_t;

edcluster_t *edcluster_new(void)
{
    edcluster_t *cluster = (edcluster_t*)malloc(sizeof(edcluster_t));
    memset(cluster, 0, sizeof(edcluster_t));

    cluster->m_rush = new CRush();

    return cluster;
}

void edcluster_free(edcluster_t *cluster)
{
    delete cluster->m_rush;
    free(cluster);
}

int edcluster_add_type(edcluster_t *cluster, int type_id, const char *type_name)
{
    CRush *rush = cluster->m_rush;
    rush->set_type_name(type_id, type_name);
    return 0;
}

const char *edcluster_get_type_name(edcluster_t *cluster, int type_id)
{
    CRush *rush = cluster->m_rush;
    return rush->get_type_name(type_id);
}

int edcluster_get_type_id(edcluster_t *cluster, const char *type_name)
{
    CRush *rush = cluster->m_rush;
    return rush->get_type_id(type_name);
}

