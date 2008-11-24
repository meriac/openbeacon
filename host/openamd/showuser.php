<?php

/***************************************************************
 *
 * amd.hope.net - index.php
 *
 * Copyright 2008 The OpenAMD Project <contribute@openamd.org>
 *
/***************************************************************

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/


//session_start();

function show_user($id, $interests) {

global $oracleconn;

if (!preg_match("/^[0-9]{4}$/",$id)) {
    echo "ID not valid.";
}
else {
 
$query = <<< ENDQUERY
select  p.id, p.handle, p.email, p.phone,
        sum(case when g.pingtype='sms' then 1 else 0 end) sms_count,
        sum(case when g.pingtype='email' then 1 else 0 end) email_count
from    person p
        left join ping g on
                p.id = g.to_id
            and g.id = :hackerid
where   p.id = $id
group by p.id, p.handle, p.email, p.phone
ENDQUERY;

$allrows = oracle_query("get user data", $oracleconn, $query, array(":hackerid"=>$_SESSION['id']));
$numrows = sizeof($allrows);

$userloc = get_user_location($id);
#$userloc = array("unknown");

if ($numrows == 1) {
    $row = $allrows[0];
    $phonebit = $row["SMS_COUNT"];
    $emailbit = $row["EMAIL_COUNT"];
    ?>
    <table><tr><td><?php echo $row["HANDLE"] . " (" . $userloc[0] . ")"; if ($interests) {?> (shared interests: <?php echo $interests?>)<?php } ?></td></tr>
    <tr><td><?
    if($row["EMAIL"] && isset($_SESSION['id'])) {
        if ($emailbit) {
            ?><img src="images/email_out.png" title="Already pinged." alt="Already pinged."><?php
        }
        else {
            ?><a href="mail.php?medium=00&to=<?php
                echo $row["ID"]; ?>&body=1"><img src="images/email.png" title="Send Email Ping" alt="Send Email Ping" border=0></a><?php
        }
    }
    if ($row["PHONE"] && isset($_SESSION['id'])) {
        if ($phonebit) {
            ?><img src="images/phone_out.png" title="Already pinged." alt="Already pinged."><?php
        }
        else {
            ?><a href="mail.php?medium=11&to=<?php
                echo $row["ID"]; ?>&body=1"><img src="images/phone.png" title="Send SMS Ping" alt="Send SMS Ping" border=0></a><?php
        }
    }
    ?></td></tr></table><?php
}
else {
    echo "$id has no profile yet.<br>";
}
}
}

?>
