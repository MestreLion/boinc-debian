#!/bin/sh
#
# This script modifies the default BOINC installation on AMD64 to
# download and use 32bit applications. See paragraph 4. in
# /usr/share/doc/boinc-client/README.Debian for more information.
#
# Copyright © 2006 Debian BOINC Maintainers
#                  <pkg-boinc-devel@lists.alioth.debian.org>
#
# This file is licensed under the terms of the GNU General Public License,
# Version 2 or any later version published by the Free Software Foundation.

BOINC_DIR=/var/lib/boinc-client

ARCH=`dpkg --print-architecture`
if [ "$ARCH" != "amd64" ]; then
    echo "Error: dpkg architecture ($ARCH) is not amd64."
    exit 1
fi

LIBC6_STATUS=`dpkg-query -f '${Status}' -W libc6-i386 | grep not-installed`
if [ -n "$LIBC6_STATUS" ]; then
    apt-get install libc6-i386
fi

invoke-rc.d boinc-client stop

PLATFORM_AMD64="<platform_name>x86_64-pc-linux-gnu<\/platform_name>"
PLATFORM_I386="<platform_name>i686-pc-linux-gnu<\/platform_name>"

echo "Trying to substitute platform name in client state file..."
sed -i -e "s/$PLATFORM_AMD64/$PLATFORM_I386/" "$BOINC_DIR/client_state.xml"

invoke-rc.d boinc-client start
