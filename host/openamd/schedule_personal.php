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


include('header.php');
?>
<h2>Personalized Schedule: <?php echo $_SESSION['handle']; ?></h2>
<a href="schedule.php">Add More Talks</a>

<?php

$query = <<<ENDQUERY
select t.id, t.talk_title, t.speaker_name, (t.talk_time-to_date('01-jan-1970'))*86400 talk_time, t.track
from   Talks t,
Talks_List x
where  x.hacker_id = :hackerid
and x.talk_id = t.id
order by t.talk_time asc
ENDQUERY;

$result = oracle_query("get personalized schedule", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));



$rows = sizeof($result);
echo "<center><table style='background-color:#111111; border: thin dotted #999999;'><tr><td>";
if ($rows > 0) {
         for ($i = 0; $i < $rows; $i++) {
                 $row=$result[$i];
                 ?><div style="border-bottom: thin dotted #999999; padding: 5px; width: 600px;"><?php
                 echo $row["TALK_TITLE"] . "<br>(";
                 echo "<font class='speaker'>" . $row["SPEAKER_NAME"] . " <i>";
                 echo date("l Hs",$row["TALK_TIME"]) . "</i> - " . $row["TRACK"] . "</font>) ";
                 ?>[ <a href="schedule_remove.php?talk=<?php
                 echo $row["ID"]; ?>">Remove</a> ]</div><?php
         }
 }
 else {
 ?>You should <a href="schedule.php">sign up</a> for some talks!
 <?php
 }
?></td></tr></table></center><?php
include('footer.php');

?>
