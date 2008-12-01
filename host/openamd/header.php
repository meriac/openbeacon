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


session_start(); ?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<link type="text/css" href="css/main.css" rel="stylesheet" />
<link type="text/css" href="css/jqModal.css" rel="stylesheet" />
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
<title>The Last HOPE</title>
</head>
<body>
<div id="wrapper"><div id="header"><img src="hopestone.png" align=left /><h1><a href="index.php"><img src="amd-project1.png" width="609" height="100" border=0></a></h1>
<br>
<div align="center">
<div class="nav" id="home"><a href="index.php">main</a></div>
<div class="nav" id="home"><a href="login.php">participate</a></div>
<div class="nav" id="home"><a href="viz.php">visualize</a></div>
<?php
if (isset($_SESSION['rfid'])) { ?>
<div class="nav" id="home"><a href="game.php">play</a></div>
<?php } ?>
<div class="nav" id="contest"><a href="schedule.php">schedule</a></div>
<div class="nav" id="home"><a href="faq.php">faq</a></div>
<?php
if (isset($_SESSION['handle'])) {
  ?><div class="nav" id="logout"><a href="logout.php">log out</a></div><?php
}
else {
  ?><div class="nav" id="login"><a href="login.php">log in</a></div><?php
}
?>
<?php
  if ($_REQUEST['testmode'] == true) {
?>
<div class="nav" id="findafriend"><a href="findafriend.php">find a friend</a></div>
<?php
  }
?>
</div>
</div>

<!-- Begin Content -->
<div id="content">

<p>UPDATE: We have improved visualization performance and restored public Internet access to the web server.  You should be able to access the site from your mobile devices again.</p>
<p/>
<p>PUBLIC WEB SERVICE: The documentation for the tag position web service have been published at <a href="https://amd.hope.net/websvcdocs.php">https://amd.hope.net/websvcdocs.php</a>.  Feel free to use the web service, but please do not abuse it.  Limit your use to one request per second.</p>
<p>&nbsp;</p>

<?php

require_once('showuser.php');    
include('config.php');

if (isset($_SESSION['error'])) {
  echo "<h2><font color=\"red\">" . $_SESSION['error'] . "</font></h2>";
  unset($_SESSION['error']);
}

function set_error($message, $redirect) {
  $_SESSION['error'] = $message;
  if ($redirect == "null") {
    // log message to a file
  }
  else {
    header("Location: $redirect");
    //print "<script>self.location='$redirect'</script>";
    die();
  }
}

?>
