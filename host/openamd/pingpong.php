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

if (!isset($_SESSION['id'])) {
    set_error("You must log in to view the ping roster.","index.php");
}

$querysent = "select Ping.*, Person.Handle from Ping, Person where Ping.id=" . $_SESSION['id'] . " and Ping.to_id=Person.ID order by Ping.pingtimestamp asc";
$resultsent = oracle_query("get ping info", $oracleconn, $querysent);

$queryreceived = "select Ping.*, Person.Handle from Ping, Person where Ping.to_id=" . $_SESSION['id'] . " and Ping.id=Person.ID order by Ping.pingtimestamp asc";
$resultreceived = oracle_query("get ping info", $oracleconn, $queryreceived);

?>
<table><th><?php echo sizeof($resultsent) . " Pings Sent"; ?></th>
<th><?php echo sizeof($resultreceived) . " Pings Received"; ?></th>
<tr><td>
    <table><th>To</th><th>Type</th><th>Time</th>
    <tr>
    <?php
    if (sizeof($resultsent) > 0) {
        foreach($resultsent as $ping) {
            echo "<tr><td>" . $ping["HANDLE"] . "</td><td>" .
            $ping["PINGTYPE"] . "</td><td>" . $ping["PINGTIMESTAMP"] . "</td></tr>";
        }
    }
    else {
        echo "<td>No pings sent.</td>";
    }
?>
    </table>
</td><td valign="top">
    <table><th>From</th><th>Type</th><th>Time</th>
    <tr>
    <?php
    if (sizeof($resultreceived) > 0) {
        foreach($resultreceived as $ping) {
            echo "<tr><td>" . $ping["HANDLE"] . "</td><td>" .
            $ping["PINGTYPE"] . "</td><td>" . $ping["PINGTIMESTAMP"] . "</td></tr>";
        }
    }
    else {
        echo "<td>No pings received.</td>";
    }
?>
    </table>
</td></tr></table>
<?php

include('footer.php');

?>
