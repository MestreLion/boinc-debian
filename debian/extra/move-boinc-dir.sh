#!/bin/sh
#
# This script can be used to move an BOINC data directory to Debian's
# default location of the BOINC data directory /var/lib/boinc-client.
# Invoke it with the old data directory as it's only argument,
# for example:
#    % move-boinc-dir.sh /home/john/BOINC
#
# Copyright © 2006 Debian BOINC Maintainers
#                  <pkg-boinc-devel@lists.alioth.debian.org>
#
# This file is licensed under the terms of the GNU General Public License,
# Version 2 or any later version published by the Free Software Foundation.

set -e

OLD_DIR="$1"
if [ -z "$OLD_DIR" ]; then
    echo "Error: Old BOINC directory not specified."
    exit 1
elif [ ! -d "$OLD_DIR" ]; then
    echo "Error: $OLD_DIR does not exists or is not a directory."
    exit 1
fi

BOINC_DIR=/var/lib/boinc-client
CONF_DIR=/etc/boinc-client

move_configfile_and_symlink()
{
    if [ ! -L $BOINC_DIR/$1 -a -f $BOINC_DIR/$1 ]; then
        mv -f $BOINC_DIR/$1 $CONF_DIR/$1
    fi
    ln -sf $CONF_DIR/$1 $BOINC_DIR/$1
    chown boinc:boinc $BOINC_DIR/$1 || true
}

# Stop the BOINC core client.
invoke-rc.d boinc-client stop

# Copy the old BOINC directory to the new location.
mkdir -p $BOINC_DIR || true
cp -R -T "$OLD_DIR" $BOINC_DIR

# Set the user boinc as the owner for
# all files in /var/lib/boinc-client.
chown -R boinc:boinc $BOINC_DIR || true

# Move old configuration files to /etc/boinc-client/ and
# create symlinks for the BOINC core client.
move_configfile_and_symlink global_prefs_override.xml
move_configfile_and_symlink gui_rpc_auth.cfg
move_configfile_and_symlink log_flags.xml
move_configfile_and_symlink remote_hosts.cfg

# Start the BOINC core client again.
invoke-rc.d boinc-client start
