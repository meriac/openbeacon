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
 ?>

<style type="text/css"> <!--

.hidden { visibility: hidden; }
.unhidden { visibility: visible; }

th { background-color:#111111; border: thin dotted #999999; }

-->

</style>
<script type="text/javascript" src="/javascripts/jquery.js"></script>
<script type="text/javascript" src="/javascripts/jqModal.js"></script>
<script type="text/javascript" src="/javascripts/jqDnR.js"></script>
<script type="text/javascript" src="/javascripts/jquery-easing.js"></script>
<script type="text/javascript">
$().ready(function() {
  $("#schedule_details")
    .jqDrag('.jqDrag')
    .jqResize('.jqResize')
    .jqm({
      overlay: 0
     //,onShow: function(h) {
     //     h.w.css('opacity',0.92).slideDown();
     //   }
     //,onHide: function(h) {
     //     h.w.slideUp("slow",function() { if(h.o) h.o.remove(); });
     //   }
      });
});
function showDetails(datasource) {
  var url = "schedule_details.php?talk=" + datasource;
  $("#schedule_details_content").load(url, {}, function() { $("#schedule_details").jqmShow(); });
}
</script>
<div style="position: absolute; margin: 0px 0 0 100px;">
<div id="schedule_details" class="jqmNotice">
  <div id="schedule_details_title" class="jqmnTitle jqDrag">Talk Detail</div>
  <div id="schedule_details_content" class="jqmnContent">
  </div>
  <img src="notice/close_icon.png" alt="close" class="jqmClose" />
  <!--<img src="dialog/resize.gif" alt="resize" class="jqResize" />-->
</div>
</div>
<div id="col2">
<?php if (isset($_SESSION['handle'])) { ?>
<h2>Schedule: <?php echo $_SESSION['handle']; ?></h2>
Get reminders for talks you want to see by checking the boxes and clicking "Update Schedule".<br>
<a href="schedule_personal.php">View Personal Schedule</a>
<form action="schedule_submit.php" method="post"><?php
}
else {?>
<h2>Schedule</h2>
<?php
}
?>
<center>
<div id="day0">
<?php if (isset($_SESSION['handle'])) { ?>
<input type="submit" class="formz" onmouseover="this.style.backgroundColor='#3080f0';" onmouseout="this.style.backgroundColor='';" value="Update Schedule">
<?php
}
// Fetch SQL containing schedule data

if (array_key_exists('id', $_SESSION)) {
    $queryid = $_SESSION['id'];
} 
else 
{
    $queryid = -1234;
}
$query = <<<ENDQUERY
with phptalks as (       
    select  id, speaker_name, talk_title, abstract
           ,round((talk_time-to_date('01-jan-1970'))*86400) talk_time
           ,track
           ,checked
    from    talks t
            left join (select talk_id, hacker_id, 'checked disabled' checked from talks_list) l on
                (t.id = l.talk_id and l.hacker_id=:hackerid)
)
select  times.talk_time,
        t1.id id1, t1.speaker_name speaker1, t1.talk_title title1, t1.abstract abstract1, nvl(t1.checked,'') checked1,
        t2.id id2, t2.speaker_name speaker2, t2.talk_title title2, t2.abstract abstract2, nvl(t2.checked,'') checked2, 
        t3.id id3, t3.speaker_name speaker3, t3.talk_title title3, t3.abstract abstract3, nvl(t3.checked,'') checked3
from    (select distinct talk_time from phptalks) times
        left join phptalks t1 on (times.talk_time = t1.talk_time and t1.track = 'Hopper')
        left join phptalks t2 on (times.talk_time = t2.talk_time and t2.track = 'Turing')
        left join phptalks t3 on (times.talk_time = t3.talk_time and t3.track = 'Engressia')
order by times.talk_time asc
ENDQUERY;

$allrows = oracle_query("fetching schedule", $oracleconn, $query, array(":hackerid" => $queryid));

?>
<?php

$day = "none";

// Ok data is there. Let's draw the table now.
foreach ($allrows as $row) {
    if ($day != date("l", $row["TALK_TIME"])) {
        if ($day != "none") {
            echo "</table>";
        }
        $day = date("l", $row["TALK_TIME"]);
        ?><table style="width:600px; background-color:#111111; border: thin dotted #999999;">
        <tr><th colspan="5"><br /><?php echo $day; ?> Schedule</th></tr>
        <tr><th width="60">Time</th><th>Hopper (A)</th><th>Turing (B)</th><th>Engressia (C)</th></tr><?php
    }
    ?><tr><?php
    // is the talk selected?
    ?>
    <td style="border-bottom: thin dotted #999999; width:60px;"><?php echo date("D Hi", $row["TALK_TIME"]); ?></td>
    <td style="border-bottom: thin dotted #999999; width:180px;">
    <?php if ($row["TITLE1"]) { if (isset($_SESSION['handle'])) { ?>
            <input type="checkbox" name="<?php echo $row["ID1"]; ?>" <?php echo $row["CHECKED1"] ?> />
        <?php } ?>
        <a href="javascript:showDetails(<?php echo $row["ID1"]; ?>)">
            <?php echo $row["TITLE1"] ?>
        </a><br>
        <i class="speaker"><?php echo $row["SPEAKER1"] ?></i>
    <?php } else { ?>
        &nbsp;
    <?php } ?>
    </td>       
    
    <td style="border-bottom: thin dotted #999999; width:180px;">
    <?php if ($row["TITLE2"]) {
        if (isset($_SESSION['handle'])) {
             ?>
            <input type="checkbox" name="<?php echo $row["ID2"]; ?>" <?php echo $row["CHECKED2"] ?> />
            <?php 
           }
           ?>
        <a href="javascript:showDetails(<?php echo $row["ID2"]; ?>)">
            <?php echo $row["TITLE2"] ?>
        </a><br>
        <i class="speaker"><?php echo $row["SPEAKER2"] ?></i>
    <?php } else 
          { ?>
        &nbsp;
    <?php 
      } ?>
    </td>

    <td style="border-bottom: thin dotted #999999; width:180px;">
    <?php if ($row["TITLE3"]) {
        if (isset($_SESSION['handle'])) { ?>
            <input type="checkbox" name="<?php echo $row["ID3"]; ?>" <?php echo $row["CHECKED3"] ?> />
        <?php } ?>
        <a href="javascript:showDetails(<?php echo $row["ID3"]; ?>)">
            <?php echo $row["TITLE3"] ?>
        </a><br>
        <i class="speaker"><?php echo $row["SPEAKER3"] ?></i>
            <?php } 
       else { 
             ?>
        &nbsp;
    <?php } 
    ?>
    </td>
    
    </tr>
    <?php 
}
if ($day != "none") {
    echo "</table>";
}

if (isset($_SESSION['handle'])) {
    ?><input type="submit" class="formz" onmouseover="this.style.backgroundColor='#3080f0';" onmouseout="this.style.backgroundColor='';"  value="Update Schedule"></form><?php
}
?>
</center>
</div>
<?php

include("footer.php");
?>
