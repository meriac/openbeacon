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


if (isset($_REQUEST['room']) && preg_match("/^[0-9A-Za-z]{1,15}$/",$_REQUEST['room'])) {
  $room = $_REQUEST['room'];
}

include('showuser.php');

include('config.php');

$query = "select Talk_Title, Speaker_Name from Talks where Talk_Time >= sysdate-1/24 and Track='$room' and rownum=1 order by Talk_Time asc";
$result = oracle_query("get talk info", $oracleconn, $query);

$row = $result[0];
?>

<b><?= $row["TALK_TITLE"]?></b><br><b>Speaker: </b><i><?=$row["SPEAKER_NAME"]?></i><br>

<div title="test">
<?
$users = get_room_users($room);
array_pop($users);

?>
<table width=100%>
<?
foreach($users as $user) {
  $user = split("[|]",$user);
  //why the FUCK are we executing queries every user?
  show_bitch($user[1]);
} 
?>
</table>

<?

function show_bitch($id) {
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

if ($numrows == 1) {
  $row = $allrows[0];
  $phonebit = $row["SMS_COUNT"];
  $emailbit = $row["EMAIL_COUNT"]; ?>

<tr>
  <td><a href="mail.php?medium=11&to=<?= $row['ID']?>&body=1" style="text-decoration: none; border: none;">
    <img src="/images/phone.png" style="cursor: pointer;"></a></td>
  <td><a href="mail.php?medium=00&to=<?= $row['ID']?>&body=1" style="text-decoration: none; border: none;">
    <img src="/images/email_out.png" style="cursor: pointer;"></a></td>
  <td><?=$row['HANDLE'];?></td>
  <td><img src="/avatars/<?= $row['ID'] ?>.png"></td>
</tr>
<? }
}
?>
</div>
