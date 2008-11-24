<?php
session_start();

include('config.php');  
include('showuser.php');

if (isset($_REQUEST['tagid']) && preg_match("/^[0-9]{1,5}$/",$_REQUEST['tagid'])) {
    $tagid = $_REQUEST['tagid'];
}
else {
    $_SESSION['error'] = "Listen, you cockmunch. Keep trying to hack this shit and we're going CCNa on your ass.";
    header("Location: index.php");
    die();
}

show_user($tagid, NULL);
?>
