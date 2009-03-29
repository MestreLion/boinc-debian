#!/usr/bin/env php

<?php

// This script for use ONLY by the BOINC-teams project.
// It generates an XML file with team and user info

require_once("../inc/boinc_db.inc");
require_once("../inc/util.inc");

function handle_team($team, $f) {
    $user = BoincUser::lookup_id($team->userid);
    if (!$user) {
        echo "no user for team $team->id\n";
        exit(1);
    }
    if ($user->teamid != $team->id) {
        echo "Founder is not member of $team->name\n";
        return;
    }
    if (!$user->email_validated) {
        echo "the founder of $team->name, $user->email_addr, is not validated\n";
        return;
    }
    $user_email_munged = str_rot13($user->email_addr);
    fwrite($f, 
"<team>
   <name>".htmlspecialchars($team->name)."</name>
   <url>".htmlspecialchars($team->url)."</url>
   <type>$team->type</type>
   <name_html>".htmlspecialchars($team->name_html)."</name_html>
   <description>
".htmlspecialchars($team->description)."
    </description>
   <country>$team->country</country>
   <id>$team->id</id>
   <user_email_munged>$user_email_munged</user_email_munged>
   <user_name>".htmlspecialchars($user->name)."</user_name>
   <user_country>".htmlspecialchars($user->country)."</user_country>
   <user_postal_code>".htmlspecialchars($user->postal_code)."</user_postal_code>
   <user_url>".htmlspecialchars($user->url)."</user_url>
</team>
"
    );
}

function main() {
    $f = fopen("temp.xml", "w");
    $teams = BoincTeam::enum();
    fwrite($f, "<teams>\n");
    foreach($teams as $team) {
        handle_team($team, $f);
    }
    fwrite($f, "</teams>\n");
    fclose($f);
    rename("temp.xml", "/home/boincadm/boinc/doc/boinc_teams.xml");
}

main();

?>