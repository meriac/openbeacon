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

<div class="body-header">25c3 OpenAMD Web Visualization:</div>
<div class="body-text">
<p>Proximity tracking by SocioPatterns:</p>
<object classid="clsid:D27CDB6E-AE6D-11cf-96B8-444553540000"
codebase="http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.ca
b#version=6,0,0,0">
<param name="movie"
value="https://ssl.openamd.org/openamd/sociopatterns/_swfs_/bt3_25c3_web_alpha.swf" />
<param name="quality" value="high" />
<param name="allowScriptAccess" value="always" />
<param name="play" value="true" />
<param name="allowFullscreen" value="true"/>
<param name="bgcolor" value="#000000" />
<embed height="434"
pluginspage="http://www.macromedia.com/go/getflashplayer"
allowScriptAccess="always"
src="https://ssl.openamd.org/openamd/sociopatterns/_swfs_/bt3_25c3_web_alpha.swf"
type="application/x-shockwave-flash" width="599" quality="high" play="true"
allowFullscreen="true" bgcolor="#000000"></embed>
</object>

<a target="blank" href="https://ssl.openamd.org/openamd/sociopatterns/_swfs_/bt3_25c3_web_alpha.swf">click here to view (flash, in new window)</a>
<p><font color="red" size="16">Other visuals will be online soon.</font></p>
</div>

    <script><!--
    var fudge = 0;
    function beginrefresh() {
        document.getElementById("areachart").src = "http://amd.hope.net/areachart.png?" + (fudge++);
        setTimeout("beginrefresh()", 2000);
    }
    setTimeout("beginrefresh()", 2000);
    //--></script>

<?php

include("footer.php"); ?>
