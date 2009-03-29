<?php

require_once("../inc/boinc_db.inc");
require_once("../inc/user.inc");
require_once("../inc/util.inc");
require_once("../inc/countries.inc");

$user = get_logged_in_user();

$name = boinc_htmlentities(process_user_text(post_str("user_name")));
if ($name != strip_tags($name)) {
    error_page("HTML tags not allowed in name");
}
if (strlen($name) == 0) {
   error_page("You must supply a name for your account.");
}
$url = process_user_text(post_str("url", true));
$url = strip_tags($url);
$country = post_str("country");
if ($country == "") {
    $country = "International";
}
if (!is_valid_country($country)) {
    error_page("bad country");
}
$postal_code = process_user_text(post_str("postal_code",true));
$postal_code = strip_tags($postal_code);

$result = $user->update("name='$name', url='$url', country='$country', postal_code='$postal_code'");
if ($result) {
    Header("Location: home.php");
} else {
    error_page("Couldn't update user info.");
}

?>