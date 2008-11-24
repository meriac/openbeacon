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

$file = file("talks");

oracle_query("deleting talk interests", $oracleconn, "delete from talks_interests", NULL, 0);
$i = 0;
foreach($file as $line_num => $line) {
    list($title, $interests) = split("[|]",$line);
    $interests = split("[,]",$interests);
    $title = addslashes($title);
    $titleq = "select ID from Talks where Talk_Title='$title'";
    $titler = oracle_query("fetching talk", $oracleconn, $titleq, NULL, 0);
    //echo "Title row count: " . count($titler) . "<br>";
    //var_dump($titler); echo "<br>";
    $i++;
    for($i = 0; $i < sizeof($interests); $i++) {
        $interestq = "select Interest_ID from Interests_List where Interest_Name='" .
            trim($interests[$i]) . "'";
        echo $interestq . "<br>";
        $interestr = oracle_query("fetching interest", $oracleconn, $interestq, NULL, 0);
        //echo "Interest row count: " . count($interestr) . "<br>";
        //var_dump($interestr); echo "<br>";
        if (count($interestr) == 0) {
          echo "Missing interest: " . trim($interests[$i]) . "<br>";
        } 
        else {
          if(count($titler) != 1) {
            $insertq = "insert into Talks_Interests (Talk_ID, Interest) values ('" .
              $title . "', '" . $interestr[0]["INTEREST_ID"] . "')";
          }
          else {
            $insertq = "insert into Talks_Interests (Talk_ID, Interest) values ('" .
              $titler[0]["ID"] . "', '" . $interestr[0]["INTEREST_ID"] ."')";
          }
        echo $insertq . "<br>";
        oracle_query("inserting talk interest row", $oracleconn, $insertq);
        }
    }
}

oci_commit($oracleconn);
echo "talks updated<br>";
