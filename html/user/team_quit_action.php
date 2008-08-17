<?php
$cvs_version_tracker[]="\$Id: team_quit_action.php 13731 2007-10-02 03:19:03Z davea $";  //Generated automatically - do not edit

require_once("../inc/db.inc");
require_once("../inc/util.inc");
require_once("../inc/team.inc");

db_init();
$user = get_logged_in_user(true);
check_tokens($user->authenticator);
$teamid = post_int("id");
$team = lookup_team($teamid);
if ($user->teamid == $team->id) {
    user_quit_team($user);
    Header("Location: home.php");
} else {
    page_head("Unable to quit team");
    echo "Team doesn't exist, or you don't belong to it.\n";
}

page_tail();

?>
