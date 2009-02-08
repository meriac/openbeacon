<?

include_once 'header.php';

if (isset($_SESSION['handle'])) { ?>
            <div class="body-header">
	     Welcome to 25c3, <? echo $_SESSION['handle']; ?>!
            </div>
<? } else { ?>
	    <div class="body-header">
            Welcome to 25c3!
            </div>
<? } ?>
	    <div class="body-text">
<?php


if (isset($_SESSION['id'])) {

$talksrec = <<< ENDQUERY
SELECT    t.id

  ,t.Talk_Title
  ,(t.talk_time-to_date('01-jan-1970'))*86400 talk_time
  ,t.speaker_name
  ,count(*) common_interest_count, collect_concat(cast(collect (i.interest_name) AS varchar2_t)) common_interests
FROM    person p1
  ,interests i1
  ,Talks t
  ,talks_interests ti
  ,interests_list i
WHERE    p1.id = i1.id

  AND i1.interest = ti.interest
  AND ti.talk_id = t.id   AND i.interest_id = ti.interest
  AND p1.id = :hackerid
  --AND rownum <= 3
  --AND t.talk_time >= sysdate
GROUP BY p1.id, p1.handle, t.id, t.Talk_Title, (t.talk_time-to_date('01-jan-1970'))*86400, t.speaker_name

ORDER BY count(*) DESC, dbms_random.random
ENDQUERY;

$talksrecr = oracle_query("fetching talks you'd like", $oracleconn, $talksrec, array(":hackerid" => $_SESSION['id']));
$talksrecrows = sizeof($talksrecr);

if ($talksrecrows>0) {
  ?><p><h3>Recommended Talks (<a href="schedule_recommended.php">full list</a>):</h3><?php
echo "<table style='background-color:#0000; border: thin dotted #999999; width: 600px;'><tr><td>";
  for($i = 0; $i < 3; $i++) {
    $row=$talksrecr[$i];
    ?><div style="border-bottom: thin dotted #999999; padding: 5px; width: 600px;"><?php
    echo $row["TALK_TITLE"] . "<br>(";
    echo "<font class='speaker'>" . $row["SPEAKER_NAME"] . " <i>";
    echo date("l Hs",$row["TALK_TIME"]) . "</i></font>) based on " . $row["COMMON_INTERESTS"];
    ?></div><?php
  }
echo "</td></tr></table>";
}
else {
?><h3>Recommended Talks</h3>
Choose interests in your <a href="login.php">profile</a> and we will recommend talks you might like.<?php
}

}
if (isset($_SESSION['id'])) {
$query = <<<ENDQUERY
select t.id, t.talk_title, t.speaker_name, (t.talk_time-to_date('01-jan-1970'))*86400 talk_time, t.track
from   Talks t,
Talks_List x
where  x.hacker_id = :hackerid
and x.talk_id = t.id
order by t.talk_time asc
ENDQUERY;

$result = oracle_query("get personalized schedule", $oracleconn, $query, array(":hackerid" => $_SESSION['id']));
?>

<h3>Upcoming Selected Talks: (<a href="schedule_personal.php">personal list</a>)</h3>

<?
$rows = sizeof($result);
echo "<table style='background-color:#0000; border: thin dotted #999999; width: 600px;'><tr><td>";
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
echo "</td></tr></table>";
 }
 
else {
 ?>You should <a href="schedule.php">sign up</a> for some talks!
</td></tr></table>
<?
}
}
else {
?>

<h3>New features are being implemented live at 25c3!</h3>

<p><font color="red" size="+1"><a href="usersrooms.php">New visual</a>: see who is in each room!</font></p>

Features of the OpenAMD system:<br>
<ul>
<li>Create a custom schedule
<li>Learn about talks you might otherwise miss
<li>Meet new people with similar interests
<li>Track yourself on the OpenAMD web visuals
</ul>

<div style="width: 600px;">
<img src="booth.jpg"><br><br>

Get your OpenAMD Sputnik badge for only 10 euros at the FoeBuD booth (<a href="http://events.ccc.de/congress/2008/wiki/Floorplans">floor B</a>, right across from the Microcontroller workshop).
</div>
<?

}

include_once 'footer.php';

?>
