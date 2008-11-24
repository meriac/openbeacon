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

if (!isset($_SESSION['handle'])) {
    set_error("GTFO noob.<br>","index.php");
}

$query = "SELECT ID from Talks";
$allrows = oracle_query("fetching talks", $oracleconn, $query);
$rows = sizeof($allrows);

$existing = "SELECT Talk_ID from Talks_List where Hacker_ID=" . $_SESSION['id'];
$existingr = oracle_query("grabbing existing subscription", $oracleconn, $existing);
$existingcount = sizeof($existingr);

$mytalks = "";
for ($i = 0; $i < $rows; $i++) {
    $temp = $allrows[$i];
    if (isset($_REQUEST[$temp["ID"]]) && preg_match("/^[on]{2}$/",$_REQUEST[$temp["ID"]])) {
    // check to make sure we don't already have this talk subscribed
        for ($j = 0; $j < $existingcount; $j++) {
            if ($temp["ID"] == $existingr[$j]["TALK_ID"]) {
                set_error("You are already subscribed to this talk.","schedule.php");
            }
        }
        $mytalks = "insert into Talks_List (Hacker_ID, Talk_ID) values (" .
        $_SESSION['id'] . ", " . $temp["ID"] . ")";
        oracle_query("inserting talk subscription", $oracleconn, $mytalks);
    }
}
set_error("You have been subscribed. Enjoy!<br>","index.php");

include('footer.php');
?>
