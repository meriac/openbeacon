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
include 'showuser.php';
?>

<div class="body-header">25c3 OpenAMD PHPat debug page:</div>
<div class="body-text">
<font color="red">This is an internal testpage.</font><br>
</div>

<?php
// get position of user in the current session
$locq = "select * from position_snapshot where TAG_ID = '". $_SESSION["id"] . "' and rownum = 1 order by snapshot_timestamp desc";
$locr = oracle_query("show location of user", $oracleconn, $locq);
if ($locr[0]) {
    echo "You are here: " . $locr[0]["AREA_ID"] ."<br>";
}


show_user($_SESSION["id"],"test");

// the footer
include("footer-phpat.php"); ?>
