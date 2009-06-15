// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2008 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// High-level logic for communicating with scheduling servers,
// and for merging the result of a scheduler RPC into the client state

// The scheduler RPC mechanism is in scheduler_op.C

#include "cpp.h"

#ifdef _WIN32
#include "boinc_win.h"
#endif

#ifndef _WIN32
#include "config.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <map>
#include <set>
#endif

#include "crypt.h"
#include "error_numbers.h"
#include "file_names.h"
#include "filesys.h"
#include "parse.h"
#include "str_util.h"
#include "util.h"

#include "client_msgs.h"
#include "scheduler_op.h"
#include "sandbox.h"

#include "client_state.h"

using std::max;
using std::vector;
using std::string;

// quantities like avg CPU time decay by a factor of e every week
//
#define EXP_DECAY_RATE  (1./(SECONDS_PER_DAY*7))

// try to report results this much before their deadline
//
#define REPORT_DEADLINE_CUSHION ((double)SECONDS_PER_DAY)


// Write a scheduler request to a disk file,
// to be sent to a scheduling server
//
int CLIENT_STATE::make_scheduler_request(PROJECT* p) {
    char buf[1024];
    MIOFILE mf;
    unsigned int i;
    RESULT* rp;
    int retval;
    double disk_total, disk_project;

    get_sched_request_filename(*p, buf, sizeof(buf));
    FILE* f = boinc_fopen(buf, "wb");
    if (!f) return ERR_FOPEN;

    double trs = total_resource_share();
    double rrs = runnable_resource_share();
    double prrs = potentially_runnable_resource_share();
    double resource_share_fraction, rrs_fraction, prrs_fraction;
    if (trs) {
        resource_share_fraction = p->resource_share / trs;
    } else {
        resource_share_fraction = 1;
    }
    if (rrs) {
        rrs_fraction = p->resource_share / rrs;
    } else {
        rrs_fraction = 1;
    }
    if (prrs) {
        prrs_fraction = p->resource_share / prrs;
    } else {
        prrs_fraction = 1;
    }

    // if hostid is zero, rpc_seqno better be also
    //
    if (!p->hostid) {
        p->rpc_seqno = 0;
    }

    mf.init_file(f);
    fprintf(f,
        "<scheduler_request>\n"
        "    <authenticator>%s</authenticator>\n"
        "    <hostid>%d</hostid>\n"
        "    <rpc_seqno>%d</rpc_seqno>\n"
        "    <core_client_major_version>%d</core_client_major_version>\n"
        "    <core_client_minor_version>%d</core_client_minor_version>\n"
        "    <core_client_release>%d</core_client_release>\n"
        "    <work_req_seconds>%f</work_req_seconds>\n"
        "    <resource_share_fraction>%f</resource_share_fraction>\n"
        "    <rrs_fraction>%f</rrs_fraction>\n"
        "    <prrs_fraction>%f</prrs_fraction>\n"
        "    <estimated_delay>%f</estimated_delay>\n"
        "    <duration_correction_factor>%f</duration_correction_factor>\n"
        "    <sandbox>%d</sandbox>\n",
        p->authenticator,
        p->hostid,
        p->rpc_seqno,
        core_client_version.major,
        core_client_version.minor,
        core_client_version.release,
        p->work_request,
        resource_share_fraction,
        rrs_fraction,
        prrs_fraction,
        time_until_work_done(p, proj_min_results(p, prrs)-1, prrs),
        p->duration_correction_factor,
        g_use_sandbox?1:0
    );

    // write client capabilities
    //
    fprintf(f,
        "    <client_cap_plan_class>1</client_cap_plan_class>\n"
    );

    write_platforms(p, mf);

    // send supported app_versions for anonymous platform clients
    //
    if (p->anonymous_platform) {
        fprintf(f, "    <app_versions>\n");
        for (i=0; i<app_versions.size(); i++) {
            APP_VERSION* avp = app_versions[i];
            if (avp->project != p) continue;
            avp->write(mf);
        }
        fprintf(f, "    </app_versions>\n");
    }
    if (strlen(p->code_sign_key)) {
        fprintf(f, "    <code_sign_key>\n%s</code_sign_key>\n", p->code_sign_key);
    }

	// send working prefs
	//
	fprintf(f, "<working_global_preferences>\n");
	global_prefs.write(mf);
	fprintf(f, "</working_global_preferences>\n");

    // send master global preferences if present and not host-specific
    //
    if (!global_prefs.host_specific && boinc_file_exists(GLOBAL_PREFS_FILE_NAME)) {
        FILE* fprefs = fopen(GLOBAL_PREFS_FILE_NAME, "r");
        if (fprefs) {
            copy_stream(fprefs, f);
            fclose(fprefs);
        }
        PROJECT* pp = lookup_project(global_prefs.source_project);
        if (pp && strlen(pp->email_hash)) {
            fprintf(f,
                "<global_prefs_source_email_hash>%s</global_prefs_source_email_hash>\n",
                pp->email_hash
            );
        }
    }

    // Of the projects with same email hash as this one,
    // send the oldest cross-project ID.
    // Use project URL as tie-breaker.
    //
    PROJECT* winner = p;
    for (i=0; i<projects.size(); i++ ) {
        PROJECT* project = projects[i];
        if (project == p) continue;
        if (strcmp(project->email_hash, p->email_hash)) continue;
        if (project->cpid_time < winner->cpid_time) {
            winner = project;
        } else if (project->cpid_time == winner->cpid_time) {
            if (strcmp(project->master_url, winner->master_url) < 0) {
                winner = project;
            }
        }
    }
    fprintf(f,
        "<cross_project_id>%s</cross_project_id>\n",
        winner->cross_project_id
    );

    retval = time_stats.write(mf, true);
    //if (retval) return retval;
    // can't return without closing file
    retval = net_stats.write(mf);
    //if (retval) return retval;

    // update hardware info, and write host info
    //
    host_info.get_host_info();
    retval = host_info.write(mf, config.suppress_net_info);
    //if (retval) return retval;

    // get and write disk usage
    //
    total_disk_usage(disk_total);
    project_disk_usage(p, disk_project);
    fprintf(f,
        "    <disk_usage>\n"
        "        <d_boinc_used_total>%f</d_boinc_used_total>\n"
        "        <d_boinc_used_project>%f</d_boinc_used_project>\n"
        "    </disk_usage>\n",
        disk_total, disk_project
    );

    if (coprocs.coprocs.size()) {
        fprintf(f, "    <coprocs>\n");
        for (i=0; i<coprocs.coprocs.size(); i++) {
            COPROC* c = coprocs.coprocs[i];
            c->write_xml(mf);
        }
        fprintf(f, "    </coprocs>\n");
    }

    // report results
    //
    p->nresults_returned = 0;
    for (i=0; i<results.size(); i++) {
        rp = results[i];
        if (rp->project == p && rp->ready_to_report) {
            p->nresults_returned++;
            rp->write(mf, true);
        }
    }

    read_trickle_files(p, f);

    // report sticky files as needed
    //
    for (i=0; i<file_infos.size(); i++) {
        FILE_INFO* fip = file_infos[i];
        if (fip->project != p) continue;
        if (!fip->report_on_rpc) continue;
        if (fip->marked_for_delete) continue;
        fprintf(f,
            "    <file_info>\n"
            "        <name>%s</name>\n"
            "        <nbytes>%f</nbytes>\n"
            "        <status>%d</status>\n"
            "        <report_on_rpc/>\n"
            "    </file_info>\n",
            fip->name, fip->nbytes, fip->status
        );
    }

    // NOTE: there's also a send_file_list flag, not currently used

    if (p->send_time_stats_log) {
        fprintf(f, "<time_stats_log>\n");
        time_stats.get_log_after(p->send_time_stats_log, mf);
        fprintf(f, "</time_stats_log>\n");
    }
    if (p->send_job_log) {
        fprintf(f, "<job_log>\n");
        job_log_filename(*p, buf, sizeof(buf));
        send_log_after(buf, p->send_job_log, mf);
        fprintf(f, "</job_log>\n");
    }

    // send names of results in progress for this project
    //
    fprintf(f, "<other_results>\n");
    for (i=0; i<results.size(); i++) {
        rp = results[i];
        if (rp->project == p && !rp->ready_to_report) {
            fprintf(f,
                "    <other_result>\n"
                "        <name>%s</name>\n"
                "    </other_result>\n",
                rp->name
            );
        }
    }
    fprintf(f, "</other_results>\n");

    // send summary of in-progress results
    // to give scheduler info on our CPU commitment
    //
    fprintf(f, "<in_progress_results>\n");
    for (i=0; i<results.size(); i++) {
        rp = results[i];
        double x = rp->estimated_time_remaining(false);
        if (x == 0) continue;
        fprintf(f,
            "    <ip_result>\n"
            "        <name>%s</name>\n"
            "        <report_deadline>%f</report_deadline>\n"
            "        <cpu_time_remaining>%f</cpu_time_remaining>\n"
            "    </ip_result>\n",
            rp->name,
            rp->report_deadline,
            x
        );
    }
    fprintf(f, "</in_progress_results>\n");
    fprintf(f, "</scheduler_request>\n");

    fclose(f);
    return 0;
}

// called from the client's polling loop.
// initiate scheduler RPC activity if needed and possible
//
bool CLIENT_STATE::scheduler_rpc_poll() {
    PROJECT *p;
    bool action=false;
    static double last_time=0;

	// check only every 5 sec
	//
    if (now - last_time < 5.0) return false;
    last_time = now;

    switch(scheduler_op->state) {
    case SCHEDULER_OP_STATE_IDLE:
        if (scheduler_op->check_master_fetch_start()) {
            action = true;
            break;
        }

        p = next_project_sched_rpc_pending();
        if (p) {
			scheduler_op->init_op_project(p, p->sched_rpc_pending);
            action = true;
            break;
        }
        if (network_suspended) break;
        p = next_project_trickle_up_pending();
        if (p) {
            scheduler_op->init_op_project(p, RPC_REASON_TRICKLE_UP);
            action = true;
            break;
        }
        
        // report overdue results
        //
        p = find_project_with_overdue_results();
        if (p) {
            scheduler_op->init_op_project(p, RPC_REASON_RESULTS_DUE);
            action = true;
            break;
        }
        if (!(exit_when_idle && contacted_sched_server)) {
            scheduler_op->init_get_work();
            if (scheduler_op->state != SCHEDULER_OP_STATE_IDLE) {
                break;
            }
        }
        break;
    default:
        scheduler_op->poll();
        if (scheduler_op->state == SCHEDULER_OP_STATE_IDLE) {
            action = true;
        }
        break;
    }
    return action;
}

// Handle the reply from a scheduler
//
int CLIENT_STATE::handle_scheduler_reply(
    PROJECT* project, char* scheduler_url, int& nresults
) {
    SCHEDULER_REPLY sr;
    FILE* f;
    int retval;
    unsigned int i;
    bool signature_valid, update_global_prefs=false, update_project_prefs=false;
    char buf[256], filename[256];
    std::string old_gui_urls = project->gui_urls;
    PROJECT* p2;

    nresults = 0;
    contacted_sched_server = true;
    project->last_rpc_time = now;

    get_sched_reply_filename(*project, filename, sizeof(filename));

    f = fopen(filename, "r");
    if (!f) return ERR_FOPEN;
    retval = sr.parse(f, project);
    fclose(f);
    if (retval) return retval;

    if (log_flags.sched_ops) {
        msg_printf(project, MSG_INFO, "Scheduler request completed: got %d new tasks", (int)sr.results.size());
    }
    if (log_flags.sched_op_debug) {
        if (sr.scheduler_version) {
            msg_printf(project, MSG_INFO,
                "[sched_op_debug] Server version %d",
                sr.scheduler_version
            );
        }
    }

    // check that master URL is correct
    //
    if (strlen(sr.master_url)) {
        canonicalize_master_url(sr.master_url);
        if (strcmp(sr.master_url, project->master_url)) {
            msg_printf(project, MSG_USER_ERROR,
                "You used the wrong URL for this project"
            );
            msg_printf(project, MSG_USER_ERROR,
                "The correct URL is %s", sr.master_url
            );
            p2 = lookup_project(sr.master_url);
            if (p2) {
                msg_printf(project, MSG_INFO,
                    "You seem to be attached to this project twice"
                );
                msg_printf(project, MSG_INFO,
                    "We suggest that you detach projects named %s,",
                    project->project_name
                );
                msg_printf(project, MSG_INFO,
                    "then reattach to %s", sr.master_url
                );
            } else {
                msg_printf(project, MSG_INFO,
                    "Using the wrong URL can cause problems in some cases."
                );
                msg_printf(project, MSG_INFO,
                    "When convenient, detach this project, then reattach to %s",
                    sr.master_url
                );
            }
        }
    }

    // make sure we don't already have a project of same name
    //
    bool dup_name = false;
    for (i=0; i<projects.size(); i++) {
        p2 = projects[i];
        if (project == p2) continue;
        if (!strcmp(p2->project_name, project->project_name)) {
            dup_name = true;
            break;
        }
    }
    if (dup_name) {
        msg_printf(project, MSG_USER_ERROR,
            "Already attached to a project named %s (possibly with wrong URL)",
            project->project_name
        );
        msg_printf(project, MSG_USER_ERROR,
            "Consider detaching this project, then trying again"
        );
    }

    // show messages from server
    //
    for (i=0; i<sr.messages.size(); i++) {
        USER_MESSAGE& um = sr.messages[i];
        sprintf(buf, "Message from server: %s", um.message.c_str());
        int prio = (!strcmp(um.priority.c_str(), "high"))?MSG_USER_ERROR:MSG_INFO;
        show_message(project, buf, prio);
    }

    if (log_flags.sched_op_debug && sr.request_delay) {
        msg_printf(project, MSG_INFO,
            "Project requested delay of %f seconds", sr.request_delay
        );
    }

    // if project is down, return error (so that we back off)
    // and don't do anything else
    //
    if (sr.project_is_down) {
        if (sr.request_delay) {
            double x = now + sr.request_delay;
			project->set_min_rpc_time(x, "project is down");
        }
        return ERR_PROJECT_DOWN;
    }

    // if the scheduler reply includes global preferences,
    // insert extra elements, write to disk, and parse
    //
    if (sr.global_prefs_xml) {
		// skip this if we have host-specific prefs
		// and we're talking to an old scheduler
		//
		if (!global_prefs.host_specific || sr.scheduler_version >= 507) {
			retval = save_global_prefs(
				sr.global_prefs_xml, project->master_url, scheduler_url
			);
			if (retval) {
				return retval;
			}
			update_global_prefs = true;
		} else {
			if (log_flags.sched_op_debug) {
				msg_printf(project, MSG_INFO,
					"ignoring prefs from old server; we have host-specific prefs"
				);
			}
		}
    }

    // see if we have a new venue from this project
    // (this must go AFTER the above, since otherwise
    // global_prefs_source_project() is meaningless)
    //
    if (strcmp(project->host_venue, sr.host_venue)) {
        safe_strcpy(project->host_venue, sr.host_venue);
        msg_printf(project, MSG_INFO, "New computer location: %s", sr.host_venue);
        update_project_prefs = true;
        if (project == global_prefs_source_project()) {
            strcpy(main_host_venue, sr.host_venue);
            update_global_prefs = true;
        }
    }

    if (update_global_prefs) {
        read_global_prefs();
    }

    // deal with project preferences (should always be there)
    // If they've changed, write to account file,
    // then parse to get our venue, and pass to running apps
    //
    if (sr.project_prefs_xml) {
        if (strcmp(project->project_prefs.c_str(), sr.project_prefs_xml)) {
            project->project_prefs = string(sr.project_prefs_xml);
            update_project_prefs = true;
        }
    }

    // the account file has GUI URLs and project prefs.
    // rewrite if either of these has changed
    //
    if (project->gui_urls != old_gui_urls || update_project_prefs) {
        retval = project->write_account_file();
        if (retval) {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Can't write account file: %s", boincerror(retval)
            );
            return retval;
        }
    }

    if (update_project_prefs) {
        project->parse_account_file();
        if (strlen(project->host_venue)) {
            project->parse_account_file_venue();
        }
        project->parse_preferences_for_user_files();
        active_tasks.request_reread_prefs(project);
    }

    // if the scheduler reply includes a code-signing key,
    // accept it if we don't already have one from the project.
    // Otherwise verify its signature, using the key we already have.
    //

    if (sr.code_sign_key) {
        if (!strlen(project->code_sign_key)) {
            safe_strcpy(project->code_sign_key, sr.code_sign_key);
        } else {
            if (sr.code_sign_key_signature) {
                retval = verify_string2(
                    sr.code_sign_key, sr.code_sign_key_signature,
                    project->code_sign_key, signature_valid
                );
                if (!retval && signature_valid) {
                    safe_strcpy(project->code_sign_key, sr.code_sign_key);
                } else {
                    msg_printf(project, MSG_INTERNAL_ERROR,
                        "New code signing key doesn't validate"
                    );
                }
            } else {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Missing code sign key signature"
                );
            }
        }
    }

    // copy new entities to client state
    //
    for (i=0; i<sr.apps.size(); i++) {
        APP* app = lookup_app(project, sr.apps[i].name);
        if (app) {
            strcpy(app->user_friendly_name, sr.apps[i].user_friendly_name);
        } else {
            app = new APP;
            *app = sr.apps[i];
            retval = link_app(project, app);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't handle application %s in scheduler reply", app->name
                );
                delete app;
            } else {
                apps.push_back(app);
            }
        }
    }
    FILE_INFO* fip;
    for (i=0; i<sr.file_infos.size(); i++) {
        fip = lookup_file_info(project, sr.file_infos[i].name);
        if (fip) {
            fip->merge_info(sr.file_infos[i]);
        } else {
            fip = new FILE_INFO;
            *fip = sr.file_infos[i];
            retval = link_file_info(project, fip);
            if (retval) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "Can't handle file %s in scheduler reply", fip->name
                );
                delete fip;
            } else {
                file_infos.push_back(fip);
            }
        }
    }
    for (i=0; i<sr.file_deletes.size(); i++) {
        fip = lookup_file_info(project, sr.file_deletes[i].c_str());
        if (fip) {
            msg_printf(project, MSG_INFO,
                "Got server request to delete file %s", fip->name
            );
            fip->marked_for_delete = true;
        }
    }
    for (i=0; i<sr.app_versions.size(); i++) {
        APP_VERSION& avpp = sr.app_versions[i];
        if (strlen(avpp.platform) == 0) {
            strcpy(avpp.platform, get_primary_platform());
        } else {
            if (!is_supported_platform(avpp.platform)) {
                msg_printf(project, MSG_INTERNAL_ERROR,
                    "App version has unsupported platform %s", avpp.platform
                );
                continue;
            }
        }
        APP* app = lookup_app(project, avpp.app_name);
        APP_VERSION* avp = lookup_app_version(
            app, avpp.platform, avpp.version_num, avpp.plan_class
        );
        if (avp) {
            // update performance-related info;
            // generally this shouldn't change,
            // but if it does it's better to use the new stuff
            //
            avp->avg_ncpus = avpp.avg_ncpus;
            avp->max_ncpus = avpp.max_ncpus;
            avp->flops = avpp.flops;
            strcpy(avp->cmdline, avpp.cmdline);
            avp->coprocs.delete_coprocs();
            avp->coprocs = avpp.coprocs;
            strlcpy(avp->api_version, avpp.api_version, sizeof(avp->api_version));

            // if we had download failures, clear them
            //
            avp->clear_errors();
            continue;
        }
        avp = new APP_VERSION;
        *avp = avpp;
        retval = link_app_version(project, avp);
        if (retval) {
             delete avp;
             continue;
        }
        app_versions.push_back(avp);
    }
    for (i=0; i<sr.workunits.size(); i++) {
        if (lookup_workunit(project, sr.workunits[i].name)) continue;
        WORKUNIT* wup = new WORKUNIT;
        *wup = sr.workunits[i];
        wup->project = project;
        retval = link_workunit(project, wup);
        if (retval) {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Can't handle task %s in scheduler reply", wup->name
            );
            delete wup;
            continue;
        }
        wup->clear_errors();
        workunits.push_back(wup);
    }
    double est_duration = 0;
    for (i=0; i<sr.results.size(); i++) {
        if (lookup_result(project, sr.results[i].name)) {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Already have task %s\n", sr.results[i].name
            );
            continue;
        }
        RESULT* rp = new RESULT;
        *rp = sr.results[i];
        retval = link_result(project, rp);
        if (retval) {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Can't handle task %s in scheduler reply", rp->name
            );
            delete rp;
            continue;
        }
        if (strlen(rp->platform) == 0) {
            strcpy(rp->platform, get_primary_platform());
            rp->version_num = latest_version(rp->wup->app, rp->platform);
        }
        rp->avp = lookup_app_version(
            rp->wup->app, rp->platform, rp->version_num, rp->plan_class
        );
        if (!rp->avp) {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "No app version for result: %s %d",
                rp->platform, rp->version_num
            );
            delete rp;
            continue;
        }
        rp->wup->version_num = rp->version_num;
        results.push_back(rp);
        rp->set_state(RESULT_NEW, "handle_scheduler_reply");
        nresults++;
        est_duration += rp->estimated_duration(false);
    }
    if (log_flags.sched_op_debug) {
        if (sr.results.size()) {
            msg_printf(project, MSG_INFO,
                "[sched_op_debug] estimated total job duration: %.0f seconds",
                est_duration
            );
        }
    }

    // update records for ack'ed results
    //
    for (i=0; i<sr.result_acks.size(); i++) {
        if (log_flags.sched_op_debug) {
            msg_printf(0, MSG_INFO,
                "[sched_op_debug] handle_scheduler_reply(): got ack for result %s\n",
                sr.result_acks[i].name
            );
        }
        RESULT* rp = lookup_result(project, sr.result_acks[i].name);
        if (rp) {
            rp->got_server_ack = true;
        } else {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Got ack for task %s, but can't find it", sr.result_acks[i].name
            );
        }
    }

    // handle result abort requests
    //
    for (i=0; i<sr.result_abort.size(); i++) {
        RESULT* rp = lookup_result(project, sr.result_abort[i].name);
        if (rp) {
            ACTIVE_TASK* atp = lookup_active_task_by_result(rp);
            if (atp) {
                atp->abort_task(ERR_ABORTED_BY_PROJECT,
                    "aborted by project - no longer usable"
                );
            } else {
                rp->abort_inactive(ERR_ABORTED_BY_PROJECT);
            }
        } else {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Server requested abort of unknown task %s",
                sr.result_abort[i].name
            );
        }
    }
    for (i=0; i<sr.result_abort_if_not_started.size(); i++) {
        RESULT* rp = lookup_result(project, sr.result_abort_if_not_started[i].name);
        if (rp) {
            ACTIVE_TASK* atp = lookup_active_task_by_result(rp);
            if (!atp) {
                rp->abort_inactive(ERR_ABORTED_BY_PROJECT);
            }
        } else {
            msg_printf(project, MSG_INTERNAL_ERROR,
                "Server requested conditional abort of unknown task %s",
                sr.result_abort_if_not_started[i].name
            );
        }
    }

    // remove acked trickle files
    //
    if (sr.message_ack) {
        remove_trickle_files(project);
    }
    if (sr.send_file_list) {
        project->send_file_list = true;
    }
    project->send_time_stats_log = sr.send_time_stats_log;
    project->send_job_log = sr.send_job_log;
    project->sched_rpc_pending = 0;
    project->trickle_up_pending = false;

    // The project returns a hostid only if it has created a new host record.
    // In that case reset RPC seqno
    //
    if (sr.hostid) {
        if (project->hostid) {
            // if we already have a host ID for this project,
            // we must have sent it a stale seqno,
            // which usually means our state file was copied from another host.
            // So generate a new host CPID.
            //
            generate_new_host_cpid();
            msg_printf(project, MSG_INFO,
                "Generated new computer cross-project ID: %s",
                host_info.host_cpid
            );
        }
        //msg_printf(project, MSG_INFO, "Changing host ID from %d to %d", project->hostid, sr.hostid);
        project->hostid = sr.hostid;
        project->rpc_seqno = 0;
    }

    if (sr.auto_update.present) {
        if (!sr.auto_update.validate_and_link(project)) {
            auto_update = sr.auto_update;
        }
    }

    project->link_project_files(true);

    set_client_state_dirty("handle_scheduler_reply");
    if (log_flags.state_debug) {
        msg_printf(0, MSG_INFO,
            "[state_debug] handle_scheduler_reply(): State after handle_scheduler_reply():"
        );
        print_summary();
    }

    // the following must precede the backoff and request_delay checks,
    // since it overrides them
    //
    if (sr.next_rpc_delay) {
        project->next_rpc_time = now + sr.next_rpc_delay;
    } else {
        project->next_rpc_time = 0;
    }

    // if we asked for work and didn't get any,
    // treat it as an RPC failure; back off this project
    //
    if (project->work_request && nresults==0) {
        scheduler_op->backoff(project, "no work from project\n");
    } else {
        project->nrpc_failures = 0;
        project->min_rpc_time = 0;
    }

    if (sr.request_delay) {
        double x = now + sr.request_delay;
        project->set_min_rpc_time(x, "requested by project");
    }

    return 0;
}

const char *BOINC_RCSID_d35a4a7711 = "$Id: cs_scheduler.cpp 16638 2008-12-07 02:29:41Z romw $";