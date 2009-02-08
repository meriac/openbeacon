 <table align="center" width="249" height="381" border="0" cellpadding="0" cellspacing="0">
        <tr>
          <td width="53" height="21"><img src="images/panelCrn_top_l.png" width="53" height="21" /></td>
          <td width="138" background="images/panelCrn_top.png">&nbsp;</td>
          <td width="58"><img src="images/panelCrn_top_r.png" width="52" height="21" /></td>
        </tr>
        <tr>

          <td height="339" background="images/panelCrn_mid_l.png">&nbsp;</td>
          <td bgcolor="#ffe6a9"> 
		  <div class="sidebar-header"><br>
		  New Friend?
		     <div class="sidebar">
<?
//TODO: if user is new, don't display the friends-list

if (isset($_SESSION["id"])) {
$friendq = "select  p2.id,
        p2.handle,
        count(*) common_interest_count,
        collect_concat(cast(collect (i.interest_name) as varchar2_t)) common_interests
from    person p1
       ,interests i1
       ,person p2
       ,interests i2
       ,interests_list i
where   p1.id = i1.id
    and p2.id = i2.id
    and i1.interest = i2.interest
    and i.interest_id = i2.interest
    and p1.id != p2.id
    and p1.id = ". $_SESSION["id"] . "
group by p1.id, p1.handle, p2.id, p2.handle
order by dbms_random.random
";
$friendr = oracle_query("show potential friends", $oracleconn, $friendq);
  if ($friendr[0]) {
    echo $friendr[0]["HANDLE"] . "<br>&nbsp;<i>likes " . $friendr[0]["COMMON_INTERESTS"] . "</i><br>";
  }

}
else {
echo "<a href='create.php'>participate</a> to meet new friends";
}
?>
                     </div>
		  </div>
		 <p> 
		  <div class="sidebar-header">
	          Recently Joined
		  </div>
		     <div class="sidebar">
<?

$recentq = "select Person.Handle, Person.ID, Creation.Timestamp from Person, Creation
where Creation.Registered = 1 and person.id = creation.id 
order by Creation.Timestamp desc";
$recentr = oracle_query("show recent people registered", $oracleconn, $recentq);
echo $recentr[0]["HANDLE"] . "<br>";
echo $recentr[1]["HANDLE"] . "<br>";
echo $recentr[2]["HANDLE"] . "<br>";
echo $recentr[3]["HANDLE"] . "<br>";

?>
                     </div>
		  </div>
		 <p> 
		  <div class="sidebar-header">
	          Your latest location
		  </div>
		     <div class="sidebar">
<?
// get position of user in the current session
if (isset($_SESSION["id"])) {
$locq = "select * from position_snapshot where TAG_ID = '". $_SESSION["id"] . "' and rownum = 1 order by snapshot_timestamp desc";
$locr = oracle_query("show location of user", $oracleconn, $locq);
if ($locr[0]) {
    echo $locr[0]["AREA_ID"] ."<br>";
}

}
else {
echo "<a href='login.php'>login</a> to see your location";
}
?>
                     </div>

           <p>&nbsp;</p>
           <p>&nbsp;</p>
		   
		   </td>
		  
		 
		  
          <td background="images/panelCrn_mid_r.png">&nbsp;</td>
        </tr>
        <tr>
          <td height="21"><div align="right"><img src="images/panelCrn_btm_l.png" width="52" height="21" /></div></td>
          <td background="images/panelCrn_btm_center.png">&nbsp;</td>
          <td><img src="images/panelCrn_btm_r.png" width="58" height="20" /></td>
        </tr>
      </table>
