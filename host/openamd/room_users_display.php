<?

include 'config.php';


if (isset($_REQUEST['zone'])) {
	$zone = $_REQUEST['zone'];
}
$users = get_room_users($zone);
$size = sizeof($users);
if ($size > 1) {
        $size2 = $size-1;
	echo "<b>" . $size2 . " people</b><br>";
}
else {
  	echo "<b>This room is lonely. You should visit.</b><br>";
}

array_pop($users);
foreach($users as $user) {
  $user = split("[|]",$user);
  //why the FUCK are we executing queries every user?
  //show_bitch($user[1]);
  $username = oracle_query("get username", $oracleconn, "select Handle from Person where ID=" . $user[1]);
  if (sizeof($username) > 0) {
	  echo $username[0]["HANDLE"] . "<br>";
  }
  else {
	  echo $user[1] . "<br>";
  }
} 

?>
