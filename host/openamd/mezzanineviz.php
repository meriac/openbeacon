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

<a href="mezzanineheat.php">Heat map version</a><br />
<a href="clickablemezz.php">Clickable AJAX version</a><br />
<br />
<h2>Visualize: Mezzanine (2nd floor)</h2>
<br>
<table><tr><td valign="top" width=110>
<a href="mezzanineviz.php">2nd Floor (Mezzanine)<br>
<img src="/images/mezzanine_thumb.png" border=0></a>
<br><br>
<a href="18thfloorviz.php">18th Floor<br>
<img src="/images/18thfloor_thumb.png" border=0></a>
<Br><br>
</td><td valign="top">
<!--<img src="/images/mezzanine.png"><br><br>-->
<img id="locationmap" src="http://amd.hope.net/mezzviz.png"/><br/><br/>
<br>

</td></tr></table>

    <script><!--
    var fudge = 0;
    function beginrefresh() {
        document.getElementById("locationmap").src = "http://amd.hope.net/mezzviz.png?" + (fudge++);
        setTimeout("beginrefresh()", 2000);
    }
    setTimeout("beginrefresh()", 2000);
    //--></script>


<?php include("footer.php"); ?>
