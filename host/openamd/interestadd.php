<?

include 'config.php';

$interests = "new tech,activism,radio,lockpicking,crypto,privacy,ethics,telephones,social engineering,
hacker spaces,hardware hacking,nostalgia,communities,science,government,network security,
malicious software,pen testing,web,niche hacks,media";

$test = split(',',$interests );
$i = 0;
foreach($test as $value) {
        $work = "insert into Interests_list (interest_id, interest_name) values ($i, '$value')";
	oracle_query("adding interests", $oracleconn, $work);
        $i++;
}

?>


