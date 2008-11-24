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

$hope = mysql_connect("127.0.0.1", "yeah","right");
mysql_select_db("lolz");

$deathtalks = "delete from talks";
oracle_query("deleting talks", $oracleconn, $deathtalks, NULL, 0);

$talkquery = "write your own query";
$talkresult = mysql_query($talkquery, $hope);
$talkrows = mysql_num_rows($talkresult);

//$insertquery = "insert into talks (id, talk_title, speaker_name, abstract, talk_time, track) " .
//  "values (amdseq.nextval, :title, :speaker, :abstract, to_date('17-07-2008 1000', 'dd-mm-yyyy hh24mi'), :track)";

for($i = 0; $i < $talkrows; $i++) {
  switch(mysql_result($talkresult, $i, "c5")) {
    case 'Friday': $date="18"; break;
    case 'Saturday': $date="19"; break;
    case 'Sunday': $date="20"; break;
  }
  $date .= "-07-2008 " . mysql_result($talkresult, $i, "c6");


  //  $params = array(":title"=>mysql_result($talkresult, $i, "c2"),
  //          ":speaker"=>mysql_result($talkresult, $i, "c3"),
  //          ":abstract"=>mysql_result($talkresult, $i, "c4"),
  //          ":track"=>mysql_result($talkresult, $i, "c4"),
  //           );
  //  var_dump($params);

  $insertquery = "insert into talks (id, talk_title, speaker_name, abstract, talk_time, track) " .
    "values (" . $i . ", " .
    "'" . addslashes(mysql_result($talkresult, $i, "c2")) . "', " .
    "'" . addslashes(mysql_result($talkresult, $i, "c3")) . "', " .
    "'" . addslashes(mysql_result($talkresult, $i, "c4")) . "', " .
    "to_date('" . $date . "', 'dd-mm-yyyy hh24mi'), " .
    "'" . addslashes(mysql_result($talkresult, $i, "c7")) . "')";

  oracle_query("inserting talk row", $oracleconn, $insertquery, NULL, 0);
}


echo "<br><br>The talk schedule has been updated.<br>";

$deathspeakers = "delete from speakers";
oracle_query("deleting speakers", $oracleconn, $deathspeakers, NULL, 0);

$speakerquery = "select c2, c3 from datatable_data where c4 is null";
$speakerresult = mysql_query($speakerquery, $hope);
$speakerrows = mysql_num_rows($speakerresult);

for ($i = 0; $i < $speakerrows; $i++) {
  $speakername = mysql_result($speakerresult, $i, "c2");
  if ($speakername) {
    $speakerdata = "insert into Speakers (Name, Bio) values (";
    $speakerdata .= "'" . addslashes($speakername) .
            "', '" . addslashes(mysql_result($speakerresult, $i, "c3")) . "')";
    echo $speakerdata . "<br>";
    oracle_query("inserting speaker row", $oracleconn, $speakerdata, NULL, 0);
  }
}

echo "<br><br>The speaker list has been updated.<br>";

oci_commit($oracleconn);

echo "<br><br>Changes have been committed.<br>";

set_error("Changes have been commmitted.<br>","index.php");

?>
