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
// Remove from schedule
if (isset($_SESSION['handle'])) {
    if (isset($_REQUEST['talk']) && preg_match("/^[0-9]{1,4}$/",$_REQUEST['talk'])) {

        $isthere = "select * from Talks_List where Hacker_ID='" . $_SESSION['id'] .
            "' and Talk_ID='" . $_REQUEST['talk'] . "'";
        $userrows = oracle_query("getting subscribed talks", $oracleconn, $isthere);
        if (count($userrows) == 1) {
            $query = "delete from Talks_List where Hacker_ID='"
                . $_SESSION['id'] . "' and Talk_ID='" . $_REQUEST['talk'] . "'";
            oracle_query("deleting subscribed talk", $oracleconn, $query);

            set_error("You have been removed from the talk.","index.php");
        }
        else {
            set_error("You are not signed up for that talk.","index.php");
        }
    }
    else {
        set_error("You fucking fail.<br>","index.php");
    }
}
else {
    set_error("You fucking fail.<br>","index.php");
}

?>
