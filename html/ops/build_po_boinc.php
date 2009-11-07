#!/usr/bin/php
<?php

// generate translation template "en.po" for BOINC web pages.
//
// Projects: don't use this.  Use build_po.php instead.
//
// Run this in boinc/html;
// move the result to boinc/locale/templates/BOINC-Web.pot

$FILE_LIST = "inc/*.inc user/*.php project.sample/*.inc";

$date = strftime('%Y-%m-%d %H:%M %Z');
$header = <<<HDR
# BOINC web translation
# Copyright (C) 2008 University of California
#
# This file is distributed under the same license as BOINC.
#
# FileID  : \$Id\$
#
msgid ""
msgstr ""
"Project-Id-Version: BOINC \$Id\$\\n"
"Report-Msgid-Bugs-To: BOINC translation team <boinc_loc@boinc.berkeley.edu>\\n"
"POT-Creation-Date: $date\\n"
"Last-Translator: Generated automatically from source files\\n"
"MIME-Version: 1.0\\n"
"Content-Type: text/plain; charset=utf-8\\n"
"Content-Transfer-Encoding: 8bit\\n"
"X-Poedit-SourceCharset: utf-8\\n"

msgid "LANG_NAME_NATIVE"
msgstr "English"

msgid "LANG_NAME_INTERNATIONAL"
msgstr "English"

HDR;

$out = fopen("en.po", "w");

fwrite($out, $header);

$pipe = popen(
    "xgettext --omit-header -o - --keyword=tra -L PHP $FILE_LIST",
    "r"
);
stream_copy_to_stream($pipe, $out);

fclose($pipe);
fclose($out);

?>