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


// Update details
include('header.php');

if (!isset($_SESSION['handle'])) {
    set_error("You need to log in, you little fuck.<br","login.php");
}

$test = "select edited from Person where ID='" . $_SESSION['id'] . "'";
$userrows = oracle_query("fetching person row", $oracleconn, $test);

if (count($userrows) != "1") {
    set_error("System error. Please contact the OpenAMD administrator.","login.php");
}

// Input validation how do I hate thee, let me count the ways
$error = "";

// one, the fatal phone number
//    its not like she's gonna call you back anyways
if (isset($_REQUEST['phone']) && !empty($_REQUEST['phone'])) {
    if (preg_match("/^[0-9A-F]{10,20}$/",$_REQUEST['phone'])) {
        $_SESSION['phone'] = $_REQUEST['phone'];
    }
    else {
        $error .= "Please enter a valid phone number, like 6461231234. Numbers only.<br>";
    }
}

// two, the peeping provider
//    at&t sells all your records to the nsa anyways...
if (isset($_REQUEST['provider']) && $_REQUEST['provider'] != "0") {
    if (preg_match("/^[0-9]{1}$/",$_REQUEST['provider'])) {
        $_SESSION['provider'] = $_REQUEST['provider'];
    }
    else {
        $error .= "Please enter a valid provider.<br>";
    }
}

// three, the curse of gender
//    at least, she *claims* they're real...
if (isset($_REQUEST['gender'])) {
    if (preg_match("/^[1-2]{1}$/",$_REQUEST['gender'])) {
        $_SESSION['gender'] = $_REQUEST['gender'];
    }
    else {
        $error .= "What are you, a tranny whore?<br>";
    }
}

// four, the throngs of time
//    blanche dubois was once young too...
if (isset($_REQUEST['age']) && !empty($_REQUEST['age'])) {
    if (preg_match("/^[0-9]{1,3}$/",$_REQUEST['age'])) {
        $_SESSION['age'] = $_REQUEST['age'];
    }
    else {
        $error .= "If there's grass on the field, play ball! And make sure your age is a number :)<br>";
    }
}

// five, because big brother is watching
//    and jerking off into an american flag
//    and yes, george dorn is screaming
if (isset($_REQUEST['location']) && !empty($_REQUEST['location'])) {
    if (preg_match("/^[0-9a-zA-Z]{1,10}$/",$_REQUEST['location'])) {
        $_SESSION['location'] = $_REQUEST['location'];
    }
    else {
        $error .= "No location? You must live in your mom's basement.<br>";
    }
}

// six, six, six, the sign of the beast
//    damn you drama, for making me update all this code
//     extra features break the poem :/

$_SESSION['reminder'] =0;
if (isset($_REQUEST['reminder_email']) && preg_match("/^[on]{2}$/",$_REQUEST['reminder_email'])) {
    $_SESSION['reminder']+=1;
}
if (isset($_REQUEST['reminder_phone']) && preg_match("/^[on]{2}$/",$_REQUEST['reminder_phone'])) {
    if (isset($_SESSION['phone'])) {
        $_SESSION['reminder']+=2;
    }
    else {
        $error .= "You need to enter a cell phone number to get an SMS reminder.<br>";
    }
}

// seven, the day of rest
//    we all know god was getting a happy ending massage

if (isset($_REQUEST['country']) && $_REQUEST['country'] != "0" ) {
    if (preg_match("/^[0-9]{1,3}$/",$_REQUEST['country'])) {
        $_SESSION['country'] = $_REQUEST['country'];
    }
    else {
        $error .= "Please enter a valid country.<br>";
    }
}
else {
    $_SESSION['country'] = "0";
}


if ($error) {
    set_error($error,"login.php");
}
/*
if (isset($_REQUEST['email']) && !empty($_REQUEST['email'])) {
    if (preg_match("/^(([A-Za-z0-9]+_+)|([A-Za-z0-9]+\-+)|([A-Za-z0-9]+\.+)|([A-Za-z0-9]+\++))*[A-Za-z0-9]+@((\w+\-+)|(\w+\.))*\w{1,63}\.[a-zA-Z]{2,6}$/", $_REQUEST['email'])) {
        $_SESSION['email'] = $_REQUEST['email'];
    }
    else {
        set_error("Uh... your email address is fucked up, dude.","login.php");
    }
}*/

$interestcount = 0;

$intquery = "select * from Interests_List";
$introws = oracle_query("fetching interests", $oracleconn, $intquery);

$intnum = count($introws);
for($i = 0; $i < $intnum; $i++) {
  // yes I know I don't need regex here, this is seeding for future ideas
  if (isset($_REQUEST[$i]) && preg_match("/^[on]{2}$/",$_REQUEST[$i])) {
    $_SESSION[$i] = $_REQUEST[$i];
    $interestcount++;
  }
}

if ($interestcount > 5) {
    set_error("You can only choose up to five interests.<br>","login.php");
}

$query = "update Person set Phone = '" .
    base64_encode(mcrypt_cbc(MCRYPT_BLOWFISH, $_SESSION['id'], $_SESSION['phone'], MCRYPT_ENCRYPT, '12345678')) .
    "', Provider = '" . $_SESSION['provider'] . "', Sex = '" . $_SESSION['gender'] . "', Age = '" .
    $_SESSION['age'] . "', Location = '" . $_SESSION['location'] . "', ";
    if (isset($_SESSION['email'])) {
        $query .= "Email = '" . $_SESSION['email'] . "', ";
    }
    $query .= "Reminder = " . $_SESSION['reminder'] . ", Country=" . $_SESSION['country'] . ",";
    $query .= " Edited = '1' where ID='" . $_SESSION['id'] . "'";
oracle_query("updating person", $oracleconn, $query, NULL, 0);

// Depreciated
//$query = "delete from interests where ID=" . $_SESSION['id'];
//oracle_query("deleting interests", $oracleconn, $query, NULL, 0);

for ($i = 0; $i < $intnum; $i++) {
        if (isset($_SESSION[$i])) {
        $interestadd = "insert into Interests (ID, Interest) values ('" . $_SESSION['id'] . "', '" . $i . "')";
        oracle_query("inserting interest", $oracleconn, $interestadd, NULL, 0);
    }
}

oci_commit($oracleconn);

// Release the hounds!
unset($_SESSION['phone']);
unset($_SESSION['provider']);
unset($_SESSION['gender']);
unset($_SESSION['age']);
unset($_SESSION['reminder']);
unset($_SESSION['location']);
unset($_SESSION['country']);

if (!$edited) {
    for ($i = 0; $i < $intnum; $i++) {
        if ($_SESSION[$i]) {
            unset($_SESSION[$i]);
        }
    }
}

set_error("Your profile has been updated.<br>","login.php");

include('footer.php');

?>
