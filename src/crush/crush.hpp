/**
 * @file   crush.hpp
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-02-27 19:18:09
 *
 * @brief
 *
 *
 */

#ifndef __CRUSH_HPP__
#define __CRUSH_HPP__

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <utility>

extern "C"{
#include "crush.h"
#include "mapper.h"
#include "builder.h"
#include "hash.h"
}

/*

# devices
device 1 osd001
device 2 osd002
device 3 osd003 down   # same as offload 1.0
device 4 osd004 offload 0       # 0.0 -> normal, 1.0 -> failed
device 5 osd005 offload 0.1
device 6 osd006 offload 0.1

# hierarchy
level 0 osd   # 'device' is actually the default for 0
level 2 cab
level 3 row
level 10 pool

# item
cab root {
       id -1         # optional
       alg tree     # required
       item osd001
       item osd002 weight 600 pos 1
       item osd003 weight 600 pos 0
       item osd004 weight 600 pos 3
       item osd005 weight 600 pos 4
}

# rules
rule normal {
     # these are required.
     pool 0
     type replicated
     min_size 1
     max_size 4
     # need 1 or more of these.
     step take root
     step choose firstn 0 type osd
     step emit
}

rule {
     pool 1
     type erasure
     min_size 3
     max_size 6
     step take root
     step choose indep 0 type osd
     step emit
}

*/

class CRush
{
    private:

        struct crush_map *m_crush;

    private:

        std::map<int32_t, std::string> level_map; /* bucket/device level names */
        std::map<int32_t, std::string> name_map; /* bucket/device names */
        std::map<int32_t, std::string> rule_name_map;
        mutable bool have_rmaps;
        mutable std::map<std::string, int> level_rmap, name_rmap, rule_name_rmap;
        void build_rmaps() const {
            if (have_rmaps) return;
            build_rmap(level_map, level_rmap);
            build_rmap(name_map, name_rmap);
            build_rmap(rule_name_map, rule_name_rmap);
            have_rmaps = true;
        }
        void build_rmap(const std::map<int, std::string> &f, std::map<std::string, int> &r) const {
            r.clear();
            for (std::map<int, std::string>::const_iterator p = f.begin(); p != f.end(); ++p)
                r[p->second] = p->first;
        }

    public:
        // bucket levels
        int get_num_level_names() const {
            return level_map.size();
        }
        int get_level_id(const std::string& name) const {
            build_rmaps();
            if (level_rmap.count(name))
                return level_rmap[name];
            return -1;
        }
        const char *get_level_name(int t) const {
            std::map<int, std::string>::const_iterator p = level_map.find(t);
            if (p != level_map.end())
                return p->second.c_str();
            return 0;
        }
        void set_level_name(int i, const std::string& name) {
            level_map[i] = name;
            if (have_rmaps)
                level_rmap[name] = i;
        }

        // item/bucket names
        bool name_exists(const std::string& name) const {
            build_rmaps();
            return name_rmap.count(name);
        }
        bool item_exists(int i) const {
            return name_map.count(i);
        }
        int get_item_id(const std::string& name) const {
            build_rmaps();
            if (name_rmap.count(name))
                return name_rmap[name];
            return 0;  /* hrm */
        }
        const char *get_item_name(int t) const {
            std::map<int, std::string>::const_iterator p = name_map.find(t);
            if (p != name_map.end())
                return p->second.c_str();
            return 0;
        }
        int set_item_name(int i, const std::string& name) {
            name_map[i] = name;
            if (have_rmaps)
                name_rmap[name] = i;
            return 0;
        }

        // rule names
        bool rule_exists(const std::string &name) const {
            build_rmaps();
            return rule_name_rmap.count(name);
        }
        int get_rule_id(const std::string &name) const {
            build_rmaps();
            if (rule_name_rmap.count(name))
                return rule_name_rmap[name];
            return -ENOENT;
        }
        const char *get_rule_name(int t) const {
            std::map<int, std::string>::const_iterator p = rule_name_map.find(t);
            if (p != rule_name_map.end())
                return p->second.c_str();
            return 0;
        }
        void set_rule_name(int i, const std::string& name) {
            rule_name_map[i] = name;
            if (have_rmaps)
                rule_name_rmap[name] = i;
        }

    public:
        CRush();
        ~CRush();
        void dump(const char *filename) const;

        void create() {
            if ( m_crush != NULL ) crush_destroy(m_crush);
            m_crush = crush_create();
            have_rmaps = false;
            set_tunables_default();
        }

        // tunables
        void set_tunables_argonaut() {
            m_crush->choose_local_tries = 2;
            m_crush->choose_local_fallback_tries = 5;
            m_crush->choose_total_tries = 19;
            m_crush->chooseleaf_descend_once = 0;
            m_crush->chooseleaf_vary_r = 0;
        }
        void set_tunables_bobtail() {
            m_crush->choose_local_tries = 0;
            m_crush->choose_local_fallback_tries = 0;
            m_crush->choose_total_tries = 50;
            m_crush->chooseleaf_descend_once = 1;
            m_crush->chooseleaf_vary_r = 0;
        }
        void set_tunables_firefly() {
            m_crush->choose_local_tries = 0;
            m_crush->choose_local_fallback_tries = 0;
            m_crush->choose_total_tries = 50;
            m_crush->chooseleaf_descend_once = 1;
            m_crush->chooseleaf_vary_r = 1;
        }
        void set_tunables_hammer() {
            m_crush->choose_local_tries = 0;
            m_crush->choose_local_fallback_tries = 0;
            m_crush->choose_total_tries = 50;
            m_crush->chooseleaf_descend_once = 1;
            m_crush->chooseleaf_vary_r = 1;
        }

        void set_tunables_legacy() {
            set_tunables_argonaut();
            m_crush->straw_calc_version = 0;
        }
        void set_tunables_optimal() {
            set_tunables_hammer();
            m_crush->straw_calc_version = 1;
        }
        void set_tunables_default() {
            set_tunables_bobtail();
            m_crush->straw_calc_version = 1;
        }

        void finalize() {
            crush_finalize(m_crush);
        }

        // rules
    private:

        crush_rule *get_rule(uint32_t ruleno) const {
            if ( ruleno < m_crush->max_rules) {
                return m_crush->rules[ruleno];
            }
            return NULL;
        }

        crush_rule_step *get_rule_step(uint32_t ruleno, uint32_t step) const {
            crush_rule *rule = get_rule(ruleno);
            if ( rule != NULL && step < rule->len ) {
                return &rule->steps[step];
            }
            return NULL;
        }
    public:

        int32_t get_max_rules() const;
        bool rule_exists(int32_t ruleno) const;
        bool ruleset_exists(uint8_t ruleset) const;

        int add_rule(const std::string &rule_name, int ruleset, int steps, int rule_type, int min_size, int max_size) {
            crush_rule *rule = crush_make_rule(steps, ruleset, rule_type, min_size, max_size);
            int ruleno = crush_add_rule(m_crush, rule, -1);
            if (ruleno >= 0 ) {
                set_rule_name(ruleno, rule_name);
                have_rmaps = false;
            }
            return ruleno;
        }

        int set_rule_step(unsigned ruleno, unsigned step, int op, int arg1, int arg2) {
            crush_rule *rule = get_rule(ruleno);
            if ( rule != NULL ) {
                crush_rule_set_step(rule, step, op, arg1, arg2);
                return 0;
            }
            return -1;
        }

        int create_sample_rule(const std::string &rule_name, const std::string &rule_mode, int rule_type, int ruleset, const std::string &root_item, const std::string &level_name);

        void do_rule(int ruleno, int x, std::vector<int>& out, int maxout, const std::vector<uint32_t>& weights) const;

        // buckets
    public:

        // bucketno = 0 for auto id.
        int add_bucket(int bucketno, int alg, int hash, int type, int size,
                int *items, int *weights, int *idout) {
            if (type == 0)
                return -EINVAL;
            crush_bucket *b = crush_make_bucket(m_crush, alg, hash, type, size, items, weights);
            return crush_add_bucket(m_crush, bucketno, b, idout);
        }

        int move_bucket(int bucketno, const std::map<std::string, std::string>& loc);
        int link_bucket(int id, const std::map<std::string, std::string>& loc);

        int create_or_move_item(int item, float weight, std::string name, const std::map<std::string, std::string>& loc);

        // loc: typename -> bucketname
        int insert_item(int item, float weight, std::string name, int alg, int hash, const std::map<std::string, std::string>& loc);

        int update_item(int item, float weight, std::string name, const std::map<std::string, std::string>& loc);
        int remove_item(int item, bool unlink_only);

        int get_item_weight(int id) const;
        float get_item_weightf(int id) const {
            return (float)get_item_weight(id) / (float)0x10000;
        }

        int get_item_weight_in_loc(int id, const std::map<std::string, std::string> &loc);
        float get_item_weightf_in_loc(int id, const std::map<std::string, std::string> &loc) {
            return (float)get_item_weight_in_loc(id, loc) / (float)0x10000;
        }

        int adjust_item_weight(int id, int weight);
        int adjust_item_weightf(int id, float weight) {
            return adjust_item_weight(id, (int)(weight * (float)0x10000));
        }

        int adjust_item_weight_in_loc(int id, int weight, const std::map<std::string, std::string>& loc);
        int adjust_item_weightf_in_loc(int id, float weight, const std::map<std::string, std::string>& loc) {
            return adjust_item_weight_in_loc(id, (int)(weight * (float)0x10000), loc);
        }

        int adjust_subtree_weight(int id, int weight) {
            //ldout(cct, 5) << "adjust_item_weight " << id << " weight " << weight << dendl;
            crush_bucket *b = get_bucket(id);
            if ( b == NULL ) return -1;
            int changed = 0;
            std::list<crush_bucket*> q;
            q.push_back(b);
            while (!q.empty()) {
                b = q.front();
                q.pop_front();
                for (unsigned i=0; i<b->size; ++i) {
                    int n = b->items[i];
                    if (n >= 0) {
                        crush_bucket_adjust_item_weight(m_crush, b, n, weight);
                    } else {
                        crush_bucket *sub = get_bucket(n);
                        if ( sub == NULL )
                            continue;
                        q.push_back(sub);
                    }
                }
            }
            return changed;
        }
        int adjust_subtree_weightf(int id, float weight) {
            return adjust_subtree_weight(id, (int)(weight * (float)0x10000));
        }

        void reweight() {
            std::set<int> roots;
            find_roots(roots);
            for (std::set<int>::iterator p = roots.begin(); p != roots.end(); ++p) {
                if (*p >= 0) continue;
                crush_bucket *b = get_bucket(*p);
                //ldout(cct, 5) << "reweight bucket " << *p << dendl;
                int r = crush_reweight_bucket(m_crush, b);
            }
        }

        bool check_item_loc(int item, const std::map<std::string, std::string>& loc, int *iweight);
        bool check_item_loc(int item, const std::map<std::string, std::string>& loc, float *weight) {
            int iweight;
            bool ret = check_item_loc(item, loc, &iweight);
            if (weight)
                *weight = (float)iweight / (float)0x10000;
            return ret;
        }

        crush_bucket *get_bucket(int id) const {
            unsigned int pos = (unsigned int)(-1 - id);
            unsigned int max_buckets = m_crush->max_buckets;
            if ( pos < max_buckets ) {
                crush_bucket *b = m_crush->buckets[pos];
                return b;
            }
            return NULL;
        }

    private:

        bool _search_item_exists(int item) const {
            for (int i = 0; i < m_crush->max_buckets; i++) {
                if (!m_crush->buckets[i])
                    continue;
                crush_bucket *b = m_crush->buckets[i];
                for (unsigned j=0; j<b->size; ++j) {
                    if (b->items[j] == item)
                        return true;
                }
            }
            return false;
        }

        bool _maybe_remove_last_instance(int item, bool unlink_only) {
            // last instance?
            if (_search_item_exists(item)) {
                return false;
            }

            if (item < 0 && !unlink_only) {
                crush_bucket *t = get_bucket(item);
                //ldout(cct, 5) << "_maybe_remove_last_instance removing bucket " << item << dendl;
                crush_remove_bucket(m_crush, t);
            }
            if ((item >= 0 || !unlink_only) && name_map.count(item)) {
                //ldout(cct, 5) << "_maybe_remove_last_instance removing name for item " << item << dendl;
                name_map.erase(item);
                have_rmaps = false;
            }
            return true;
        }

        int detach_bucket(int item) {
            if (item >= 0)
                return (-EINVAL);

            // get the bucket's weight
            crush_bucket *b = get_bucket(item);
            if ( b == NULL ) return -EINVAL;
            unsigned bucket_weight = b->weight;

            // get where the bucket is located. return <level_name, item_name>.
            std::pair<std::string, std::string> bucket_location = get_immediate_parent(item);

            // get the id of the parent bucket
            int parent_id = get_item_id(bucket_location.second);

            // get the parent bucket
            crush_bucket *parent_bucket = get_bucket(parent_id);

            if (parent_bucket != NULL){
                // zero out the bucket weight
                crush_bucket_adjust_item_weight(m_crush, parent_bucket, item, 0);
                adjust_item_weight(parent_bucket->id, parent_bucket->weight);

                // remove the bucket from the parent
                crush_bucket_remove_item(m_crush, parent_bucket, item);
            } else {
                return -1;
            }

            // check that we're happy
            //int test_weight = 0;
            //std::map<std::string, std::string> test_location;
            //test_location[ bucket_location.first ] = (bucket_location.second);

            //bool successful_detach = !(check_item_loc(cct, item, test_location, &test_weight));
            //assert(successful_detach);
            //assert(test_weight == 0);

            return bucket_weight;
        }

        // <level_name, item_name>
        std::pair<std::string, std::string> get_immediate_parent(int id, int *_ret = NULL) const {
            std::pair <std::string, std::string> loc;
            int ret = -ENOENT;

            for (int bidx = 0; bidx < m_crush->max_buckets; bidx++) {
                crush_bucket *b = m_crush->buckets[bidx];
                if (b == NULL) continue;
                for (uint32_t i = 0; i < b->size; i++)
                    if (b->items[i] == id) {
                        std::map<int32_t, std::string>::const_iterator it = level_map.find(b->type);
                        std::string parent_bucket_level = it->second;
                        std::map<int32_t, std::string>::const_iterator it1 = name_map.find(b->id);
                        std::string parent_bucket_name = it1->second;
                        loc = std::make_pair(parent_bucket_level, parent_bucket_name);
                        ret = 0;
                        break;
                    }
            }

            if (_ret)
                *_ret = ret;

            return loc;
        }

        // devices
    public:
        int get_max_devices() const {
            return m_crush->max_devices;
        }

    private:

        std::map<std::string, std::string> get_full_location(int id) const {
            std::vector<std::pair<std::string, std::string> > full_location_ordered;
            std::map<std::string, std::string> full_location;

            get_full_location_ordered(id, full_location_ordered);

            std::copy(full_location_ordered.begin(),
                    full_location_ordered.end(),
                    std::inserter(full_location, full_location.begin()));

            return full_location;
        }

        int get_full_location_ordered(int id, std::vector<std::pair<std::string, std::string> >& path) const {
            if (!item_exists(id))
                return -ENOENT;
            int cur = id;
            int ret;
            while (true) {
                std::pair<std::string, std::string> parent_coord = get_immediate_parent(cur, &ret);
                if (ret != 0)
                    break;
                path.push_back(parent_coord);
                cur = get_item_id(parent_coord.second);
            }
            return 0;
        }


        std::map<int, std::string> get_parent_hierarchy(int id) const {
            std::map<int, std::string> parent_hierarchy;
            std::pair<std::string, std::string> parent_coord = get_immediate_parent(id);
            int parent_id;

            // get the integer type for id and create a counter from there

            const crush_bucket *b = get_bucket(id);
            if ( b == NULL ) return parent_hierarchy;
            int type_counter = b->type;

            // if we get a negative type then we can assume that we have an OSD
            // change behavior in get_item_type FIXME
            if (type_counter < 0)
                type_counter = 0;

            // read the type map and get the name of the type with the largest ID
            int high_type = 0;
            for ( std::map<int, std::string>::const_iterator it = level_map.begin(); it != level_map.end(); ++it){
                if ( (*it).first > high_type )
                    high_type = (*it).first;
            }

            parent_id = get_item_id(parent_coord.second);

            while (type_counter < high_type) {
                type_counter++;
                parent_hierarchy[ type_counter ] = parent_coord.first;

                if (type_counter < high_type){
                    // get the coordinate information for the next parent
                    parent_coord = get_immediate_parent(parent_id);
                    parent_id = get_item_id(parent_coord.second);
                }
            }
            return parent_hierarchy;
        }

        int get_children(int id, std::list<int> *children) const {
            // leaf?
            if (id >= 0) {
                return 0;
            }

            crush_bucket *b = get_bucket(id);
            if (!b) {
                return -ENOENT;
            }

            for (unsigned n=0; n<b->size; n++) {
                children->push_back(b->items[n]);
            }
            return b->size;
        }

        bool subtree_contains(int root, int item) const {
            if (root == item)
                return true;

            if (root >= 0)
                return false;  // root is a leaf

            const crush_bucket *b = get_bucket(root);
            if (!b)
                return false;

            for (unsigned j=0; j<b->size; j++) {
                if (subtree_contains(b->items[j], item))
                    return true;
            }
            return false;
        }

        void find_roots(std::set<int>& roots) const {
            for (int i = 0; i < m_crush->max_buckets; i++) {
                if (!m_crush->buckets[i]) continue;
                crush_bucket *b = m_crush->buckets[i];
                if (!_search_item_exists(b->id))
                    roots.insert(b->id);
            }
        }

};

#endif // __CRUSH_HPP__

