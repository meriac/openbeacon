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

session_start();

include 'config.php';

?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
<title>Welcome to 25c3 OpenAMD</title>
<link type="text/css" href="css/main2.css" rel="stylesheet" />
<link type="text/css" href="css/jqModal.css" rel="stylesheet" />
<style type="text/css">
<!--
body {
	background-color: #a0a0a0;
	background:url(images/bg.jpg) no-repeat left;
	
}
-->
</style></head>

<body>
<noscript><p><font size=12 color="red">This Application needs Javascript enabled.</font></p></noscript>
<table align="center" width="200" border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td>
	<img src="images/logo.png" />

	</td>
  </tr>
</table>

<div id="mainContentContainer" align="center">
  <table width="919" border="0" cellspacing="0" cellpadding="0">
    <tr>
      <td width="804" align="left" valign="top">
      <table align="center" width="644" border="0" cellspacing="0" cellpadding="0">
        <tr>

          <td width="50"><img src="images/bPanelCrnTopLeft.png" width="50" height="50" /></td>
          <td width="592" background="images/bPanelCrnBtm.png">&nbsp;</td>
          <td width="50"><img src="images/bPanelCrnTopRight.png" width="50" height="50" /></td>
        </tr>
        <tr>
          <td height="424" background="images/bPanelLeftCenter.png">&nbsp;</td>
          <td align="left" valign="top" bgcolor="#fff2c7">
            <div id="mainNav" style="position:relative; left:25px; color:#000; font-family: arial, helvetica, sans-serif; font-weight:bolder;">
              <table width="500" border="0" cellspacing="0" cellpadding="0">
		<!-- Begin TOP MENU links and formatting -->
                <tr>
                  <td>
                    <div class="nav"><a href="index.php">Home</a></div>
	          </td>
                  <td>
                    <? if (isset($_SESSION['handle'])) { ?>
                    <div class="nav"><a href="login.php">Profile</a></div>
                    <? }
                       else { ?> 
                    <div class="nav"><a href="create.php">Participate</a></div>
                    <? } ?>
	          </td>
                  <td>
	    	    <div class="nav"><a href="viz.php">Visualize</a></div>
                  </td>
                  <td>
		    <div class="nav"><a href="schedule.php">Schedule</a></div>
                  </td>
                  <td>
		    <div class="nav"><a href="faq.php">FAQ</a></div>
               	  </td>
		  <!--
                  <td>
                    <div class="nav"><a href="http://25c3.openamd.org">Wiki</a></div>
                  </td> -->
                  <td>
                    <? if (isset($_SESSION['handle'])) { ?>
		    <div class="nav"><a href="logout.php">Log out</a.</div>
                    <? }
                       else { ?>
                    <div class="nav"><a href="login.php">Log in</a></div>
                    <? } ?>
                  </td>
                </tr>
                <!-- End TOP MENU links and formatting -->
              </table>  
            </div>
	    
            <div id=content style="position:relative; left:50px; top:50px;">
<? 
// Display error message, if set 

if (isset($_SESSION['error'])) { ?>
<font color="red" size="+1"><b><? 
	echo $_SESSION['error']; 
        $_SESSION['error'] = ""; 
?></b></font>
<? } ?>
