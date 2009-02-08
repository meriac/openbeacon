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

if(isset($_REQUEST['change']) && $_REQUEST['change'] == '1') {
    if(isset($_REQUEST['current']) && isset($_REQUEST['new1']) && isset($_REQUEST['new2'])) {
        $current = hash('sha256',$_REQUEST['current']);
        $new1 = hash('sha256',$_REQUEST['new1']);
        $new2 = hash('sha256',$_REQUEST['new2']);
    }
    else {
        set_error("Please enter all password fields.<br>","change_password.php");
    }
    if($new1 != $new2) {
        set_error("Your new passwords do not match. Learn to spell.<br>","change_password.php");
    }
    $currentq = "select Password from Person where ID='" . $_SESSION['id'] . "'";
    $currentr = oracle_query("get current password",$oracleconn, $currentq);
    if (sizeof($currentr) != 1) {
        set_error("You have issues, dude. Fix your fucking password.<br>","index.php");
    }
    if($current != $currentr[0]["PASSWORD"]) {
        die();
        set_error("The current password you entered does not match the password in the database.<br>","change_password.php");
    }
    $updateq = "update Person set Password='$new1' where ID='" . $_SESSION['id'] . "'";
    oracle_query("updating password...", $oracleconn, $updateq);
    set_error("Your password has been changed!<br>","index.php");
}
else {
  ?>
  <form action="change_password.php" method="post">
  <input type="hidden" name="change" value="1">
  <h3>Change your password</h3>
  Current password: <input type="password" name="current" class="formz"><br>
  New password: <input type="password" name="new1" class="formz"><br>
  New pass (again): <input type="password" name="new2" class="formz"><br>
  <input type="submit" value="Change Password">
  </form>
  <?php
}

include('footer.php');

?>
