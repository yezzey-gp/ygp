#ifndef GPMONDB_H
#define GPMONDB_H

#include "apr_general.h"
#include "apr_md5.h"
#include "apr_hash.h"
#include "libpq-fe.h"

/**
 * Validate the the gpperfmon database is correct and
 * gpmon user has correct access.
 */
APR_DECLARE(int) gpdb_validate_gpperfmon(void);

/**
 * Check if gpperfmon database exists.
 */
APR_DECLARE(int) gpdb_gpperfmon_db_exists(void);

/**
 * Check if perfmon is enabled
 */
APR_DECLARE(int) gpdb_gpperfmon_enabled(void);

/**
 *  Retrieve the gpmon_port from GPDB. (SHOW GPMON_PORT)
 */
APR_DECLARE(int) gpdb_get_gpmon_port(void);

/**
 *  Retrieve a list of all hosts in the GPDB.
 *  @param hostcnt return # elements in hostvec
 *  @param hostvec return array of hostnames.
 *  @param pool where to allocate hostvec and its contents.
 */
APR_DECLARE(void) gpdb_get_hostlist(int* hostcnt, host_t** host_table, apr_pool_t* global_pool, mmon_options_t* opt);

/**
 *  Get the master data directory in the GPDB.
 *  @param mstrdir return the master data directory
 *  @param hostname return the master hostname
 *  @param pool where to allocate hostname and mstrdir
 */
APR_DECLARE(void) gpdb_get_master_data_dir(char** hostname, char** mstrdir, apr_pool_t* pool);

/**
 * check if new historical partitions are required and create them
 */
APR_DECLARE(apr_status_t) gpdb_check_partitions(mmon_options_t *opt);

APR_DECLARE (apr_hash_t *) get_active_queries(apr_pool_t* pool);

int find_token_in_config_string(char*, char**, const char*);
void process_line_in_hadoop_cluster_info(apr_pool_t*, apr_hash_t*, char*, char*, char*);
int get_hadoop_hosts_and_add_to_hosts(apr_pool_t*, apr_hash_t*, mmon_options_t*);
apr_status_t truncate_file(char*, apr_pool_t*);
const char* gpdb_exec_only(PGconn* conn, PGresult** pres, const char* query);
const char* insert_into_table(PGconn* conn, char* tuple, const char* table);
const char* insert_query(PGconn* conn, char* tuple, int32 tmid, int32 ssid, int32 ccnt);
void gpdb_conn_string(char* connstr);

#define GPDB_MAX_HISTORY_QUERY_SIZE 65536
/* GPDB_MAX_TUPLE_SIZE = GPDB_MAX_HISTORY_QUERY_SIZE + 2**10 (for other fields in tuple) */
#define GPDB_MAX_TUPLE_SIZE 66560
/* GPDB_MAX_QUERY_FOR_INSERT_SIZE = GPDB_MAX_TUPLE_SIZE + 2**7 (for "insert into ... values ..." part of query) */
#define GPDB_MAX_QUERY_FOR_INSERT_SIZE 66688
#define GPDB_CONNSTR_SIZE 100

#endif /* GPMONDB_H */

