<?

include_once 'header.php';


$users = get_room_users("Sputnik");
array_pop($users);

foreach($users as $user) {
  $user = split("[|]",$user);
  //why the FUCK are we executing queries every user?
  echo $user[1] . "<br>";
} 


/*$result = oracle_query("get users from saal1", $oracleconn, $query);
$rows = sizeof($result);

for($i = 0; $i < $rows; $i++) {
	echo $result[$i]["TAG_ID"] . "<br>";
}
*/
include_once 'footer.php';

?>
