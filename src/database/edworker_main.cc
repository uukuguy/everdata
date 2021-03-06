/**
 * @file   main.c
 * @author Jiangwen Su <uukuguy@gmail.com>
 * @date   2014-11-08 01:23:17
 *
 * @brief
 *
 *
 */

#include "common.h"
#include "daemon.h"
#include "filesystem.h"
#include "sysinfo.h"
#include "bucketdb.h"
#include "logger.h"

static char program_name[] = "edworker";

typedef struct{
    const char *broker_endpoint;
    uint32_t total_buckets;
    uint32_t total_channels;
    int storage_type;

    int is_daemon;
    int log_level;
} program_options_t;

static struct option const long_options[] = {
	{"endpoint", required_argument, NULL, 'e'},
	{"buckets", required_argument, NULL, 'w'},
	{"channels", required_argument, NULL, 'c'},
	{"storage", required_argument, NULL, 's'},
	{"daemon", no_argument, NULL, 'd'},
	{"verbose", no_argument, NULL, 'v'},
	{"trace", no_argument, NULL, 't'},
	{"help", no_argument, NULL, 'h'},

	{NULL, 0, NULL, 0},
};
static const char *short_options = "e:u:w:c:s:dvth";

extern int run_edworker(const char *broker_endpoint, uint32_t total_buckets, uint32_t total_channels, int storage_type, int verbose);

/* ==================== daemon_loop() ==================== */
int daemon_loop(void *data)
{
    notice_log("In daemon_loop()");

    const program_options_t *po = (const program_options_t *)data;
    return run_edworker(po->broker_endpoint, po->total_buckets, po->total_channels, po->storage_type, po->log_level >= LOG_DEBUG ? 1 : 0);
}

/* ==================== usage() ==================== */
static void usage(int status)
{
    if ( status )
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_name);
    else {
        printf("Usage: %s [OPTION] [PATH]\n", program_name);
        printf("Everdata Worker\n\
                -e, --endpoint          specify the edbroker endpoint\n\
                -w, --buckets           count of buckets\n\
                -w, --channels           count of channels\n\
                -s, --storage      NONE, LOGFILE, LMDB, EBLOB, LEVELDB, ROCKSDB, LSM\n\
                -d, --daemon            run in the daemon mode. \n\
                -v, --verbose           print debug messages\n\
                -t, --trace             print trace messages\n\
                -h, --help              display this help and exit\n\
");
    }
    exit(status);
}

int main(int argc, char *argv[])
{
    program_options_t po;
    memset(&po, 0, sizeof(program_options_t));

    po.broker_endpoint = "tcp://127.0.0.1:19978";
    po.total_buckets = 4;
    po.total_channels = 2;
    po.storage_type = BUCKETDB_NONE;
    po.is_daemon = 0;
    po.log_level = LOG_INFO;

    const char *sz_storage_type = NULL;
	int ch, longindex;
	while ((ch = getopt_long(argc, argv, short_options, long_options,
				 &longindex)) >= 0) {
		switch (ch) {
            case 'e':
                po.broker_endpoint = optarg;
                break;
            case 'w':
                {
                    int total_buckets = atoi(optarg);
                    if ( total_buckets < 0 ) {
                        po.total_buckets = 1;
                    } else {
                        po.total_buckets = total_buckets;
                    }
                }
                break;
            case 'c':
                {
                    int total_channels = atoi(optarg);
                    if ( total_channels < 0 ) {
                        po.total_channels = total_channels;
                    } else {
                        po.total_channels = total_channels;
                    }
                }
                break;
            case 's':
                sz_storage_type = optarg;
                break;
            case 'd':
                po.is_daemon = 1;
                break;
            case 'v':
                po.log_level = LOG_DEBUG;
                break;
            case 't':
                po.log_level = LOG_TRACE;
                break;
            case 'h':
                usage(0);
                break;
            default:
                usage(1);
                break;
        }
	}

    if ( sz_storage_type != NULL ){
        if ( strcmp(sz_storage_type, "LOGFILE") == 0 ){
            po.storage_type = BUCKETDB_LOGFILE;
        } else if ( strcmp(sz_storage_type, "LMDB") == 0 ){
            po.storage_type = BUCKETDB_KVDB_LMDB;
        } else if ( strcmp(sz_storage_type, "EBLOB") == 0 ){
            po.storage_type = BUCKETDB_KVDB_EBLOB;
        } else if ( strcmp(sz_storage_type, "LEVELDB") == 0 ){
            po.storage_type = BUCKETDB_KVDB_LEVELDB;
        } else if ( strcmp(sz_storage_type, "ROCKSDB") == 0 ){
            po.storage_type = BUCKETDB_KVDB_ROCKSDB;
        } else if ( strcmp(sz_storage_type, "LSM") == 0 ){
            po.storage_type = BUCKETDB_KVDB_LSM;
        }
    }

    /* -------- Init logger -------- */
    char root_dir[NAME_MAX];
    get_instance_parent_full_path(root_dir, NAME_MAX);

    char log_dir[NAME_MAX];
    sprintf(log_dir, "%s/log", root_dir);
    mkdir_if_not_exist(log_dir);

    char logfile[PATH_MAX];
    sprintf(logfile, "%s/%s.log", log_dir, program_name);

    if (log_init(program_name, LOG_SPACE_SIZE, po.is_daemon, po.log_level, logfile))
        return -1;

    if ( po.is_daemon ){
        return daemon_fork(daemon_loop, (void*)&po);
    } else
        return run_edworker(po.broker_endpoint, po.total_buckets, po.total_channels, po.storage_type, po.log_level >= LOG_DEBUG ? 1 : 0);
}

