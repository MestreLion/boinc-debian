<?php
$cvs_version_tracker[]="\$Id: team_join_action.php 13985 2007-10-29 16:38:25Z davea $";  //Generated automatically - do not edit

require_once("../inc/boinc_db.inc");
require_once("../inc/util.inc");
require_once("../inc/team.inc");

$user = get_logged_in_user(true);
check_tokens($user->authenticator);

$teamid = post_int("teamid");
$team = BoincTeam::lookup_id($teamid);
require_team($team);
if ($user->teamid == $team->id) {
    page_head("Unable to add $user->name");
    echo "You are already a member of $team->name.";
} else {
    $success = user_join_team($team, $user);
    if ($success) {
        page_head("Joined $team->name");
        echo "You have joined
            <a href=team_display.php?teamid=$team->id>$team->name</a>.
        ";
    } else {
        error_page("Couldn't join team - please try later.");
    }
}

page_tail();

?>