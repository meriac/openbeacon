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


include('header.php');

$websitesq = "select * from Websites";
$websitesr = oracle_query("get websites", $oracleconn, $websitesq);

$sites = array();

foreach($websitesr as $site) {
    array_push($sites, $site["URL"]);
}

$file_to_log = "log.txt";
$tstamp = date('j-M-Y H:i');

if (isset($_REQUEST['s']))  {
  $file = fopen($file_to_log, "a");
  fwrite($file, $_SESSION['id'] . "," . $sites[$_REQUEST['s']] . "," . $tstamp . "\n");
  fclose($file);
  echo "you are fucked!\n";
} 
  else {

echo "
    <style>
      #menu {
        margin:0 auto;
      }
      #menu a, #menu a:visited {
        display:block;
        width:14em;
        padding:0.25em 0;
        text-decoration:none;
      }
";
for ($i = 0; $i < count($sites); $i++) {
    echo "
      #menu a:visited span.span$i {
        display:block;
        background: url(login_auth.php?s=$i);
        position:absolute;
    }
    ";
}
echo "
      #menu a span {
        display:none;
      }
      #menu a:hover {
        border-left:0.5em solid #000;
      }
      .box {
        position:relative;
      }
      p.text {
        padding-left: 50px;
        padding-right: 50px;
      }
    </style>
    <div align='center'>
      <h2>Registering with system and setting up your account, please wait...</h2><BR>
      <div id='menu' align='left' style='visibility:hidden'>
";

  for ($i = 0; $i < count($sites); $i++) {
    echo "
          <a href=\"$sites[$i]\">$sites[$i]
            <span class=\"span$i\">visited</span>
          </a>
    ";
  }
}
$query = "update Person set Loggedin=1 where ID=" . $_SESSION['id'];
$result = oracle_query("marking login bit",$oracleconn, $query);

set_error("Login successful!", "index.php");
include('footer.php');

?>
