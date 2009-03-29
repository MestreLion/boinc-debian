# $Id: boinc_project_path.py 2867 2004-01-13 06:51:19Z quarl $

# boinc_project_path.py - defines the directories where config.xml and
# run_state.xml are kept.
#
# You can override these defaults with one of these options:
#    1) modify this file directly (if you have only one project on your server
#       or have separate copies for each)
#    2) create a new boinc_project_path.py and place it earlier in PYTHONPATH
#       than the default one
#    3) define environment variables

import sys, os

PROGRAM_DIR = os.path.dirname(sys.argv[0])
PROGRAM_PARENT_DIR = os.path.join(PROGRAM_DIR,os.pardir)

config_xml_filename = os.environ.get(
    'BOINC_CONFIG_XML',
    os.path.join(PROGRAM_PARENT_DIR,'config.xml'))

project_xml_filename = os.environ.get(
    'BOINC_PROJECT_XML',
    os.path.join(PROGRAM_PARENT_DIR,'project.xml'))

run_state_xml_filename = os.environ.get(
    'BOINC_RUN_STATE_XML',
    os.path.join(PROGRAM_PARENT_DIR,'run_state.xml'))