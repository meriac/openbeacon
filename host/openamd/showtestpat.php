<?

error_reporting(E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

echo "starting";
include 'config.php';
echo "included config";
include 'showuser.php';
echo "included showuser.php";

if (isset($_REQUEST["userid"]))  {
	echo "inside showuser";
	try {
		show_user($_REQUEST["userid"],"test");
	}
	catch (exception $e) {
		print_r($e);
	}
}
else {
	show_user(1234,"test");
}
?>
