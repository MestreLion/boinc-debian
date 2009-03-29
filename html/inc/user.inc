<?php

require_once("../inc/credit.inc");
require_once("../inc/email.inc");
require_once("../inc/util.inc");
require_once("../inc/team.inc");
require_once("../inc/friend.inc");
require_once("../inc/forum_db.inc");
require_once("../inc/notify.inc");

function parse_project($f) {
    $p->total_credit = 0.0;
    $p->expavg_credit = 0.0;
    while (!feof($f)) {
        $buf = fgets($f);
        if (strstr($buf, "</project>")) break;
        if ($x = parse_element($buf, "<name>")) {
            $p->name = $x;
        }
        if ($x = parse_element($buf, "<name>")) {
            $p->name = $x;
        }
        if ($x = parse_element($buf, "<url>")) {
            $p->url = $x;
        }
        if ($x = parse_element($buf, "<total_credit>")) {
            $p->total_credit = $x;
        }
        if ($x = parse_element($buf, "<expavg_credit>")) {
            $p->expavg_credit = $x;
        }
        if ($x = parse_element($buf, "<id>")) {
            $p->id = $x;
        }
        if ($x = parse_element($buf, "<country>")) {
            $p->country = $x;
        }
        if ($x = parse_element($buf, "<team_id>")) {
            $p->team_id = $x;
        }
        if ($x = parse_element($buf, "<team_name>")) {
            $p->team_name = $x;
        }
        if ($x = parse_element($buf, "<create_time>")) {
            $p->create_time = $x;
        }
    }
    return $p;
}

function parse_user($f, $user) {
    $user->projects = array();
    while (!feof($f)) {
        $buf = fgets($f);
        if (strstr($buf, "</user>")) break;
        if (strstr($buf, "<project>")) {
            $user->projects[] = parse_project($f);
        }
    }
    return $user;
}

function get_other_projects($user) {
    $cpid = md5($user->cross_project_id . $user->email_addr);
    $url = "http://boinc.netsoft-online.com/get_user.php?cpid=$cpid";
    $f = fopen($url, "r");
    if (!$f) {
        return $user;
    }
    $u = parse_user($f, $user);
    fclose($f);
    return $u;
}

function show_project($project) {
    if ($project->url == "http://www.worldcommunitygrid.org/") {
        $x = $project->name;
    } else {
        $x = "<a href=\"$project->url"."show_user.php?userid=$project->id\">$project->name</a>";
    }
    echo "<tr>
        <td>$x</td>
        <td align=right>".number_format($project->total_credit, 0)."</td>
        <td align=right>".number_format($project->expavg_credit, 0)."</td>
        <td align=right>".date_str($project->create_time)."</td>
        </tr>
    ";
}

function cmp($a, $b) {
    if ($a->expavg_credit == $b->expavg_credit) return 0;
    return ($a->expavg_credit < $b->expavg_credit)? 1 : -1;
}

function show_other_projects($user, $personal) {
    if (count($user->projects) > 1) {
        usort($user->projects, "cmp");
        if ($personal) {
            echo "<h3>Projects in which you are participating</h3>";
        } else {
            echo "<h3>Projects in which $user->name is participating</h3>";
        }
        start_table();
        row_heading_array(array(
            "Project<br><span class=note>Click for user page</span>", "Total credit", "Average credit", "Since"
        ));
        foreach($user->projects as $project) {
            show_project($project);
        }
        end_table();
    }
}

function total_posts($user) {
    return BoincPost::count("user=$user->id");
}

function show_credit($user) {
    row2(tra("Total credit"), format_credit_large($user->total_credit));
    row2(tra("Recent average credit"), format_credit($user->expavg_credit));
    project_user_credit($user);
}

require_once("../inc/stats_sites.inc");
// show dynamic user info (private)
//
function show_user_stats_private($user) {
    global $cpid_stats_sites;
    row1(tra("Computing and credit"));
    show_credit($user);
    $config = get_config();
    if (parse_bool($config, "show_results")) {
        row2(tra("Pending credit"), "<a href=\"pending.php\">".tra("View")."</a>");
    }
    row2(tra("Computers on this account"),
        "<a href=\"hosts_user.php\">".tra("View")."</a>"
    );
    row2(tra("Tasks"), "<a href=\"results.php?userid=$user->id\">".tra("View")."</a>");
    $cpid = md5($user->cross_project_id . $user->email_addr);
    $x = "";
    shuffle($cpid_stats_sites);
    foreach ($cpid_stats_sites as $site) {
        $name = $site[0];
        $y = sprintf($site[1], $cpid);
        $x .= "<a href=\"$y\">$name</a><br>";
    }
    $x .= "<br><span class=\"smalltext\">".tra("Cross-project ID").": $cpid</span>\n";
    row2(tra("Cross-project statistics"), $x);
    row2(tra("Stats on your cell phone"), URL_BASE."userw.php?id=$user->id");
}

function notify_description($notify) {
    switch ($notify->type) {
    case NOTIFY_FRIEND_REQ:
        return friend_notify_req_web_line($notify);
    case NOTIFY_FRIEND_ACCEPT:
        return friend_notify_accept_web_line($notify);
    case NOTIFY_PM:
        return pm_web_line($notify);
    case NOTIFY_SUBSCRIBED_POST:
        return subscribed_post_web_line($notify);
    }
    return "Unknown notification type: $notify->type";
}

function weak_auth($user) {
    $x = md5($user->authenticator.$user->passwd_hash);
    return "{$user->id}_$x";
}

// show static user info (private)
//
function show_user_info_private($user) {
    if (is_valid_email_addr($user->email_addr)) {
        $email_text = $user->email_addr;
    } else {
        $email_text = "Verification pending";
    }

    row1(tra("Account information"));
    row2(tra("Name"), $user->name);
    row2(tra("Email address"), $email_text);
    if (strlen($user->url)) {
        $x = "http://$user->url";
    } else {
        $x = "none";
    }
    row2(tra("URL"), $x);
    row2(tra("Country"), $user->country);
    row2(tra("Postal code"), $user->postal_code);
    row2(tra("%1 member since", PROJECT), date_str($user->create_time));
    row2(tra("Change"), "<a href=\"edit_email_form.php\">".tra("email address")."</a> | <a href=\"edit_passwd_form.php\">".tra("password")."</a> | <a href=\"edit_user_info_form.php\">".tra("other account info")."</a>");
    row2("", "<a href=\"logout.php?".url_tokens($user->authenticator)."\">".tra("Log out")."</a>");
    row2("User ID<br><span class=note>Used in community functions</span>", $user->id);
    row2("Account key<br><span class=note>Provides full access to your account</span>", $user->authenticator);
    row2("Weak account key<br><span class=note>Provides <a href=weak_auth.php> limited access</a> to your account</span>", weak_auth($user));

    row1("<a name=\"prefs\"></a>".tra("Preferences"));
    row2(
        tra("When and how BOINC uses your computer"),
        "<a href=\"prefs.php?subset=global\">".tra("Computing preferences")."</a>"
    );
    row2(tra("Message boards and private messages"),
        "<a href=\"edit_forum_preferences_form.php\">".tra("Community preferences")."</a>"
    );
    row2(tra("Resource share and graphics"),
        "<a href=\"prefs.php?subset=project\">".tra("%1 preferences", PROJECT)."</a>"
    );
}

function show_community_private($user) {
    row1(tra("Community"));

    if ($user->has_profile) {
        $x = "<a href=\"view_profile.php?userid=$user->id\">".tra("View")."</a> | <a href=\"delete_profile.php\">".tra("Delete")."</a>";
    } else {
        $x = "<a href=\"create_profile.php\">".tra("Create")."</a>";
    }
    row2(tra("Profile"), $x);
    $tot = total_posts($user);
    if ($tot) {
        row2(tra("Message boards"), "<a href=".URL_BASE."forum_user_posts.php?userid=$user->id>".tra("%1 posts", $tot)."</a>");
    }
    
    row2(tra("Private messages"), pm_notification($user).pm_email_remind($user));

    $notifies = BoincNotify::enum("userid=$user->id");
    if (count($notifies)) {
        $x = "";
        foreach ($notifies as $notify) {
            $x .= "&bull; ".notify_description($notify)."<br>";
        }
        $x .= "<a href=".notify_rss_url($user)."><img vspace=4 border=0 src=img/rss_icon.gif alt=RSS></a>";
        row2("Notifications", $x);
    }

    if ($user->teamid) {
        $team = lookup_team($user->teamid);
        $x = "<a href=\"team_display.php?teamid=$team->id\">$team->name</a>
            | <a href=\"team_quit_form.php\">".tra("Quit team")."</a>";
        if (is_team_admin($user, $team)) {
            $x .= " | <a href=\"team_manage.php?teamid=$user->teamid\">".tra("Administer")."</a>";
        }

        // if there's a foundership request, notify the founder
        //
        if ($user->id==$team->userid && $team->ping_user >0) {
            $x .= " <font color=red>(foundership change request pending)</font>";
        }
        row2(tra("Member of team"), $x);
    } else {
        row2(tra("Team"), tra("None")." | <a href=\"team_search.php\">".tra("find a team")."</a>");
    }
    
    $teams_founded = BoincTeam::enum("userid=$user->id");
    foreach ($teams_founded as $team) {
        if ($team->id != $user->teamid) {
            $x = "<a href=\"team_display.php?teamid=$team->id\">$team->name</a>";
            $x .= "| <a href=\"team_manage.php?teamid=".$team->id."\">".tra("Administer")."</a>";
            if ($team->ping_user > 0) {
                $x .= " <font color=red>(foundership change request pending)</font>";
            }
            row2(tra("Founder but not member of"), $x);
        }
    }

    $friends = BoincFriend::enum("user_src=$user->id and reciprocated=1");
    $x = "<a href=user_search.php>Find friends</a><br>\n";
    $n = count($friends);
    if ($n) {
        foreach($friends as $friend) {
            $fuser = BoincUser::lookup_id($friend->user_dest);
            $x .= friend_links($fuser);
        }
        row2("Friends ($n)", $x);
    } else {
        row2("Friends", $x);
    }
}

// show summary of dynamic and static info (public)
//
function show_user_summary_public($user) {
    global $g_logged_in_user;
    row2(PROJECT." member since", date_str($user->create_time));
    row2("Country", $user->country);
    if (strlen($user->url)) {
        row2("URL", "<a href=\"http://$user->url\">http://$user->url</a>");
    }
    show_credit($user);

    if ($user->show_hosts) {
        row2("Computers", "<a href=\"".URL_BASE."hosts_user.php?userid=$user->id\">View</a>");
    } else {
        row2("Computers", "hidden");
    }
    if ($user->donated == 1) { 
        if (file_exists("../project/donations.inc")) {
            require_once("../project/donations.inc");
            $x .= DONATION_LINK;
            row2("Donor",$x);
        }
    }
    
}

function community_links($user) {
    global $g_logged_in_user;
    if ($user->teamid && ($team = lookup_team($user->teamid))) {
        row2("Team", "<a href=\"".URL_BASE."team_display.php?teamid=$team->id\">$team->name</a>");
    } else {
        row2("Team", "None");
    }
    $tot = total_posts($user);
    if ($tot) {
        row2("Message boards", "<a href=\"".URL_BASE."forum_user_posts.php?userid=$user->id\">$tot posts</a>");
    }
    if ($g_logged_in_user && $g_logged_in_user->id != $user->id) {
        row2("Contact", "<a href=\"pm.php?action=new&userid=".$user->id."\">Send private message</a>");
        $friend = BoincFriend::lookup($g_logged_in_user->id, $user->id);
        if ($friend && $friend->reciprocated) {
            row2("This person is a friend", 
                "<a href=friend.php?action=cancel_confirm&userid=$user->id>Cancel friendship</a>"
            );
        } else if ($friend) {
            row2("Friends", "<a href=friend.php?action=add&userid=$user->id>Request pending</a>");
        } else {
            row2("Friends", "<a href=friend.php?action=add&userid=$user->id>Add as friend</a>");
        }
    }
    $friends = BoincFriend::enum("user_src=$user->id and reciprocated=1");
    $n = count($friends);
    if ($n) {
        $x = null;
        foreach($friends as $friend) {
            $fuser = BoincUser::lookup_id($friend->user_dest);
            $x .= friend_links($fuser);
        }
        row2("Friends ($n)", $x);
    }

}

function show_profile_link($user) {
    if ($user->has_profile) {
        row2("Profile", "<a href=\"view_profile.php?userid=$user->id\">View</a>");
    }
}

function make_user(
    $email_addr, $name, $passwd_hash,
    $country=null, $postal_code=null, $project_prefs=null, $teamid=0
) {
    if (!is_valid_email_addr($email_addr)) return null;
    $authenticator = random_string();
    $cross_project_id = random_string();
    $now = time();
    if (!is_valid_country($country)) return null;
    $country = BoincDb::escape_string($country);
    $postal_code = strip_tags(process_user_text($postal_code));

    $ret = BoincUser::insert("(create_time, email_addr, name, authenticator, country, postal_code, total_credit, expavg_credit, expavg_time, project_prefs, teamid,  send_email, show_hosts, cross_project_id, passwd_hash) values($now, '$email_addr', '$name', '$authenticator', '$country', '$postal_code', 0, 0, unix_timestamp(), '$project_prefs', $teamid, 1, 1, '$cross_project_id', '$passwd_hash')");

    if ($ret) {
        $db = BoincDb::get();
        $id = $db->insert_id();
        return BoincUser::lookup_id($id);
    } else {
        return null;
    }
}

$cvs_version_tracker[]="\$Id: user.inc 15070 2008-04-21 14:04:01Z boincadm $";  //Generated automatically - do not edit

?>