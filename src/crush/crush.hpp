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

/*

# devices
device 1 osd001
device 2 osd002
device 3 osd003 down   # same as offload 1.0
device 4 osd004 offload 0       # 0.0 -> normal, 1.0 -> failed
device 5 osd005 offload 0.1
device 6 osd006 offload 0.1

# hierarchy
type 0 osd   # 'device' is actually the default for 0
type 2 cab
type 3 row
type 10 pool

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

struct crush_bucket;
struct crush_rule;
struct crush_rule_step;

class CRush
{
    private:

        struct crush_map *m_crush;

    private:

        std::map<int32_t, std::string> type_map; /* bucket/device type names */
        std::map<int32_t, std::string> name_map; /* bucket/device names */
        std::map<int32_t, std::string> rule_name_map;
        mutable bool have_rmaps;
        mutable std::map<std::string, int> type_rmap, name_rmap, rule_name_rmap;

        void build_rmaps() const;
        void build_rmap(const std::map<int, std::string> &f, std::map<std::string, int> &r) const;

    public:
        // types
        int get_max_types() const;
        int get_type_id(const std::string& name) const;
        const char *get_type_name(int type_id) const;
        void set_type_name(int type_id, const std::string& name);

        // item/bucket names
        bool name_exists(const std::string& name) const;
        bool item_exists(int i) const;
        int get_item_id(const std::string& name) const;
        const char *get_item_name(int t) const;
        int set_item_name(int i, const std::string& name);

        // rule names
        bool ruleset_exists(uint8_t ruleset) const;
        bool rule_exists(int32_t ruleno) const;
        bool rule_exists(const std::string &name) const;
        int get_rule_id(const std::string &name) const;
        const char *get_rule_name(int ruleid) const;
        void set_rule_name(int ruleid, const std::string& name);

    private:
        void create();

    public:
        CRush();
        ~CRush();
        void dump(const char *filename) const;
        void finalize();

        // tunables
    private:
        void set_tunables_argonaut();
        void set_tunables_bobtail();
        void set_tunables_firefly();
        void set_tunables_hammer();

    public:
        void set_tunables_legacy();
        void set_tunables_optimal();
        void set_tunables_default();


    // ---------------- rules ----------------
    private:

        crush_rule *get_rule(uint32_t ruleno) const;
        crush_rule_step *get_rule_step(uint32_t ruleno, uint32_t step) const;

    public:

        int add_rule(const std::string &rule_name, int ruleset, int steps, int rule_type, int min_size, int max_size);
        int set_rule_step(unsigned ruleno, unsigned step, int op, int arg1, int arg2);
        void do_rule(int ruleno, int x, std::vector<int>& out, int maxout, const std::vector<uint32_t>& weights) const;

        int create_sample_rule(const std::string &rule_name, const std::string &rule_mode, int rule_type, int ruleset, const std::string &root_item, const std::string &type_name);

        int32_t get_max_rules() const;

    // ---------------- buckets ----------------
    public:

        // bucketno = 0 for auto id.
        int add_bucket(int bucket_id, int type, int size, int *items, int *weights, int *idout, int alg = 0);
        int move_bucket(int bucket_id, const std::map<std::string, std::string>& loc);
        int link_bucket(int bucket_id, const std::map<std::string, std::string>& loc);
        crush_bucket *get_bucket(int bucket_id) const;

        int create_or_move_item(int itemid, float weight, std::string name, const std::map<std::string, std::string>& loc);

        // loc: typename -> bucketname
        int insert_item(int itemid, float weight, std::string name, const std::map<std::string, std::string>& loc);

        int update_item(int itemid, float weight, std::string name, const std::map<std::string, std::string>& loc);
        int remove_item(int itemid, bool unlink_only);

        int get_max_devices() const;

        // -------- bucket/item weight --------

        int get_item_weight(int id) const;
        float get_item_weightf(int id) const;

        int get_item_weight_in_loc(int id, const std::map<std::string, std::string> &loc);
        float get_item_weightf_in_loc(int id, const std::map<std::string, std::string> &loc);

        int adjust_item_weight(int id, int weight);
        int adjust_item_weightf(int id, float weight);

        int adjust_item_weight_in_loc(int id, int weight, const std::map<std::string, std::string>& loc);
        int adjust_item_weightf_in_loc(int id, float weight, const std::map<std::string, std::string>& loc);

        int adjust_subtree_weight(int bucket_id, int weight);
        int adjust_subtree_weightf(int bucket_id, float weight);

        void reweight();

    private:

        bool check_item_loc(int item, const std::map<std::string, std::string>& loc, int *iweight);
        bool check_item_loc(int item, const std::map<std::string, std::string>& loc, float *weight);

        bool _search_item_exists(int item) const;

        bool _maybe_remove_last_instance(int item, bool unlink_only);

        int detach_bucket(int item);

        // <type_name, item_name>
        std::pair<std::string, std::string> get_immediate_parent(int id, int *_ret = NULL) const;

        std::map<std::string, std::string> get_full_location(int id) const;

        int get_full_location_ordered(int id, std::vector<std::pair<std::string, std::string> >& path) const;

        std::map<int, std::string> get_parent_hierarchy(int id) const;

        int get_children(int id, std::list<int> *children) const;

        bool subtree_contains(int root, int item) const;

        void find_roots(std::set<int>& roots) const;

};

#endif // __CRUSH_HPP__

