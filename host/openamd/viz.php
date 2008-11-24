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


include("header.php"); ?>

<h2>Visualize: <?php echo $_SESSION['handle']; ?></h2>
<br><center>
<table cellspacing=3 border=0><tr><td width="10%" valign="top">
<a href="mezzanineviz.php">2nd Floor<br>(Mezzanine)<br>
<img src="/images/mezzanine_thumb.png" border=0></a>
<br><br>
<a href="18thfloorviz.php">18th Floor<br>
<img src="/images/18thfloor_thumb.png" border=0></a>
<br><br>
</td><td align="center" valign="top">
<center><h3>Welcome to the OpenAMD Web Visualization system:</h3><br>
&nbsp;<img id="areachart" src="http://amd.hope.net/areachart.png">
</center>
</td>
</table>
</center>

    <script><!--
    var fudge = 0;
    function beginrefresh() {
        document.getElementById("areachart").src = "http://amd.hope.net/areachart.png?" + (fudge++);
        setTimeout("beginrefresh()", 2000);
    }
    setTimeout("beginrefresh()", 2000);
    //--></script>

<?php

include('room_details.php');

 include("footer.php"); ?>
