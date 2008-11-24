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

if (isset($_REQUEST['top']) && is_numeric($_REQUEST['top'])) {
  $top = $_REQUEST['top'];
} else { $top = 10; }
?>

<style type="text/css"> <!--

  th { background-color:#111111; border: thin dotted #999999; }

-->
</style>

<h1>Random Statistics</h1>
<p>
<br>
<table style="width:500px; background-color:#111111; border: thin dotted #999999;">
<tr><th colspan="3">Top Zones</th></tr>
<tr><th>#</th><th>Since Last Hour</th><th>Overall</th></tr>
<?php
  $td_style = "<td style='border-bottom: thin dotted #999999'>";

$query = <<<ENDQUERY
  select area_id,sum(normalized_time_in_area) 
  from hopeamd.snapshot_summary  where time_period > sysdate - 1/24
  group by area_id
  order by 2 desc
ENDQUERY;

$tzhallrows = oracle_query("fetching top hour zones", $oracleconn, $query);
$rows = sizeof($tzhallrows);

$query = <<<ENDQUERY
  select area_id,sum(normalized_time_in_area) 
  from hopeamd.snapshot_summary
  group by area_id
  order by 2 desc
ENDQUERY;

$tzallrows = oracle_query("fetching top zones", $oracleconn, $query);
$rows = sizeof($tzallrows);

if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo "<tr><td>".($i+1)."</td>".$td_style.$tzhallrows[$i]["AREA_ID"]."</td>".$td_style.$tzallrows[$i]["AREA_ID"]."</tr>";
  }
}

$allrows = oracle_query("fetching top talks", $oracleconn, $query);
$rows = sizeof($allrows);

?>
</table>
<br><br>
<table style="width:500px; background-color:#111111; border: thin dotted #999999;">
<tr><th colspan="3">Top Talks</th></tr>
<tr><th>Attendees</th><th>Talks</th></tr>  
<?php

$query = <<<ENDQUERY
  select t.speaker_name, t.talk_title, count(distinct p.person_id) attendees
  from    talk_presence p , talks t
  where p.talk_id = t.id
  group by t.talk_title, t.speaker_name
  order by 3 desc
ENDQUERY;

$allrows = oracle_query("fetching top interests", $oracleconn, $query);
$rows = sizeof($allrows);

if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo $td_style.$allrows[$i]["ATTENDEES"]."</td>".$td_style.$allrows[$i]["TALK_TITLE"]."<br />";
    echo "<i class=\"speaker\">".$allrows[$i]["SPEAKER_NAME"]."</i></td></tr>";
  }
}

?>
</table>
<br><br>
<table style="width:500px; background-color:#111111; border: thin dotted #999999;">
<tr><th colspan="3">Top Interests</th></tr>
<tr><th>#</th><th>Interests</th></tr>
<?php

$query = <<<ENDQUERY
  select  l.interest_name, count(u.interest) tms
  from    interests u , interests_list l
  where u.interest = l.interest_id
  group by l.interest_name
  order by 2 desc
ENDQUERY;

$allrows = oracle_query("fetching top interests", $oracleconn, $query);
$rows = sizeof($allrows);

if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo "<tr>".$td_style.$allrows[$i]["TMS"]."</td>".$td_style.$allrows[$i]["INTEREST_NAME"]."</td></tr>";
  }
}
?>
</table>
<br><br>
<table style="width:500px; background-color:#111111; border: thin dotted #999999;">
<tr><th colspan="3">Daily Activity</th></tr>
<tr><th>Hour</th><th>Users</th></tr>
<?php
  $hours = array();

  for ($i=0; $i < 25; $i++) {
    $query = "select count(distinct tag_id) u from snapshot_summary where ";
    $cond = "time_period < sysdate - ((1/24)*$i) and ";
    $cond = $cond . "time_period > sysdate - ((1/24)*($i+1))";

    $query2 = "select TO_CHAR(TRUNC(sysdate-($i/24),'HH24') ,'HH24') dd from snapshot_summary where ";
    $timerows = oracle_query("fetching top interests", $oracleconn, $query2.$cond);
    $allrows = oracle_query("fetching top interests", $oracleconn, $query.$cond);
    $rows = sizeof($allrows);
    if ($rows > 0) {
      for ($j = 0; $j < $rows; $j++) {
    echo "<tr>".$td_style.$timerows[$j]["DD"].":00:00"."</td>".$td_style.$allrows[$j]["U"]."</td></tr>";
      }
    }

  }
?>
</table>
<br><br>
<table style="width:500px; background-color:#111111; border: thin dotted #999999;">
    <tr><th colspan="3">User Stats for <?php echo $_SESSION['handle']; ?></th></tr>
    <tr><th colspan="3">You were last seen in
            <?php $userloc = get_user_location($_SESSION['id']); echo $userloc[0]; ?></th></tr>
    <tr><th colspan="3">Favorite Zones</th></tr>   

<?php

$query = <<<ENDQUERY
  select area_id,sum(normalized_time_in_area) 
  from hopeamd.snapshot_summary
  where tag_id = :hackerid
  group by area_id
  order by 2 desc
ENDQUERY;

$allrows = oracle_query("fetching top user zones", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
$rows = sizeof($allrows);
if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo "<tr>".$td_style.$allrows[$i]["AREA_ID"]."</td></tr>";
  }
}
?>

<tr><th colspan="3">Talks Attended</th></tr>
<?php

$query = <<<ENDQUERY
  select  t.speaker_name, t.talk_title
  from    talk_presence p , talks t
  where p.talk_id = t.id and
  p.person_id = :hackerid
ENDQUERY;

$allrows = oracle_query("fetching attended talks", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
$rows = sizeof($allrows);
if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo $td_style.$allrows[$i]["TALK_TITLE"]."<br />";
    echo "<i class=\"speaker\">".$allrows[$i]["SPEAKER_NAME"]."</i></td></tr>";
  }
}
?>

<tr><th colspan="3">People in your area</th></tr>

<?php
$query = <<<ENDQUERY
  select distinct p.handle 
  from person p, snapshot_summary z
  where p.id = z.tag_id and
  z.area_id = :hackerloc
ENDQUERY;

$allrows = oracle_query("fetching attended talks", $oracleconn, $query, array(":hackerloc" => $userloc));
$rows = sizeof($allrows);
if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo "<tr>".$td_style.$allrows[$i]["HANDLE"]."</td></tr>";
  }
 }

?>

<tr><th colspan="3">You have have pinged</th></tr>

<?php
$query = <<<ENDQUERY
  select  distinct p.handle
  from    person p , ping c
  where c.id = :hackerid and
        c.to_id = p.id
ENDQUERY;

$allrows = oracle_query("fetching pings", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
$rows = sizeof($allrows);
if ($rows > 0) {
  for ($i = 0; $i < $rows; $i++) {
    echo "<tr>".$td_style.$allrows[$i]["HANDLE"]."</td></tr>";
  }
}
?>
</table>

<?php include("footer.php"); ?>
