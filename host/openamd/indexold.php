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

include("header.php");
if (!isset($_SESSION['handle'])) {
?>
Please <a href="login.php">sign in</a> to take advantage of the OpenAMD system.
<?php
}
else {  // What do show if user is logged in
?><br /><p><img src="/avatars/<?= $_SESSION['id'] ?>.png"></p><br /><h2>Welcome, <?php echo $_SESSION['handle']; ?></h2>
<?php if (isset($_SESSION['rfid'])) { ?>
<br>You are in <?php $userloc = get_user_location($_SESSION['id']); echo $userloc[0]; ?>.
<?php }
else { echo "<br>"; }
?>
<table cellspacing=10><tr><td valign="top">
<div id="upcomingtalks" style="background-color:#111111; border: thin dotted #999999; width:500px;">
<h3>Your Schedule (<a href="schedule_personal.php">full list</a>):</h3><?php

$query = <<<ENDQUERY
    select t.id, t.talk_title, t.speaker_name, (t.talk_time-to_date('01-jan-1970'))*86400 talk_time, t.track
    from   Talks t,
           Talks_List x
       where  x.hacker_id = :hackerid
       and x.talk_id = t.id and rownum<=3 and t.talk_time > sysdate
    order by t.talk_time asc
ENDQUERY;

$allrows = oracle_query("fetching upcoming talks", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
$rows = sizeof($allrows);

if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    $row=$allrows[$i];
    ?><div style="border-bottom: thin dotted #999999; padding: 5px;"><?php
    echo $row["TALK_TITLE"] . "<br>(";
    echo "<font class='speaker'>" . $row["SPEAKER_NAME"] . " <i>";
    echo date("l Hs",$row["TALK_TIME"]) . "</i> - " . $row["TRACK"] . "</font>) ";
    ?>[ <a href="schedule_remove.php?talk=<?php
    echo $row["ID"]; ?>">Remove</a> ]</div><?php
  }
} 
else { 
  ?>You have no upcoming talks. You should <a href="schedule.php">sign up</a> for some talks!
  <?php
}
?></div>
</td><td valign="top">
<div id="similarusers">
<h3>Similar Users</h3><?php

$query = <<< ENDQUERY
select  p2.id,
        p2.handle,
        count(*) common_interest_count,
        collect_concat(cast(collect (i.interest_name) as varchar2_t)) common_interests
from    person p1
       ,interests i1
       ,person p2
       ,interests i2
       ,interests_list i
where   p1.id = i1.id
    and p2.id = i2.id
    and i1.interest = i2.interest
    and i.interest_id = i2.interest
    and p1.id != p2.id
    and p1.id = :hackerid
    and rownum <= 3
group by p1.id, p1.handle, p2.id, p2.handle
order by count(*) desc
ENDQUERY;

$allrows = oracle_query("fetching similar users", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
$rows = sizeof($allrows);

if ($rows > 0) {
  ?><p><h3>Users like You:</h3><?php
  for ($i = 0; $i < $rows; $i++) {
    $row=$allrows[$i];
    ?><div style="border-bottom: thin dotted #999999;"><?php
    show_user($row["ID"], $row["COMMON_INTERESTS"]);
    ?></div><?php
  }
} 
else { ?>
Choose interests in your <a href="login.php">profile</a> and we will recommend people with similar interests.
<?php
}
?>
</div></td></tr>
<tr><td valign="top">
<?php
$talksrec = <<< ENDQUERY
      SELECT  t.id,
              t.Talk_Title,
          (t.talk_time-to_date('01-jan-1970'))*86400 talk_time,
              t.speaker_name,
              count(*) common_interest_count,
              collect_concat(cast(collect (i.interest_name) AS varchar2_t)) common_interests
      FROM    person p1
             ,interests i1
             ,Talks t
             ,talks_interests ti
             ,interests_list i
      WHERE   p1.id = i1.id
          AND i1.interest = ti.interest
          AND ti.talk_id = t.id
          AND i.interest_id = ti.interest
          AND p1.id = :hackerid
          AND rownum <= 3
      GROUP BY p1.id, p1.handle, t.id, t.Talk_Title, (t.talk_time-to_date('01-jan-1970'))*86400, t.speaker_name
      ORDER BY count(*) DESC, dbms_random.random
ENDQUERY;
$talksrecr = oracle_query("fetching talks you'd like", $oracleconn, $talksrec, array(":hackerid" => $_SESSION['id']));
$talksrecrows = sizeof($talksrecr);

if ($talksrecrows>0) {
  ?><p><h3>Recommended Talks (<a href="schedule_recommended.php">full list</a>):</h3><?php
  for($i = 0; $i < $talksrecrows; $i++) {
    $row=$talksrecr[$i];
    ?><div style="border-bottom: thin dotted #999999; padding: 5px;"><?php
    echo $row["TALK_TITLE"] . "<br>(";
    echo "<font class='speaker'>" . $row["SPEAKER_NAME"] . " <i>";
    echo date("l Hs",$row["TALK_TIME"]) . "</i></font>) ";
    ?></div><?php
  }
}
else {
?><h3>Recommended Talks</h3>
Choose interests in your <a href="login.php">profile</a> and we will recommend talks you might like.<?php
}
?>
</td>
<td style="background-color:#111111; border: thin dotted #999999">

<div id="newusers">
<h3>New Users</h3><?php
$newquery = "select Person.ID from Person, Creation where creation.id = person.id and Creation.Registered=1 and rownum <= 3 order by Creation.Timestamp";
$newresult = $oracle_query("fetching new users", $oracleconn, $newquery);
$rows = sizeof($newresult);
if ($rows > 0) {
  for($i = 0; $i < $rows; $i++) {
    ?><div style="border-bottom: thin dotted #999999; padding: 5px;"><?php
    show_user($newresult[$i]["ID"], NULL);
    ?></div><?php
  }
}
else {
  ?>There is a problem showing new users. Tell the OpenAMD administrator.<?php
}
}
?>
</div></td></tr></table>
<?php
include("footer.php");

?>
