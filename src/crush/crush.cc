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
            std::cout << "bucket[" << i << "]" << " id:" << b->id
                << " type:" << b->type
                << " alg:" << b->alg
                << " hash:" << b->hash
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

int32_t CRush::get_max_rules() const
{
    return m_crush->max_rules;
}

int CRush::move_bucket(int bucketno, const std::map<std::string, std::string>& loc)
{
  // sorry this only works for buckets
  if (bucketno >= 0)
    return -EINVAL;

  if (!item_exists(bucketno))
    return -ENOENT;

  // get the name of the bucket we are trying to move for later
  std::string bucket_name = get_item_name(bucketno);

  // detach the bucket
  int bucket_weight = detach_bucket(bucketno);

  // insert the bucket back into the hierarchy
  int alg = CRUSH_BUCKET_STRAW;
  int hash = CRUSH_HASH_DEFAULT;
  return insert_item(bucketno, bucket_weight / (float)0x10000, bucket_name, alg, hash, loc);
}

int CRush::link_bucket(int id, const std::map<std::string, std::string>& loc)
{
  // sorry this only works for buckets
  if (id >= 0)
    return -EINVAL;

  if (!item_exists(id))
    return -ENOENT;

  // get the name of the bucket we are trying to move for later
  std::string id_name = get_item_name(id);

  crush_bucket *b = get_bucket(id);
  unsigned bucket_weight = b->weight;

  int alg = CRUSH_BUCKET_STRAW;
  int hash = CRUSH_HASH_DEFAULT;
  return insert_item(id, bucket_weight / (float)0x10000, id_name, alg, hash, loc);
}

int CRush::create_or_move_item(int item, float weight, std::string name, const std::map<std::string, std::string>& loc)
{
    int ret = 0;
    int old_iweight;

    //if (check_item_loc(cct, item, loc, &old_iweight)) {
        //ldout(cct, 5) << "create_or_move_item " << item << " already at " << loc << dendl;
    //} else {
        if ( _search_item_exists(item) ) {
            weight = get_item_weightf(item);
            //ldout(cct, 10) << "create_or_move_item " << item << " exists with weight " << weight << dendl;
            remove_item(item, true);
        }
        //ldout(cct, 5) << "create_or_move_item adding " << item << " weight " << weight
            //<< " at " << loc << dendl;
        int alg = CRUSH_BUCKET_STRAW;
        int hash = CRUSH_HASH_DEFAULT;
        ret = insert_item(item, weight, name, alg, hash, loc);
        if (ret == 0)
            ret = 1;  // changed
    //}
    return ret;
}

int CRush::insert_item(int item, float weight, std::string name, int alg, int hash, const std::map<std::string, std::string>& loc)  // typename -> bucketname
{
    if (name_exists(name)) {
        if (get_item_id(name) != item) {
            std::cout << "device name '" << name << "' already exists as id "
                << get_item_id(name) << std::endl;
            return -EEXIST;
        }
    } else {
        set_item_name(item, name);
    }

    int cur = item;

    std::cout << "===============" << std::endl;
    // create locations if locations don't exist and add child in location with 0 weight
    // the more detail in the insert_item method declaration in CrushWrapper.h
    for (std::map<int, std::string>::iterator p = level_map.begin(); p != level_map.end(); ++p) {
        // ignore device type
        if (p->first == 0)
            continue;

        std::cout << "*** cur: " << cur << " p->second: " << p->second << std::endl;

        // skip types that are unspecified
        std::map<std::string, std::string>::const_iterator q = loc.find(p->second);
        if (q == loc.end()) {
            std::cout << "warning: did not specify location for '" << p->second << "' level (levels are "
                << ")" << std::endl;
            continue;
        }

        std::cout << "*** q: " << q->first << ", " << q->second << std::endl;

        if (!name_exists(q->second)) {
            std::cout << "insert_item creating bucket " << q->second << std::endl;
            int empty = 0, newid;
            int r = add_bucket(0, alg, hash, p->first, 1, &cur, &empty, &newid);
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
                << "'" << level_map[b->type] << "' != "
                << "'" << level_map[p->first] << "'" << std::endl;
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
    if(adjust_item_weightf_in_loc(item, weight, loc) > 0) {
        if (item >= m_crush->max_devices) {
            m_crush->max_devices = item + 1;
            std::cout << "insert_item max_devices now " << m_crush->max_devices << std::endl;
        }
        return 0;
    }

    //std::cout << "error: didn't find anywhere to add item " << item << " in " << loc << std::endl;
    return -1;
}

int CRush::update_item(int item, float weight, std::string name, const std::map<std::string, std::string>& loc)  // typename -> bucketname
{
    //ldout(cct, 5) << "update_item item " << item << " weight " << weight
        //<< " name " << name << " loc " << loc << dendl;
    int ret = 0;

    //if (!is_valid_crush_name(name))
        //return -EINVAL;

    //if (!is_valid_crush_loc(cct, loc))
        //return -EINVAL;

    // compare quantized (fixed-point integer) weights!
    int iweight = (int)(weight * (float)0x10000);
    int old_iweight;
    if (check_item_loc(item, loc, &old_iweight)) {
        //ldout(cct, 5) << "update_item " << item << " already at " << loc << dendl;
        if (old_iweight != iweight) {
            //ldout(cct, 5) << "update_item " << item << " adjusting weight "
                //<< ((float)old_iweight/(float)0x10000) << " -> " << weight << dendl;
            adjust_item_weight_in_loc(item, iweight, loc);
            ret = 1;
        }
        if (get_item_name(item) != name) {
            //ldout(cct, 5) << "update_item setting " << item << " name to " << name << dendl;
            set_item_name(item, name);
            ret = 1;
        }
    } else {
        if (item_exists(item)) {
            remove_item(item, true);
        }
        //ldout(cct, 5) << "update_item adding " << item << " weight " << weight
            //<< " at " << loc << dendl;

        int alg = CRUSH_BUCKET_STRAW;
        int hash = CRUSH_HASH_DEFAULT;
        ret = insert_item(item, weight, name, alg, hash, loc);
        if (ret == 0)
            ret = 1;  // changed
    }
    return ret;
}

int CRush::remove_item(int item, bool unlink_only)
{
    //ldout(cct, 5) << "remove_item " << item << (unlink_only ? " unlink_only":"") << dendl;

    int ret = -ENOENT;

    if (item < 0 && !unlink_only) {
        crush_bucket *t = get_bucket(item);
        if (t && t->size) {
            //ldout(cct, 1) << "remove_item bucket " << item << " has " << t->size
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
            if (id == item) {
                //ldout(cct, 5) << "remove_item removing item " << item
                    //<< " from bucket " << b->id << dendl;
                crush_bucket_remove_item(m_crush, b, item);
                adjust_item_weight(b->id, b->weight);
                ret = 0;
            }
        }
    }

    if (_maybe_remove_last_instance(item, unlink_only))
        ret = 0;

    return ret;
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

bool CRush::ruleset_exists(uint8_t ruleset) const
{
    for ( uint32_t i = 0; i < m_crush->max_rules; ++i) {
        if ( rule_exists(i) && m_crush->rules[i]->mask.ruleset == ruleset) {
            return true;
        }
    }

    return false;
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

bool CRush::check_item_loc(int item, const std::map<std::string, std::string>& loc, int *weight)
{
    //ldout(cct, 5) << "check_item_loc item " << item << " loc " << loc << dendl;

    for ( std::map<int, std::string>::const_iterator p = level_map.begin(); p != level_map.end(); ++p) {
        // ignore device
        if (p->first == 0)
            continue;

        // ignore types that aren't specified in loc
        std::map<std::string, std::string>::const_iterator q = loc.find(p->second);
        if (q == loc.end()) {
            //ldout(cct, 2) << "warning: did not specify location for '" << p->second << "' level (levels are "
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

int CRush::create_sample_rule(const std::string &rule_name, const std::string &rule_mode, int rule_type, int ruleset, const std::string &root_item, const std::string &level_name)
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

    int level_id = 0;
    if ( level_name.length() > 0 ) {
        level_id = get_level_id(level_name);
    }
    if ( level_id < 0 ) level_id = 0;

    if ( level_id != 0 )
        set_rule_step(ruleno, step++,
                rule_mode == "firstn" ? CRUSH_RULE_CHOOSELEAF_FIRSTN : CRUSH_RULE_CHOOSELEAF_INDEP,
                CRUSH_CHOOSE_N,
                level_id);
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

