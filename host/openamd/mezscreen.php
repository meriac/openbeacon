<!--

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

-->

<html>
<body bgcolor="#000000">
<center>
<h2 style="font-family:Arial,sans-serif;color:#00a300;">Live Visualization of the Mezzanine</h2></font>
<img id="locationmap" src="http://amd.hope.net/mezzviz.png"/></center>

    <script><!--
    var fudge = 0;
    function beginrefresh() {
        document.getElementById("locationmap").src = "http://amd.hope.net/mezzviz.png?" + (fudge++);
        setTimeout("beginrefresh()", 2000);
    }
    setTimeout("beginrefresh()", 2000);
    //--></script>



</body>
</html>
