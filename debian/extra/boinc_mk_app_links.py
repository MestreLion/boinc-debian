#!/usr/bin/python
# -*- coding: utf-8 -*-

# This script creates symlinks for anonymous BOINC applications.
# Copyright © 2006 Debian BOINC Maintainers
#                  <pkg-boinc-devel@lists.alioth.debian.org>
#
# This file is licensed under the terms of the GNU General Public License,
# Version 2 or any later version published by the Free Software Foundation.
#
# $Id$

import os
import sys

app_info_dir = '/usr/share/boinc-client/app_info/'

def main():
    if not os.path.isdir(app_info_dir):
        sys.exit(0)

    if len(sys.argv) < 2:
        print >> sys.stderr, 'Error: missing path argument'
        sys.exit(1)

    projects = {}

    for file in os.listdir(app_info_dir):
        if file.endswith('.py'):
            execfile(app_info_dir + file)

    for project in projects:
        for url in projects[project]['urls']:
            project_dir = sys.argv[1] + '/projects/' + url + '/'
            try:
                os.makedirs(project_dir)
            except OSError:
                pass

            renew_symlink(projects[project]['app_info'],
                project_dir + 'app_info.xml')

            for app in projects[project]['apps']:
                renew_symlink(app, project_dir + os.path.basename(app))
    return

def renew_symlink(src, dst):
    if os.path.isfile(dst) and not os.path.islink(dst):
        return
    elif os.path.exists(dst):
        os.remove(dst)

    os.symlink(src, dst)
    return

if __name__ == '__main__':
    main()
