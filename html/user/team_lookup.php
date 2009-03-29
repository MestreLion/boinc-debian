<?php

require_once("../inc/boinc_db.inc");
require_once("../inc/util.inc");
require_once("../inc/team.inc");

$format = get_str("format", true);
$team_id = get_int("team_id", true);

if ($team_id || ($format == 'xml')) {
    require_once ('../inc/xml.inc');
    xml_header();
    $retval = db_init_xml();
    if ($retval) xml_error($retval);
}

if ($team_id) {
    $team = lookup_team($team_id);
    if ($team) {
        show_team_xml($team);
    } else {
        xml_error(-136);
    }
    exit();
}

$team_name = get_str("team_name");
$name_lc = strtolower($team_name);
$name_lc = escape_pattern($name_lc);
 
$clause = "name like '%".boinc_real_escape_string($name_lc)."%' order by expavg_credit desc limit 100";
$teams = BoincTeam::enum($clause);

if ($format == 'xml') {
    echo "<teams>\n";
    $total = 0;
    foreach($teams as $team) {
        show_team_xml($team);
        $total++;
        if ($total == 100) break;
    }
    echo "</teams>\n";
    exit();
}

page_head("Search Results");
if (count($teams)) {
    echo "<h2>Search results for '".strip_tags($team_name)."'</h2>";
    echo "<p>";
    echo "You may view these teams' members, statistics, and information.";
    echo "<ul>";
    foreach($teams as $team) {
        echo "<li>";
        echo "<a href=team_display.php?teamid=$team->id>";
        echo "$team->name</a></li>";
    }
    echo "</ul>";
    if (count($teams)==100) {
        echo "
            More than 100 teams match your search.
            The first 100 are shown.<br>
        ";
    }
}
echo "End of results<br>";
echo "If you cannot find the team you are looking for, you may create a team ";
echo "by clicking <a href=team_create_form.php>here</a>.";
page_tail();

?>