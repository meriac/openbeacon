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

if (isset($_REQUEST['forgot'])) {
    
    if ($_REQUEST['rfid'] != "" && $_REQUEST['handle'] != '') {
        set_error("Please only fill one field.<br>","forgot_password.php");
    }
    if ($_REQUEST['rfid'] != "") {
        if (preg_match("/^[0-9]{4}$/",$_REQUEST['rfid'])) {
            $rfid = $_REQUEST['rfid'];
        }
        else {
            set_error("Please enter a 4 digit number for RFID.<br>","forgot_password.php");
        }
    }
    if ($_REQUEST['handle'] != "") {
        if (preg_match("/^[A-Za-z0-9 ]{1,30}$/",$_REQUEST['handle'])) {
            $handle = $_REQUEST['handle'];
        }
        else {
            set_error("Please enter a valid handle.<br>","forgot_password.php");
        }
    }
    if (isset($rfid)) {
        $query = "select * from Person where ID=$rfid";
        $result = oracle_query("validating for forgot", $oracleconn, $query);
        if (sizeof($result) == 1) {
            set_error("resetting password","mail.php?medium=00&to=$rfid&body=3");
            die();
        }
    }
    elseif (isset($handle)) {
        $query = "select * from Person where Handle='$handle'";
        $result = oracle_query("validating for forgot", $oracleconn, $query);
        if (sizeof($result) == 1) {
            $id = $result[0]["ID"];
            set_error("resetting password","mail.php?medium=00&to=$id&body=3");
            die();
        }
    }

    if (sizeof($result) == 1) {
        set_error("resetting password","mail.php?medium=00&to=$rfid&body=3");
    }
    else {
        set_error("The handle or badge number you entered could not be found. Please try again.<br>","forgot_password.php");
    }
}


?>

<form action="forgot_password.php" method="post">
<input type="hidden" name="forgot" value="1">
Enter your RFID badge number or Handle. You will be sent a new password.<br>
RFID number: <input type="text" name="rfid"><br>
Handle: <input type="text" name="handle"><br>
<input type="submit">
</form>
<?php

include('footer.php');

?>
