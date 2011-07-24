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

// get_file [options]
// --host_id N              ID of host to upload from
// --file_name name         name of specific file, dominates workunit
//
// Create a result entries, initialized to sent, and corresponding
// messages to the host that is assumed to have the file.
//
// Run from the project root dir.

#include "config.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string>
#include <time.h>

#include "backend_lib.h"

#include "sched_config.h"
#include "sched_util.h"
#include "svn_version.h"

void usage() {
    fprintf(stderr, "Gets a file from a specific host.\n"
        "Usage: get_file [options]\n\n"
        "Retrieve a file from a host.\n"
        "Options:\n"
        "  --host_id id           Host to get file from\n"
        "  --file_name name       Name of filE\n"
        "  [ -v | --version ]     Show version\n"
        "  [ -h | --help ]        Show help\n"
    );
}

int main(int argc, char** argv) {
    int i, retval;
    char file_name[256];
    int host_id;

    strcpy(file_name, "");
    host_id = 0;

    check_stop_daemons();

    for (i=1; i<argc; i++) {
        if (is_arg(argv[i], "host_id")) {
            if (!argv[++i]) {
                usage();
                exit(1);
            }
            host_id = atoi(argv[i]);
        } else if (is_arg(argv[i], "file_name")) {
            if (!argv[++i]) {
                usage();
                exit(1);
            }
            strcpy(file_name, argv[i]);
        } else if (is_arg(argv[i], "h") || is_arg(argv[i], "help")) {
            usage();
            exit(0);
        } else if (is_arg(argv[i], "v") || is_arg(argv[i], "version")) {
            printf("%s\n", SVN_VERSION);
            exit(0);
        } else {
            usage();
            exit(1);
        }
    }

    if (!strlen(file_name) || host_id == 0) {
        usage();
        exit(1);
    }

    retval = config.parse_file();
    if (retval) {
        fprintf(stderr, "Can't parse config.xml: %s\n", boincerror(retval));
        exit(1);
    }

    retval = boinc_db.open(
        config.db_name, config.db_host, config.db_user, config.db_passwd
    );
    if (retval) {
        fprintf(stderr, "boinc_db.open failed: %s\n", boincerror(retval));
        exit(1);
    }

    retval = get_file(host_id, file_name);
    boinc_db.close();
    return retval;
}

const char *BOINC_RCSID_37238a0141 = "$Id: get_file.cpp 23425 2011-04-24 02:00:27Z davea $";
