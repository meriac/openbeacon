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

$query = <<<ENDQUERY
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
          AND t.talk_time >= sysdate
      GROUP BY p1.id, p1.handle, t.id, t.Talk_Title, (t.talk_time-to_date('01-jan-1970'))*86400, t.speaker_name
      ORDER BY count(*) DESC, dbms_random.random
ENDQUERY;

$result = oracle_query("get personalized schedule", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));

?>
<h2>Recommended Talks: <?php echo $_SESSION['handle']; ?></h2><br><?php
 
 
 $rows = sizeof($result);
 echo "<center><table style='background-color:#0000; border: thin dotted #999999;'><tr><td>";
 if ($rows > 0) {
          for ($i = 0; $i < $rows; $i++) {
                  $row=$result[$i];
                  ?><div style="border-bottom: thin dotted #999999; padding: 5px; width: 600px;"><?php
                  echo $row["TALK_TITLE"] . "<br>(";
                  echo "<font class='speaker'>" . $row["SPEAKER_NAME"] . " <i>";
                  echo date("l Hs",$row["TALK_TIME"]) . "</i></font>)  based on " . $row["COMMON_INTERESTS"];
          ?></div><?php
          }
  }
  else {
  ?>You should <a href="schedule.php">sign up</a> for some talks!
  <?php
  }
 ?>

</td></tr></table></center><?php
 include('footer.php');

?>
