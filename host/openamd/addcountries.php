<?
// schedule.en.xml

include 'config.php';

$file = file("countries.xml");

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

//oracle_query("deleting talks from db...", $oracleconn, "delete from Talks");

foreach($file as $line_num => $line) {

    if (preg_match("/<day index=\"(.*)\" date=\"(.*)\"/",$line,$lolz)) {
	echo $lolz[2] . "<br";
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

    if (preg_match("/<\/day>/",$line)) {
	echo "<p>";
    }
/*
    if (preg_match("/<person id=\"(.*)\">(.*)<\/person>/",$line,$lolz)) {
            echo "person id=" . $lolz[1] . "<br>";
            echo "person name=" . $lolz[2] . "<br>";
    }
    if (preg_match("/<link href=\"(.*)\">/",$line,$lolz)) {
            echo "link=" . $lolz[1] . "<br>";
    }
*/

    // Done colleting data? Time to submit to the database
    if (preg_match("/<\/event>/",$line)) { 
	echo "<p>"; $i = 0;
        $addtalk = "insert into Talks 
                (id, Room, Talk_Title, Track, Subtitle, Tag, Talk_Time, Duration, Type, Description, Abstract) values
                ($eventid,'$room','$title','$track','$subtitle','$tag', 
                 to_date('$day $start','yyyy-mm-dd hh24:mi:ss'),'$duration','$type', '$description', '$abstract')";
//	echo $addtalk;
        oracle_query("adding talks from xml", $oracleconn, $addtalk);
    }
  
}

    //list($title, $interests) = split("[|]",$line);
    //$interests = split("[,]",$interests);
    //$title = addslashes($title);
    //$titleq = "select ID from Talks where Talk_Title='$title'";
    //$titler = oracle_query("fetching talk", $oracleconn, $titleq, NULL, 0);

