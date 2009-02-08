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


// Create new user


include('header.php');
//$_SESSION['error'] = "";

if (isset($_REQUEST['reg']) && $_REQUEST['reg'] == '1') {
    /* ok, time to perform some checks
        -make sure both are 4 digit numbers
        -make sure the rfid is in the system
        -make sure the rfid has not been registered already
        -make sure the pin matches the rfid in the system
        -if this is all good, show the account creation page
        -upon hitting submit, make sure to set the registered bit on, and timestamp it */
    if (isset($_REQUEST['rfid']) && $_REQUEST['rfid'] != "") {
        if (preg_match("/^[0-9]{4}$/",$_REQUEST['rfid'])) {
            $rfid = $_REQUEST['rfid'];
        }
        else {
            set_error("Please make sure the RFID is 4 numbers.<br>","create.php");
        }
    }
    else {
        set_error("Please fill in the RFID field.<br>","create.php");
    }
    if (isset($_REQUEST['pin']) && $_REQUEST['pin'] != "") {
        if (preg_match("/^[0-9]{1,4}$/",$_REQUEST['pin'])) {
            $pin = $_REQUEST['pin'];
        }
        else {
            set_error("Please verify your PIN is correct.<br>","create.php");
        }
    }
    else {
        set_error("Please enter your PIN.<br>","create.php");
    }
 
    $rfidquery = "select ID, PIN, Registered from Creation where ID='$rfid'";
    $rfidresult = oracle_query("check if user is registered...", $oracleconn, $rfidquery);
    if (sizeof($rfidresult) > 1) {
        set_error("System error. Please contact the OpenAMD administrator.<br>","create.php");
    }
    if (sizeof($rfidresult) < 1) {
        set_error("RFID not found. Please doublecheck your tag.<br>","create.php");
    }
    if (sizeof($rfidresult) == 1) {
        $row=$rfidresult[0];
        if ($row['PIN'] != $pin) {
            set_error("PIN does not match the system. Please try again.<br>","create.php");
        }
        if ($row['REGISTERED']) {
            set_error("This RFID has already been registered.<br>","create.php");
        }
        // Looks like they passed the gauntlet. Let's register them now!
        $_SESSION['id'] = $rfid;
        set_error("","create2.php");
    }
}
else {

?>
<div class="body-header">Create a 25c3 Account</div><p>
<div class="body-text">
Fill out the form to create your OpenAMD account:<p>
<form action="create.php?reg=1" method="post">
<center><table>
<tr><td>RFID number:</td><td><input type="text" name="rfid" size=4 class="formz"></td></tr>
<tr><td>PIN:</td><td><input type="text" name="pin" size=4 class="formz"></td></tr>
</table>
<input type="submit" class="formz" value="Create"></center>
</form>
</div>
<?php
}
include('footer.php');

?>
