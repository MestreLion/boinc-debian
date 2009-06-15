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

// scheduler code related to sending work

#include <cstdlib>
#include <string>
#include <cstring>

#include "config.h"
#include "main.h"
#include "server_types.h"
#include "sched_shmem.h"
#include "sched_hr.h"
#include "sched_config.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "sched_send.h"

#include "sched_array.h"


#ifdef _USING_FCGI_
#include "boinc_fcgi.h"
#endif

// Make a pass through the wu/results array, sending work.
// If reply.wreq.infeasible_only is true,
// send only results that were previously infeasible for some host
//
void scan_work_array(SCHEDULER_REQUEST& sreq, SCHEDULER_REPLY& reply) {
    int i, j, retval, n, rnd_off, last_retval=0;;
    WORKUNIT wu;
    DB_RESULT result;
    char buf[256];
    APP* app;

    lock_sema();
    
    rnd_off = rand() % ssp->max_wu_results;
    for (j=0; j<ssp->max_wu_results; j++) {
        i = (j+rnd_off) % ssp->max_wu_results;
        if (!work_needed(sreq, reply, false)) break;

        WU_RESULT& wu_result = ssp->wu_results[i];

        // do fast checks on this wu_result;
        // i.e. ones that don't require DB access
        // if any check fails, continue

        if (wu_result.state != WR_STATE_PRESENT && wu_result.state != g_pid) {
            continue;
        }
        
        // If we are looking for beta results and result is not a beta result
        // then move on
        //
        app = ssp->lookup_app(wu_result.workunit.appid);
        if (app == NULL) continue; // this should never happen
        if (reply.wreq.beta_only) {
            if (!app->beta) {
                continue;
            }
            log_messages.printf(MSG_DEBUG,
                "[HOST#%d] beta work found.  [RESULT#%d]\n",
                reply.host.id, wu_result.resultid
            );
        } else {
             if (app->beta) {
                continue;
            }
        }
        
        // If this is a reliable host and we are checking for results that
        // need a reliable host, then continue if the result is a normal result
        // skip if the app is beta (beta apps don't use the reliable mechanism)
        //
        if (!app->beta) {
            if (reply.wreq.reliable_only && (!wu_result.need_reliable)) {
                continue;
            } else if (!reply.wreq.reliable_only && wu_result.need_reliable) {
                continue;
            }
        }
        
        // don't send if we are looking for infeasible results
        // and the result is not infeasible
        //
        if (reply.wreq.infeasible_only && (wu_result.infeasible_count==0)) {
            continue;
        }
        
        wu = wu_result.workunit;

        // check app filter if needed
        //
        if (reply.wreq.user_apps_only &&
            (!reply.wreq.beta_only || config.distinct_beta_apps)
        ) {
            if (app_not_selected(wu, sreq, reply)) {
                reply.wreq.no_allowed_apps_available = true;
                if (config.debug_send) {
                    log_messages.printf(MSG_DEBUG,
                        "[USER#%d] [WU#%d] user doesn't want work for this application\n",
                        reply.user.id, wu.id
                    );
                }
                continue;
            }
        }

        // don't send if host can't handle it
        //
        retval = wu_is_infeasible_fast(wu, sreq, reply, *app);
        if (retval && retval != last_retval) {
            if (config.debug_send) {
                log_messages.printf(MSG_DEBUG,
                    "[HOST#%d] [WU#%d %s] WU is infeasible: %s\n",
                    reply.host.id, wu.id, wu.name, infeasible_string(retval)
                );
            }
            last_retval = retval;
            continue;
        }

        // Find the app and best app_version for this host.
        //
        BEST_APP_VERSION* bavp;
        bavp = get_app_version(sreq, reply, wu);
        if (!bavp) {
            continue;
        }

        // End of fast checks;
        // mark wu_result as checked out and release semaphore.
        // from here on in this loop, don't continue on failure;
        // instead, goto dont_send (so that we reacquire semaphore)
        //
        // Note: without the semaphore we don't have mutual exclusion;
        // ideally we should use a transaction from now until when
        // we commit to sending the results.

        wu_result.state = g_pid;
        unlock_sema();

        // Don't send if we've already sent a result of this WU to this user.
        //
        if (config.one_result_per_user_per_wu) {
            sprintf(buf,
                "where workunitid=%d and userid=%d",
                wu_result.workunit.id, reply.user.id
            );
            retval = result.count(n, buf);
            if (retval) {
                log_messages.printf(MSG_CRITICAL,
                    "send_work: can't get result count (%d)\n", retval
                );
                goto dont_send;
            } else {
                if (n>0) {
                    log_messages.printf(MSG_DEBUG,
                        "send_work: user %d already has %d result(s) for WU %d\n",
                        reply.user.id, n, wu_result.workunit.id
                    );
                    goto dont_send;
                }
            }
        } else if (config.one_result_per_host_per_wu) {
            // Don't send if we've already sent a result
            // of this WU to this host.
            // We only have to check this
            // if we don't send one result per user.
            //
            sprintf(buf,
                "where workunitid=%d and hostid=%d",
                wu_result.workunit.id, reply.host.id
            );
            retval = result.count(n, buf);
            if (retval) {
                log_messages.printf(MSG_CRITICAL,
                    "send_work: can't get result count (%d)\n", retval
                );
                goto dont_send;
            } else {
                if (n>0) {
                    log_messages.printf(MSG_DEBUG,
                        "send_work: host %d already has %d result(s) for WU %d\n",
                        reply.host.id, n, wu_result.workunit.id
                    );
                    goto dont_send;
                }
            }
        }

        if (app_hr_type(*app)) {
            if (already_sent_to_different_platform_careful(
                sreq, reply.wreq, wu_result.workunit, *app
            )) {
                 log_messages.printf(MSG_DEBUG,
                    "[HOST#%d] [WU#%d %s] WU is infeasible (assigned to different platform)\n",
                    reply.host.id, wu.id, wu.name
                );
                // Mark the workunit as infeasible.
                // This ensures that jobs already assigned to a platform
                // are processed first.
                //
                wu_result.infeasible_count++;
                goto dont_send;
            }
        }

        result.id = wu_result.resultid;

        // mark slot as empty AFTER we've copied out of it
        // (since otherwise feeder might overwrite it)
        //
        wu_result.state = WR_STATE_EMPTY;

        // reread result from DB, make sure it's still unsent
        // TODO: from here to add_result_to_reply()
        // (which updates the DB record) should be a transaction
        //
        retval = result.lookup_id(result.id);
        if (retval) {
            log_messages.printf(MSG_CRITICAL,
                "[RESULT#%d] result.lookup_id() failed %d\n",
                result.id, retval
            );
            goto done;
        }
        if (result.server_state != RESULT_SERVER_STATE_UNSENT) {
            log_messages.printf(MSG_DEBUG,
                "[RESULT#%d] expected to be unsent; instead, state is %d\n",
                result.id, result.server_state
            );
            goto done;
        }
        if (result.workunitid != wu.id) {
            log_messages.printf(MSG_CRITICAL,
                "[RESULT#%d] wrong WU ID: wanted %d, got %d\n",
                result.id, wu.id, result.workunitid
            );
            goto done;
        }

        retval = add_result_to_reply(result, wu, sreq, reply, bavp);

        // add_result_to_reply() fails only in fairly pathological cases -
        // e.g. we couldn't update the DB record or modify XML fields.
        // If this happens, don't replace the record in the array
        // (we can't anyway, since we marked the entry as "empty").
        // The feeder will eventually pick it up again,
        // and hopefully the problem won't happen twice.
        //
        goto done;

dont_send:
        // here we couldn't send the result for some reason --
        // set its state back to PRESENT
        //
        wu_result.state = WR_STATE_PRESENT;
done:
        lock_sema();
    }
    unlock_sema();
}

const char *BOINC_RCSID_d9f764fd14="$Id: sched_array.cpp 16246 2008-10-21 23:16:07Z davea $";