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



/* Mail.php
    Four arguments:
    Medium:      left|right: from|to
            0: email
            1: sms
            2: both
    To: user ID
    (From is determined by $_SESSION['id'])
    Body: Welcome message? Ping greeting? Talk reminder? Forgot password (0, 1, 2, 3)
    Talk: talk ID number (only use if Body == 2)

    Checks:
    Regex on all the 
    For "ping feature:
        has email already been sent?
*/

include('header.php');

if (!(isset($_REQUEST['medium']) && isset($_REQUEST['body']))) {
  // if the glove don't fit you must acquit
  set_error("Come on, dude. RTFM. Sending mail isn't that hard.<br>","index.php");
}

if (preg_match("/^[0-2]{1,2}$/",$_REQUEST['medium'])) {
  $medium = $_REQUEST['medium'];
}
else {
  set_error("Please enter a valid email medium.<br>","index.php");
}

if (isset($_REQUEST['to'])) {
  if (preg_match("/^[0-9]{4}$/",$_REQUEST['to'])) {
    $to = $_REQUEST['to'];
  }
  else {
    set_error("Please enter a valid ID.<br>","index.php");
  }
}

if (preg_match("/^[0-9]{1}$/",$_REQUEST['body'])) {
  $body = $_REQUEST['body'];
}
else {
  set_error("Please enter a valid body.<br>","index.php");
}

if (isset($_REQUEST['talk']) && preg_match("/^[0-9]{1,3}$/",$_REQUEST['talk'])) {
  if (isset($body) && $body==2) {
    $talk = $_REQUEST['talk'];
  }
  else {
    set_error("There is no reason for you to choose a talk. Stop trying to hack this, noob.<br>","index.php");
  }
}


/* Message structure
    Welcome message- email only
    Ping greeting - email only, sms maybe
    Talk reminder - both email and sms, you can choose one or both
    Forgot password - both email and sms, you can choose one or both
*/

switch($body) {
  // welcome message
  case '0':    
    $from = "From: contribute@openamd.org\r\nReply-to: contribute@openamd.org\r\nX-Mailer: PHP/";
    $subject = "Welcome to OpenAMD!";
    welcome($from,$to);
    break;
  // ping greeting
  case '1':    
    $subject = "Ping!";
    ping();
    break;
  // reminder
  case '2':    
    $subject = "Upcoming Talk";
    $from = "From: contribute@openamd.org\r\nReply-to: contribute@openamd.org\r\nX-Mailer: PHP/";
    reminder();
    break;
  // forgot password
  case '3':
    $subject = "Forgot your password?";
    $from = "From: contribute@openamd.org\r\nReply-to: contribute@openamd.org\r\nX-Mailer: PHP/";
    forgot();
    break;
}

function welcome() {
  global $oracleconn, $from, $subject, $to;
  $query = "select Email from Person where ID='$to'";
  $result = oracle_query("select email to", $oracleconn, $query);
  $body = "testtest"; // enter welcome message here
  if (sizeof($result) != 1) {
    set_error("Error sending welcome messages.<br>","index.php");
  }
  else {
    if (mail($result[0]["EMAIL"], $subject, $body, $from)) {
      set_error("Welcome to OpenAMD!<br>","index.php");
    }
    else {
      set_error("Cannot send welcome message.<br>","index.php");
    }
  }
}

function ping() {
    global $oracleconn, $medium, $to, $subject;
    // Determine medium

    $from = $_SESSION['id'];
    
    switch ($medium) {
        case 00: $pingtype= "email"; break;
        case 11: $pingtype= "sms"; break;
        default: set_error("you fail.","index.php"); break;
    }

    $pingedout = "select id, to_id from Ping where id=$from and to_id=$to and pingtype='$pingtype'";
    $pingedoutr = oracle_query("have they been pinged?", $oracleconn, $pingedout);
    
    if (sizeof($pingedoutr) > 0) {
        set_error("Sorry, you have already pinged this user.","index.php");
    }

    $queryfromemail = "select Handle, email from Person where ID=$from";
    $resultfromemail = oracle_query("get all from data email", $oracleconn, $queryfromemail);
    
    $queryfromphone  = "select Person.Handle, Person.email, Person.phone, Person.provider,
        Providers.Address from Person, Providers where Person.ID=$from and Providers.ID=Person.provider";
    $resultfromphone = oracle_query("get all from data", $oracleconn, $queryfromphone);
    
    if (sizeof($resultfromemail) != 1) {
        set_error("error getting from data", "index.php");
    }
    else {
        // Process "From" bit
        switch(substr($medium,0,1)) {
            case '0':    $emailfrom = $resultfromemail[0]["EMAIL"];
                    break;
            case '1':     if ($resultfromphone[0]["PHONE"]) {
                        $phonefrom = ereg_replace("[^A-Za-z0-9]","",mcrypt_cbc(MCRYPT_BLOWFISH, $from, base64_decode($resultfromphone[0]["PHONE"]), MCRYPT_DECRYPT, '12345678')); // . "@" . $resultfromphone[0]["ADDRESS"];
                    }
                    else {
                        set_error("Please update your profile with a valid phone number and try again.<br>","index.php");
                    }
                    break;
            case '2':    $emailfrom = $resultfromemail[0]["EMAIL"];
                    if ($resultfromphone[0]["PHONE"]) {
                        $phonefrom = ereg_replace("[^A-Za-z0-9]","",mcrypt_cbc(MCRYPT_BLOWFISH, $from, base64_decode($resultfromphone[0]["PHONE"]), MCRYPT_DECRYPT, '12345678')); // . "@" . $resultfromphone[0]["ADDRESS"];
                    }
                    else {
                        set_error("Please update your profile with a valid phone number and try again.<br>","index.php");
                    }
                    break;
            default:    set_error("Error processing from data","index.php");
                    break;
        }
    }

    $querytoemail = "select Handle, email from Person where ID=$to";
    $resulttoemail = oracle_query("get all to data email", $oracleconn, $querytoemail);

    $querytophone = "select Person.Handle, Person.email, Person.phone, Person.provider, Providers.Address from Person, Providers where Person.ID=$to and Providers.ID=Person.provider";
    $resulttophone = oracle_query("select data for to", $oracleconn, $querytophone);
    if (sizeof($resulttoemail) != 1) {
        set_error("Error fetching to data for ping.","index.php");
    }
    else {
        switch(substr($medium,1,1)) {
            case '0':    $emailto = $resulttoemail[0]["EMAIL"];
                    break;
            case '1':    if ($resulttophone[0]["PHONE"]) {
                        $phoneto = ereg_replace("[^A-Z0-9]","",mcrypt_cbc(MCRYPT_BLOWFISH, $to, base64_decode($resulttophone[0]["PHONE"]), MCRYPT_DECRYPT, '12345678')) . "@" . $resulttophone[0]["ADDRESS"];
                    }
                    else {
                        set_error("The user you are trying to ping has not entered a phone number.<br>","index.php");
                    }
                    break;
            case '2':    $emailto = $resulttoemail[0]["EMAIL"];
                    break;
                    $phoneto = ereg_replace("[^A-Za-z0-9]","",mcrypt_cbc(MCRYPT_BLOWFISH, $to, base64_decode($resulttophone[0]["PHONE"]), MCRYPT_DECRYPT, '12345678')) . "@" . $resulttophone[0]["ADDRESS"];
                    break;
            default:    set_error("Error processing to data","index.php");
                    break;
        }
    }
    /* Compile the message body
        This will involve checking to see whether email, phone, or both is defined.
        Create two bodies, $emailbody and $phonebody, for email and text messaging.*/

    if (isset($emailto)) {
            // email message body to victom
        $body = "You've been pinged! " . $resultfromemail[0]["HANDLE"] . " wants to meet you. You can reach him/her at " .
            $resultfromemail[0]["EMAIL"] .".";
        if (mail($emailto, $subject, $body, $resultfromemail[0]["HANDLE"])) {
            $update = "insert into Ping (id, to_id, pingtype, pingtimestamp)
                values (" . $_SESSION['id'] . ", $to, 'email', sysdate)";
            oracle_query("inserting email ping", $oracleconn, $update);
            set_error("Your ping has been sent!!<br>","index.php");
        }
        else {
            set_error("Cannot send ping message. Is your email address valid?<br>","index.php");
        }
    }
    if (isset($phoneto)) {
            // sms message body to victim
        if (!isset($phonefrom)) {
            set_error("Sorry, you must enter your own phone number to use this feature.","index.php");
        }
        $body = "You've been pinged! " . $resultfromphone[0]["HANDLE"] .
                " wants to meet you. You can reach him/her at $phonefrom.";
        if (mail($phoneto, $subject, $body, $resultfromphone[0]["HANDLE"])) {
            $update = "insert into Ping (id, to_id, pingtype, pingtimestamp)
                values (" . $_SESSION['id'] . ", $to, 'sms', sysdate)";
            oracle_query("inserting sms ping", $oracleconn, $update);
            set_error("Your ping has been sent!<br>","index.php");
        }
        else {
            set_error("Cannot send ping message. Is your phone number valid?<br>","index.php");
        }
    }
    
}

function reminder() {
    set_error("This function is no longer being used.","index.php");
    die();
}

function forgot() {
    global $medium;

    // We had to remove the "Forgot Password" feature due to abuse at
    // the conference. Feel free to re-enable it.
    set_error("This function has been removed. Please contact the info desk to get your password changed.","index.php");
    if (substr($medium, 0, 1) != '0') {
        set_error("The email method you have selected is not appropriate. loldongz.<br>", "index.php");
    }
    // forgot function
    global $oracleconn, $from, $subject, $to;

    $query = "select * from Person where ID=$to";
    $result = oracle_query("fetching personal details", $oracleconn, $query);

    // Create new password
    $pass = substr(md5(rand(1,1000000000)), 0, 10);
    $commitpass = hash('sha256',$pass);
    

    // The following query needs to be updated to only allow one request per hour
    $secureq = "select time from forgot where ID=$to and rownum=1 and sysdate-time < 1/24 order by time desc";
    $securer = oracle_query("verifying there is no abuse", $oracleconn, $secureq);
    
    if (sizeof($securer) > 0) {
        set_error("There was an error sending your password reminder. Please wait an hour and try again.","index.php");
    }
    $newpass = "update Person set password='$commitpass' where ID=$to";
    $newresult = oracle_query("resetting password for $to", $oracleconn, $newpass);
    
    $body = "Your password has been reset. Your new password is $pass.";
    if (mail($result[0]["EMAIL"],$subject,$body,$from)) {
        set_error("Password reset for $to","index.php");
    }
    else {
        set_error("Unable to reset password.","index.php");
    }
}

?>
