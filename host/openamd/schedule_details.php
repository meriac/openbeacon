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


if (isset($_REQUEST['talk']) && preg_match("/^[0-9]{1,4}$/",$_REQUEST['talk'])) {
  $talkid = $_REQUEST['talk'];
}
else {
  $_SESSION['error'] = "Listen, you cockmunch. Keep trying to hack this shit and we're going CCNa on your ass.";
  header("Location: index.php");
  die();
}

include('config.php');

$query = "select * from Talks where ID='$talkid'";
$result = oracle_query("fetching talk info",$oracleconn,$query);

if (sizeof($result) > 0) {
    $result = $result[0];
    ?><b><?php echo $result["TALK_TITLE"]; ?></b><br><?php

    echo $result["ABSTRACT"] . "<br>";
    echo "<a href='http://events.ccc.de/congress/2008/Fahrplan/events/" . 
	$result["ID"] . "." . $result["LANGUAGE"] . ".html'>More talk details</a><br><br>";

    //$speaker_names = split(", ", $result["SPEAKER_NAME"]);
      $speakerq = "select Speakers.Name from Speakers, Speakers_Talks where Speakers.ID = Speakers_Talks.Speaker_ID and Speakers_Talks.Talk_ID =$talkid";
      $speakerr = oracle_query("speakers", $oracleconn, $speakerq);
      for ($i = 0; $i < sizeof($speakerr); $i++) {
	echo "<b>" . $speakerr[$i]["NAME"] . "</b><br>";
      }    
/*      for($i = 0; $i < sizeof($speaker_names); $i++) {
        $bioquery = "select Bio from Speakers where Name='" . $speaker_names[$i] . "'";
        $bioresult = oracle_query("get bio data", $oracleconn, $bioquery);
        $bioresult = $bioresult[0];
        echo "<b>" . $speaker_names[$i] . "</b> <i>" . $bioresult["BIO"] . "</i><p>";
    }*/

?><?php

}
?>
