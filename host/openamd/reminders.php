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


// Reminder script

include('header.php');


$talksquery = "select ID from Talks where Talk_Time > sysdate and talk_time < sysdate + 15/1440";
$talksresult = oracle_query("fetching talk IDs", $oracleconn, $talksquery);
$talksnum = sizeof($talksresult);

foreach($talksresult as $talk) {
    $personquery = "select Talks.Talk_Title, Talks.Speaker_Name, Talks.Talk_Time, Talks.Track,
    Person.Handle, Person.Email, Person.Phone, Person.ID, Person.Reminder,
    Providers.Address
    From Talks, Person, Talks_List, Providers
    Where Talks.ID=" . $talk["ID"] . " and Talks_List.Talk_ID=Talks.ID
    and Person.Provider=Providers.ID and Talks_List.Hacker_ID=Person.ID";
    $personresult = oracle_query("get data for reminders", $oracleconn, $personquery);
    $personnum = sizeof($personresult);
    foreach($personresult as $person) {

        // send out the emails

        $subject = "OpenAMD Talk Reminder";
        $from = "From: amd-noreply@hope.net\r\n";
        $body = "This is an automated message to remind you that a talk you have chosen is coming up. ";
        $body .= $person["TALK_TITLE"] . " with " . $person["SPEAKER_NAME"] . " is coming up at ";
        $body .= $person["TALK_TIME"] . " in " . $person["TRACK"] . ". Hope you enjoy it!";
        
        $email = $person["EMAIL"];
        $phone = ereg_replace("[^A-Za-z0-9]","",mcrypt_cbc(MCRYPT_BLOWFISH, $person["ID"], base64_decode($person["PHONE"]), MCRYPT_DECRYPT, '12345678')) . "@" . $person["ADDRESS"];
        
        switch($person["REMINDER"]) {
            case 0: break;
            case 1: if (!mail($email, $subject, $body, $from)) {
                    echo "could not email " . $person["ID"] . " talk reminder.";
                }
                else {
                    echo "sent " . $person["ID"] . " talk reminder.";
                }
            case 2: if (!mail($phone, $subject, $body, $from)) { // send sms reminder only
                    echo "could not sms " . $person["ID"] . " talk reminder.";
                }
                else {
                    echo "sent " . $person["ID"] . " talk reminder.";
                }
            case 3: if (!mail($email, $subject, $body, $from)) {
                    echo "could not email " . $person["ID"] . " talk reminder.";
                }
                else {
                    echo "sent " . $person["ID"] . " talk reminder.";
                }
                if (!mail($phone, $subject, $body, $from)) {
                    echo "could not sms " . $person["ID"] . " talk reminder.";
                }
                else {
                    echo "sent " . $person["ID"] . " talk reminder.";
                }
            default: break;
        }

    }
}

?>
