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


//Create user, 2nd part

include('header.php');

if (isset($_REQUEST['reg']) && $_REQUEST['reg'] == '1') {
    /* validate stuff
        -is the handle valid?
        -is it already being used?
        -is the password valid?
        -is the email valid? */

    if (preg_match("/^[a-zA-Z0-9 ]{1,30}$/",$_REQUEST['handle'])) {
        $handlequery = "SELECT Handle from Person where Handle='" . $_REQUEST['handle'] . "'";
        $handleresult = oracle_query("select handle from person", $oracleconn, $handlequery);    
        if (sizeof($handleresult) > 1) {
            set_error("System error. Please contact the OpenAMD administrator.<br>","index.php");
        }
        if (sizeof($handleresult) == 1) {
            set_error("The handle you selected is already in use. Please choose another.<br>","create2.php");
        }
        else {
            $handle = $_REQUEST['handle'];
        }
    }
    else {
        set_error("Please choose a handle.<br>","create2.php");
    }
    if ($_REQUEST['password'] == $_REQUEST['password2']) {
        $password = hash('sha256',$_REQUEST['password']); // hash this bitch yo
    }
    else {
        set_error("The passwords you entered do not match. Please try again.","create2.php");
    }
    
    // validate email
    if (preg_match("/^(([A-Za-z0-9]+_+)|([A-Za-z0-9]+\-+)|([A-Za-z0-9]+\.+)|([A-Za-z0-9]+\++))*[A-Za-z0-9]+@((\w+\-+)|(\w+\.))*\w{1,63}\.[a-zA-Z]{2,6}$/", $_REQUEST['email'])) {
        $email = $_REQUEST['email'];
    }
    else {
        set_error("Please enter a proper email address.<br>","create2.php");
    }
    $query = "insert into Person (ID, Handle, Password, Email) values ('" .
            $_SESSION['id'] . "', '$handle', '$password', '$email')";
    $result = oracle_query("creating account...", $oracleconn, $query);
    $regquery = "update Creation set Registered = '1', Timestamp = systimestamp where ID = '" . $_SESSION['id'] . "'";
    $regresult = oracle_query("updating Creation.", $oracleconn, $regquery);
    $_SESSION['handle'] = $handle;
    if ($_SESSION['id'] <= 5000) {
        $_SESSION['rfid'] = true;
    }
    set_error("Your account has been created successfully.<br>","index.php");
}

else {

?>

Please choose a handle and password. Enter an email address in case you lose your password.<br>
<h3>The system will send a verification email, do don't enter fake info.</h3>
<form action="create2.php?reg=1" method="post">
<table>
<tr><td>Handle:</td><td><input type="text" name="handle"></td></tr>
<tr><td>Password:</td><td><input type="password" name="password"></td></tr>
<tr><td>Password (again):</td><td><input type="password" name="password2"></td></tr>
<tr><td>Email:</td><td><input type="text" name="email"></td></tr></table>
<input type="submit">
</form>

<?php

}

include('footer.php');

?>
