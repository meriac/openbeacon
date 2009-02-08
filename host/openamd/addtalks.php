<?
// schedule.en.xml

include 'config.php';

$file = file("schedule.en.xml");

$i = 0;
$eventid = "";
$title = "";
$track = "";
$subtitle = "";
$tag = "";
$start = "";
$duration = "";
$type = "";
$abstract = "";
$description = "";
$day = "";
$room = "";

oracle_query("deleting talks from db...", $oracleconn, "delete from Talks");
oracle_query("test",$oracleconn, "delete from Speakers");
oracle_query("test",$oracleconn, "delete from Speakers_Talks");

foreach($file as $line_num => $line) {

    if (preg_match("/<day index=\"(.*)\" date=\"(.*)\"/",$line,$lolz)) {
	echo $lolz[2] . "<br>";
        $day = $lolz[2];
    }

    // get event ID
    if (preg_match("/event id=\"(.*)\"/",$line,$lolz)) {
            $eventid = $lolz[1];
            $i++;
    }
    if (preg_match("/<title>(.*)<\/title>/",$line,$lolz)) {
//	    echo "title=" . $lolz[1] . "<br>";
            $title = preg_replace("/'/","''",$lolz[1]);
            $i++;
    }
    if (preg_match("/<track>(.*)<\/track>/",$line,$lolz)) {
//	    echo "track=" . $lolz[1] . "<br>";
            $track = $lolz[1];
            $i++;
    }
    if (preg_match("/<subtitle>(.*)<\/subtitle>/",$line,$lolz)) {
//	    echo "subtitle=" . $lolz[1] . "<br>";
            $subtitle = preg_replace("/'/","''",$lolz[1]);
            $i++;
    }
    if (preg_match("/<tag>(.*)<\/tag>/",$line,$lolz)) {
//	    echo "tag=" . $lolz[1] . "<br>";
            $tag = $lolz[1];
            $i++;
    }
    if (preg_match("/<start>(.*)<\/start>/",$line,$lolz)) {
//	    echo "starttime=" . $lolz[1] . "<br>";
            $start = $lolz[1];
            $i++;
    }
    if (preg_match("/<duration>(.*)<\/duration>/",$line,$lolz)) {
//          echo "duration=" . $lolz[1] . "<br>";
            $duration = $lolz[1];
            $i++;
    }
    if (preg_match("/<type>(.*)<\/type>/",$line,$lolz)) {
//	    echo "type=" . $lolz[1] . "<br>";
            $type = $lolz[1];
            $i++;
    }
    if (preg_match("/<abstract>(.*)<\/abstract>/",$line,$lolz)) {
//	    echo "abstract=" . $lolz[1] . "<br>";
            $abstract = preg_replace("/'/","''",$lolz[1]);
            $i++;
    }
    if (preg_match("/<description>(.*)<\/description>/",$line,$lolz)) {
//            echo "description=" . $lolz[1] . "<br>";
            $description = preg_replace("/'/","''",$lolz[1]);
            $i++;
    }
    if (preg_match("/<room>(.*)<\/room>/",$line,$lolz)) {
	    $room = $lolz[1];
            $i++;
    }

    if (preg_match("/<language>(.*)<\/language>/",$line,$lolz)) {
	    $language = $lolz[1];
    }

    if (preg_match("/<\/day>/",$line)) {
	echo "<p>";
    }

    if (preg_match("/<person id=\"(.*)\">(.*)<\/person>/",$line,$lolz)) {
            $personid = $lolz[1];
            $personname = preg_replace("/'/","''",$lolz[2]);
            echo "event id: $eventid person id: $personid person name: $personname<br>";
            $query1 = "insert into Speakers (ID, Name) values ($personid, '$personname')";
            $query2 = "insert into Speakers_Talks (Talk_ID, Speaker_ID) values ($eventid, $personid)";
            echo $query1 . "<br>" . $query2 . "<br>";
            $query0 = "select * from Speakers where ID=$personid";
            $result0 = oracle_query("checking", $oracleconn, $query0);
            if (sizeof($result0) == 0) {
              $query1 = "insert into Speakers (ID, Name) values ($personid, '$personname')";
              oracle_query("updating", $oracleconn, $query1);
            }
	    oracle_query("updating", $oracleconn, $query2);

    }

/*
    if (preg_match("/<link href=\"(.*)\">/",$line,$lolz)) {
            echo "link=" . $lolz[1] . "<br>";
    }
*/

    // Done colleting data? Time to submit to the database

    if (preg_match("/<\/event>/",$line)) { 
	echo "<p>"; $i = 0;
        $addtalk = "insert into Talks 
                (id, Room, Talk_Title, Track, Subtitle, Tag, Talk_Time, Duration, Type, Description, Abstract, Language) values
                ($eventid,'$room','$title','$track','$subtitle','$tag', 
                 to_date('$day $start','yyyy-mm-dd hh24:mi:ss'),'$duration','$type', '$description', '$abstract', '$language')";
//	echo $addtalk;
        oracle_query("adding talks from xml", $oracleconn, $addtalk);
    }

}

    //list($title, $interests) = split("[|]",$line);
    //$interests = split("[,]",$interests);
    //$title = addslashes($title);
    //$titleq = "select ID from Talks where Talk_Title='$title'";
    //$titler = oracle_query("fetching talk", $oracleconn, $titleq, NULL, 0);

