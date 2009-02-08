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

$_SESSION['error'] = "";

if (isset($_REQUEST['user'])) {
    if (preg_match("/^[a-zA-Z0-9 ]{1,30}$/",$_REQUEST['user'])) {
        $user = $_REQUEST['user']; // call it user, we'll use handle after auth
    }
    else {
        $_SESSION['error'] .= "Please enter a valid username.\n";
    }
}

if (isset($_REQUEST['password'])) {
    if (preg_match("/^[a-zA-Z0-9]{1,30}$/",$_REQUEST['password'])) {

        // THIS NEEDS TO BE ENCRYPTED
        $password = hash('sha256',$_REQUEST['password']);
    }
    else {
        $_SESSION['error'] .= "Please enter a valid password.\n";
    }
}

if (isset($_SESSION['error']) && $_SESSION['error'] != "") {
    set_error("Does not work.","index.php");
}

$userquery = "SELECT Handle from Person where Handle='$user'";
$userrows = oracle_query("fetching person", $oracleconn, $userquery);

$passquery = "SELECT Handle, ID, admin, Loggedin from Person where Handle='$user' and password='$password'";
$passrows = oracle_query("checking password", $oracleconn, $passquery);


if (count($userrows) > 1) {
    set_error("System error has occured. Please contact the OpenAMD admin.","index.php");
}
elseif (count($userrows) < 1) {
    set_error("User does not exist.","index.php");
}
elseif (count($passrows) > 1) {
    set_error("System error has occured. Please contact the OpenAMD admin.","index.php");
}
elseif (count($passrows) < 1) {
    set_error("Incorrect password.<br>","index.php");
}
else {

    // If it's the first sign in, activate the account
    $isactivatedr = "select registered from Creation where id=" . $passrows[0]["ID"];
    $isactivatedq = oracle_query("pull registered yesno", $oracleconn, $isactivatedr);
    if (!$isactivatedq[0]["REGISTERED"]) {
         $activater = "update Creation set registered = 1, timestamp=sysdate where id=" . $passrows[0]["ID"];
         $activateq = oracle_query("activate account", $oracleconn, $activater);
    }

    $_SESSION['handle'] = $passrows[0]["HANDLE"];
    $_SESSION['id'] = $passrows[0]["ID"];
    $_SESSION['admin'] = $passrows[0]["ADMIN"];
    if ($_SESSION['id'] <= 5000) {
        $_SESSION['rfid'] = true;
    }
    if ($passrows[0]["LOGGEDIN"]) {
        set_error("You have successfully logged in.","index.php");
    }
    else {
        set_error("You have successfully logged in.","index.php");
//    print "<script src='jqmod.js'></script>";
    print "<noscript>You have been logged in, but javascript is disabled.<br>If you enable it, you will have access to some of the more advanced features of the OpenAMD project.<br><a href='index.php'>Home</a></noscript>";
    }
}

include ('footer.php');

?>
