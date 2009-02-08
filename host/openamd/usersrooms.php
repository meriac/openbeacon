<?php

include 'header.php';


// ajax to room_users_display.php?zone=$zone
?>


<script language="javascript">

function showUsers(zone) {
 
      var url="room_users_display.php?zone="+zone;
	var page;
         if(window.XMLHttpRequest) {
		page = new XMLHttpRequest();
	}
	else if (window.ActiveXObject) {
		page = new ActiveXObject("Microsoft.XMLHTTP");
	}


         var obj=document.getElementById("users");
		page.open("GET", url);
		page.onreadystatechange = function()
		{
			if (page.readyState == 4 && page.status == 200) {
			 	obj.innerHTML = page.responseText;
			}
		}
		page.send(null);
}
  
</script>

Click a room to view people in it.<br>
If a user has not registered, it will display their RFID.<br>

<table width="600px"><tr><td>
<b>Rooms:</b><br>
<?

$zones = array("Saal1", "Saal2", "Saal3", "BasementStairs", "FloorCLounge", "FloorCStairs",
          "HackCenter", "Information", "Lego", "Lounge", "Motodrone", "PublicTerminals",
          "Sputnik", "Microcontrollers","NOC","HardwareWorkshop","Merchandise",
           "unknown","EntranceArea");

foreach($zones as $zone) {
	?><a href="javascript:showUsers('<? echo $zone; ?>')"><? echo $zone; ?></a><br><?

}
?>

</td><td valign="top">
<div id="users" style="background-color: #ffff"></div>
</td></tr></table>

<?

include 'footer.php';

?>
