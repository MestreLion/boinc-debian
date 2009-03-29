<?php

require_once("../inc/db.inc");
require_once("../inc/util.inc");
require_once("../inc/email.inc");

db_init();
$user = get_logged_in_user();

page_head("Change email address");

if (is_valid_email_addr($user->email_addr)) {
    $email_text = $user->email_addr;
}

echo "<form method=post action=edit_email_action.php>\n";
start_table();
row1("Change the email address of your account");
row2("New email address
    <br><font size=-2>Must be a valid address of the form 'name@domain'</font>",
    "<input name=email_addr size=50 value='$email_text'>"
);

// we need the password here not for verification,
// but because we store it salted with email address,
// which is about to change.

row2(
    "Password
    <br><a href=edit_passwd_form.php><font size=-2>No password?</font></a>",
    "<input type=password name=passwd>"
);
row2("", "<input type=submit value='Change email address'>");
end_table();
echo "</form>\n";
page_tail();

?>