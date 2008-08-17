<?php
$cvs_version_tracker[]="\$Id: result_summary.php 5353 2005-02-08 04:38:31Z ballen $";  //Generated automatically - do not edit
// show summary of results that have been received or timed out recently

require_once("../inc/util_ops.inc");

db_init();
admin_page_head("Result summary");

show_result_summary();

admin_page_tail();
?>
