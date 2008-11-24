<?php

/* Mail.php
	Four arguments:
	Medium:  	left|right: from|to
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

if (!isset($_SESSION['handle'])) {
	set_error("lolz. you thinkg you're a hax0r.<br>","index.php");
}



$from = "From: amd-noreply@hope.net\r\n";

$userquery = "
SELECT  p.id person_id, p.email, p.phone, p.provider,

        c.address provider_address,

        t.id talk_id, t.talk_title, t.speaker_name, talk_time

FROM    Talks t,

        Talks_List x,

        person p,

        providers c

WHERE   x.hacker_id = p.id

    AND x.talk_id = t.id

    AND p.provider = c.id

    AND talk_time > sysdate

    AND talk_time < sysdate+15/1440

ORDER BY t.talk_time ASC";


function reminder() {
	global $oracleconn, $subject, $to, $from, $talk, $medium;
	if (substr($medium, 0, 1) != '0') {
		set_error("The email method you have selected is not appropriate. Bitch.<br>","index.php");
	}
	// reminder function
	
	$queryto = "select Person.reminder, Person.email, Person.phone, Person.provider, Providers.Address from Person, Providers where Person.ID=$to and Providers.ID=Person.provider";
	$resultto = oracle_query("fetch to info", $oracleconn, $queryto);
	if (sizeof($resultto) != 1) {
		set_error("Error fetching info for reminder.<br>","index.php");
	}
	else {
		$talkq = "select * from Talks where ID=$talk";
		$talkr = oracle_query("select from talks", $oracleconn, $talkq);
		if (sizeof($talkr) != 1) {
			set_error("Error fetching talk info for reminder.","null");
		}
		else { 
			$reminderbit = 0;
			switch($resultto[0]["REMINDER"]) {
				case 0: set_error("You have no reminders set, dumbass.<br>","index.php");
					break;
				case 1: if ($resultto[0]["EMAIL"]) {
						$reminderbit = 1; 
					}
					else {
						set_error("You have no email set. How are you going to get a reminder?<br>","null");
					}
					break;
				case 2: if ($resultto[0]["PHONE"]) {
						$reminderbit = 2; 
					}
					else {
						set_error("You have no phone set. How are you goin to get a reminder?<br>","null");
					}
					break;
				case 3: if ($resultto[0]["EMAIL"] && $queryto[0]["PHONE"]) {
						$reminderbit = 3; 
					}
					else {
						set_error("You have been doublefisted by the reminder system. That sucks, man...<br>","null");
					}
					break;
				default: break;
			}
			
			$body = "This is an automated message to reminder you that a talk you have chosen is coming up. ";
			$body .= $talkr[0]["TALK_TITLE"] . " with " . $talkr[0]["SPEAKER_NAME"] . "is coming up at ";
			$body .= $talkr[0]["TALK_TIME"] . " in " . $talkr[0]["TRACK"] . ". Hope you enjoy it!";
			
			if ($reminderbit == 1) {		
				if (mail($resultto[0]["EMAIL"], $subject, $body, $from)) {
					set_error("Talk reminder for $talk has been sent to $to","null");
				}
				else {
					set_error("Could not send talk reminder for $talk to $to","null");
				}
			}
			elseif ($reminderbit == 2) {
				if (mail($resultto[0]["PHONE"], $subject, $body, $from)) {
					set_error("Talk reminder for $talk has been sent to $to","null");
				}
				else {
					set_error("Could not send talk reminder for $talk to $to","null");
				}
			}
			elseif ($reminderbit == 3) {
				if (mail($resultto[0]["EMAIL"], $subject, $body, $from)) {
					set_error("Talk reminder for $talk has been sent to $to","null");
				}
				else {
					set_error("Could not send talk reminder for $talk to $to","null");
				}
				if (mail($resultto[0]["PHONE"], $subject, $body, $from)) {
					set_error("Talk reminder for $talk has been sent to $to","null");
				}
				else {
					set_error("Could not send talk reminder for $talk to $to","null");
				}
			}
			else {
				set_error("Error in talk reminder script","null");
			}
		}
	}
}
