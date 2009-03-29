// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _SCHED_CONFIG_
#define _SCHED_CONFIG_

#include "regex.h"
#include <vector>
using std::vector;

// parsed version of server configuration file
//
class SCHED_CONFIG {
public:
    char master_url[256];
    char long_name[256];
    char db_name[256];
    char db_user[256];
    char db_passwd[256];
    char db_host[256];
    int shmem_key;
    char key_dir[256];
    char download_url[256];
    char download_dir[256];
    char upload_url[256];
    char upload_dir[256];
    char sched_lockfile_dir[256];
    int grace_period_hours;
    int delete_delay_hours;
    bool one_result_per_user_per_wu;
    bool one_result_per_host_per_wu;
    bool msg_to_host;
    int min_sendwork_interval;
    int max_wus_to_send;
    int max_wus_in_progress;
        // max jobs in progress on a given host (per CPU)
    bool non_cpu_intensive;
    bool verify_files_on_app_start;
    int homogeneous_redundancy;
    bool locality_scheduling;
    bool locality_scheduling_sorted_order;
    bool ignore_upload_certificates;
    bool dont_generate_upload_certificates;
    bool ignore_delay_bound;
    int daily_result_quota;     // max results per host per day
    int uldl_dir_fanout;        // fanout of ul/dl dirs; 0 if none
    int uldl_dir_levels;
    int locality_scheduling_wait_period;
    int locality_scheduling_send_timeout;
    int min_core_client_version;
    int min_core_client_version_announced;
    int min_core_client_upgrade_deadline;
    bool choose_download_url_by_timezone;
    bool cache_md5_info;
    bool nowork_skip;
    bool resend_lost_results;
    bool send_result_abort;
    bool use_benchmark_weights;
    double fp_benchmark_weight;
    double default_disk_max_used_gb;
    double default_disk_max_used_pct;
    double default_disk_min_free_gb;
    char symstore[256];
    double next_rpc_delay;
    int sched_debug_level;
    int fuh_debug_level;
    double reliable_max_error_rate;  // max error rate for a host to be declared reliable
    int reliable_max_avg_turnaround;
        // max average turnaround for a host to be declared reliable
    int reliable_priority_on_over;
        // additional results generated after at least one result
        // is over will have their priority boosted by this amount    
    int reliable_priority_on_over_except_error;
        // additional results generated after at least one result is over
        // (unless their is an error) will have their priority boosted
        // by this amount
    int reliable_on_priority;
        // results with a priority equal or greater than this value
        // will be sent to reliable hosts
    double reliable_reduced_delay_bound;
        // Reduce the delay bounds for reliable hosts by this percent
	int granted_credit_ramp_up; 
	double granted_credit_weight;
    bool distinct_beta_apps;
        // allow users to select beta apps independently
    bool workload_sim;
        // Do workload simulation in deciding whether to send a result
    bool ended;
        // Project has ended - tell clients to detach
    int shmem_work_items;
        // number of work items in shared memory
    int feeder_query_size;
        // number of work items to request in each feeder query
    bool no_darwin_6;
        // don't allow Mac OS X 10.2 or earlier
    bool no_amd_k6;
        // don't allow AMD K6 CPUs
    char httpd_user[256];
        // user name under which web server runs (default: apache)
    int file_deletion_strategy;
        // select method of automatically deleting files from host
    bool request_time_stats_log;
    bool enable_assignment;
    int max_ncpus;
    vector<regex_t> ban_os;
    vector<regex_t> ban_cpu;
    // log flags
    //
    bool debug_version_select;
    bool debug_assignment;
    bool debug_prefs;
    bool debug_send;
    bool debug_request_headers;
    bool debug_user_messages;
    bool debug_request_details;
    bool debug_handle_results;

    int parse(FILE*);
    int parse_file(const char* dir=".");

    int upload_path(const char*, char*);
    int download_path(const char*, char*);
};

extern SCHED_CONFIG config;

// get the project's home directory
// (assumed to be the parent of the CWD)
//
void get_project_dir(char*, int);

#endif