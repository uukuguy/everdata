/**
 * @file   crush.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-02-27 19:17:28
 *
 * @brief
 *
 *
 */

#include "crush.hpp"

extern "C"{
#include "crush.h"
#include "mapper.h"
#include "builder.h"
#include "hash.h"
}

CRush::CRush()
    : m_crush(NULL), have_rmaps(false)
{
    create();
}

CRush::~CRush()
{
}

void CRush::dump(const char *filename) const
{
    std::cout << "max_buckets: " << m_crush->max_buckets << std::endl;
    for ( int i = 0 ; i <m_crush->max_buckets ; i++ ) {
        struct crush_bucket *b = m_crush->buckets[i];
        if ( b != NULL ) {
            const std::string bucket_name = get_item_name(b->id);
            std::cout << "bucket[" << i << "] " << "\"" << bucket_name << "\"" << " id:" << b->id
                << " type:" << b->type
                << " alg:" << (int)b->alg
                << " hash:" << (int)b->hash
                << " weight:" << b->weight
                << " size:" << b->size
                << std::endl;
            std::cout << "    items: ";
            for ( int j = 0 ; j < b->size ; j++ ){
                std::cout << b->items[j] << " ";
            }
            std::cout << std::endl;
        } else {
            std::cout << "bucket[" << i << "]" << std::endl;
        }
    }
}

void CRush::create()
{
    if ( m_crush != NULL ) crush_destroy(m_crush);
    m_crush = crush_create();
    have_rmaps = false;
    set_tunables_default();
}

void CRush::finalize()
{
    crush_finalize(m_crush);
}

// ================ rules ================

crush_rule* CRush::get_rule(uint32_t ruleid) const
{
    if ( ruleid < m_crush->max_rules) {
        return m_crush->rules[ruleid];
    }
    return NULL;
}

crush_rule_step* CRush::get_rule_step(uint32_t ruleid, uint32_t step) const
{
    crush_rule *rule = get_rule(ruleid);
    if ( rule != NULL && step < rule->len ) {
        return &rule->steps[step];
    }
    return NULL;
}

int CRush::add_rule(const std::string &rule_name, int ruleset, int steps, int rule_type, int min_size, int max_size)
{
    crush_rule *rule = crush_make_rule(steps, ruleset, rule_type, min_size, max_size);
    int ruleno = crush_add_rule(m_crush, rule, -1);
    if (ruleno >= 0 ) {
        set_rule_name(ruleno, rule_name);
        have_rmaps = false;
    }
    return ruleno;
}

int CRush::set_rule_step(unsigned ruleno, unsigned step, int op, int arg1, int arg2)
{
    crush_rule *rule = get_rule(ruleno);
    if ( rule != NULL ) {
        crush_rule_set_step(rule, step, op, arg1, arg2);
        return 0;
    }
    return -1;
}

int32_t CRush::get_max_rules() const
{
    return m_crush->max_rules;
}

// ================ buckets ================

int CRush::add_bucket(int bucket_id, int type, int size, int *items, int *weights, int *idout, int alg)
{
    if (type == 0)
        return -EINVAL;

    if ( alg == 0 ) alg = CRUSH_BUCKET_STRAW;
    int hash = CRUSH_HASH_DEFAULT;
    crush_bucket *b = crush_make_bucket(m_crush, alg, hash, type, size, items, weights);
    return crush_add_bucket(m_crush, bucket_id, b, idout);
}

int CRush::move_bucket(int bucket_id, const std::map<std::string, std::string>& loc)
{
  // sorry this only works for buckets
  if (bucket_id >= 0)
    return -EINVAL;

  if (!item_exists(bucket_id))
    return -ENOENT;

  // get the name of the bucket we are trying to move for later
  std::string bucket_name = get_item_name(bucket_id);

  // detach the bucket
  int bucket_weight = detach_bucket(bucket_id);

  // insert the bucket back into the hierarchy
  return insert_item(bucket_id, bucket_weight / (float)0x10000, bucket_name, loc);
}

int CRush::link_bucket(int bucket_id, const std::map<std::string, std::string>& loc)
{
  // sorry this only works for buckets
  if (bucket_id >= 0)
    return -EINVAL;

  if (!item_exists(bucket_id))
    return -ENOENT;

  // get the name of the bucket we are trying to move for later
  std::string bucket_name = get_item_name(bucket_id);

  crush_bucket *b = get_bucket(bucket_id);
  unsigned bucket_weight = b->weight;

  return insert_item(bucket_id, bucket_weight / (float)0x10000, bucket_name, loc);
}

crush_bucket* CRush::get_bucket(int bucket_id) const
{
    unsigned int pos = (unsigned int)(-1 - bucket_id);
    unsigned int max_buckets = m_crush->max_buckets;
    if ( pos < max_buckets ) {
        crush_bucket *b = m_crush->buckets[pos];
        return b;
    }
    return NULL;
}

int CRush::create_or_move_item(int item_id, float weight, std::string name, const std::map<std::string, std::string>& loc)
{
    int ret = 0;
    int old_iweight;

    //if (check_item_loc(cct, item, loc, &old_iweight)) {
        //ldout(cct, 5) << "create_or_move_item " << item_id << " already at " << loc << dendl;
    //} else {
        if ( _search_item_exists(item_id) ) {
            weight = get_item_weightf(item_id);
            //ldout(cct, 10) << "create_or_move_item " << item_id << " exists with weight " << weight << dendl;
            remove_item(item_id, true);
        }
        //ldout(cct, 5) << "create_or_move_item adding " << item_id << " weight " << weight
            //<< " at " << loc << dendl;
        ret = insert_item(item_id, weight, name, loc);
        if (ret == 0)
            ret = 1;  // changed
    //}
    return ret;
}

int CRush::insert_item(int item_id, float weight, std::string name, const std::map<std::string, std::string>& loc)  // typename -> bucketname
{
    if (name_exists(name)) {
        if (get_item_id(name) != item_id) {
            std::cout << "device name '" << name << "' already exists as id "
                << get_item_id(name) << std::endl;
            return -EEXIST;
        }
    } else {
        set_item_name(item_id, name);
    }

    int cur = item_id;

    std::cout << "===============" << std::endl;
    // create locations if locations don't exist and add child in location with 0 weight
    // the more detail in the insert_item method declaration in CrushWrapper.h
    for (std::map<int, std::string>::iterator p = type_map.begin(); p != type_map.end(); ++p) {
        // ignore device type
        if (p->first == 0)
            continue;

        std::cout << "*** cur: " << cur << " p->second: " << p->second << std::endl;

        // skip types that are unspecified
        std::map<std::string, std::string>::const_iterator q = loc.find(p->second);
        if (q == loc.end()) {
            std::cout << "warning: did not specify location for '" << p->second << "' type (types are "
                << ")" << std::endl;
            continue;
        }

        std::cout << "*** q: " << q->first << ", " << q->second << std::endl;

        if (!name_exists(q->second)) {
            std::cout << "insert_item creating bucket " << q->second << std::endl;
            int empty = 0, newid;
            int r = add_bucket(0, p->first, 1, &cur, &empty, &newid);
            if (r < 0) {
                return r;
            }
            set_item_name(newid, q->second);

            cur = newid;
            continue;
        }

        // add to an existing bucket
        int id = get_item_id(q->second);
        std::cout << "*** cur: " << cur << " id: " << id << " q: " << q->first << ", " << q->second << std::endl;

        crush_bucket *b = get_bucket(id);
        if ( b == NULL ) {
            std::cout << "insert_item doesn't have bucket " << id << std::endl;
            return -1;
        }

        // check that we aren't creating a cycle.
        //if (subtree_contains(id, cur)) {
            //ldout(cct, 1) << "insert_item item " << cur << " already exists beneath " << id << dendl;
            //return -EINVAL;
        //}

        if (p->first != b->type) {
            std::cout << "insert_item existing bucket has type "
                << "'" << type_map[b->type] << "' != "
                << "'" << type_map[p->first] << "'" << std::endl;
            return -1;
        }

        // are we forming a loop?
        //if (subtree_contains(cur, b->id)) {
            //ldout(cct, 1) << "insert_item " << cur << " already contains " << b->id
                //<< "; cannot form loop" << dendl;
            //return -ELOOP;
        //}

        std::cout << "insert_item adding " << cur << " weight " << weight
            << " to bucket " << id << std::endl;
        crush_bucket_add_item(m_crush, b, cur, 0);
        break;
    }

    // adjust the item's weight in location
    if(adjust_item_weightf_in_loc(item_id, weight, loc) > 0) {
        if (item_id >= m_crush->max_devices) {
            m_crush->max_devices = item_id + 1;
            std::cout << "insert_item max_devices now " << m_crush->max_devices << std::endl;
        }
        return 0;
    }

    //std::cout << "error: didn't find anywhere to add item " << item << " in " << loc << std::endl;
    return -1;
}

int CRush::update_item(int item_id, float weight, std::string name, const std::map<std::string, std::string>& loc)  // typename -> bucketname
{
    //ldout(cct, 5) << "update_item item " << item_id << " weight " << weight
        //<< " name " << name << " loc " << loc << dendl;
    int ret = 0;

    //if (!is_valid_crush_name(name))
        //return -EINVAL;

    //if (!is_valid_crush_loc(cct, loc))
        //return -EINVAL;

    // compare quantized (fixed-point integer) weights!
    int iweight = (int)(weight * (float)0x10000);
    int old_iweight;
    if (check_item_loc(item_id, loc, &old_iweight)) {
        //ldout(cct, 5) << "update_item " << item_id << " already at " << loc << dendl;
        if (old_iweight != iweight) {
            //ldout(cct, 5) << "update_item " << item_id << " adjusting weight "
                //<< ((float)old_iweight/(float)0x10000) << " -> " << weight << dendl;
            adjust_item_weight_in_loc(item_id, iweight, loc);
            ret = 1;
        }
        if (get_item_name(item_id) != name) {
            //ldout(cct, 5) << "update_item setting " << item_id << " name to " << name << dendl;
            set_item_name(item_id, name);
            ret = 1;
        }
    } else {
        if (item_exists(item_id)) {
            remove_item(item_id, true);
        }
        //ldout(cct, 5) << "update_item adding " << item_id << " weight " << weight
            //<< " at " << loc << dendl;

        ret = insert_item(item_id, weight, name, loc);
        if (ret == 0)
            ret = 1;  // changed
    }
    return ret;
}

int CRush::remove_item(int item_id, bool unlink_only)
{
    //ldout(cct, 5) << "remove_item " << item_id << (unlink_only ? " unlink_only":"") << dendl;

    int ret = -ENOENT;

    if ( item_id < 0 && !unlink_only ) {
        crush_bucket *t = get_bucket(item_id);
        if (t && t->size) {
            //ldout(cct, 1) << "remove_item bucket " << item_id << " has " << t->size
                //<< " items, not empty" << dendl;
            return -ENOTEMPTY;
        }
    }

    for (int i = 0; i < m_crush->max_buckets; i++) {
        if (!m_crush->buckets[i])
            continue;
        crush_bucket *b = m_crush->buckets[i];

        for (unsigned i=0; i < b->size; ++i) {
            int id = b->items[i];
            if (id == item_id) {
                //ldout(cct, 5) << "remove_item removing item " << item_id
                    //<< " from bucket " << b->id << dendl;
                crush_bucket_remove_item(m_crush, b, item_id);
                adjust_item_weight(b->id, b->weight);
                ret = 0;
            }
        }
    }

    if (_maybe_remove_last_instance(item_id, unlink_only))
        ret = 0;

    return ret;
}

int CRush::get_max_devices() const
{
    return m_crush->max_devices;
}


int CRush::get_item_weight(int id) const
{
    for (int bidx = 0; bidx < m_crush->max_buckets; bidx++) {
        crush_bucket *b = m_crush->buckets[bidx];
        if (b == NULL)
            continue;
        for (unsigned i = 0; i < b->size; i++)
            if (b->items[i] == id)
                return crush_get_bucket_item_weight(b, i);
    }
    return -ENOENT;
}

float CRush::get_item_weightf(int id) const
{
    return (float)get_item_weight(id) / (float)0x10000;
}

int CRush::get_item_weight_in_loc(int id, const std::map<std::string, std::string> &loc)
{
    for (std::map<std::string, std::string>::const_iterator l = loc.begin(); l != loc.end(); ++l) {
        int bid = get_item_id(l->second);
        crush_bucket *b = get_bucket(bid);
        if ( b == NULL ) continue;
        for (unsigned int i = 0; i < b->size; i++) {
            if (b->items[i] == id) {
                return crush_get_bucket_item_weight(b, i);
            }
        }
    }
    return -ENOENT;
}

float CRush::get_item_weightf_in_loc(int id, const std::map<std::string, std::string> &loc)
{
    return (float)get_item_weight_in_loc(id, loc) / (float)0x10000;
}

int CRush::adjust_item_weight(int id, int weight)
{
    int changed = 0;

    for (int bidx = 0; bidx < m_crush->max_buckets; bidx++) {
        crush_bucket *b = m_crush->buckets[bidx];
        if ( b == NULL ) continue;
        for (unsigned i = 0; i < b->size; i++) {
            if (b->items[i] == id) {
                int diff = crush_bucket_adjust_item_weight(m_crush, b, id, weight);
                adjust_item_weight(-1 - bidx, b->weight);
                changed++;
            }
        }
    }

    return changed;
}

int CRush::adjust_item_weightf(int id, float weight)
{
    return adjust_item_weight(id, (int)(weight * (float)0x10000));
}

int CRush::adjust_item_weight_in_loc(int id, int weight, const std::map<std::string, std::string>& loc)
{
    //std::cout << "adjust_item_weight_in_loc " << id << " weight " << weight << " in " << loc << std::endl;
    int changed = 0;

    for ( std::map<std::string, std::string>::const_iterator l = loc.begin(); l != loc.end(); ++l) {
        int bid = get_item_id(l->second);
        crush_bucket *b = get_bucket(bid);
        if ( b == NULL ) continue;

        for (uint32_t i = 0; i < b->size; i++) {
            if (b->items[i] == id) {
                int diff = crush_bucket_adjust_item_weight(m_crush, b, id, weight);
                std::cout << "adjust_item_weight_in_loc " << id << " diff " << diff << " in bucket " << bid << std::endl;
                adjust_item_weight(bid, b->weight);
                changed++;
            }
        }
    }

    if (!changed)
        return -1;

    return changed;
}

int CRush::adjust_item_weightf_in_loc(int id, float weight, const std::map<std::string, std::string>& loc)
{
    return adjust_item_weight_in_loc(id, (int)(weight * (float)0x10000), loc);
}

int CRush::adjust_subtree_weight(int bucket_id, int weight)
{
    //ldout(cct, 5) << "adjust_item_weight " << bucket_id << " weight " << weight << dendl;
    crush_bucket *b = get_bucket(bucket_id);
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

int CRush::adjust_subtree_weightf(int bucket_id, float weight)
{
    return adjust_subtree_weight(bucket_id, (int)(weight * (float)0x10000));
}

void CRush::reweight()
{
    std::set<int> roots;
    find_roots(roots);
    for (std::set<int>::iterator p = roots.begin(); p != roots.end(); ++p) {
        if (*p >= 0) continue;
        crush_bucket *b = get_bucket(*p);
        //ldout(cct, 5) << "reweight bucket " << *p << dendl;
        crush_reweight_bucket(m_crush, b);
    }
}

int CRush::create_sample_rule(const std::string &rule_name, const std::string &rule_mode, int rule_type, int ruleset, const std::string &root_item, const std::string &type_name)
{
    int rule_min_size = 1;
    int rule_max_size = 10;

    int steps = 3;
    if ( rule_mode == "indep" ) steps = 4;
    int step = 0;

    int ruleno = add_rule(rule_name, ruleset, steps, rule_type, rule_min_size, rule_max_size);

    int item_id = get_item_id(root_item);

    if ( rule_mode == "indep" )
        set_rule_step(ruleno, step++, CRUSH_RULE_SET_CHOOSELEAF_TRIES, 5, 0);
    set_rule_step(ruleno, step++, CRUSH_RULE_TAKE, item_id, 0);

    int type_id = 0;
    if ( type_name.length() > 0 ) {
        type_id = get_type_id(type_name);
    }
    if ( type_id < 0 ) type_id = 0;

    if ( type_id != 0 )
        set_rule_step(ruleno, step++,
                rule_mode == "firstn" ? CRUSH_RULE_CHOOSELEAF_FIRSTN : CRUSH_RULE_CHOOSELEAF_INDEP,
                CRUSH_CHOOSE_N,
                type_id);
    else
        set_rule_step(ruleno, step++,
                rule_mode == "firstn" ? CRUSH_RULE_CHOOSE_FIRSTN : CRUSH_RULE_CHOOSE_INDEP,
                CRUSH_CHOOSE_N,
                0);

    set_rule_step(ruleno, step++, CRUSH_RULE_EMIT, 0, 0);

    return ruleno;
}

void CRush::do_rule(int ruleno, int x, std::vector<int>& out, int maxout, const std::vector<uint32_t>& weights) const
{
    //Mutex::Locker l(mapper_lock);
    int rawout[maxout];
    int scratch[maxout * 3];
    int numrep = crush_do_rule(m_crush, ruleno, x, rawout, maxout, &weights[0], weights.size(), scratch);
    if (numrep < 0)
        numrep = 0;
    out.resize(numrep);
    for (int i=0; i<numrep; i++)
        out[i] = rawout[i];
}

// ================ type/bucket/devices/rule names ================

void CRush::build_rmaps() const
{
    if (have_rmaps) return;
    build_rmap(type_map, type_rmap);
    build_rmap(name_map, name_rmap);
    build_rmap(rule_name_map, rule_name_rmap);
    have_rmaps = true;
}

void CRush::build_rmap(const std::map<int, std::string> &f, std::map<std::string, int> &r) const
{
    r.clear();
    for (std::map<int, std::string>::const_iterator p = f.begin(); p != f.end(); ++p)
        r[p->second] = p->first;
}

// ---------------- type names ----------------

int CRush::get_max_types() const
{
    return type_map.size();
}

int CRush::get_type_id(const std::string& name) const
{
    build_rmaps();
    if (type_rmap.count(name))
        return type_rmap[name];
    return -1;
}

const char* CRush::get_type_name(int type_id) const
{
    std::map<int, std::string>::const_iterator p = type_map.find(type_id);
    if (p != type_map.end())
        return p->second.c_str();
    return 0;
}

void CRush::set_type_name(int type_id, const std::string& name)
{
    type_map[type_id] = name;
    if (have_rmaps)
        type_rmap[name] = type_id;
}

// ---------------- item name ----------------

bool CRush::name_exists(const std::string& name) const
{
    build_rmaps();
    return name_rmap.count(name);
}

bool CRush::item_exists(int i) const
{
    return name_map.count(i);
}

int CRush::get_item_id(const std::string& name) const
{
    build_rmaps();
    if (name_rmap.count(name))
        return name_rmap[name];
    return 0;  /* hrm */
}

const char* CRush::get_item_name(int t) const
{
    std::map<int, std::string>::const_iterator p = name_map.find(t);
    if (p != name_map.end())
        return p->second.c_str();
    return 0;
}

int CRush::set_item_name(int i, const std::string& name)
{
    name_map[i] = name;
    if (have_rmaps)
        name_rmap[name] = i;
    return 0;
}

// ---------------- rule name ----------------

bool CRush::ruleset_exists(uint8_t ruleset) const
{
    for ( uint32_t i = 0; i < m_crush->max_rules; ++i) {
        if ( rule_exists(i) && m_crush->rules[i]->mask.ruleset == ruleset) {
            return true;
        }
    }

    return false;
}

bool CRush::rule_exists(int32_t ruleno) const
{
    if ( m_crush != NULL ) {
        if ( ruleno < m_crush->max_rules && m_crush->rules[ruleno] != NULL ) {
            return true;
        }
    }
    return false;
}

bool CRush::rule_exists(const std::string &name) const
{
    build_rmaps();
    return rule_name_rmap.count(name);
}

int CRush::get_rule_id(const std::string &name) const
{
    build_rmaps();
    if (rule_name_rmap.count(name))
        return rule_name_rmap[name];
    return -ENOENT;
}

const char* CRush::get_rule_name(int ruleid) const
{
    std::map<int, std::string>::const_iterator p = rule_name_map.find(ruleid);
    if (p != rule_name_map.end())
        return p->second.c_str();
    return 0;
}

void CRush::set_rule_name(int ruleid, const std::string& name)
{
    rule_name_map[ruleid] = name;
    if (have_rmaps)
        rule_name_rmap[name] = ruleid;
}

// ================ tunable ================

void CRush::set_tunables_argonaut()
{
    m_crush->choose_local_tries = 2;
    m_crush->choose_local_fallback_tries = 5;
    m_crush->choose_total_tries = 19;
    m_crush->chooseleaf_descend_once = 0;
    m_crush->chooseleaf_vary_r = 0;
}

void CRush::set_tunables_bobtail()
{
    m_crush->choose_local_tries = 0;
    m_crush->choose_local_fallback_tries = 0;
    m_crush->choose_total_tries = 50;
    m_crush->chooseleaf_descend_once = 1;
    m_crush->chooseleaf_vary_r = 0;
}

void CRush::set_tunables_firefly()
{
    m_crush->choose_local_tries = 0;
    m_crush->choose_local_fallback_tries = 0;
    m_crush->choose_total_tries = 50;
    m_crush->chooseleaf_descend_once = 1;
    m_crush->chooseleaf_vary_r = 1;
}

void CRush::set_tunables_hammer()
{
    m_crush->choose_local_tries = 0;
    m_crush->choose_local_fallback_tries = 0;
    m_crush->choose_total_tries = 50;
    m_crush->chooseleaf_descend_once = 1;
    m_crush->chooseleaf_vary_r = 1;
}

void CRush::set_tunables_legacy()
{
    set_tunables_argonaut();
    m_crush->straw_calc_version = 0;
}

void CRush::set_tunables_optimal()
{
    set_tunables_hammer();
    m_crush->straw_calc_version = 1;
}

void CRush::set_tunables_default()
{
    set_tunables_bobtail();
    m_crush->straw_calc_version = 1;
}

// ================ utils ================

bool CRush::check_item_loc(int item, const std::map<std::string, std::string>& loc, int *weight)
{
    //ldout(cct, 5) << "check_item_loc item " << item << " loc " << loc << dendl;

    for ( std::map<int, std::string>::const_iterator p = type_map.begin(); p != type_map.end(); ++p) {
        // ignore device
        if (p->first == 0)
            continue;

        // ignore types that aren't specified in loc
        std::map<std::string, std::string>::const_iterator q = loc.find(p->second);
        if (q == loc.end()) {
            //ldout(cct, 2) << "warning: did not specify location for '" << p->second << "' type (types are "
                //<< type_map << ")" << dendl;
            continue;
        }

        if (!name_exists(q->second)) {
            //ldout(cct, 5) << "check_item_loc bucket " << q->second << " dne" << dendl;
            return false;
        }

        int id = get_item_id(q->second);
        if (id >= 0) {
            //ldout(cct, 5) << "check_item_loc requested " << q->second << " for type " << p->second
                //<< " is a device, not bucket" << dendl;
            return false;
        }

        crush_bucket *b = get_bucket(id);

        // see if item exists in this bucket
        for (unsigned j=0; j<b->size; j++) {
            if (b->items[j] == item) {
                //ldout(cct, 2) << "check_item_loc " << item << " exists in bucket " << b->id << dendl;
                if (weight)
                    *weight = crush_get_bucket_item_weight(b, j);
                return true;
            }
        }
        return false;
    }

    //ldout(cct, 1) << "check_item_loc item " << item << " loc " << loc << dendl;
    return false;
}

bool CRush::check_item_loc(int item, const std::map<std::string, std::string>& loc, float *weight)
{
    int iweight;
    bool ret = check_item_loc(item, loc, &iweight);
    if (weight)
        *weight = (float)iweight / (float)0x10000;
    return ret;
}

bool CRush::_search_item_exists(int item) const
{
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

bool CRush::_maybe_remove_last_instance(int item, bool unlink_only)
{
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

int CRush::detach_bucket(int item)
{
    if (item >= 0)
        return (-EINVAL);

    // get the bucket's weight
    crush_bucket *b = get_bucket(item);
    if ( b == NULL ) return -EINVAL;
    unsigned bucket_weight = b->weight;

    // get where the bucket is located. return <type_name, item_name>.
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

// <type_name, item_name>
std::pair<std::string, std::string> CRush::get_immediate_parent(int id, int *_ret) const
{
    std::pair <std::string, std::string> loc;
    int ret = -ENOENT;

    for (int bidx = 0; bidx < m_crush->max_buckets; bidx++) {
        crush_bucket *b = m_crush->buckets[bidx];
        if (b == NULL) continue;
        for (uint32_t i = 0; i < b->size; i++)
            if (b->items[i] == id) {
                std::map<int32_t, std::string>::const_iterator it = type_map.find(b->type);
                std::string parent_bucket_type = it->second;
                std::map<int32_t, std::string>::const_iterator it1 = name_map.find(b->id);
                std::string parent_bucket_name = it1->second;
                loc = std::make_pair(parent_bucket_type, parent_bucket_name);
                ret = 0;
                break;
            }
    }

    if (_ret)
        *_ret = ret;

    return loc;
}

std::map<std::string, std::string> CRush::get_full_location(int id) const
{
    std::vector<std::pair<std::string, std::string> > full_location_ordered;
    std::map<std::string, std::string> full_location;

    get_full_location_ordered(id, full_location_ordered);

    std::copy(full_location_ordered.begin(),
            full_location_ordered.end(),
            std::inserter(full_location, full_location.begin()));

    return full_location;
}

int CRush::get_full_location_ordered(int id, std::vector<std::pair<std::string, std::string> >& path) const
{
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


std::map<int, std::string> CRush::get_parent_hierarchy(int id) const
{
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
    for ( std::map<int, std::string>::const_iterator it = type_map.begin(); it != type_map.end(); ++it){
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

int CRush::get_children(int id, std::list<int> *children) const
{
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

bool CRush::subtree_contains(int root, int item) const
{
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

void CRush::find_roots(std::set<int>& roots) const
{
    for (int i = 0; i < m_crush->max_buckets; i++) {
        if (!m_crush->buckets[i]) continue;
        crush_bucket *b = m_crush->buckets[i];
        if (!_search_item_exists(b->id))
            roots.insert(b->id);
    }
}

