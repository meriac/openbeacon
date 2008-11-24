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

if (!$_SESSION['admin']) {
  set_error("You fucking fail.<br>","index.php");
  die();
}

$rows = oracle_query("fetching pin", $oracleconn,
  "select id, pin, registered from creation where id=:tagid",
  array("tagid"=>$_REQUEST["tagid"]));

?><table><?php
if (sizeof($rows) < 1) {
  ?><tr><td>Invalid PIN</td></tr><?php
} 
else {
  ?>
  <tr><td width="100px">ID</td><td><?php echo $rows[0]["ID"] ?></td></tr>
  <tr><td>PIN</td><td><?php echo $rows[0]["PIN"] ?></td></tr>
  <tr><td>Registered</td><td><?php if ($rows[0]["REGISTERED"]) { echo "YES"; } else { echo "NO"; } ?></td></tr>
  <?php
}
?></table>
<a href="passwordreset.php">Would you like to reset this account?</a><?php

include('footer.php');
?>
