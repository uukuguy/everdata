/**
 * @file   crush_test.cc
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2015-02-27 19:17:28
 *
 * @brief
 *
 *
 */
#include "crush.hpp"

int main(int argc, char *argv[])
{
    CRush crush;

    crush.set_level_name(0, "bucket");
    crush.set_level_name(1, "datanode");
    crush.set_level_name(2, "host");
    crush.set_level_name(10, "root");

    int rootno;
    int bucketno = 0; // 0 for auto id.
	int alg = CRUSH_BUCKET_STRAW;
    int hash = CRUSH_HASH_DEFAULT;
    std::string item_level = "root";
    int level_id = crush.get_level_id(item_level);
    crush.add_bucket(bucketno, alg, hash, level_id,
            0, NULL, NULL, &rootno);
    crush.set_item_name(rootno, "root.0");

    int item_id = crush.get_item_id("root.0");
    std::cout << "rootno: " << rootno << " item_id: " << item_id << std::endl;

    int num_host = 1;
    int num_datanode = 3;
    int num_osd = 4;
    std::map<std::string, std::string> loc;
    loc["root"] = "root.0";
    int osd = 0;

    int level_host_id = crush.get_level_id("host");
    int level_datanode_id = crush.get_level_id("datanode");
    for (int h = 0 ; h < num_host; ++h) {
        char host_name[256];
        sprintf(host_name, "host.%d", h);
        loc["host"] = std::string(host_name);
        //int hostno;
        //crush.add_bucket(0, alg, hash, level_host_id,
                //0, NULL, NULL, &hostno);
        //crush.set_item_name(hostno, host_name);
        for (int n = 0 ; n < num_datanode; ++n) {
            char datanode_name[256];
            sprintf(datanode_name, "datanode.%d.%d", h, n);
            loc["datanode"] = std::string(datanode_name);
            //int datanodeno;
            //crush.add_bucket(0, alg, hash, level_datanode_id,
                    //0, NULL, NULL, &datanodeno);
            //crush.set_item_name(datanodeno, datanode_name);
            for (int o = 0 ; o < num_osd ; ++o, ++osd) {
                char osd_name[256];
                sprintf(osd_name, "osd.%d", osd);
                crush.insert_item(osd, 1.0, std::string(osd_name), alg, hash, loc);
            }
            //if (n == num_datanode - 1){
                //osd++;
                //char osd_name[256];
                //sprintf(osd_name, "osd.%d", osd);
                //crush.insert_item(osd, 1.0, std::string(osd_name), alg, hash, loc);
            //}
        }
    }


    std::string rule_name = "default";
    std::string rule_mode = "indep"; //firstn"; // indep"; // "firstn"
    int rule_type = 0; // user defined. REPLICATED or ERASURE

    int ruleset = 0;
    std::string root_item = "root.0";
    //std::string root_item = "datanode.0.0";
    std::string level_name = "bucket";
    //std::string level_name = "datanode";
    int ruleno = crush.create_sample_rule(rule_name, rule_mode, rule_type, ruleset, root_item, level_name);

    crush.finalize();

    std::vector<uint32_t> weights(crush.get_max_devices(), 0x10000);
    //std::vector<uint32_t> weights(crush.get_max_devices(), 1);

    for (int x = 0; x < 5; ++x) {
        std::vector<int> out;
        crush.do_rule(ruleno, x, out, 3, weights);
        std::cout << "x: " << x  << " get " << out.size() << std::endl;
        for (int n = 0 ; n < out.size() ; n++ ) {
            std::cout << out[n] << ", ";
        }
        std::cout << std::endl;
    }

    crush.dump("");

    return 0;
}

